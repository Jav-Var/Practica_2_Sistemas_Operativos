#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include "common.h" // Para off_t (aunque sys/types.h lo trae)

#define SERVER_IP "127.0.0.1" // IP del servidor (localhost)
#define SERVER_PORT 8080
#define MAX_QUERY_LEN 1024

/**
 * @brief Se conecta al servidor, envía la consulta y muestra los resultados.
 */
static int run_query(const char *query) {
    // --- 1. Conectar al Servidor ---
    int sock_fd;
    struct sockaddr_in server_addr;

    sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (sock_fd < 0) {
        perror("socket");
        return -1;
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
    if (inet_pton(AF_INET, SERVER_IP, &server_addr.sin_addr) <= 0) {
        perror("inet_pton (IP inválida)");
        close(sock_fd);
        return -1;
    }

    if (connect(sock_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("connect (¿Está el servidor corriendo?)");
        close(sock_fd);
        return -1;
    }

    printf("Conectado al servidor. Enviando consulta: '%s'\n", query);

    // --- 2. Enviar Petición ---
    
    uint32_t query_len = (uint32_t)strlen(query);
    // uint32_t net_len = htonl(query_len); // Opcional: endianness

    // Enviar longitud
    if (write(sock_fd, &query_len, sizeof(query_len)) != sizeof(query_len)) {
        perror("write (longitud)");
        close(sock_fd);
        return -1;
    }
    // Enviar consulta
    if (write(sock_fd, query, query_len) != (ssize_t)query_len) {
        perror("write (consulta)");
        close(sock_fd);
        return -1;
    }

    // --- 3. Recibir Respuesta ---

    // Leer el conteo de resultados
    int32_t result_count;
    if (read(sock_fd, &result_count, sizeof(result_count)) != sizeof(result_count)) {
        perror("read (conteo)");
        close(sock_fd);
        return -1;
    }
    // result_count = ntohl(result_count); // Opcional: endianness

    if (result_count < 0) {
        fprintf(stderr, "Error en el servidor al procesar la consulta.\n");
        close(sock_fd);
        return -1;
    }
    
    if (result_count == 0) {
        printf("No se encontraron resultados para '%s'.\n", query);
        close(sock_fd);
        return 0;
    }

    printf("Recibidos %d resultados (offsets en el CSV):\n", result_count);
    
    // Leer los offsets
    size_t results_size = result_count * sizeof(off_t);
    off_t *offsets = malloc(results_size);
    if (!offsets) {
        perror("malloc");
        close(sock_fd);
        return -1;
    }

    if (read(sock_fd, offsets, results_size) != (ssize_t)results_size) {
        perror("read (offsets)");
        free(offsets);
        close(sock_fd);
        return -1;
    }

    // --- 4. Mostrar Resultados ---
    for (int i = 0; i < result_count; i++) {
        printf("  -> Resultado %d en offset: %ld\n", i + 1, (long)offsets[i]);
    }
    
    // NOTA: Aquí iría la lógica futura para usar estos offsets,
    // volver a conectar y pedir los datos completos del CSV.

    // --- 5. Limpieza ---
    free(offsets);
    close(sock_fd);
    return 0;
}


int main(int argc, char *argv[]) {
    // Modo 1: Consulta por argumento
    if (argc > 1) {
        // Unir todos los argumentos en una sola consulta
        char query_buf[MAX_QUERY_LEN] = {0};
        size_t current_len = 0;
        for (int i = 1; i < argc; i++) {
            size_t arg_len = strlen(argv[i]);
            if (current_len + arg_len + 1 > MAX_QUERY_LEN) {
                fprintf(stderr, "Consulta demasiado larga.\n");
                return 1;
            }
            memcpy(query_buf + current_len, argv[i], arg_len);
            current_len += arg_len;
            if (i < argc - 1) {
                query_buf[current_len] = ' '; // Añadir espacio entre argumentos
                current_len++;
            }
        }
        run_query(query_buf);

    } else {
        // Modo 2: Interactivo (bucle)
        char line_buf[MAX_QUERY_LEN];
        printf("Cliente de Búsqueda. Escriba 'exit' para salir.\n");
        while (1) {
            printf("query> ");
            if (fgets(line_buf, sizeof(line_buf), stdin) == NULL) {
                break; // EOF (Ctrl+D)
            }
            
            // Quitar el newline final de fgets
            line_buf[strcspn(line_buf, "\n")] = 0;

            if (strcmp(line_buf, "exit") == 0 || strcmp(line_buf, "quit") == 0) {
                break;
            }
            if (strlen(line_buf) == 0) {
                continue;
            }
            
            run_query(line_buf);
        }
    }

    return 0;
}