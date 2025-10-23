#ifndef HASH_H
#define HASH_H

#include <stdint.h>
#include <stddef.h>


/* Compute 64-bit hash based on first up to HASH_KEY_PREFIX_LEN bytes */
uint64_t hash_key_prefix(const char *key, size_t len, uint64_t seed);

/* Given hash and mask (num_buckets is power of two) */
static inline uint64_t bucket_id_from_hash(uint64_t h, uint64_t mask) {
    return h & mask;
}

#endif // HASH_H
