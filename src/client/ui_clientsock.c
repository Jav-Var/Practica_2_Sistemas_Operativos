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


static int ensure_connect(int connect_result) {
   if (connect_result < 0) {
       perror("Error al conectar");
   }
   return 0;
}


static int ensure_accept(int accept_result) {
   if (accept_result < 0) {
       perror("Error al aceptar la conexion");
   }
   return 0;
}


static int ensure_send(int send_result) {
   if (send_result < 0) {
       perror("Error al enviar datos");
   }
   return 0;
}


int main(){
   int sock, r;
   struct sockaddr_in server;
   char buffer[1024]; //Mirar lo de malloc


   const char *index_dir = INDEX_DIR;
   const char *csv_path = CSV_PATH;


   //Creacion del socket
   sock = socket(AF_INET, SOCK_STREAM, 0); // AF_INET: IPv4, SOCK_STREAM: TCP
   ensure_socket(sock);


   //Configuracion de la estructura del cliente
   server.sin_family = AF_INET;
   server.sin_port = htons(PORT);
   server.sin_addr.s_addr = inet_addr("127.0.0.1");


   if (inet_pton(AF_INET, "127.0.0.1", &server.sin_addr) <= 0) {
       perror("inet_pton"); exit(1);
   }


   //Conexion con el servidor
   r = connect(sock, (struct sockaddr *)&server, sizeof(server));
   ensure_connect(r);


   printf("Conectado al servidor en el puerto %d...\n", PORT);


   // Interfaz
   char *current_title = NULL;


   while (1) {
       printf("\n\tMenu de busqueda\n\n");
       printf("Título actual: %s\n", display_or_empty(current_title));
       printf("\n1. Ingresar titulo\n");
       printf("2. Realizar Busqueda\n"); // To do: Re enumerar las opciones
       printf("3. Salir\n");
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
           if (t && t[0] == '\0') { free(t); t = NULL; }
           free(current_title);
           current_title = t;
       } else if (strcmp(opt, "2") == 0) {
           // Titulo
           const char *t = current_title ? current_title : "";
           if (t[0] == '\0') {
               printf("Error: la búsqueda debe tener al menos un parámetro (titulo o autor).\n");
               free(opt);
               continue;
           }


           /* REQUEST */
           char req[MAX_LINE];
           snprintf(req, sizeof(req), "%s|%s", t, ""); // To do: Eliminar autor


           r = send(sock,req,strlen(req),0);
           ensure_send(r);

       } else if (strcmp(opt, "3") == 0) {
           free(opt);
           break;
       } else {
           printf("Opción no válida. Por favor elige 1..3.\n");
       }


       free(opt);
   }


   free(current_title);
   close(sock);
   printf("Cliente finalizado.\n");
   return 0;
}
