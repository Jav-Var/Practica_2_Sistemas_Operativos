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

#define REQ_FIFO "/tmp/index_req.fifo"
#define RSP_FIFO "/tmp/index_rsp.fifo"
#define MAX_LINE 8192


int main() {
    /* Check FIFOs existence */
    if (access(REQ_FIFO, F_OK) != 0 || access(RSP_FIFO, F_OK) != 0) {
        fprintf(stderr, "Error: no se encuentran las FIFOs (%s, %s).\n"
                        "Asegúrate de que index_server esté corriendo y haya creado las FIFOs.\n",
                REQ_FIFO, RSP_FIFO);
        return -1;
    }

    int req_fd = open(REQ_FIFO, O_RDWR);
    if (req_fd < 0) {
        fprintf(stderr, "No pude abrir FIFO de peticiones '%s': %s\n", REQ_FIFO, strerror(errno));
        return 1;
    }
    int rsp_fd = open(RSP_FIFO, O_RDWR);
    if (rsp_fd < 0) {
        fprintf(stderr, "No pude abrir FIFO de respuestas '%s': %s\n", RSP_FIFO, strerror(errno));
        close(req_fd);
        return 1;
    }

    char *current_title = NULL;
    char *current_author = NULL;

    while (1) {
        printf("\n\tMenu de busqueda\n\n");
        printf("Título actual: %s\n", display_or_empty(current_title));
        printf("Autor actual : %s\n", display_or_empty(current_author));
        printf("\n1. Ingresar titulo\n");
        printf("2. Ingresar autor\n");
        printf("3. Realizar Busqueda\n");
        printf("4. Salir\n");
        printf("Selecciona una opción: ");
        fflush(stdout);

        char *opt = getline_trimmed_stdin();
        if (!opt) {
            printf("\nEOF o error de entrada. Saliendo.\n");
            break;
        }

        if (strcmp(opt, "1") == 0) {
            printf("Ingrese titulo (enter para dejar vacío): ");
            char *t = getline_trimmed_stdin();
            /*if (t == NULL || t[0] == '\0') {
                printf("Se ingreso (vacio)");
            }
            else {
            printf("Se ingreso %s\n", t);
            }*/ // La interfaz queda mas limpia sin este mensaje
            //printf("Se ingreso %s\n", t ? t : "(vacio)");
            if (t && t[0] == '\0') { free(t); t = NULL; }
            free(current_title);
            current_title = t;
        } else if (strcmp(opt, "2") == 0) {
            printf("Ingrese autor (enter para dejar vacío): ");
            char *a = getline_trimmed_stdin();
            printf("Se ingreso %s\n", a);
            if (a && a[0] == '\0') { free(a); a = NULL; }
            free(current_author);
            current_author = a;
        } else if (strcmp(opt, "3") == 0) {
            const char *t = current_title ? current_title : "";
            const char *a = current_author ? current_author : "";
            if (t[0] == '\0' && a[0] == '\0') {
                printf("Error: la búsqueda debe tener al menos un parámetro (titulo o autor).\n");
                free(opt);
                continue;
            }

            /* Compose request safely (use empty strings if NULL) */
            char req[MAX_LINE];
            snprintf(req, sizeof(req), "%s|%s", t, a);

            if (write_line_fd(req_fd, req) != 0) {
                fprintf(stderr, "Error escribiendo petición en FIFO: %s\n", strerror(errno));
            } else {
                /* Read response and print */
                read_and_print_response(rsp_fd);
            }
        } else if (strcmp(opt, "4") == 0) {
            free(opt);
            break;
        } else {
            printf("Opción no válida. Por favor elige 1..4.\n");
        }

        free(opt);
    }

    free(current_title);
    free(current_author);
    close(req_fd);
    close(rsp_fd);
    printf("Cliente finalizado.\n");
    return 0;
}
