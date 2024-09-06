#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <semaphore.h>
#include <string.h>
#include "bmp.h"

#define SMOBJ_NAME "/img_sm"
#define SEM_BLUR_DONE "/sem_blur_done"
#define SEM_EDGE_DONE "/sem_edge_done"

int save_result_image(BMP_Image *image, const char *output) {
	FILE *file = fopen(output, "wb");
	if (file == NULL) {
		fprintf(stderr, "Combinador: Error al abrir archivo de salida\n");
		exit(EXIT_FAILURE);
	}

	printf("Combinador: Proceso terminado con exito.\n");

	fclose(file);

	return 0;
}

int main(int argc, char *argv[]) {

	if (argc != 2) {
		fprintf(stderr, "Combinador: Uso %s <ruta_imagen_salida>\n", argv[0]);
		exit(EXIT_FAILURE);
	}

	const char *output_filename = argv[1];

	printf("Combinador: Iniciando proceso de combinacion...\n");

	// esperar a que el desenfocador termine
	sem_t *sem_blur_done = sem_open(SEM_BLUR_DONE, 0);
	if (sem_blur_done == SEM_FAILED) {
		fprintf(stderr, "Combinador: Error al abrir semaforo de desenfoque\n");
		exit(EXIT_FAILURE);
	}
	sem_wait(sem_blur_done);
	sem_close(sem_blur_done);
	//sem_unlink(SEM_BLUR_DONE);

	// esperar a que el realzador termine
	sem_t *sem_edge_done = sem_open(SEM_EDGE_DONE, 0);
	if (sem_edge_done == SEM_FAILED) {
		fprintf(stderr, "Combinador: Error al abrir semaforo de realce\n");
		exit(EXIT_FAILURE);
	}
	sem_wait(sem_edge_done);
	sem_close(sem_edge_done);
	//sem_unlink(SEM_EDGE_DONE);

	// abrir memoria compartida
	int sm_fd = shm_open(SMOBJ_NAME, O_RDWR, 0);
	if (sm_fd == -1) {
		fprintf(stderr, "Combinador: Error al abrir memoria compartida\n");
		exit(EXIT_FAILURE);
	}

	// obtener tamano de memoria compartida
	struct stat shmobj_st;
	if (fstat(sm_fd, &shmobj_st) == -1) {
		fprintf(stderr, "Combinador: Error al obtener tamano de memoria compartida\n");
		close(sm_fd);
		exit(EXIT_FAILURE);
	}

	size_t sm_size = shmobj_st.st_size;

	// mapeo de imagen de memoria compartida
	BMP_Image *dataImage = mmap(NULL, sm_size, PROT_READ | PROT_WRITE, MAP_SHARED, sm_fd, 0);
	if (dataImage == MAP_FAILED) {
		fprintf(stderr, "Combinador: Error al mapear imagen en memoria compartida\n");
		close(sm_fd);
		exit(EXIT_FAILURE);
	}

	/* combinacion de ambas mitades */

	// guardar imagen en archivo de salida
	if (save_result_image(dataImage, output_filename) != 0) {
		fprintf(stderr, "Combinador: Error al guardar imagen en disco\n");
		munmap(dataImage, sm_size);
		close(sm_fd);
		exit(EXIT_FAILURE);
	}

	munmap(dataImage, sm_size);
	close(sm_fd);

	printf("Combinador: Imagen combinada guardada con exito en %s!\n\n", output_filename);

	exit(EXIT_SUCCESS);

}
