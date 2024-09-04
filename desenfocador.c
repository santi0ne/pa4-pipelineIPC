#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>

#include "bmp.h"

#define SHM_SIZE sizeof(BMP_Image)
#define SHM_NAME "/bmp_img_shm"

void apply_blur() {
	
	// abrir la memoria compartida
	int shm_fd = shm_open(SHM_NAME, O_RDWR, 0666);
	if (shm_fd == -1) {
		perror("Desenfocador: Error al abrir memoria compartida\n");
		exit(EXIT_FAILURE);
	}

	// mapeo de memoria compartida
	unsigned char *shm_ptr = mmap(0, SHM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
	if (shm_ptr == MAP_FAILED) {
		perror("Desenfocador: Error al mapear memoria compartida\n");
		close(shm_fd);
		exit(EXIT_FAILURE);
	}

	// aplicar desenfoque a la primera mitad de la imagen
	// implementacion del kernel de desenfoque aqui
	// ejemplo para prueba
	for (int i = 0; i < SHM_SIZE / 2; i++) {
		shm_ptr[i] = shm_ptr[i];
	}

	printf("Desenfocador: Proceso de desenfoque aplicado...\n");

	// limpiar
	munmap(shm_ptr, SHM_SIZE);
	close(shm_fd);
}
