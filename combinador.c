#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <semaphore.h>
#include "bmp.h"

#define SMOBJ_NAME "/img_sm"
#define SEM_EDGE_DONE "/sem_edge_done"

// Función para guardar la imagen combinada en disco
void save_image(const char *filename, BMP_Image *img) {
    FILE *fptr = fopen(filename, "wb");
    if (fptr == NULL) {
        fprintf(stderr, "Combinador: Error al abrir archivo de salida para guardar imagen\n");
        exit(EXIT_FAILURE);
    }

    // Escribir encabezado BMP
    fwrite(&img->header, sizeof(BMP_Header), 1, fptr);

    // Escribir datos de la imagen
    int row_size = ((img->header.width_px * 3 + 3) / 4) * 4;  // Tamaño de fila alineada a 4 bytes
    for (int i = 0; i < img->header.height_px; i++) {
        fwrite(img->pixels[i], row_size, 1, fptr);
    }

    fclose(fptr);
}

// Función para combinar las dos mitades de la imagen
void combine_image_halves(BMP_Image *img, BMP_Image *upper_half, BMP_Image *lower_half) {
    int width = img->header.width_px;
    int height = img->header.height_px;

    // Asumimos que la imagen se divide en dos mitades horizontales
    int half_height = height / 2;

    // Copiar la mitad superior
    for (int y = 0; y < half_height; y++) {
        memcpy(img->pixels[y], upper_half->pixels[y], width * sizeof(Pixel));
    }

    // Copiar la mitad inferior
    for (int y = 0; y < height - half_height; y++) {
        memcpy(img->pixels[half_height + y], lower_half->pixels[y], width * sizeof(Pixel));
    }
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Combinador: Uso %s <ruta_imagen_salida>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    printf("Combinador: Iniciando proceso de combinación...\n");

    sem_t *sem_edge_done = sem_open(SEM_EDGE_DONE, 0);
    if (sem_edge_done == SEM_FAILED) {
        fprintf(stderr, "Combinador: Error al abrir semáforo SEM_EDGE_DONE\n");
        exit(EXIT_FAILURE);
    }

    int sm_fd = shm_open(SMOBJ_NAME, O_RDONLY, 0);
    if (sm_fd == -1) {
        fprintf(stderr, "Combinador: Error al abrir memoria compartida\n");
        sem_close(sem_edge_done);
        exit(EXIT_FAILURE);
    }

    struct stat shmobj_st;
    if (fstat(sm_fd, &shmobj_st) == -1) {
        fprintf(stderr, "Combinador: Error al obtener tamaño de memoria compartida\n");
        sem_close(sem_edge_done);
        exit(EXIT_FAILURE);
    }

    size_t sm_size = shmobj_st.st_size;

    BMP_Image *dataImage = mmap(NULL, sm_size, PROT_READ, MAP_SHARED, sm_fd, 0);
    if (dataImage == MAP_FAILED) {
        fprintf(stderr, "Combinador: Error al mapear imagen en memoria compartida\n");
        sem_close(sem_edge_done);
        close(sm_fd);
        exit(EXIT_FAILURE);
    }

    // Esperar a que el realzador termine
    sem_wait(sem_edge_done);

    // Aquí se asume que la imagen en memoria compartida contiene la mitad superior y la mitad inferior
    // Se deben ajustar los punteros según cómo se dividan las mitades en la memoria compartida.
    
    // Para fines demostrativos, vamos a simular que la imagen está dividida en dos mitades
    // En una implementación real, necesitarás ajustar cómo se obtiene y combina estas mitades.

    BMP_Image *upper_half = malloc(sizeof(BMP_Image));
    BMP_Image *lower_half = malloc(sizeof(BMP_Image));

    // Combinar las dos mitades
    combine_image_halves(dataImage, upper_half, lower_half);

    // Guardar la imagen combinada en disco
    save_image(argv[1], dataImage);

    // Liberar recursos
    munmap(dataImage, sm_size);
    close(sm_fd);
    sem_close(sem_edge_done);
    free(upper_half);
    free(lower_half);

    printf("Combinador: Proceso de combinación terminado con éxito!\n");

    exit(EXIT_SUCCESS);
}

