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
    // inserta en el handle h (struct) la informacion del indice
    uint64_t num_buckets = 0, hash_seed = 0;
    int bfd = buckets_open_readwrite(buckets_path, &num_buckets, &hash_seed);
    if (bfd < 0) return -1;
    int afd = arrays_open(arrays_path);
    if (afd < 0) { close(bfd); return -1; }
    h->buckets_fd = bfd;
    h->arrays_fd = afd;
    h->num_buckets = NUM_BUCKETS;
    h->hash_seed = DEFAULT_HASH_SEED;
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

/*
int lookup_by_title_author(index_handle_t *title_h, index_handle_t *author_h,
    const char *title_key, const char *author_key, off_t **out_offsets, uint32_t *out_count) {}
*/