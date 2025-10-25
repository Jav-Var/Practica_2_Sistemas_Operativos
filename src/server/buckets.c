#include "buckets.h"
#include "common.h"
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>
#include <fcntl.h>

// Crea el archivo de buckets
int buckets_create(const char *path) {
    if (mkdir("data/index", 0755) < 0 && errno != EEXIST) { // Crea un directorio si hace falta, si ya existe ignora el error
        printf("mkdir data/index failed: %s\n", strerror(errno));
        return -1;
    }
    int fd = open(path, O_CREAT | O_TRUNC | O_RDWR, 0644); // File descriptor del archivo de buckets
    if (fd < 0) {
        printf("open %s fallo al crear archivo de buckets %s\n", path, strerror(errno));
        return -1;
    }
    off_t entries_offset = 0; // Llenamos de ceros desde el inicio del archivo
    size_t entries_size = (size_t)NUM_BUCKETS * BUCKET_ENTRY_SIZE;

    int r = posix_fallocate(fd, entries_offset, entries_size); // posix_fallocate para preasignar ceros al archivo
    if (r != 0) {
        fprintf(stderr, "posix_fallocate fallo al crear archivo de buckets: %s\n", strerror(r));
    }
    fsync(fd);
    close(fd);
    return 0;
}

// Abre el archivo de buckets, retorna el file descriptor del archivo
int buckets_open_readwrite(const char *path) { // To do: Revisar si es factible puede borrar esta funcion y solo hacer open cuando se vaya a abrir el archivo 
    int fd = open(path, O_RDWR);
    if (fd < 0) return -1;
    return fd;
}

// Retorna el offset dado un bucket_id (El hash despues de truncarlo al numero de buckets)
off_t buckets_entry_offset(uint64_t bucket_id) { // To do: Revisar si es factible eliminar esta funcion y implementar el calculo en lugar de llamar la funcion
    return (off_t)bucket_id * BUCKET_ENTRY_SIZE;
}

// Lee el archivo de buckets en un bucket_id
off_t buckets_read_head(int fd, uint64_t bucket_id) {
    if (bucket_id >= NUM_BUCKETS) {
        fprintf(stderr, "Se intento acceder a un bucket fuera del rango del archivo\n");
        return 0;
    }
    off_t pos = buckets_entry_offset(bucket_id); // To do: Revisar si se puede implementar la funcion aqui mismo (Una sola linea)
    unsigned char buf[8]; // To do: Revisar si esto debe ser 'char'
    if (safe_pread(fd, buf, 8, pos) != 8) {
        fprintf(stderr, "Error, no se pudo leer el offset del nodo\n");
        return 0;
    }
    off_t bucket_offset;
    memcpy(&bucket_offset, buf, sizeof(bucket_offset));
    return bucket_offset;
}

// Escribe 'head' en el bucket_id dado
int buckets_write_head(int fd, uint64_t num_buckets, uint64_t bucket_id, off_t head) {
    if (bucket_id >= NUM_BUCKETS) {
        fprintf(stderr, "Se intento acceder a un bucket fuera del rango del archivo\n");
        return 1;
    }
    off_t pos = buckets_entry_offset(bucket_id);
    if (safe_pwrite(fd, &head, 8, pos) != 8) return -1;
    return 0;
}
