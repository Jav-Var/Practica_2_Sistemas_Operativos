#define _GNU_SOURCE
#include "util.h"
#include "reader.h"
#include "builder.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <errno.h>

#define DEFAULT_HASH_SEED 0x12345678abcdefULL
#define REQ_FIFO "/tmp/index_req.fifo"
#define RSP_FIFO "/tmp/index_rsp.fifo"
#define BUF_SZ 8192

/* ensure pipe exists */
static int ensure_fifo(const char *path) {
    struct stat st;
    if (stat(path, &st) == 0) {
        if (!S_ISFIFO(st.st_mode)) {
            fprintf(stderr, "La ruta existe y no es un FIFO: %s\n", path);
            return -1;
        }
        return 0;
    }
    if (mkfifo(path, 0666) != 0) {
        perror("mkfifo");
        return -1;
    }
    return 0;
}

static char *read_line_fd(int fd) {
    char *buf = malloc(BUF_SZ);
    if (!buf) return NULL;
    size_t pos = 0;
    while (1) {
        ssize_t r = read(fd, buf + pos, 1);
        if (r <= 0) {
            if (pos == 0) { free(buf); return NULL; } 
            break;
        }
        if (buf[pos] == '\n') { buf[pos] = '\0'; return buf; }
        pos++;
        if (pos + 1 >= BUF_SZ) break; // if too long, truncate
    }
    buf[pos] = '\0';
    return buf;
}

static int write_line_fd(int fd, const char *s) {
    size_t len = strlen(s);
    ssize_t w = write(fd, s, len);
    if (w != (ssize_t)len) return -1;
    if (write(fd, "\n", 1) != 1) return -1;
    return 0;
}

int main() {
    const char *index_dir = INDEX_DIR;
    const char *csv_path = CSV_PATH;

    if (ensure_fifo(REQ_FIFO) != 0) return 1;
    if (ensure_fifo(RSP_FIFO) != 0) return 1;

    int req_fd = open(REQ_FIFO, O_RDWR);
    if (req_fd < 0) { printf("fifo de peticiones"); return 1; }
    int rsp_fd = open(RSP_FIFO, O_RDWR);
    if (rsp_fd < 0) { perror("abrir fifo de respuestas"); close(req_fd); return 1; }

    char title_buckets[1024], title_arrays[1024], author_buckets[1024], author_arrays[1024];
    snprintf(title_buckets, sizeof(title_buckets), "%s/title_buckets.dat", index_dir);
    snprintf(title_arrays, sizeof(title_arrays), "%s/title_arrays.dat", index_dir);
    snprintf(author_buckets, sizeof(author_buckets), "%s/author_buckets.dat", index_dir);
    snprintf(author_arrays, sizeof(author_arrays), "%s/author_arrays.dat", index_dir);

    int need_build = 0;
    if (access(title_buckets, F_OK) != 0) {
        printf("Falta archivo de índice: %s\n", title_buckets);
        need_build = 1;
    }
    if (access(author_arrays, F_OK) != 0) {
        printf("Falta archivo de índice: %s\n", author_arrays);
        need_build = 1;
    }

    if (need_build) {
        uint64_t num_buckets_title = next_pow2(4096);
        uint64_t num_buckets_author = next_pow2(4096);
        uint64_t hash_seed = DEFAULT_HASH_SEED;
        printf("Construyendo índices en '%s'...\n", INDEX_DIR);
        if (build_both_indices_stream(CSV_PATH, INDEX_DIR, num_buckets_title, num_buckets_author, hash_seed) != 0) {
            fprintf(stderr, "Fallo al construir los índices\n");
            return 1;
        }
        printf("Índices construidos.\n");
    } else {
        printf("Archivos de índice encontrados.\n");
    }

    index_handle_t th, ah;
    if (index_open(&th, title_buckets, title_arrays) != 0) {
        fprintf(stderr, "Fallo al abrir el índice de títulos\n");
    }
    if (index_open(&ah, author_buckets, author_arrays) != 0) {
        fprintf(stderr, "Fallo al abrir el índice de autores\n");
    }
    printf("Esperando peticiones de busqueda\n");
    fflush(stdout);

    FILE *csvf = fopen(csv_path, "rb");
    if(!csvf) {
        printf("Fatal: No se puede abrir el archivo CSV %s:\n", csv_path);
        close(req_fd);
        close(rsp_fd);
        return 1;
    }
    
    while (1) {
        char *req = read_line_fd(req_fd);
        if (!req) {
            usleep(100000);
            continue;
        }
        char *sep = strchr(req, '|');
        char *title = NULL;
        char *author = NULL;
        if (sep) {
            *sep = '\0';
            title = req;
            author = sep + 1;
        } else {
            title = req;
            author = "";
        }

        if ((title[0] == '\0') && (author[0] == '\0')) {
            write_line_fd(rsp_fd, "ERR|La búsqueda debe tener al menos un parámetro");
            write_line_fd(rsp_fd, "<END>");
            free(req);
            continue;
        }

        printf("Buscando título: '%s', autor: '%s'\n", title, author);
        off_t *offs = NULL;
        uint32_t count = 0;
        int rc = lookup_by_title_author(&th, &ah, title, author, &offs, &count);
        if (rc != 0) {
            write_line_fd(rsp_fd, "ERR|Error interno en la búsqueda");
            write_line_fd(rsp_fd, "<END>");
            free(req);
            continue;
        }
        

        if (count == 0) {
            write_line_fd(rsp_fd, "OK");
            write_line_fd(rsp_fd, "<END>");
            free(req);
            continue;
        }

        
        if (!csvf) {
            write_line_fd(rsp_fd, "ERR|No se puede abrir el archivo CSV");
            write_line_fd(rsp_fd, "<END>");
            free(offs);
            free(req);
            continue;
        }

        write_line_fd(rsp_fd, "OK");
        for (uint32_t i = 0; i < count; ++i) {
            off_t off = (off_t)offs[i];
            if (fseeko(csvf, off, SEEK_SET) != 0) {
                continue;
            }
            char *line = NULL;
            size_t llen = 0;
            ssize_t r = getline(&line, &llen, csvf);
            if (r > 0) {
                if (line[r-1] == '\n') line[r-1] = '\0';
                write_line_fd(rsp_fd, line);
            }
            free(line);
        }
        write_line_fd(rsp_fd, "<END>");

        free(offs);
        free(req);
    }
    fclose(csvf);
    index_close(&th);
    index_close(&ah);
    close(req_fd);
    close(rsp_fd);
    return 0;
}