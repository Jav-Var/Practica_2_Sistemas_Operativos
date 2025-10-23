#include "hash.h"
#include "util.h" 
#include "common.h"
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

/* FNV-1a 64-bit mixed with hashseed. */
uint64_t hash_key_prefix(const char *key, size_t len, uint64_t seed) {
    const uint64_t FNV_OFFSET = 14695981039346656037ULL;
    const uint64_t FNV_PRIME = 1099511628211ULL;

    char *norm = normalize_string(key ? key : "");
    const unsigned char *data = NULL;
    size_t max = 0;

    if (norm) {
        data = (const unsigned char *)norm;
        size_t nlen = strlen(norm);
        max = (nlen < KEY_PREFIX_LEN) ? nlen : KEY_PREFIX_LEN;
    } else {
        data = (const unsigned char *)key;
        max = (len < KEY_PREFIX_LEN) ? len : KEY_PREFIX_LEN;
    }

    uint64_t h = FNV_OFFSET ^ seed;
    for (size_t i = 0; i < max; ++i) {
        h ^= (uint64_t)data[i];
        h *= FNV_PRIME;
    }

    h ^= h >> 33;
    h *= 0xff51afd7ed558ccdULL;
    h ^= h >> 33;
    h *= 0xc4ceb9fe1a85ec53ULL;
    h ^= h >> 33;

    if (norm) free(norm);
    return h;
}
