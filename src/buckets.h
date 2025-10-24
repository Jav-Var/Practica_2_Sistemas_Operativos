#ifndef BUCKETS_H
#define BUCKETS_H

#include <stdint.h>
#include "common.h"

/* buckets.h
 * Functions for managing the title_buckets.dat file
 * Each buckets file contains a header followed by a contiguous table of bucket entries.
 *
 * File layout:
 * - header (fixed size BUCKETS_HEADER_SIZE bytes)
 *   - magic         : 4 bytes  (ASCII, e.g. "IDX1")
 *   - version       : uint16  (2 bytes)
 *   - reserved      : uint16  (2 bytes)
 *   - page_size     : uint32  (4 bytes)  // typically BUCKETS_HEADER_SIZE
 *   - num_buckets   : uint64  (8 bytes)  // number of bucket entries (prefer power-of-two)
 *   - hash_seed     : uint64  (8 bytes)  // seed used by the hash function
 *   - entry_size    : uint32  (4 bytes)  // size of each bucket entry in bytes (typically 8)
 *   - reserved/pad  : rest of header to fill BUCKETS_HEADER_SIZE
 *
 * - head_offset == 0 means the bucket is empty (no nodes).
 *
 * This module exposes functions to create/open the buckets file, read a bucket head, and write a bucket head.
 */

/* Create buckets file with header and num_buckets entries zeroed */
int buckets_create(const char *path, uint64_t num_buckets, uint64_t hash_seed);

/* Open buckets file and read header (returns fd or -1) */
int buckets_open_readwrite(const char *path, uint64_t *num_buckets_out, uint64_t *hash_seed_out);

/* Read head offset for bucket_id (0..num_buckets-1) */
off_t buckets_read_head(int fd, uint64_t num_buckets, uint64_t bucket_id);

/* Write head offset for bucket_id */
int buckets_write_head(int fd, uint64_t num_buckets, uint64_t bucket_id, off_t head);

/* Helper to compute offset in file for bucket entry */
off_t buckets_entry_offset(uint64_t bucket_id);

#endif // BUCKETS_H
