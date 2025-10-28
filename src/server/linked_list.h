#ifndef LINKED_LIST_H
#define LINKED_LIST_H

#include <stdint.h>
#include "common.h"

typedef struct {
    uint16_t key_len;    // tamaño de la key (titulo)  
    char *key;           // key (titulo)
    off_t entry_offset;  // offset (en el csv) del libro 
    off_t next_ptr;      // siguiente puntero de la lista enlazada
} linked_list_node_t;

// Crea el archivo de nodos
int linked_list_nodes_create(const char *path);

// Abre el archivo de nodos, retorna el file descriptor
int linked_list_open(const char *path);

// Añade un nodo y retorna su offset (Retorna offset 0 en caso de error)
off_t linked_list_append_node(int fd, const linked_list_node_t *node);

// Lee los datos de un nodo
int linked_list_read_node(int fd, off_t node_off, linked_list_node_t *node);

// Retorna el tamaño en bytes de un nodo
size_t linked_list_node_size(uint16_t key_len);

// Libera los datos de un struct linked_list_node_t
void linked_list_free_node(linked_list_node_t *node);

#endif // LINKED_LIST_H
