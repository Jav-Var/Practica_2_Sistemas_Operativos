#ifndef BUILDER_H
#define BUILDER_H

#include <stdint.h>

/* Functions for building the two index files from dataset CSV */

int build_index_stream(const char *csv_path);
int build_index_line(const char *csv_path, const char *line);

#endif // BUILDER_H
