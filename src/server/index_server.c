#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include "reader.h" // Nuestro motor de búsqueda
#include "common.h" // Para las rutas y safe_pread/pwrite

#define SERVER_PORT 8080
#define LISTEN_BACKLOG 10

// Definición de las rutas (ajusta si es necesario)
const char *BUCKETS_PATH = "data/index/title_buckets.dat";
const char *ARRAYS_PATH = "data/index/title_arrays.dat";

/**
 * @brief Maneja una única conexión de cliente.
 * * Lee una petición (protocolo: [uint32_t len][char* query]),
 * busca en el índice y envía una respuesta 
 * (protocolo: [int32_t count][off_t* results]).
 * * @param h Handle del índice (fd's de buckets y arrays)
 * @param client_fd File descriptor del socket del cliente
 */
static void handle_client(index_handle_t *h, int client_fd) {
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
    // int32_t net_count = htonl(response_count); // Opcional: manejar endianness
    ssize_t w = write(client_fd, &response_count, sizeof(response_count));
    if (w != sizeof(response_count)) {
        fprintf(stderr, "Error al escribir el conteo de respuesta.\n");
    }

    // Enviar los offsets si hay resultados
    if (count > 0) {
        size_t results_size = count * sizeof(off_t);
        w = write(client_fd, offsets, results_size);
        if (w != (ssize_t)results_size) {
            fprintf(stderr, "Error al escribir los offsets de respuesta.\n");
        }
    }

    // --- 4. Limpieza ---
    free(query_buf);
    if (offsets) {
        free(offsets); // index_lookup alocó esto, lo liberamos aquí.
    }
    close(client_fd);
    printf("Cliente desconectado.\n");
}


int main() {
    // --- 1. Abrir el Índice ---
    index_handle_t index_h;
    if (index_open(&index_h, BUCKETS_PATH, ARRAYS_PATH) != 0) {
        fprintf(stderr, "Error: No se pudo abrir el índice. ¿Ejecutaste el builder?\n");
        return 1;
    }
    printf("Índice cargado (FDs: b=%d, a=%d)\n", index_h.buckets_fd, index_h.arrays_fd);

    // --- 2. Configurar el Socket ---
    int server_fd, client_fd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len = sizeof(client_addr);

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        perror("socket");
        return 1;
    }

    // Opcional: Reusar el puerto si está en TIME_WAIT
    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("setsockopt SO_REUSEADDR");
    }

    // Configurar la dirección del servidor
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY); // Escuchar en todas las IPs
    server_addr.sin_port = htons(SERVER_PORT);

    // --- 3. Bind y Listen ---
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
        handle_client(&index_h, client_fd);
    }

    // --- 5. Cierre (nunca se alcanza en este bucle) ---
    printf("Cerrando servidor...\n");
    close(server_fd);
    index_close(&index_h);
    return 0;
}