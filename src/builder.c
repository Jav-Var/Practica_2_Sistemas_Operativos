#define _GNU_SOURCE
#include "builder.h"
#include "buckets.h"
#include "arrays.h"
#include "common.h"
#include "hash.h"
#include "util.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>

/* Extract CSV field by index (0-based), support quoted fields with double quotes.
   The returned pointer is malloc'd and must be freed (or ownership transferred).
*/
static char *csv_get_field_copy(const char *line, int field_idx) {
    const char *p = line;
    int idx = 0;
    while (*p && idx < field_idx) {
        if (*p == '"') {
            p++;
            while (*p && !(*p == '"' && (*(p+1) == ',' || *(p+1) == '\0' || *(p+1) == '\n' || *(p+1)=='\r'))) {
                /* skip until closing quote followed by comma or EOL */
                if (*p == '"' && *(p+1) == '"') { p += 2; continue; } /* escaped quote */
                p++;
            }
            if (*p == '"') p++;
            if (*p == ',') p++;
            idx++;
            continue;
        } else {
            while (*p && *p != ',') p++;
            if (*p == ',') p++;
            idx++;
            continue;
        }
    }
    /* extract field at idx == field_idx */
    if (!*p) return strdup("");
    char *out = NULL;
    if (*p == '"') {
        p++;
        size_t cap = 256;
        out = malloc(cap);
        size_t len = 0;
        while (*p) {
            if (*p == '"' && *(p+1) == '"') {
                /* escaped quote -> copy one quote */
                if (len + 1 >= cap) { cap *= 2; out = realloc(out, cap); }
                out[len++] = '"';
                p += 2;
                continue;
            } else if (*p == '"') {
                p++;
                break;
            } else {
                if (len + 1 >= cap) { cap *= 2; out = realloc(out, cap); }
                out[len++] = *p++;
            }
        }
        out[len] = '\0';
        /* skip optional comma */
        if (*p == ',') p++;
    } else {
        const char *start = p;
        while (*p && *p != ',' && *p != '\n' && *p != '\r') p++;
        size_t len = p - start;
        out = malloc(len + 1);
        memcpy(out, start, len);
        out[len] = '\0';
        if (*p == ',') p++;
    }
    return out;
}

/* field index: title = 0, author_name = 1 */
static int get_field_index_for(const char *index_name) {
    if (strcmp(index_name, "title") == 0) return 0;
    if (strcmp(index_name, "author") == 0 || strcmp(index_name, "author_name") == 0) return 1;
    return -1;
}

/* Build single index in streaming mode: for each CSV row (after header),
   read the key, get current file byte offset for the line, create an arrays_node_t
   with list_len=1 and next_ptr = old_head, append and update bucket head.
*/
int build_index_stream(const char *csv_path, const char *out_dir, const char *index_name, uint64_t num_buckets, uint64_t hash_seed) {
    int field_idx = get_field_index_for(index_name);
    if (field_idx < 0) return -1;

    char buckets_path[1024];
    char arrays_path[1024];
    snprintf(buckets_path, sizeof(buckets_path), "%s/%s_buckets.dat", out_dir, index_name);
    snprintf(arrays_path, sizeof(arrays_path), "%s/%s_arrays.dat", out_dir, index_name);

    if (buckets_create(buckets_path, num_buckets, hash_seed) != 0) {
        fprintf(stderr, "Failed to create buckets file %s\n", buckets_path);
        return -1;
    }
    if (arrays_create(arrays_path) != 0) {
        fprintf(stderr, "Failed to create arrays file %s\n", arrays_path);
        return -1;
    }

    int bfd = buckets_open_readwrite(buckets_path, NULL, NULL);
    if (bfd < 0) { fprintf(stderr,"open buckets failed\n"); return -1; }
    int afd = arrays_open(arrays_path);
    if (afd < 0) { close(bfd); fprintf(stderr,"open arrays failed\n"); return -1; }

    FILE *f = fopen(csv_path, "rb");
    if (!f) { close(bfd); close(afd); fprintf(stderr,"open csv failed\n"); return -1; }

    /* read header line and skip */
    off_t line_off = ftello(f);
    char *line = NULL;
    size_t llen = 0;
    ssize_t nread;
    nread = getline(&line, &llen, f);
    if (nread <= 0) { fclose(f); close(bfd); close(afd); if (line) free(line); return -1; }

    /* iterate rows */

    while (1) {
        line_off = ftello(f);
        if (line_off == (off_t)-1) {
            perror("ftello");
            break;
        }
        nread = getline(&line, &llen, f);
        if (nread <= 0) break;

        char *field = csv_get_field_copy(line, field_idx);
        if (!field) continue;

        char *normalized_key;
        normalized_key = normalize_string(field);
        
        free(field);

        uint64_t h = hash_key_prefix(normalized_key, strlen(normalized_key), hash_seed);
        uint64_t mask = num_buckets - 1;
        uint64_t bucket = bucket_id_from_hash(h, mask);

        off_t old_head = buckets_read_head(bfd, num_buckets, bucket);

        arrays_node_t node;
        node.key_len = (uint16_t)strlen(normalized_key);
        node.key = strdup(normalized_key); 

        free(normalized_key);

        node.list_len = 1;
        node.offsets = malloc(sizeof(off_t) * 1);
        if (!node.offsets) {
            arrays_free_node(&node);
            fprintf(stderr, "malloc failed for offsets\n");
            continue;
        }
        node.offsets[0] = (off_t)line_off;
        node.next_ptr = old_head;

        off_t new_node_off = arrays_append_node(afd, &node);
        if (new_node_off == 0) {
            fprintf(stderr, "failed append node\n");
            arrays_free_node(&node);
            continue;
        }

        arrays_free_node(&node);

        if (buckets_write_head(bfd, num_buckets, bucket, new_node_off) != 0) {
            fprintf(stderr, "failed write bucket head\n");
        }
    }

    if (line) free(line);
    fclose(f);
    close(bfd);
    close(afd);
    return 0;
}

int build_both_indices_stream(const char *csv_path, const char *out_dir, uint64_t num_buckets_title, uint64_t num_buckets_author, uint64_t hash_seed) {
    /* We will do two passes (one per index) to keep code simple */
    if (build_index_stream(csv_path, out_dir, "title", num_buckets_title, hash_seed) != 0) {
        perror("build title index");
        return -1;
    }
    if (build_index_stream(csv_path, out_dir, "author", num_buckets_author, hash_seed) != 0) {
        perror("build author index");   
        return -1;
    }
    return 0;
}
