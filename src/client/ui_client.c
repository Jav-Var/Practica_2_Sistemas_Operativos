#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include "common.h" 
#include "util.h" // Necesario para normalizar

#define SERVER_IP "127.0.0.1" 
#define SERVER_PORT 8080
#define MAX_QUERY_LEN 1024

/**
 * @brief Se conecta al servidor, envía una consulta de BÚSQUEDA y muestra los resultados.
 * (Esta es la función 'run_query' de la Fase 2, renombrada)
 */
static int perform_search(const char *query) {
    if (query == NULL || query[0] == '\0') {
        printf("Error: No hay título para buscar. Use la opción 1 primero.\n");
        return -1;
    }

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

    const char *op_code = "OP_LOOKUP";
    uint32_t op_len = (uint32_t)strlen(op_code);
    write(sock_fd, &op_len, sizeof(op_len));
    write(sock_fd, op_code, op_len);


    printf("Conectado. Buscando: '%s'\n", query);

    // --- 2. Enviar Petición de Búsqueda ---
    // NOTA: La lógica de 'Agregar Título' necesitaría un protocolo diferente.
    // Por ahora, solo enviamos el query para búsqueda.
    
    uint32_t query_len = (uint32_t)strlen(query);
    if (write(sock_fd, &query_len, sizeof(query_len)) != sizeof(query_len)) {
        perror("write (longitud)");
        close(sock_fd);
        return -1;
    }
    if (write(sock_fd, query, query_len) != (ssize_t)query_len) {
        perror("write (consulta)");
        close(sock_fd);
        return -1;
    }

    // --- 3. Recibir Respuesta ---
    int32_t result_count;
    if (read(sock_fd, &result_count, sizeof(result_count)) != sizeof(result_count)) {
        perror("read (conteo)");
        close(sock_fd);
        return -1;
    }

    if (result_count < 0) {
        fprintf(stderr, "Error en el servidor al procesar la consulta.\n");
        close(sock_fd);
        return -1;
    }
    
    if (result_count == 0) {
        printf("==> No se encontraron resultados para '%s'.\n", query);
        close(sock_fd);
        return 0;
    }

    printf("==> Recibidos %d resultados:\n", result_count);
    
    // Bucle para leer cada línea de resultado
    for (int i = 0; i < result_count; i++) {
        uint32_t line_len;
        if (read(sock_fd, &line_len, sizeof(line_len)) != sizeof(line_len)) {
            perror("read (line_len)");
            close(sock_fd);
            return -1;
        }

        char *line_buf = malloc(line_len + 1);
        if (!line_buf) {
            perror("malloc (line_buf)");
            close(sock_fd);
            return -1;
        }

        if (read(sock_fd, line_buf, line_len) != (ssize_t)line_len) {
            perror("read (line_data)");
            free(line_buf);
            close(sock_fd);
            return -1;
        }
        line_buf[line_len] = '\0'; 

        printf("  [%d] %s", i + 1, line_buf);
        if (line_buf[line_len - 1] != '\n') {
            printf("\n");
        }
        
        free(line_buf);
    }

    close(sock_fd);
    return 0;
}

void trim_newline(char *str) {
    str[strcspn(str, "\n")] = 0;
}

/**
 * @brief (STUB) Función placeholder para 'Agregar Título'.
 */
