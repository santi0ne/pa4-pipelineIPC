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
#define SEM_READY "/sem_ready"
#define SEM_BLUR_DONE "/sem_blur_done"

void* apply_blur(BMP_Image *img) {
	// algoritmo
	printf("Desenfocador: Aplicando desenfoque a imagen...\n");
	return NULL;
}

int main(int argc, char *argv[]) {
	
	// verificacion para argumentos que recibe
	if (argc != 2) {
		fprintf(stderr, "Desenfocador: Uso %s <ruta_imagen_salida>\n", argv[0]);
		exit(EXIT_FAILURE);
	}

	printf("Desenfocador: Iniciando proceso de blur...\n");

	// esperar a que la imagen este lista en la memoria compartida
    	sem_t *sem_ready = sem_open(SEM_READY, 0);
    	if (sem_ready == SEM_FAILED) {
        	fprintf(stderr, "Desenfocador: Error al abrir semáforo de listo\n");
        	exit(EXIT_FAILURE);
    	}
    	sem_wait(sem_ready);
    	sem_close(sem_ready);

	// abriendo memoria compartida
	int sm_fd = shm_open(SMOBJ_NAME, O_RDONLY, 0);
	if (sm_fd == -1) {
		fprintf(stderr, "Desenfocador: Error al abrir memoria compartida\n");
		exit(EXIT_FAILURE);
	}

	// obtener el tamano de la memoria compartida
	struct stat shmobj_st;
	if (fstat(sm_fd, &shmobj_st) == -1) {
		fprintf(stderr, "Desenfocador: Error al obtener tamano de memoria compartida\n");
		exit(EXIT_FAILURE);
	}
	
	size_t sm_size = shmobj_st.st_size;

	// mapear el objeto de memoria compartida (imagen)
	BMP_Image *dataImage = mmap(NULL, sm_size, PROT_READ, MAP_SHARED, sm_fd, 0);
	if (dataImage == MAP_FAILED) {
		fprintf(stderr, "Desenfocador: Error al mapear imagen en memoria compartida\n");
		close(sm_fd);
		exit(EXIT_FAILURE);
	}

	/* Aqui va el algoritmo para creacion de hilos */
	apply_blur(dataImage);
	
	// signal: desenfoque aplicado
	sem_t *sem_blur_done = sem_open(SEM_BLUR_DONE, O_CREAT, 0644, 0);
	if (sem_blur_done == SEM_FAILED) {
		fprintf(stderr, "Desenfocador: Error al crear semaforo de blur");
		munmap(dataImage, sm_size);
		close(sm_fd);
		exit(EXIT_FAILURE);
	}
	sem_post(sem_blur_done);
	
	// esperar a que realzador termine su trabajo
	/*
	sem_t *sem_edge_done = sem_open(SEM_EDGE_DONE, 0);
    	if (sem_edge_done == SEM_FAILED) {
        	fprintf(stderr, "Desenfocador: Error al abrir semáforo de realce de bordes\n");
        	munmap(dataImage, sm_size);
        	close(sm_fd);
        	exit(EXIT_FAILURE);
    	}
	sem_wait(sem_edge_done); */

	munmap(dataImage, sm_size);
	close(sm_fd);
	sem_close(sem_blur_done);
	//sem_close(sem_edge_done);

	printf("Desenfocador: Proceso de desenfoque terminado con exito!\n");

	exit(EXIT_SUCCESS);
}

