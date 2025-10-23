#define _GNU_SOURCE
#include "common.h"
#include "ui.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <errno.h>
#include <stdbool.h>

/* Trim trailing newline/carriage return */
void rtrim_newline(char *s) {
    if (!s) return;
    size_t L = strlen(s);
    while (L > 0 && (s[L-1] == '\n' || s[L-1] == '\r')) {
        s[--L] = '\0';
    }
}

/* returns (vacio) if the string is empty */
const char *display_or_empty(const char *s) {
    return (s != NULL && s[0] != '\0') ? s : "(vacío)";
}

/* write a line (without trailing newline) to fd and append '\n' */
int write_line_fd(int fd, const char *s) {
    size_t len = strlen(s);
    ssize_t w = write(fd, s, len);
    if (w != (ssize_t)len) return -1;
    if (write(fd, "\n", 1) != 1) return -1;
    return 0;
}

void press_enter_to_continue() {
    printf("Presione enter para continuar.");
    fflush(stdout);
    int c;
    while ((c = getchar()) != '\n' && c != EOF) { }
}

void print_record(const char *record) {
    if (!record) {
        return;
    }

    char *copy = strdup(record);
    if (!copy) {
        printf("\t<OOM>\n");
        return;
    }

    /* Punteros a los 14 campos (0..12 primeros, 13 = genres) */
    char *fields[NUM_DATASET_FIELDS];
    for (int k = 0; k < NUM_DATASET_FIELDS; ++k) fields[k] = NULL;

    char *p = copy;
    char *comma;
    for (int i = 0; i < NUM_DATASET_FIELDS - 1; ++i) {
        comma = strchr(p, ',');
        if (!comma) break;
        *comma = '\0';
        fields[i] = p;
        p = comma + 1;
    }
    /* todo lo que quede es genres (puede contener comas) */
    fields[NUM_DATASET_FIELDS - 1] = p;

    /* imprimir tal cual (si un campo es NULL o vacío, se imprime vacío) */
    printf("- Titulo: %s\n", (fields[0] ? fields[0] : ""));
    printf("- Autor: %s\n", (fields[1] ? fields[1] : ""));
    printf("- Image URL: %s\n", (fields[2] ? fields[2] : ""));
    printf("- Numero de Paginas: %s\n", (fields[3] ? fields[3] : ""));
    printf("- Calificacion media: %s\n", (fields[4] ? fields[4] : ""));
    printf("- Numero reseñas (texto): %s\n", (fields[5] ? fields[5] : ""));
    printf("- Descripcion: %s\n", (fields[6] ? fields[6] : ""));
    printf("- Calificaciones 5 estrellas: %s\n", (fields[7] ? fields[7] : ""));
    printf("- Calificaciones 4 estrellas: %s\n", (fields[8] ? fields[8] : ""));
    printf("- Calificaciones 3 estrellas: %s\n", (fields[9] ? fields[9] : ""));
    printf("- Calificaciones 2 estrellas: %s\n", (fields[10] ? fields[10] : ""));
    printf("- Calificaciones 1 estrella: %s\n", (fields[11] ? fields[11] : ""));
    printf("- Total calificaciones: %s\n", (fields[12] ? fields[12] : ""));
    printf("- Generos: %s\n", (fields[13] ? fields[13] : ""));

    free(copy);
}


/* Read response from rsp_fd and print until "<END>" is found.
   Uses fdopen on a dup so it doesn't close original fd used elsewhere. */
void read_and_print_response(int rsp_fd) {
    int dupfd = dup(rsp_fd);
    if (dupfd < 0) {
        perror("dup(rsp_fd)");
        return;
    }
    FILE *f = fdopen(dupfd, "r");
    if (!f) {
        perror("fdopen");
        close(dupfd);
        return;
    }
    char buf[MAX_LINE];
    int rec_count = 0;
    bool header_printed = false;
    while (fgets(buf, sizeof(buf), f)) {
        rtrim_newline(buf);
        if (strcmp(buf, "<END>") == 0) {
            break; 
        }
        if (strncmp(buf, "ERR|", 4) == 0) {
            printf("ERROR (server): %s\n", buf + 4);
            continue;
        }
        if (strcmp(buf, "OK") == 0) {
            continue;
        }
        rec_count++;
        if (!header_printed) {
            printf("\n\tSe encontraron los siguientes resultados:\n\n");
            header_printed = true;
        }
        printf("Resultado %d:\n", rec_count);
        print_record(buf);
        printf("\n");
    }
    if (ferror(f)) {
        perror("Error leyendo respuesta");
    }
    fclose(f);
    if (rec_count == 0) {
        printf("No se encontraron resultados\n");
    } 
    press_enter_to_continue();
}

/* Read a trimmed line from stdin (malloc'd). Caller must free.
   Returns NULL on EOF or error. */
char *getline_trimmed_stdin(void) {
    char *line = NULL;
    size_t len = 0;
    ssize_t r = getline(&line, &len, stdin);
    if (r <= 0) {
        if (line) free(line);
        return NULL;
    }
    rtrim_newline(line);
    char *s = line;
    while (*s && (*s == ' ' || *s == '\t')) s++;
    if (s != line) memmove(line, s, strlen(s) + 1);
    size_t L = strlen(line);
    while (L > 0 && (line[L-1] == ' ' || line[L-1] == '\t')) line[--L] = '\0';
    return line;
}