#ifndef UTIL_H
#define UTIL_H

#include <stdint.h>
#include <stddef.h>

uint64_t next_pow2(uint64_t v);

int normalized_strcmp(const char *a, const char *b);
char *normalize_string(const char *s);
#endif // UTIL_H
