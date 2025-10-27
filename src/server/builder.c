#define _GNU_SOURCE
#include "builder.h"
#include "buckets.h"
#include "arrays.h"
#include "common.h"
#include "hash.h"
#include "util.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>

// Extrae un campo de una linea formato csv, si no lo encuentra retorna NULL
static char *csv_get_field_copy(const char *line, int field_idx) {
    // Esta funcion tambien puede ser util en el cliente, si se necesita, mover a common
    if (line == NULL || field_idx < 0) return NULL;
    size_t pos = 0;
    size_t field_len = 0;
    int current_field = 0;
    while (current_field != field_idx) { // Busca el inicio del campo
        if (line[pos] == '\0') return NULL;
        if (line[pos] == ',')  current_field++;
        pos++;
    }

    const char *start = line + pos;
    while (start[field_len] != ',' && start[field_len] != '\0') { // Busca hasta la siguiente coma (Obtiene la longitud del campo)
        field_len++;
    }

    char *output_field = malloc(field_len + 1); // Se reserva un espacio adicional para '\0'
    if (output_field == NULL) { // Si ocurre error de malloc
        perror("malloc");
        return NULL;
    }
    memcpy(output_field, start, field_len);
    output_field[field_len] = '\0';
    return output_field;
}

//Similiar a build_index_stream, pero solo para una línea
int build_index_line(const char *csv_path, const char *line) {

    char buckets_path[1024] = "data/index/title_buckets.dat";
    char arrays_path[1024] = "data/index/title_arrays.dat";

    int bfd = buckets_open_readwrite(buckets_path); // buckets file descriptor
    if (bfd < 0) {
        fprintf(stderr,"open buckets failed\n");
        return -1;
    } 
    int afd = arrays_open(arrays_path); // nodes file descriptor
    if (afd < 0) { 
        close(bfd);
        fprintf(stderr,"open arrays failed\n");
        return -1;
    }

    FILE *csv_fp = fopen(csv_path, "a+"); // Abre el dataset
    if (!csv_fp) {
        close(bfd);
        close(afd);
        fprintf(stderr,"open csv failed\n");
        return -1;
    }

    if (fseeko(csv_fp, 0, SEEK_END) != 0) {
        perror("fseeko");
        fclose(csv_fp);
        close(bfd);
        close(afd);
        return -1;
    }

    off_t start_offset = ftello(csv_fp);
    if (start_offset == (off_t)-1) {
        perror("ftello");
        fclose(csv_fp);
        close(bfd);
        close(afd);
        return -1;
    }

    // Escribir la línea en el archivo CSV
    if (fprintf(csv_fp, "%s\n", line) < 0) {
        perror("fprintf");
        return -1;
    }

    fflush(csv_fp);

    //Indexar la linea
    char *title = csv_get_field_copy(line, TITLE_FIELD); // Obtiene titulo de la linea
        if (title == NULL){
            return -1;
        }

        // printf("Añadiendo %s al indice\n", title); // Para debuggear

        char *normalized_title = normalize_string(title); // Obtiene el titulo normalizado (Solo alfanumericos)
        free(title);
        if (normalized_title == NULL) {
            return -1;
        }

        // Hash 
        uint64_t h = hash_key_prefix(normalized_title, strlen(normalized_title), DEFAULT_HASH_SEED);
        uint64_t mask = NUM_BUCKETS - 1;
        uint64_t bucket = bucket_id_from_hash(h, mask);

        // Obtiene la cabeza de la lista enlazada
        off_t old_head = buckets_read_head(bfd, bucket);

        // Inserta los datos del nodo en el archivo
        arrays_node_t node;
        node.key_len = (uint16_t)strlen(normalized_title);
        node.key = strdup(normalized_title);
        node.entry_offset = start_offset;
        node.next_ptr = old_head;      

        off_t new_node_off = arrays_append_node(afd, &node);
        free(normalized_title);

        if (new_node_off == 0) {
            fprintf(stderr, "Error al insertar nodo\n");
            arrays_free_node(&node);
            return -1;
        }
        
        arrays_free_node(&node);
        
        if (buckets_write_head(bfd, bucket, new_node_off) != 0) {
            fprintf(stderr, "failed write bucket head\n");
        }

        fclose(csv_fp);
        close(bfd);
        close(afd);
        return 0;
}

