#include "reader.h"
#include "buckets.h"
#include "arrays.h"
#include "common.h"
#include "hash.h"
#include "util.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>

// inserta en el handle (struct) la informacion del indice
int index_open(index_handle_t *h, const char *buckets_path, const char *arrays_path) {
    int bfd = buckets_open_readwrite(buckets_path);
    if (bfd < 0) return -1;
    int afd = arrays_open(arrays_path);
    if (afd < 0) { close(bfd); return -1; }
    h->buckets_fd = bfd; // Buckets file descriptor
    h->arrays_fd = afd;  // Nodes file descriptor (arrays)
    return 0;
}

void index_close(index_handle_t *h) {
    if (h == NULL) return;
    close(h->buckets_fd);
    close(h->arrays_fd);
    h->buckets_fd = -1;
    h -> arrays_fd = -1;
}

int index_lookup(index_handle_t *h, const char *key, off_t **out_offsets, uint32_t *out_count) {
    if (!h || !key || !out_offsets || !out_count) {
        return -1;
    }
    *out_offsets = NULL;
    *out_count = 0;

    printf("Buscando: %s\n", key);

    // Halla el bucket a partir del hash
    uint64_t hval = hash_key_prefix(key, strlen(key), DEFAULT_HASH_SEED);
    uint64_t mask = NUM_BUCKETS - 1;
    uint64_t bucket = bucket_id_from_hash(hval, mask);

    // Obtiene el offset de la cabeza de la lista enlazada
    off_t head = buckets_read_head(h->buckets_fd, bucket); 
    
    if (head == 0) { // offset 0 representa null
        fprintf(stderr, "Error: No se pudo leer el bucket\n");
        return 0;
    }

    // Array dinamico de resultados
    uint32_t cap = 16; // Capacidad del array de resultados
    uint32_t cnt = 0;  // Numero de resultados

    off_t *results = malloc(sizeof(off_t) * cap);
    if (results == NULL) return -1;
    
    off_t cur = head; 
    char *normalized_key = normalize_string(key); // Solo usamos la llave normalizada
    if (!normalized_key){
        free(normalized_key);
        return -1;
    }
    size_t nkey_len = strlen(normalized_key);
    while (cur != 0) { // Recorre la lista enlazada
        arrays_node_t node = {.key_len = 0, .key = NULL, .entry_offset = 0, .next_ptr = 0};
        if (arrays_read_node_full(h->arrays_fd, cur, &node) != 0) { // Lee los datos del nodo
            fprintf(stderr, "Error, no se pudo leer los datos del nodo\n");
            break;
        }

        off_t next = node.next_ptr; 
        printf("Leyendo nodo con key %s\n", node.key); //.

        if (node.key) { 
            printf("Read: %s\n", node.key); //
            if (strncmp(node.key, normalized_key,nkey_len) == 0) { // nota: node.key ya es una llave normalizada
                if (cnt >= cap) { // Si se excede el tamaño del array dinamico, realocar con doble de tamaño
                    uint32_t new_cap = cap * 2;
                    off_t *tmp = realloc(results, sizeof(off_t) * new_cap); // Copia del array con doble de tamaño
                    if (tmp == NULL) {
                        arrays_free_node(&node);
                        free(results);
                        return -1;
                    }
                    results = tmp;
                    cap = new_cap;
                }
                results[cnt] = node.entry_offset;
                cnt++;
            }
        }

        /* free node buffers */
        arrays_free_node(&node);
        cur = next;
    }

    free(normalized_key);
    if (cnt == 0) {
        free(results);
        *out_offsets = NULL;
        *out_count = 0;
    } else {
        *out_offsets = results;
        *out_count = cnt;
    }
    return 0;
}
