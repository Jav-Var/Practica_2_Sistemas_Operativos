#ifndef BUILDER_H
#define BUILDER_H

#include <stdint.h>

/* Functions for building the two index files from dataset CSV */

int build_index_stream(const char *csv_path, const char *out_dir, const char *index_name, uint64_t num_buckets, uint64_t hash_seed);

/* Build both indices title and author */
int build_both_indices_stream(const char *csv_path, const char *out_dir, uint64_t num_buckets_title, uint64_t num_buckets_author, uint64_t hash_seed);

#endif // BUILDER_H
