# PRÁCTICA 2: MOTOR DE BÚSQUEDA CLIENTE-SERVIDOR CON SOCKETS
## Integrantes
Javier Vargas

Sara Fajardo

Samuel Palacios 
 
## Descripción general

Este programa implementa un sistema de búsqueda eficiente sobre un conjunto de datos en formato CSV, utilizando el dataset books processed dataset obtenido de Kaggle.

A diferencia de la práctica anterior, este sistema está diseñado para manejar datasets masivos (>2GB) sin consumir memoria RAM, mediante un sistema de indexación basado en una Tabla Hash Persistente en Disco.

La arquitectura sigue un modelo Cliente-Servidor, comunicando los dos procesos (index_server y ui_client) a través de Sockets (TCP/IP).

## Campos del dataset

| Campo                   | Descripción |
|--------------------------|--------------|
| `title`                  | Título del libro. |
| `author_name`            | Nombre del autor o autores. |
| `image_url`              | Enlace a la imagen de la portada del libro. |
| `num_pages`              | Número total de páginas del libro. |
| `average_rating`         | Calificación promedio otorgada por los usuarios. |
| `text_review_count`      | Número total de reseñas escritas por los usuarios. |
| `description`            | Sinopsis o resumen del contenido del libro. |
| `5_star_rating_counts`   | Cantidad de calificaciones de 5 estrellas. |
| `4_star_rating_counts`   | Cantidad de calificaciones de 4 estrellas. |
| `3_star_rating_counts`   | Cantidad de calificaciones de 3 estrellas. |
| `2_star_rating_counts`   | Cantidad de calificaciones de 2 estrellas. |
| `1_star_rating_counts`   | Cantidad de calificaciones de 1 estrella. |
| `total_rating_counts`    | Total de calificaciones. |
| `genres`                 | Géneros literarios asociados al libro. |

## Arquitectura de Indexación Persistente
Para cumplir el requisito de no cargar el índice en memoria, el sistema implementa una Tabla Hash en disco.
## `1.Construcción (Offline)`
Al ejecutar el servidor con el flag --build (./build/index_server --build), el proceso builder lee el archivo CSV y genera dos archivos de índice en el directorio data/index/:

    title_buckets.dat: Almacena los "cubos" (buckets) de la tabla hash. Es un array de punteros (off_t) que apuntan a la cabeza de una lista de colisiones en el archivo arrays.dat.

    title_arrays.dat: Almacena los nodos de datos. Cada nodo contiene la clave normalizada, el offset (off_t) de la línea correspondiente en el archivo CSV, y un puntero (next_ptr) al siguiente nodo en la cadena de colisiones.
### 2. `Búsqueda (Online)`
Cuando el servidor está corriendo, el reader (buscador) realiza las siguientes operaciones de I/O en disco por cada consulta:

   Calcula el hash de la consulta y accede al buckets.dat para encontrar el bucket correspondiente (1 pread).

   Recorre la lista enlazada dentro de arrays.dat, saltando de un nodo a otro (múltiples preads) para encontrar todas las coincidencias.
### Criterios de búsqueda implementados
Para esta práctica, el único criterio de búsqueda indexado es el campo title

## Rangos de Valores

### 1. Titulo
El título del libro es la clave principal de indexación. La búsqueda implementada requiere una coincidencia exacta.

El sistema funciona de la siguiente manera:

   El builder normaliza el título completo (minúsculas, sin puntuación) y lo usa para generar un hash. El título normalizado se almacena en el índice (arrays.dat).

  - El client envía una consulta.

  - El reader normaliza la consulta del cliente y genera un hash.

  - Si los hashes coinciden, se comparan las cadenas normalizadas con strcmp.

Esto implica que una búsqueda por prefijo (ej. "Harry") no devolverá resultados para "Harry Potter", ya que el hash de "Harry" es diferente al de "Harry Potter" y el buscador no explorará el mismo bucket. El usuario debe proveer el título completo.
## Comunicación entre procesos (Sockets)
El sistema implementa una arquitectura Cliente-Servidor que se comunica a través de Sockets TCP/IP:

   - index_server:

1. Inicia y abre los descriptores de archivo (fd) de los archivos de índice (.dat) y del archivo .csv.
2. Abre un socket (socket) en un puerto (ej. 8080), lo vincula (bind) y se pone a escuchar (listen).
3. Espera y acepta (accept) conexiones de nuevos clientes en un bucle infinito.
4. El servidor actual es iterativo: maneja un cliente a la vez.

   - ui_client:
1. Provee un menú interactivo al usuario.
2. Al realizar una búsqueda, crea un socket y se conecta (connect) al servidor.
3. Envía la consulta al servidor y recibe los resultados (las líneas completas del CSV).
### Protocolo de Red
Se definió un protocolo simple de prefijo de longitud para la comunicación:
1. Petición (Cliente -> Servidor): [uint32_t query_len][char* query]
2. Respuesta (Servidor -> Cliente): [int32_t count] (número de resultados), seguido de un bucle de count items, donde cada item es: [uint32_t line_len][char* line_data]
## Observaciones del funcionamiento
- El sistema no diferencia entre mayúsculas y minúsculas e ignora tildes y la mayoría de signos de puntuación (normalización), garantizando una búsqueda flexible.

- Se mostrarán todas las coincidencias encontradas en el conjunto de datos que coincidan exactamente con la consulta (una vez normalizada)..

- El cliente (ui_client) ofrece un menú para (1) Ingresar un título, (2) Agregar un título (función stub no implementada), (3) Buscar el título, y (4) Salir.

## Ejemplos de uso
### Búsqueda por título de libro
<img width="1809" height="950" alt="image" src="https://github.com/user-attachments/assets/dab73139-af65-4e05-8bd3-56189067d39f" />

### Búsqueda sin resultados
<img width="1668" height="585" alt="Captura desde 2025-10-13 17-28-51" src="https://github.com/user-attachments/assets/59ad0ec2-981c-4a70-9cb6-02f9e74e3292" />

### Búsqueda con más de un resultado
<img width="1810" height="937" alt="Captura desde 2025-10-13 17-32-23" src="https://github.com/user-attachments/assets/b0b67272-d036-4074-ba90-21649662125e" />

### Agregar un título

### Buscar el título agregado