/* Construye los archivos de indices */
int build_index_stream(const char *csv_path) {
    char buckets_path[1024] = "data/index/title_buckets.dat";
    char arrays_path[1024] = "data/index/title_arrays.dat";

    // Verificar si se puede eliminar buckets_create y simplemente implementar aqui
    if (buckets_create(buckets_path) != 0) {
        fprintf(stderr, "Failed to create buckets file %s\n", buckets_path);
        return -1;
    }

    // Verificar si se puede eliminar arrays_create y simplemente implementar aqui
    if (arrays_create(arrays_path) != 0) {
        fprintf(stderr, "Failed to create arrays file %s\n", arrays_path);
        return -1;
    }

    int bfd = buckets_open_readwrite(buckets_path); // buckets file descriptor
    if (bfd < 0) {
        fprintf(stderr,"open buckets failed\n");
        return -1;
    } 
    int afd = arrays_open(arrays_path); // nodes file descriptor
    if (afd < 0) { 
        close(bfd);
        fprintf(stderr,"open arrays failed\n");
        return -1;
    }

    FILE *csv_fp = fopen(csv_path, "rb"); // Abre el dataset
    if (!csv_fp) {
        close(bfd);
        close(afd);
        fprintf(stderr,"open csv failed\n");
        return -1;
    }

    char *line = NULL;
    size_t line_size = 0; // Tamaño del buffer line
    ssize_t read_bytes; // Cantidad de bytes leidos

    read_bytes = getline(&line, &line_size, csv_fp); // Descarta la primera linea
    if (read_bytes == -1) {
        if (feof(csv_fp)) { // No hay contenido en el archivo
            fprintf(stderr, "Error: El archivo csv esta vacio\n");
            free(line);
            fclose(csv_fp);
            return 0; 
        } else { 
            fprintf(stderr, "Error al leer el csv\n");
            free(line);
            fclose(csv_fp);
            return -1;
        }
    }

    // Itera sobre las lineas del csv
    while (1) {
        off_t start_offset = ftello(csv_fp); // posición en bytes del comienzo de la linea
        if (start_offset == (off_t) -1) {
            perror("ftello");
            break;
        }

        read_bytes = getline(&line, &line_size, csv_fp);
        if (read_bytes == -1) { 
            if (feof(csv_fp)) break; // Fin del archivo
            perror("getline"); // Si error
            break;
        }
        char *title = csv_get_field_copy(line, TITLE_FIELD); // Obtiene titulo de la linea
        if (title == NULL) continue; 
        printf("Añadiendo %s a la tabla hash\n", title);

        char *normalized_title = normalize_string(title); // Obtiene el titulo normalizado (Solo alfanumericos)
        free(title);
        if (normalized_title == NULL) continue;

        // Hash 
        uint64_t h = hash_key_prefix(normalized_title, strlen(normalized_title), DEFAULT_HASH_SEED);
        uint64_t mask = NUM_BUCKETS - 1;
        uint64_t bucket = bucket_id_from_hash(h, mask);

        // Obtiene la cabeza de la lista enlazada
        off_t old_head = buckets_read_head(bfd, bucket);

        // Inserta los datos del nodo en el archivo
        arrays_node_t node;
        node.key_len = (uint16_t)strlen(normalized_title);
        node.key = strdup(normalized_title);
        node.entry_offset = start_offset;
        node.next_ptr = old_head;      
        off_t new_node_off = arrays_append_node(afd, &node);
        free(normalized_title);
        printf("Datos del nodo leidos\n");

        if (new_node_off == 0) {
            fprintf(stderr, "Error al insertar nodo\n");
            arrays_free_node(&node);
            continue;
        }
        
        arrays_free_node(&node);
        
        if (buckets_write_head(bfd, bucket, new_node_off) != 0) {
            fprintf(stderr, "failed write bucket head\n");
        }
    }
    free(line);
    fclose(csv_fp);
    close(bfd);
    close(afd);
    return 0;
}
  