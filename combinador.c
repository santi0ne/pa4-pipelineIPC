#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>

#include "bmp.h"

#define SHM_SIZE sizeof(BMP_Image)
#define SHM_NAME "/bmp_img_shm"

void combine_images(const char *output_path) {

	// abrir la memoria compartida
	int shm_fd = shm_open(SHM_NAME, O_RDWR, 0666);
	if (shm_fd == -1) {
		perror("Combinador: Error al abrir memoria compartida\n");
		exit(EXIT_FAILURE);
	}

	// mapeo de memoria compartida
	unsigned char *shm_ptr = mmap(0, SHM_SIZE, PROT_READ, MAP_SHARED, shm_fd, 0);
	if (shm_ptr == MAP_FAILED) {
        	perror("Combinador: Error al mapear memoria compartida\n");
        	close(shm_fd);
        	exit(EXIT_FAILURE);
    	}

	// combinar las dos mitades de la imagen
	FILE *output_file = fopen(output_path, "wb");
    	if (output_file == NULL) {
        	perror("Combinador: Error al abrir archivo de salida\n");
        	munmap(shm_ptr, SHM_SIZE);
        	close(shm_fd);
        	exit(EXIT_FAILURE);
    	}
	
	fwrite(shm_ptr, 1, SHM_SIZE, output_file);
	fclose(output_file);
	printf("Imagenes combinadas y guardadas en %s\n", output_path);

	// limpiar
    	munmap(shm_ptr, SHM_SIZE);
    	close(shm_fd);
}
