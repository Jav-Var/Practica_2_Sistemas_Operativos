#ifndef BUCKETS_H
#define BUCKETS_H

#include <stdint.h>
#include "common.h"

/* Create buckets file with header and num_buckets entries zeroed */
int buckets_create(const char *path);

/* Open buckets file and read header (returns fd or -1) */
int buckets_open_readwrite(const char *path);

/* Read head offset for bucket_id (0..num_buckets-1) */
off_t buckets_read_head(int fd, uint64_t bucket_id);

/* Write head offset for bucket_id */
int buckets_write_head(int fd, uint64_t num_buckets, uint64_t bucket_id, off_t head);

/* Helper to compute offset in file for bucket entry */
off_t buckets_entry_offset(uint64_t bucket_id);

#endif // BUCKETS_H
