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
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>


#define PORT 3735
#define BACKLOG 10




static int ensure_socket(int server_fd) {
   if(server_fd<0){
       perror("Error al crear el socket");
   }
   return 0;
}


static int ensure_bind(int bind_result) {
   if (bind_result < 0) {
       perror("Error al hacer bind");
   }
   return 0;
}


static int ensure_listen(int listen_result) {
   if (listen_result < 0) {
       perror("Error al escuchar");
   }
   return 0;
}


static int ensure_accept(int accept_result) {
   if (accept_result < 0) {
       perror("Error al aceptar la conexion");
   }
   return 0;
}


int main(){
   int server_fd, client_fd, r;
   struct sockaddr_in server, client;
   char buffer[1024]; //Mirar lo de malloc
   socklen_t size = sizeof(struct sockaddr_in);


   const char *index_dir = INDEX_DIR;
   const char *csv_path = CSV_PATH;




   //Creacion del socket
   server_fd = socket(AF_INET, SOCK_STREAM, 0); // AF_INET: IPv4, SOCK_STREAM: TCP
   ensure_socket(server_fd);


   //Configuracion de la estructura del servidor
   server.sin_family = AF_INET;
   server.sin_addr.s_addr = INADDR_ANY;
   server.sin_port = htons(PORT);
   memset(server.sin_zero,0,sizeof(server.sin_zero));


   //Conexion del socket con la direccion y el puerto
   r = bind(server_fd, (struct sockaddr *)&server, sizeof(server));
   ensure_bind(r);


   //"Escuchar" la conexion
   r = listen(server_fd, BACKLOG);
   ensure_listen(r);


   printf("Servidor escuchando en el puerto %d...\n", PORT);




   // Indices y csv
   char title_buckets[1024], title_arrays[1024];
   snprintf(title_buckets, sizeof(title_buckets), "%s/title_buckets.dat", index_dir);
   snprintf(title_arrays, sizeof(title_arrays), "%s/title_arrays.dat", index_dir);


   /* CONSTRUYE EL INDICE */
   int need_build = 0;
   if (access(title_buckets, F_OK) != 0) {
       printf("Falta archivo de índice: %s\n", title_buckets);
       need_build = 1;
   }


   if (need_build) {
       printf("Construyendo índices en '%s'...\n", INDEX_DIR);
       if (build_index_stream(CSV_PATH) != 0) {
           fprintf(stderr, "Fallo al construir los índices\n");
           return 1;
       }
       printf("Índices construidos.\n");
   } else {
       printf("Archivos de índice encontrados.\n");
   }


   index_handle_t th; // Handle de autor y titulo
   if (index_open(&th, title_buckets, title_arrays) != 0) {
       fprintf(stderr, "Fallo al abrir el índice de títulos\n");
   }
   printf("Esperando peticiones de busqueda\n");
   fflush(stdout);


   FILE *csvf = fopen(csv_path, "rb");
   if(!csvf) {
       return 1;
   }


   while(1){
       client_fd = accept(server_fd, (struct sockaddr *)&client, &size);
       ensure_accept(client_fd);


       printf("Cliente conectado. \n");


       r = recv(client_fd, buffer, sizeof(buffer),0);
       //asegurar que no hay error en recv
       buffer[r] = '\0'; //Asegurarse de que el string este terminado


       //BLOQUE DE BUSQUEDA
       char *title = buffer;
       if(title[0] == '\0'){
           const char *error_msg = "ERR|La búsqueda debe tener al menos un parámetro\n<END>\n";
           send(client_fd, error_msg, strlen(error_msg), 0);
           close(client_fd);
           continue;
       }


       printf("Buscando titulo : %s\n", buffer);


       off_t *offs = NULL;
       uint32_t count = 0;


       int rc = index_lookup(&th, title, &offs, &count);
       if (rc != 0) {
           const char *mensaje = "ERR/Error interno en la busqeda\n<END>\n";
           send(client_fd, mensaje, strlen(mensaje), 0);
           free(offs);
           close(client_fd);
           continue;
       }
      


       if (count == 0) {
           send(client_fd, "OK\n<END>\n", 10, 0);
           free(offs);
           close(client_fd);
           continue;
       }


       if (!csvf) {
           send(client_fd, "ERR|No se puede abrir el archivo CSV\n<END>\n", 42, 0);
           free(offs);
           close(client_fd);
           continue;
       }


       send(client_fd, "OK\n", 3, 0);
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
               send(client_fd, line, strlen(line), 0);
               send(client_fd, "\n", 1, 0);
           }
           free(line);
       }


       const char *mensaje = "Fin de resultados\n<END>\n";
       send(client_fd, mensaje, strlen(mensaje), 0);
       free(offs);
       close(client_fd);
   }
   close(server_fd);
   return 0;


   //Falta: cerrar csv y liberar memoria?, comprobar recv y send.
}