int perform_add_book(void) {
    char titulo[MAX_QUERY_LEN] = {0};
    char autor[MAX_QUERY_LEN] = {0};
    char imagen_url[MAX_QUERY_LEN] = {0};
    char num_pages[MAX_QUERY_LEN] = {0};
    char average_rating[MAX_QUERY_LEN] = {0};
    char text_review_account[MAX_QUERY_LEN] = {0};
    char descripcion[MAX_QUERY_LEN] = {0};
    char star_5_count[MAX_QUERY_LEN] = {0};
    char star_4_count[MAX_QUERY_LEN] = {0};
    char star_3_count[MAX_QUERY_LEN] = {0};
    char star_2_count[MAX_QUERY_LEN] = {0};
    char star_1_count[MAX_QUERY_LEN] = {0};
    char total_rating[MAX_QUERY_LEN] = {0};
    char genres[MAX_QUERY_LEN] = {0};

    printf("Título del libro nuevo: ");
    fgets(titulo, sizeof(titulo), stdin);
    trim_newline(titulo);

    printf("Autor del libro nuevo: ");
    fgets(autor, sizeof(autor), stdin);
    trim_newline(autor);

    printf("Enlace a la imagen de portada: ");
    fgets(imagen_url, sizeof(imagen_url), stdin);
    trim_newline(imagen_url);

    printf("Número de páginas: ");
    fgets(num_pages, sizeof(num_pages), stdin);
    trim_newline(num_pages);

    printf("Calificación promedio: ");
    fgets(average_rating, sizeof(average_rating), stdin);
    trim_newline(average_rating);

    printf("Número total de reseñas: ");
    fgets(text_review_account, sizeof(text_review_account), stdin);
    trim_newline(text_review_account);

    printf("Descripción: ");
    fgets(descripcion, sizeof(descripcion), stdin);
    trim_newline(descripcion);

    printf("Cantidad de calificaciones de 5 estrellas: ");
    fgets(star_5_count, sizeof(star_5_count), stdin);
    trim_newline(star_5_count);

    printf("Cantidad de calificaciones de 4 estrellas: ");
    fgets(star_4_count, sizeof(star_4_count), stdin);
    trim_newline(star_4_count);

    printf("Cantidad de calificaciones de 3 estrellas: ");
    fgets(star_3_count, sizeof(star_3_count), stdin);
    trim_newline(star_3_count);

    printf("Cantidad de calificaciones de 2 estrellas: ");
    fgets(star_2_count, sizeof(star_2_count), stdin);
    trim_newline(star_2_count);

    printf("Cantidad de calificaciones de 1 estrella: ");
    fgets(star_1_count, sizeof(star_1_count), stdin);
    trim_newline(star_1_count);

    printf("Calificación total: ");
    fgets(total_rating, sizeof(total_rating), stdin);
    trim_newline(total_rating);

    printf("Géneros del libro: ");
    fgets(genres, sizeof(genres), stdin);
    trim_newline(genres);

    // Aquí iría la lógica futura:
    // 1. Conectar al socket

    int sock_fd;
    struct sockaddr_in server_addr;

    sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (sock_fd < 0) {
        perror("socket");
        return -1;
    }

    memset(&server_addr, 0, sizeof(server_addr));
    //Configuracion del servidor
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
    // 2. Enviar comando OP_ADD_BOOK

    const char *op_code = "OP_ADD_BOOK";
    uint32_t op_code_len = (uint32_t)strlen(op_code);
    
    if(write(sock_fd, &op_code_len, sizeof(op_code_len)) != sizeof(op_code_len)) {
        perror("write (op_code length)");
        close(sock_fd);
        return -1;
    }
    if(write(sock_fd, op_code, op_code_len) != (ssize_t)op_code_len) {
        perror("write (op_code)");
        close(sock_fd);
        return -1;
    }

    // 3. Enviar datos del título

    //Construir el mensaje csv 
    char buffer[MAX_QUERY_LEN * 2];
    snprintf(buffer, sizeof(buffer), "%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s",
             titulo, autor, imagen_url, num_pages, average_rating,
             text_review_account, descripcion, star_5_count, star_4_count,
             star_3_count, star_2_count, star_1_count, total_rating, genres);


    uint32_t data_len = (uint32_t)strlen(buffer);
    if (write(sock_fd, &data_len, sizeof(data_len)) != sizeof(data_len)) { //Envia tamano del mensaje
        perror("write (data_len)");
        close(sock_fd);
        return -1;
    }
    if (write(sock_fd, buffer, data_len) != (ssize_t)data_len) { //Envia el mensaje
        perror("write (data)");
        close(sock_fd);
        return -1;
    }   
    // 4. Recibir confirmación (OK o Error)
    char response[256];
    ssize_t resp_len = read(sock_fd, response, sizeof(response) - 1);
    if (resp_len > 0) {
        response[resp_len] = '\0';
        printf("Respuesta del servidor: %s\n", response);
    } else {
        perror("read (response)");
    }

    close(sock_fd);
    return 0;
}


/**
 * @brief Limpia el búfer de entrada (stdin)
 */
static void clean_stdin(void) {
    int c;
    while ((c = getchar()) != '\n' && c != EOF);
}

/**
 * @brief Muestra el menú principal
 */
static void print_menu(const char *current_title) {
    printf("\n--- Cliente de Búsqueda de Libros ---\n");
    if (current_title[0] == '\0') {
        printf("Título actual: [NINGUNO]\n");
    } else {
        printf("Título actual: [%s]\n", current_title);
    }
    printf("---------------------------------------\n");
    printf("1. Escribir/Modificar título\n");
    printf("2. Agregar libro a la base de datos\n");
    printf("3. Buscar título\n");
    printf("4. Salir\n");
    printf("Seleccione una opción: ");
}

int main(int argc, char *argv[]) {
    // Modo 1: Consulta por argumento (útil para testing rápido)
    if (argc > 1) {
        char query_buf[MAX_QUERY_LEN] = {0};
        size_t current_len = 0;
        for (int i = 1; i < argc; i++) {
            size_t arg_len = strlen(argv[i]);
            if (current_len + arg_len + 1 > MAX_QUERY_LEN) break;
            memcpy(query_buf + current_len, argv[i], arg_len);
            current_len += arg_len;
            if (i < argc - 1) query_buf[current_len++] = ' ';
        }
        return perform_search(query_buf);
    }

    // Modo 2: Interactivo (Menú)
    char current_title[MAX_QUERY_LEN] = {0};
    char input_buffer[MAX_QUERY_LEN]; // Buffer temporal para fgets
    int choice = 0;

    while (choice != 4) {
        print_menu(current_title);
        
        if (fgets(input_buffer, sizeof(input_buffer), stdin) == NULL) {
            break; // EOF
        }

        if (sscanf(input_buffer, "%d", &choice) != 1) {
            choice = -1; // Opción inválida
        }

        switch (choice) {
            case 1: // Escribir/Modificar título
                printf("Ingrese el nuevo título: ");
                if (fgets(current_title, MAX_QUERY_LEN, stdin) == NULL) {
                    choice = 4; // Salir en EOF
                    break;
                }
                current_title[strcspn(current_title, "\n")] = 0; // Quitar newline
                break;

            case 2: // Agregar libro a la base de datos
                perform_add_book();
                break;

            case 3: // Buscar título
                perform_search(current_title);
                break;
            
            case 4: // Salir
                printf("Saliendo...\n");
                break;
            
            default:
                printf("Opción inválida. Intente de nuevo.\n");
                // Si el sscanf falló, el '\n' sigue en stdin, limpiarlo
                if (strchr(input_buffer, '\n') == NULL) {
                    clean_stdin();
                }
                break;
        }
    }

    return 0;
}