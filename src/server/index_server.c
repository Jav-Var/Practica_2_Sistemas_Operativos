#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include "reader.h" // Nuestro motor de búsqueda
#include "common.h" // Para las rutas y safe_pread/pwrite
#include "builder.h" // Para agregar nuevos libros al índice

#define SERVER_PORT 8080
#define LISTEN_BACKLOG 10

// Definición de las rutas (ajusta si es necesario)
const char *BUCKETS_PATH = "data/index/title_buckets.dat";
const char *linked_list_PATH = "data/index/title_linked_list.dat";

/**
 * @brief Maneja una única conexión de cliente.
 * * Lee una petición (protocolo: [uint32_t len][char* query]),
 * busca en el índice y envía una respuesta 
 * (protocolo: [int32_t count][off_t* results]).
 * * @param h Handle del índice (fd's de buckets y linked_list)
 * @param client_fd File descriptor del socket del cliente
 */

static void handle_add_book(int client_fd) {
    uint32_t line_len;

    // Leer la longitud de la línea CSV
    if (read(client_fd, &line_len, sizeof(line_len)) != sizeof(line_len)) {
        perror("read (line_len)");
        close(client_fd);
        return;
    }
                  
    // Leer la línea completa
    char *line_buf = malloc(line_len + 1);
    if (!line_buf) {
        perror("malloc");
        close(client_fd);  
        return;
    }

    if (read(client_fd, line_buf, line_len) != (ssize_t)line_len) {
        perror("read (csv_line)");
        free(line_buf);
        close(client_fd);
        return;
    }
    line_buf[line_len] = '\0';

    printf("Recibido nuevo libro:\n%s\n", line_buf);

    // Guardar en el CSV e indexar
    if (build_index_line(CSV_PATH, line_buf) == 0) {
        printf("Libro indexado correctamente.\n");
        uint32_t ok = 1;
        write(client_fd, &ok, sizeof(ok));
    } else {
        fprintf(stderr, "Error al agregar libro.\n");
        uint32_t fail = 0;
        write(client_fd, &fail, sizeof(fail));
    }

    free(line_buf);
    close(client_fd);
}

static void handle_lookup(index_handle_t *h, FILE *csv_fp, int client_fd) {
    printf("Cliente conectado. Esperando consulta...\n");

    // --- 1. Leer Petición del Cliente ---
    
    // Leer la longitud de la consulta (4 bytes)
    uint32_t query_len;
    ssize_t r = read(client_fd, &query_len, sizeof(query_len));
    if (r != sizeof(query_len)) {
        fprintf(stderr, "Error al leer la longitud de la consulta.\n");
        close(client_fd);
        return;
    }
    
    // query_len = ntohl(query_len); // Opcional: manejar endianness si cliente/servidor difieren

    // Reservar memoria y leer la consulta
    char *query_buf = malloc(query_len + 1);
    if (!query_buf) {
        perror("malloc");
        close(client_fd);
        return;
    }

    r = read(client_fd, query_buf, query_len);
    if (r != query_len) {
        fprintf(stderr, "Error al leer la consulta.\n");
        free(query_buf);
        close(client_fd);
        return;
    }
    query_buf[query_len] = '\0'; // Asegurar NUL-terminator para index_lookup

    // --- 2. Procesar la Consulta ---
    
    off_t *offsets = NULL;
    uint32_t count = 0;
    int lookup_status = index_lookup(h, query_buf, &offsets, &count);

    int32_t response_count;
    if (lookup_status != 0) {
        response_count = -1; // Código de error
        count = 0; // No enviaremos datos
        fprintf(stderr, "Error durante index_lookup.\n");
    } else {
        response_count = (int32_t)count;
        printf("Consulta '%s' procesada. Resultados: %u\n", query_buf, count);
    }

    // --- 3. Enviar Respuesta al Cliente ---

    // Enviar el número de resultados (o código de error)

    ssize_t w = write(client_fd, &response_count, sizeof(response_count));
    if (w != sizeof(response_count)) {
        fprintf(stderr, "Error al escribir el conteo de respuesta.\n");
        free(query_buf);
        if (offsets) free(offsets);
        close(client_fd);
        return;
    }

    // Si encontramos resultados, los leemos del CSV y los enviamos
    if (count > 0) {
        char *line_buf = NULL; // Buffer para getline
        size_t line_buf_size = 0;
        ssize_t line_len;

        for (uint32_t i = 0; i < count; i++) {
            // Buscamos el offset en el CSV
            // ADVERTENCIA: fseek/getline NO es thread-safe.
            if (fseek(csv_fp, offsets[i], SEEK_SET) != 0) {
                perror("fseek");
                continue; // Saltar este resultado
            }
            
            // Leemos la línea completa
            line_len = getline(&line_buf, &line_buf_size, csv_fp);
            if (line_len == -1) {
                perror("getline");
                continue; // Saltar este resultado
            }
            
            // Enviamos la longitud de la línea
            uint32_t net_line_len = (uint32_t)line_len;
            if (write(client_fd, &net_line_len, sizeof(net_line_len)) != sizeof(net_line_len)) {
                fprintf(stderr, "Error al escribir longitud de línea\n");
                break; // Salir del bucle for
            }
            
            // Enviamos la línea
            if (write(client_fd, line_buf, net_line_len) != (ssize_t)net_line_len) {
                fprintf(stderr, "Error al escribir datos de línea\n");
                break; // Salir del bucle for
            }
        }
        free(line_buf); // Liberar el buffer de getline
    }

    // --- 4. Limpieza ---
    free(query_buf);
    if (offsets) {
        free(offsets); // index_lookup alocó esto, lo liberamos aquí.
    }
    close(client_fd);
    printf("Cliente desconectado.\n");
}

