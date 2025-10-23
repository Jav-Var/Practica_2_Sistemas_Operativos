#include "reader.h"
#include "buckets.h"
#include "arrays.h"
#include "common.h"
#include "hash.h"
#include "util.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>

int index_open(index_handle_t *h, const char *buckets_path, const char *arrays_path) {
    uint64_t num_buckets = 0, hash_seed = 0;
    int bfd = buckets_open_readwrite(buckets_path, &num_buckets, &hash_seed);
    if (bfd < 0) return -1;
    int afd = arrays_open(arrays_path);
    if (afd < 0) { close(bfd); return -1; }
    h->buckets_fd = bfd;
    h->arrays_fd = afd;
    h->num_buckets = num_buckets;
    h->hash_seed = hash_seed;
    return 0;
}

void index_close(index_handle_t *h) {
    if (!h) return;
    if (h->buckets_fd >= 0) close(h->buckets_fd);
    if (h->arrays_fd >= 0) close(h->arrays_fd);
    h->buckets_fd = h->arrays_fd = -1;
    h->num_buckets = 0;
    h->hash_seed = 0;
}

int index_lookup(index_handle_t *h, const char *key, off_t **out_offsets, uint32_t *out_count) {
    if (!h || !key || !out_offsets || !out_count) return -1;
    *out_offsets = NULL;
    *out_count = 0;

    uint64_t hval = hash_key_prefix(key, strlen(key), h->hash_seed);
    uint64_t mask = h->num_buckets - 1;
    uint64_t bucket = bucket_id_from_hash(hval, mask);
    off_t head = buckets_read_head(h->buckets_fd, h->num_buckets, bucket);
    if (head == 0) return 0;

    /* dynamic array for results */
    uint32_t cap = 16;
    uint32_t cnt = 0;
    off_t *results = malloc(sizeof(off_t) * cap);
    if (!results) return -1;

    off_t cur = head;
    while (cur != 0) {
        arrays_node_t node = {0, NULL, 0, NULL, 0};
        if (arrays_read_node_full(h->arrays_fd, cur, &node) != 0) {
            break;
        }

        off_t next = node.next_ptr; /* save next before freeing node */

        if (node.key) {
            if (normalized_strcmp(node.key, key) == 0) {
                for (uint32_t i = 0; i < node.list_len; ++i) {
                    if (cnt >= cap) {
                        uint32_t new_cap = cap * 2;
                        off_t *tmp = realloc(results, sizeof(off_t) * new_cap);
                        if (!tmp) {
                            arrays_free_node(&node);
                            free(results);
                            return -1;
                        }
                        results = tmp;
                        cap = new_cap;
                    }
                    results[cnt++] = node.offsets[i];
                }
            }
        }

        /* free node buffers */
        arrays_free_node(&node);
        cur = next;
    }

    if (cnt == 0) {
        free(results);
        *out_offsets = NULL;
        *out_count = 0;
    } else {
        *out_offsets = results;
        *out_count = cnt;
    }
    return 0;
}

static int cmp_offset(const void *a, const void *b) {
    const off_t va = *(const off_t *)a;
    const off_t vb = *(const off_t *)b;
    if (va < vb) return -1;
    if (va > vb) return 1;
    return 0;
}

int lookup_by_title_author(index_handle_t *title_h, index_handle_t *author_h,
    const char *title_key, const char *author_key, off_t **out_offsets, uint32_t *out_count)
{
    if (!out_offsets || !out_count) return -1;

    /* validate inputs */
    int has_title = (title_key && title_key[0] != '\0');
    int has_author = (author_key && author_key[0] != '\0');

    if (!has_title && !has_author) {
        fprintf(stderr, "Search must have at least one parameter\n");
        return -1;
    }

    *out_offsets = NULL;
    *out_count = 0;

    off_t *title_offs = NULL;
    uint32_t title_cnt = 0;
    off_t *author_offs = NULL;
    uint32_t author_cnt = 0;
    int rc = 0;

    /* perform lookups as needed */
    if (has_title) {
        rc = index_lookup(title_h, title_key, &title_offs, &title_cnt);
        if (rc != 0) {
            /* index_lookup failure */
            if (title_offs) free(title_offs);
            return -1;
        }
    }

    if (has_author) {
        rc = index_lookup(author_h, author_key, &author_offs, &author_cnt);
        if (rc != 0) {
            if (title_offs) free(title_offs);
            if (author_offs) free(author_offs);
            return -1;
        }
    }

    /* Cases:
     * 1) only title -> return title_offs
     * 2) only author -> return author_offs
     * 3) both -> intersect title_offs and author_offs
     */

    if (has_title && !has_author) {
        /* return title results as-is (title_offs was malloc'd by index_lookup) */
        if (title_cnt == 0) {
            /* nothing found */
            if (title_offs) free(title_offs);
            *out_offsets = NULL;
            *out_count = 0;
            return 0;
        }
        *out_offsets = title_offs;
        *out_count = title_cnt;
        return 0;
    }

    if (!has_title && has_author) {
        if (author_cnt == 0) {
            if (author_offs) free(author_offs);
            *out_offsets = NULL;
            *out_count = 0;
            return 0;
        }
        *out_offsets = author_offs;
        *out_count = author_cnt;
        return 0;
    }

    /* Both present: intersect. If either empty, intersection is empty. */
    if (title_cnt == 0 || author_cnt == 0) {
        if (title_offs) free(title_offs);
        if (author_offs) free(author_offs);
        *out_offsets = NULL;
        *out_count = 0;
        return 0;
    }

    /* Sort both arrays in-place so we can do two-pointer intersection.
       index_lookup returns malloc'd arrays which we own here, so it's fine to sort them. */
    qsort(title_offs, title_cnt, sizeof(off_t), cmp_offset);
    qsort(author_offs, author_cnt, sizeof(off_t), cmp_offset);

    /* allocate result buffer (at most min(title_cnt, author_cnt)) */
    uint32_t i = 0, j = 0;
    uint32_t cap = (title_cnt < author_cnt) ? title_cnt : author_cnt;
    off_t *res = NULL;
    if (cap > 0) res = malloc(sizeof(off_t) * cap);
    if (!res && cap > 0) {
        /* memory error */
        free(title_offs);
        free(author_offs);
        return -1;
    }

    uint32_t res_cnt = 0;
    off_t last_added = (off_t)0; /* used to deduplicate equal consecutive offsets */
    int have_last = 0;

    while (i < title_cnt && j < author_cnt) {
        if (title_offs[i] < author_offs[j]) {
            i++;
        } else if (title_offs[i] > author_offs[j]) {
            j++;
        } else {
            /* equal */
            off_t v = title_offs[i];
            if (!have_last || v != last_added) {
                res[res_cnt++] = v;
                last_added = v;
                have_last = 1;
            }
            /* advance both */
            i++; j++;
        }
    }

    /* free source arrays */
    free(title_offs);
    free(author_offs);

    if (res_cnt == 0) {
        if (res) free(res);
        *out_offsets = NULL;
        *out_count = 0;
        return 0;
    }

    /* shrink to fit */
    off_t *final = realloc(res, sizeof(off_t) * res_cnt);
    if (!final) {
        /* if realloc fails keep original res */
        final = res;
    }
    *out_offsets = final;
    *out_count = res_cnt;
    return 0;
}

