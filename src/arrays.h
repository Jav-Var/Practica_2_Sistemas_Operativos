#ifndef ARRAYS_H
#define ARRAYS_H

#include <stdint.h>
#include "common.h"

/* arrays.h
 *
 * Functions for managing the title_arrays.dat and author_arrays.dat files
 * Each file contains a header followed by a sequence of nodes
 * Each node contains:
 * - key_len   : uint16  (2 bytes)   -> length of key (bytes)
 * - key_bytes : key_len bytes       -> UTF-8 bytes of the key (not NUL-terminated on disk)
 * - list_len  : uint32  (4 bytes)   -> number of record offsets stored in this node
 * - offsets[] : uint64 * list_len   -> each offset is a uint64 pointing to a byte offset in combined_dataset.csv
 * - next_ptr  : uint64  (8 bytes)   -> absolute byte offset in arrays.dat of the next node (0 == null)
 *
 * The file begins with a fixed-size header of ARRAYS_HEADER_SIZE bytes (reserved area).
 * Nodes should start at offsets >= ARRAYS_HEADER_SIZE; offset 0 is reserved/sentinel.
 *
 */

typedef struct {
    uint16_t key_len;         
    char *key;           // key (string) of the node (title/author)
    uint32_t list_len;   // number of offsets
    off_t *offsets;   // array of length list_len
    off_t next_ptr;   // next pointer to manage collisions (hash table implemented by chaining)
} arrays_node_t;

/* create an empty arrays file with header space reserved */
int arrays_create(const char *path);

/* open arrays file for read/write, returns the file descriptor */
int arrays_open(const char *path);

/* appends a node and return offset where node starts (absolute offset in arrays.dat) */
off_t arrays_append_node(int fd, const arrays_node_t *node);

/* read a node fully: caller must free key_out and offsets_out */
int arrays_read_node_full(int fd, off_t node_off, arrays_node_t *node);

/* returns the size of a node with a key (string) of size key_len, and a list of offsets of size list_len */
size_t arrays_calc_node_size(uint16_t key_len, uint32_t list_len);

void arrays_free_node(arrays_node_t *node);

#endif // ARRAYS_H
