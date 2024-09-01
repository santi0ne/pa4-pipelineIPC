#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <string.h>

#include "bmp.h"

#define SHM_SIZE sizeof(BMP_Image)
#define SHM_NAME "/bmp_img_shm"

void publish_img(char *filename) {
	
	// abrir el archivo de la imagen
	FILE *srcFile = fopen(filename, "rb");
	if (srcFile == NULL) {
		perror("Publicador: Error al abrir el archivo BMP\n");
		exit(EXIT_FAILURE);
	}

	// creacion de la estructura BMP_Image
	BMP_Image *dataImage = createBMPImage(srcFile);
	if (dataImage == NULL) {
		perror("Publicador: Error al crear la estructura BMP_Image\n");
		fclose(srcFile);
		exit(EXIT_FAILURE);
	}

	// lectura de la imagen
	readImage(srcFile, dataImage);
	fclose(srcFIle);
	
	// imagen valida?
	if (!checkBMPValid(&dataImage->header) {
		fprintf(stderr, "Publicador: La imagen BMP no es valida\n");
		freeImage(dataImage);
		exit(EXIT_FAILURE);
	}
	
	// creacion de la memoria compartida
	int shm_fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);
	if (sfm_fd == -1) {
		perror("Publicador: Error al crear memoria compartida\n");
		freeImage(dataImage);
		exit(EXIT_FAILURE);
	}
	
	// ajustar el tamanio de memoria compartida
	if (ftruncate(shm_fd, SHM_SIZE) == -1) {
		perror("Publicador: Error al ajustar tamanio de memoria compartida\n");
		shm_unlink(SHM_NAME);
		freeImage(dataImage);
		exit(EXIT_FAILURE);
	}

	// mapeo de memoria compartida
	BMP_Image *shared_img = mmap(NULL, SHM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
	if (shared_img == MAP_FAILED) {
		perror("Publicador: Error al mapear la memoria compartida\n");
		shm_unlink(SHM_NAME);
		freeImage(dataImage);
		exit(EXIT_FAILURE);
	}

	// copiar la imagen leida a la memoria compartida
	memcpy(shared_img, dataImage, SHM_SIZE);

	// liberacion de la memoria de la imagen leida
	freeImage(dataImage);

	// desmapear la memoria compartida
	if (munmap(shared_img, SHM_SIZE) == -1) {
		perror("Publicador: Error al desmapear la memoria compartida\n");
		shm_unlink(SHM_NAME);
		exit(EXIT_FAILURE):
	}

	// cerrar el archivo de la memoria compartida
	if (close(shm_fd) == -1) {
		perror("Publicador: Error al cerrar el archivo de la memoria compartida\n");
		shm_unlink(SHM_NAME);
		exit(EXIT_FAILURE);
	}

	printf("Publicador: La imagen BMP ha sido publicada en la memoria compartida...\n");

}
