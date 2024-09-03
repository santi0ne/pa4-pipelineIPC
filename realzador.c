#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>

#include "bmp.h"

#define SHM_SIZE sizeof(BMP_Image)
#define SHM_NAME "/bmp_image_shm"

void apply_edge_detection () {

	// abrir la memoria compartida
	int shm_fd = shm_open(SHM_NAME, O_RDWR, 0666);
	if (shm_fd == -1) {
		perror("Realzador: Error al abrir memoria compartida\n");
		exit(EXIT_FAILURE);
	}

	// mapeo de memoria compartida
	unsigned char *shm_ptr = mmap(0, SHM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
	if (shm_ptr == MAP_FAILED) {
        	perror("Realzador: Error al mapear memoria compartida\n");
        	close(shm_fd);
        	exit(EXIT_FAILURE);
    	}

	// Aplicar realce de bordes a la segunda mitad de la imagen
    	// Aquí deberías implementar el kernel de realce de bordes
    	// Este es un ejemplo de cómo podrías hacerlo:
    	for (int i = SHM_SIZE / 2; i < SHM_SIZE; i++) {
        	// Aplicar realce de bordes (este código debe ser reemplazado con el algoritmo real)
        	shm_ptr[i] = shm_ptr[i]; // Ejemplo de operación
    	}
    	printf("Realzador: Proceso de realce aplicado...\n");

    	// Limpiar
    	munmap(shm_ptr, SHM_SIZE);
    	close(shm_fd);
}
