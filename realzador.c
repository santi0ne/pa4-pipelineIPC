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
#include "bmp.h"

#define SMOBJ_NAME "/img_sm"
#define SEM_BLUR_DONE "/sem_blur_done"
#define SEM_EDGE_DONE "/sem_edge_done"

typedef struct {
    BMP_Image *img;
    Pixel **edge_pixels;
    int start;
    int end;
} ThreadData;

// Kernel de detección de bordes 3x3
int kernel[3][3] = {
    {-1, -1, -1},
    {-1,  8, -1},
    {-1, -1, -1}
};

void* apply_edge_section(void *arg) {
    ThreadData *data = (ThreadData*)arg;
    BMP_Image *img = data->img;
    Pixel **pixels = img->pixels;
    Pixel **edge_pixels = data->edge_pixels;
    int width = img->header.width_px;
    int start = data->start;
    int end = data->end;

    for (int y = start; y < end; y++) {
        for (int x = 1; x < width - 1; x++) {
            int sum_r = 0, sum_g = 0, sum_b = 0;
            for (int dy = -1; dy <= 1; dy++) {
                for (int dx = -1; dx <= 1; dx++) {
                    Pixel *p = &pixels[y + dy][x + dx];
                    int k = kernel[dy + 1][dx + 1];
                    sum_r += p->red * k;
                    sum_g += p->green * k;
                    sum_b += p->blue * k;
                }
            }

            edge_pixels[y][x].red = (sum_r > 255) ? 255 : ((sum_r < 0) ? 0 : sum_r);
            edge_pixels[y][x].green = (sum_g > 255) ? 255 : ((sum_g < 0) ? 0 : sum_g);
            edge_pixels[y][x].blue = (sum_b > 255) ? 255 : ((sum_b < 0) ? 0 : sum_b);
        }
    }

    return NULL;
}

void* apply_edge(BMP_Image *img, int num_threads) {
	// algoritmo
	int width = img->header.width_px;
        int height = img->header.height_px;

        // Crear una copia de la imagen original para almacenar el resultado
        Pixel **edge_pixels = malloc(height * sizeof(Pixel *));
        for (int i = 0; i < height; i++) {
            edge_pixels[i] = malloc(width * sizeof(Pixel));
        }

        // Dividir el trabajo entre los hilos
        pthread_t threads[num_threads];
        ThreadData thread_data[num_threads];

        int rows_per_thread = height / num_threads;
        for (int i = 0; i < num_threads; i++) {
            thread_data[i].img = img;
            thread_data[i].edge_pixels = edge_pixels;
            thread_data[i].start = i * rows_per_thread;
            thread_data[i].end = (i == num_threads - 1) ? height : (i + 1) * rows_per_thread;
            pthread_create(&threads[i], NULL, apply_edge_section, &thread_data[i]);
        }

        // Esperar a que todos los hilos terminen
        for (int i = 0; i < num_threads; i++) {
            pthread_join(threads[i], NULL);
        }

        // Reemplazar los píxeles originales con los de detección de bordes
        for (int y = 1; y < height - 1; y++) {
            for (int x = 1; x < width - 1; x++) {
                img->pixels[y][x] = edge_pixels[y][x];
            }
        }

        // Liberar la memoria temporal
        for (int i = 0; i < height; i++) {
            free(edge_pixels[i]);
        }
        free(edge_pixels);
        
	printf("Realzador: Aplicando realce de bordes a imagen...\n");
	return NULL;
}

int main(int argc, char *argv[]) {

	// verificacion para argumentos que recibe
	if (argc != 2) {
		fprintf(stderr, "Realzador: Uso %s <numero_hilos>\n", argv[0]);
		exit(EXIT_FAILURE);
	}
	
	int num_threads = atoi(argv[1]);

	printf("Realzador: Iniciando proceso de realce de bordes...\n");
	
	// esperar a que desenfocador termine
	sem_t *sem_blur_done = sem_open(SEM_BLUR_DONE, 0);
    	if (sem_blur_done == SEM_FAILED) {
        	fprintf(stderr, "Realzador: Error al abrir semáforo de desenfoque completado\n");
        	exit(EXIT_FAILURE);
    	}
    	//sem_wait(sem_blur_done);
    	
    	  sem_t *sem_edge_done = sem_open(SEM_EDGE_DONE, O_CREAT, 0666, 0);
          if (sem_edge_done == SEM_FAILED) {
              fprintf(stderr, "Realzador: Error al crear semáforo SEM_EDGE_DONE\n");
	      exit(EXIT_FAILURE);
          }
    	
	// abriendo memoria compartida
	int sm_fd = shm_open(SMOBJ_NAME, O_RDONLY, 0);
	if (sm_fd == -1) {
		fprintf(stderr, "Realzador: Error al abrir memoria compartida\n");
		exit(EXIT_FAILURE);
	}

	// obtener el tamano de la memoria compartida
	struct stat shmobj_st;
	if (fstat(sm_fd, &shmobj_st) == -1) {
		fprintf(stderr, "Realzador: Error al obtener tamano de memoria compartida\n");
		exit(EXIT_FAILURE);
	}

	size_t sm_size = shmobj_st.st_size;

	// mapear el objeto de memoria compartida (imagen)
	BMP_Image *dataImage = mmap(NULL, sm_size, PROT_READ, MAP_SHARED, sm_fd, 0);
	if (dataImage == MAP_FAILED) {
		fprintf(stderr, "Realzador: Error al mapear imagen en memoria compartida\n");
		close(sm_fd);
		exit(EXIT_FAILURE);
	}

	/* Aqui va el algoritmo para creacion de hilos */
	sem_wait(sem_blur_done);
	apply_edge(dataImage, num_threads);

	sem_post(sem_edge_done);

	munmap(dataImage, sm_size);
	close(sm_fd);

	// ojo: post necesario a blur para evitar errores en combinador
	sem_post(sem_blur_done);

	sem_close(sem_blur_done);
	sem_close(sem_edge_done);
	
	printf("Realzador: Proceso de realce terminado con exito!\n\n");

	exit(EXIT_SUCCESS);
}


