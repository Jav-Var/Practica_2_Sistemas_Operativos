#ifndef ARRAYS_H
#define ARRAYS_H

#include <stdint.h>
#include "common.h"

typedef struct {
    uint16_t key_len;    // tamaño de la key (titulo)  
    char *key;           // key (titulo)
    off_t entry_offset;  // offset (en el csv) del libro 
    off_t next_ptr;      // siguiente puntero de la lista enlazada
} arrays_node_t;

// Crea el archivo de nodos
int arrays_create(const char *path);

// Abre el archivo de nodos, retorna el file descriptor
int arrays_open(const char *path);

// Añade un nodo y retorna su offset (Retorna offset 0 en caso de error)
off_t arrays_append_node(int fd, const arrays_node_t *node);

// Lee los datos de un nodo
int arrays_read_node_full(int fd, off_t node_off, arrays_node_t *node);

// Retorna el tamaño en bytes de un nodo
size_t arrays_calc_node_size(uint16_t key_len);

// Libera los datos de un struct arrays_node_t
void arrays_free_node(arrays_node_t *node);

#endif // ARRAYS_H
