#include "arrays.h"
#include "common.h"
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/stat.h>
#include <errno.h>

// Crea el archivo de nodos
int arrays_create(const char *path) {
    int fd = open(path, O_CREAT | O_TRUNC | O_RDWR, 0644);
    /* O_CREAT: creates the file if it doesn't exist.
     * O_TRUNC: if it exists, it truncates it to length 0 (deletes its content).
     * O_RDWR: opens it in read/write mode (necessary for pread/pwrite).
     */
    if (fd < 0) {
        printf("open %s fallo al crear archivo de nodos %s\n", path, strerror(errno));
        return -1;
    }
    fsync(fd);
    close(fd);
    return 0;
}

// Abre el archivo de nodos, retorna el file descriptor
int arrays_open(const char *path) { // Revisar si es factible eliminar esta funcion
    int fd = open(path, O_RDWR);
    return fd;
}

// Retorna el tamaño en bytes de un nodo
size_t arrays_calc_node_size(uint16_t key_len) {
    // Tamaño de key_len + key + entry_offset + next_ptr
    return sizeof(uint16_t) + (size_t)key_len + sizeof(off_t) + sizeof(off_t);
}

// Añade un nodo al archivo, retorna el offset del nodo retorna offset -1 si hay error
off_t arrays_append_node(int fd, const arrays_node_t *node) {
    if (!node || !node->key) return -1; // Cambiar a una macro (como NULL_NODE)
    uint16_t key_len = node -> key_len; // Cantidad de caracteres de la key (titulo)
    size_t node_size = arrays_calc_node_size(key_len);
    unsigned char *buf = malloc(node_size); 
    if (buf == NULL) {
        return -1;
    }
    size_t pos = 0; 

    memcpy(buf + pos, &key_len, sizeof(key_len)); // Escribe key_len (en el buffer)
    pos += sizeof(key_len);

    memcpy(buf + pos, node->key, key_len); // Escribe la key (titulo)
    pos += key_len;

    memcpy(buf + pos, node->entry_offset, sizeof(node->entry_offset));
    pos += sizeof(node->entry_offset);

    memcpy(buf + pos, &node->next_ptr, sizeof node->next_ptr);
    pos += sizeof(uint64_t);

    off_t new_node_off = lseek(fd, 0, SEEK_END); // Devuelve el offset del final del archivo
    if (new_node_off == (off_t)-1) {
        free(buf);
        return 0;
    }
    if (safe_pwrite(fd, buf, node_size, new_node_off) != (ssize_t)node_size) {
        free(buf);
        return 0;
    }
    free(buf);
    fsync(fd);
    return (off_t)new_node_off;
}

// Lee la informacion de un nodo (en el archivo de nodos) a un struct
int arrays_read_node_full(int fd, off_t node_off, arrays_node_t *node) {
    if (node == NULL) return -1; 

    // Leer el tamaño de la key
    uint16_t key_len;
    if (safe_pread(fd, &key_len, sizeof(uint16_t), node_off) != (ssize_t)sizeof(uint16_t)) {
        return -1;
    }
    node->key_len = key_len;

    // Reservar memoria para la key (+1 byte para '\0')
    node->key = malloc(key_len + 1);
    if (node->key == NULL) {
        return -1;
    }
    // Leer la key
    off_t key_off = node_off + sizeof(uint16_t);
    if (safe_pread(fd, node->key, key_len, key_off) != key_len) {
        free(node->key);
        return -1;
    }
    node->key[key_len] = '\0';  // asegurarse de que la cadena esté terminada en '\0'

    // Leer entry_offset
    off_t entry_offset_off = key_off + key_len;
    if (safe_pread(fd, &node->entry_offset, sizeof(off_t), entry_offset_off) != (ssize_t)sizeof(off_t)) {
        free(node->key);
        return -1;
    }

    // Leer next_ptr
    off_t next_ptr_off = entry_offset_off + sizeof(off_t);
    if (safe_pread(fd, &node->next_ptr, sizeof(off_t), next_ptr_off) != (ssize_t)sizeof(off_t)) {
        free(node->key);
        return -1;
    }

    return 0;
}


void arrays_free_node(arrays_node_t *node) {
    if (!node) return;
    if (node->key) { free(node->key); node->key = NULL; }
    node->key_len = 0;
    node->next_ptr = 0;
}