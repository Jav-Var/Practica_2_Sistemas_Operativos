#ifndef READER_H
#define READER_H

#include "common.h"

// index_handle_t
typedef struct {
    int buckets_fd;
    int arrays_fd;
} index_handle_t;

/* Open an index given paths to buckets and arrays files */
int index_open(index_handle_t *h, const char *buckets_path, const char *arrays_path);

/* Close index */
void index_close(index_handle_t *h);

/* Lookup key: returns array of offsets (malloc'd) and count via out_count. Caller frees *out_offsets. */
int index_lookup(index_handle_t *h, const char *key, off_t **out_offsets, uint32_t *out_count);

#endif // READER_H