static void handle_client(index_handle_t *h, FILE *csv_fp, int client_fd) {
    uint32_t op_len;
    read(client_fd, &op_len, sizeof(op_len));

    char op_buf[64];
    read(client_fd, op_buf, op_len);
    op_buf[op_len] = '\0';

    if (strcmp(op_buf, "OP_LOOKUP") == 0) {
        handle_lookup(h, csv_fp, client_fd);
    } else if (strcmp(op_buf, "OP_ADD_BOOK") == 0) {
        handle_add_book(client_fd);
    } else {
        fprintf(stderr, "Operación desconocida: %s\n", op_buf);
    }

    close(client_fd);
}

int main(int argc, char *argv[]) {
    for (int i = 1; i < argc; ++i) { // Si se pasa --build como argumento, construye los indices
        if (strcmp(argv[i], "--build") == 0) {
            build_index_stream(CSV_PATH);
            return 0;
        }
    }

    // --- Abrir el Índice ---
    index_handle_t index_h;
    if (index_open(&index_h, BUCKETS_PATH, linked_list_PATH) != 0) {
        fprintf(stderr, "Error: No se pudo abrir el índice. Para construir el indice use --build\n");
        return 1;
    }
    printf("Índice cargado (FDs: b=%d, a=%d)\n", index_h.buckets_fd, index_h.linked_list_fd);

    FILE *csv_fp = fopen(CSV_PATH, "r"); // Dataset
    if (csv_fp == NULL) {
        perror("fopen (CSV_PATH)");
        fprintf(stderr, "Error: No se pudo abrir el archivo CSV: %s\n", CSV_PATH);
        index_close(&index_h);
        return 1;
    }
    printf("Archivo CSV '%s' abierto para lectura.\n", CSV_PATH);

    // --- Configurar el Socket ---
    int server_fd, client_fd;
    struct sockaddr_in server_addr, client_addr; // Direcciones del servidor y cliente
    socklen_t client_len = sizeof(client_addr);

    server_fd = socket(AF_INET, SOCK_STREAM, 0); // Socket del server
    if (server_fd < 0) {
        perror("socket");
        return 1;
    }

    // Reusar el puerto si está en TIME_WAIT (Opciones de socket)
    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("setsockopt SO_REUSEADDR");
    }

    // Configurar la dirección del servidor
    memset(&server_addr, 0, sizeof(server_addr)); 
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY); // Escuchar en todas las IPs
    server_addr.sin_port = htons(SERVER_PORT);

    // --- Bind y Listen ---
    if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("bind");
        close(server_fd);
        return 1;
    }

    if (listen(server_fd, LISTEN_BACKLOG) < 0) {
        perror("listen");
        close(server_fd);
        return 1;
    }

    printf("Servidor escuchando en el puerto %d...\n", SERVER_PORT);

    // --- 4. Bucle de Aceptación ---
    while (1) {
        client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &client_len);
        if (client_fd < 0) {
            perror("accept");
            continue; // Seguir intentando
        }

        // Manejar al cliente. 
        // (Nota: Esto es iterativo. Para múltiples clientes simultáneos
        // se necesitarían hilos, forks o I/O no bloqueante, 
        // pero mantengámoslo simple por ahora).
        handle_client(&index_h, csv_fp, client_fd);
    }

    // --- 5. Cierre (nunca se alcanza en este bucle) ---
    printf("Cerrando servidor...\n");
    close(server_fd);
    fclose(csv_fp);
    index_close(&index_h);
    return 0;
}