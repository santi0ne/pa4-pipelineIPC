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
#define SEM_EDGE_DONE "/sem_edge_done"

void* apply_edge(void* arg) {
	// algoritmo
	return NULL;
}

int main(int argc, char *argv[]) {

	// verificacion para argumentos que recibe
	if (argc != 2) {
		fprintf(stderr, "Realzador: Uso %s <ruta_imagen_salida>\n", argv[0]);
		exit(EXIT_FAILURE);
	}

	printf("Realzador: Iniciando proceso de realce de bordes...\n");

	// esperar a que la imagen este lista en la memoria compartida
	sem_t *sem_ready = sem_open(SEM_READY, 0);
	if (sem_ready == SEM_FAILED) {
		fprintf(stderr, "Realzador: Error al abrir memoria compartida\n");
		exit(EXIT_FAILURE);
	}
	sem_wait(sem_ready);
	sem_close(sem_ready);

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
	void *ptr = mmap(NULL, sm_size, PROT_READ, MAP_SHARED, sm_fd, 0);
	if (ptr == MAP_FAILED) {
		fprintf(stderr, "Realzador: Error al mapear imagen en memoria compartida\n");
		close(sm_fd);
		exit(EXIT_FAILURE);
	}

	/* Aqui va l algoritmo para creacion de hilos */

	// signal: realce aplicado
	sem_t *sem_edge_done = sem_open(SEM_EDGE_DONE, O_CREAT, 0644, 0);
	if (sem_edge_done == SEM_FAILED) {
		fprintf(stderr, "Realzador: Error al crear semaforo de edge\n");
		munmap(ptr, sm_size);
		close(sm_fd);
		exit(EXIT_FAILURE);
	}

	sem_post(sem_edge_done);

	munmap(ptr, sm_size);
	close(sm_fd);
	sem_close(sem_edge_done);
	
	printf("Realzador: Proceso de realce terminado con exito!\n");

	exit(EXIT_SUCCESS);
}


