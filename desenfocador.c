#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <semaphore.h>
#include <pthread.h>
#include "bmp.h"

#define SMOBJ_NAME "/img_sm" 
#define SEM_READY "/sem_ready"
#define SEM_BLUR_DONE "/sem_blur_done"
#define SEM_EDGE_DONE "/sem_edge_done"

// Estructura para pasar los parámetros a los hilos
typedef struct {
    BMP_Image *img;
    Pixel **blurred_pixels;
    int start;
    int end;
} ThreadData;

// Kernel de desenfoque 3x3
int kernel[3][3] = {
    {1, 2, 1},
    {2, 4, 2},
    {1, 2, 1}
};


void* apply_blur_section(void *arg) {
ThreadData *data = (ThreadData*)arg;
    BMP_Image *img = data->img;
    Pixel **pixels = img->pixels;
    Pixel **blurred_pixels = data->blurred_pixels;
    int width = img->header.width_px;
    int start = data->start;
    int end = data->end;

    for (int y = start; y < end; y++) {
        for (int x = 1; x < width - 1; x++) {
            int sum_r = 0, sum_g = 0, sum_b = 0, sum_k = 0;
            for (int dy = -1; dy <= 1; dy++) {
                for (int dx = -1; dx <= 1; dx++) {
                    Pixel *p = &pixels[y + dy][x + dx];
                    int k = kernel[dy + 1][dx + 1];
                    sum_r += p->red * k;
                    sum_g += p->green * k;
                    sum_b += p->blue * k;
                    sum_k += k;
                }
            }

            blurred_pixels[y][x].red = sum_r / sum_k;
            blurred_pixels[y][x].green = sum_g / sum_k;
            blurred_pixels[y][x].blue = sum_b / sum_k;
        }
    }

    return NULL;
}

// Función para aplicar el desenfoque en paralelo usando hilos
void* apply_blur(BMP_Image *img, int num_threads) {
    int width = img->header.width_px;
    int height = img->header.height_px;

    // Crear una copia de la imagen original para almacenar el resultado
    Pixel **blurred_pixels = malloc(height * sizeof(Pixel *));
    for (int i = 0; i < height; i++) {
        blurred_pixels[i] = malloc(width * sizeof(Pixel));
    }

    // Dividir el trabajo entre los hilos
    pthread_t threads[num_threads];
    ThreadData thread_data[num_threads];

    int rows_per_thread = height / num_threads;
    for (int i = 0; i < num_threads; i++) {
        thread_data[i].img = img;
        thread_data[i].blurred_pixels = blurred_pixels;
        thread_data[i].start = i * rows_per_thread;
        thread_data[i].end = (i == num_threads - 1) ? height : (i + 1) * rows_per_thread;
        pthread_create(&threads[i], NULL, apply_blur_section, &thread_data[i]);
    }

    // Esperar a que todos los hilos terminen
    for (int i = 0; i < num_threads; i++) {
        pthread_join(threads[i], NULL);
    }

    // Reemplazar los píxeles originales con los desenfocados
    for (int y = 1; y < height - 1; y++) {
        for (int x = 1; x < width - 1; x++) {
            img->pixels[y][x] = blurred_pixels[y][x];
        }
    }

    // Liberar la memoria temporal
    for (int i = 0; i < height; i++) {
        free(blurred_pixels[i]);
    }
    free(blurred_pixels);
    
    printf("Desenfocador: Aplicando desenfoque a imagen...\n");
    return NULL;
}
int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Desenfocador: Uso %s <numero_hilos>\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    
    int num_threads = atoi(argv[1]);

    printf("Desenfocador: Iniciando proceso de blur...\n");

    sem_t *sem_ready = sem_open(SEM_READY, 0);
    if (sem_ready == SEM_FAILED) {
        fprintf(stderr, "Desenfocador: Error al abir semáforo de listo\n");
        exit(EXIT_FAILURE);
    }
    
    sem_wait(sem_ready);

    int sm_fd = shm_open(SMOBJ_NAME, O_RDONLY, 0);
    if (sm_fd == -1) {
        fprintf(stderr, "Desenfocador: Error al abrir memoria compartida\n");
        exit(EXIT_FAILURE);
    }

    struct stat shmobj_st;
    if (fstat(sm_fd, &shmobj_st) == -1) {
        fprintf(stderr, "Desenfocador: Error al obtener tamaño de memoria compartida\n");
        exit(EXIT_FAILURE);
    }

    size_t sm_size = shmobj_st.st_size;

    BMP_Image *dataImage = mmap(NULL, sm_size, PROT_READ, MAP_SHARED, sm_fd, 0);
    if (dataImage == MAP_FAILED) {
        fprintf(stderr, "Desenfocador: Error al mapear imagen en memoria compartida\n");
        close(sm_fd);
        exit(EXIT_FAILURE);
    }

    apply_blur(dataImage, num_threads);
    
    sem_t *sem_blur_done = sem_open(SEM_BLUR_DONE, O_CREAT, 0666, 0);
    if (sem_blur_done == SEM_FAILED) {
        fprintf(stderr, "Desenfocador: Error al crear semáforo\n");
        exit(EXIT_FAILURE);
    }
    
    sem_post(sem_blur_done);

    munmap(dataImage, sm_size);
    close(sm_fd);

    sem_close(sem_blur_done);
    
    printf("Desenfocador: Proceso de desenfoque terminado con éxito!\n\n");

    exit(EXIT_SUCCESS);
}

