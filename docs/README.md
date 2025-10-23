# PRÁCTICA 1 : COMUNICACIÓN ENTRE PROCESOS 
## Integrantes
Javier Vargas

Sara Fajardo

Samuel Palacios 
 
## Descripción general

Este programa implementa un sistema de busqueda eficiente sobre un conjunto de datos en formato csv, utilizando 
el dataset **books processed dataset** obtenido de Kaggle. Permite al usuario realizar consultas rápidas
mediante un sistema de indexación basado en una **Tabla Hash**, comunicando dos procesos no emparentados a tráves de **Tuberías Nombradas (FIFO)**.

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

## Criterios de búsqueda implementados

Para esta práctica se utilizaron los campos *`title`* y *`author_name`* como criterios de búsqueda. El usuario puede buscar por cualquiera de los criterios o ambos para realizar la busqueda.

### 1. `title`
El título del libro es el campo mas intuitivo y directo para buscar en el dataset.

### 2. `author_name`
El nombre del autor permite agrupar libros relacionados y facilita la búsqueda entre obras de un mismo autor. Este sirve como segundo criterio en casos donde existan titulos similares o repetidos.

## Rangos de Valores

### 1. Titulo
Para la construcción de la tabla hash se utilizaron los **primeros 20 caracteres del titulo** de cada libro como clave principal de indexación, esto facilita la busqueda de libros con titulos muy largos.

Al realizar la consulta, el usuario puede ingresar cualquier cantidad de carácteres del título que desee buscar, el programa se encargará de calcular el valor hash correspondiente y localizar el registro correspondiente.

### 2. Autor
Para la construcción de la tabla hash se utilizaron los **primero 20 caracteres del nombre del autor** de cada libro como clave principal de indexacion. 

Al realizar la consulta. el usuario puede ingresar cualquier cantidad de carácteres del autor que desee buscar, el programa se encargará de calcular el valor hash correspondiente y localizar el registro correspondiente.

## Comunicación entre procesos (FIFO)
El sistema implementa tuberías nombradas (FIFO) para la comunicación entre procesos no emparentados:

- El proceso `index_server` genera el archivo de indíces si no existe y espera consultas a tráves de una FIFO de entrada.
- El proceso `ui_client` envía las consultas ingresadas por el usuario (título y/o autor) y recibe la respuesta a tráves de un FIFO de salida.
  
## Observaciones del funcionamiento

### Consulta del usuario
- Al ingresar un **título** y un **autor**, el sistema mostrará únicamente los resultados donde **ambos campos coincidan** dentro del dataset.  
- El sistema **no diferencia entre mayúsculas y minúsculas**, e **ignora tildes, signos de puntuación y caracteres especiales**, garantizando una búsqueda más flexible.  
- Se mostrarán **todas las coincidencias** encontradas en el conjunto de datos, no solo la primera.  
- La búsqueda puede realizarse de forma **independiente** por **título**, por **autor**, o por **ambos simultáneamente**.

## Ejemplos de uso
### Búsqueda por título de libro
<img width="1809" height="950" alt="image" src="https://github.com/user-attachments/assets/dab73139-af65-4e05-8bd3-56189067d39f" />

### Búsqueda por autor de libro
<img width="1809" height="950" alt="Captura desde 2025-10-13 17-16-07" src="https://github.com/user-attachments/assets/c0192e81-cee4-4153-a287-88a44a1d34df" />

### Búsqueda por título y autor del libro
<img width="1809" height="950" alt="Captura desde 2025-10-13 17-27-45" src="https://github.com/user-attachments/assets/def58532-286e-44e7-b7bd-d9d64959d0b0" />

### Búsqueda sin resultados
<img width="1668" height="585" alt="Captura desde 2025-10-13 17-28-51" src="https://github.com/user-attachments/assets/59ad0ec2-981c-4a70-9cb6-02f9e74e3292" />

### Búsqueda con más de un resultado
<img width="1810" height="937" alt="Captura desde 2025-10-13 17-32-23" src="https://github.com/user-attachments/assets/b0b67272-d036-4074-ba90-21649662125e" />


