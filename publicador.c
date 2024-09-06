#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <semaphore.h>
#include "bmp.h"

#define SMOBJ_NAME "/img_sm"
#define SEM_READY "/sem_ready"
#define SMOBJ_SIZE sizeof(BMP_Image)

int main (int argc, char *argv[]) {
	
	// validacion para argumentos que recibe
    	if (argc != 2) {
        	fprintf(stderr, "Publicador: Uso %s <ruta_imagen>\n", argv[0]);
        	exit(EXIT_FAILURE);
    	}

    	// iniciando programa
    	printf("Publicador: Iniciando lectura de imagen en formato BMP...\n");

    	char *img_path = argv[1];

    	FILE *srcFile = fopen(img_path, "rb");
    	if (srcFile == NULL) {
        	fprintf(stderr, "Publicador: Error al abrir la imagen para lectura\n");
        	exit(EXIT_FAILURE);
    	}

    	// creacion de la estructura BMP_Image
    	BMP_Image *dataImage = createBMPImage(srcFile);
    	if (!dataImage) {
        	fprintf(stderr, "Error al crear la imagen BMP.\n");
        	fclose(srcFile);
        	exit(EXIT_FAILURE);
    	}

	// lectura de imagen BMP
    	readImage(srcFile, dataImage);
    	fclose(srcFile);
	if (dataImage == NULL) {
		fprintf(stderr, "Publicador: Error al leer la imagen BMP\n");
		exit(EXIT_FAILURE);
	}
	
	// creacion de la memoria compartida
	int sm_fd = shm_open(SMOBJ_NAME, O_CREAT | O_RDWR, 00600);
	if (sm_fd == -1) {
		fprintf(stderr, "Publicador: Error al crear la memoria compartida\n");
		freeImage(dataImage);
		exit(EXIT_FAILURE);
	}
	
	// asignar espacio para objeto
	if (ftruncate(sm_fd, SMOBJ_SIZE) == -1) {
		fprintf(stderr, "Publicador: Error al asignar espacio para imagen\n");
		close(sm_fd);
		freeImage(dataImage);
	}
	 	
	// mapear la imagen en memoria compartida para ser usada en la memoria virtual de publicador
	BMP_Image *ptr = mmap(0, SMOBJ_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, sm_fd, 0);
	if (ptr == MAP_FAILED) {
		fprintf(stderr, "Publicador: Error al mapear imagen en memoria compartida\n");
		close(sm_fd);
		freeImage(dataImage);
	}

	// copiar la imagen a memoria compartida
	memcpy(ptr, dataImage, SMOBJ_SIZE);

	// signal: imagen lista en memoria compartida
	sem_t *sem_ready = sem_open(SEM_READY, O_CREAT, 0666, 0);
	if (sem_ready == SEM_FAILED) {
		fprintf(stderr, "Publicador: Error al crear semaforo de listo\n");
		munmap(ptr, SMOBJ_SIZE);
		close(sm_fd);
		freeImage(dataImage);
		exit(EXIT_FAILURE);
	}

	sem_post(sem_ready);

	munmap(ptr, SMOBJ_SIZE);
	close(sm_fd);
	sem_close(sem_ready);
	freeImage(dataImage);

	printf("Publicador: Imagen cargada en memoria compartida...\n\n");
	    	
	exit(EXIT_SUCCESS);
}
