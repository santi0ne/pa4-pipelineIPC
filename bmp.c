#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include "bmp.h"
/* USE THIS FUNCTION TO PRINT ERROR MESSAGES
   DO NOT MODIFY THIS FUNCTION
*/
void printError(int error){
	switch(error){
  		case ARGUMENT_ERROR:
    			printf("Usage:ex5 <source> <destination>\n");
    			break;
  		case FILE_ERROR:
    			printf("Unable to open file!\n");
    			break;
  		case MEMORY_ERROR:
    			printf("Unable to allocate memory!\n");
    			break;
  		case VALID_ERROR:
    			printf("BMP file not valid!\n");
    			break;
  		default:
    		break;
  	}
}

/* The input argument is the source file pointer. The function will first construct a BMP_Image image by allocating memory to it.
 * Then the function read the header from source image to the image's header.
 * Compute data size, width, height, and bytes_per_pixel of the image and stores them as image's attributes.
 * Finally, allocate menory for image's data according to the image size.
 * Return image;
*/
BMP_Image* createBMPImage(FILE* fptr) {

	//Allocate memory for BMP_Image*;
	BMP_Image* image = (BMP_Image *)malloc(sizeof(BMP_Image));
	if (!image) {
		printf("error al asignar memoria con malloc BMP_Image\n");
		printError(MEMORY_ERROR);
		return NULL;
	}
	
	//Read the first 54 bytes of the source into the header
	size_t bytes_read = fread(&(image->header), sizeof(BMP_Header), 1, fptr);
	if (bytes_read != 1) {
		printf("error al leer los 54 bytes de header\n");
		printError(FILE_ERROR);
		free(image);
		return NULL;
	}
	
	//Compute data size, width, height, and bytes per pixel;
	image->norm_height = abs(image->header.height_px);
    	int width = image->header.width_px;
    	int height = image->norm_height;
    	int bytesPerPixel = image->header.bits_per_pixel / 8;
    	image->bytes_per_pixel = bytesPerPixel;

    	int row_size = ((width * bytesPerPixel + 3) / 4) * 4;

	//Allocate memory for image data
	image->pixels = (Pixel **)malloc(height * sizeof(Pixel *));
	if (!image->pixels) {
		printf("error al alojar memoria para pixeles de image\n");
		printError(MEMORY_ERROR);
		free(image);
		return NULL;
	}
	
	for (int i = 0; i < height; i++) {
        	image->pixels[i] = (Pixel *)malloc(row_size);
        	if (!image->pixels[i]) {
            		printError(MEMORY_ERROR);
            		for (int j = 0; j < i; j++) {
                		free(image->pixels[j]);
            		}
            		free(image->pixels);
            		free(image);
            		return NULL;
        	}
		memset(image->pixels[i], 0, row_size);
    	}

	fseek(fptr, image->header.offset, SEEK_SET);

	return image;
}

/* The input arguments are the source file pointer, the image data pointer, and the size of image data.
 * The functions reads data from the source into the image data matriz of pixels.
*/
void readImageData(FILE* srcFile, BMP_Image * image, int dataSize) {
	int height = image->norm_height;
	int width = image->header.width_px;
	int bytesPerPixel = image->bytes_per_pixel;
	int row_size = ((width * bytesPerPixel + 3) / 4) * 4;

	if (image->header.height_px > 0) {
		// imagen almacenada de abajo hacia arriba
		for (int i=0; i<height; i++) {
			if (fread(image->pixels[height-1-i], row_size, 1, srcFile) != 1) {
				printf("error al leer imagen almacenada de abajo hacia arriba\n");
				printError(FILE_ERROR);
				for (int j=height-1; j>=i; j--) {
					free(image->pixels[j]);
				}
				free(image->pixels);
				free(image);
				return;
			}
		}
	}
	else {
		// imagen almacenada de arriba hacia abajo
		for (int i=0; i<height; i++) {
			if (fread(image->pixels[i], row_size, 1, srcFile) != 1) {
				printf("error al leer imagen almacenada de arriba hacia abajo\n");
				printError(FILE_ERROR);
				for (int j=0; j<=i; j++) {
					free(image->pixels[j]);
				}
				free(image->pixels);
				free(image);
				return;
			}
		}
	}
}

/* The input arguments are the pointer of the binary file, and the image data pointer.
 * The functions open the source file and call to CreateBMPImage to load de data image.
*/
void readImage(FILE *srcFile, BMP_Image *dataImage) {
	BMP_Image *image = createBMPImage(srcFile);
	if (!image) {
		printf("error al crear bmp de source\n");
		printError(VALID_ERROR);
		return;
	}

	int row_size = ((image->header.width_px * image->bytes_per_pixel + 3) / 4) * 4;
    	int data_size = row_size * image->norm_height;

	readImageData(srcFile, image, data_size);
	
	// Alojar memoria para dataImage en caso de este ser NULL
    	if (dataImage == NULL) {
        	dataImage = (BMP_Image *)malloc(sizeof(BMP_Image));
        	if (!dataImage) {
            		printf("Error allocating memory for dataImage\n");
            		printError(MEMORY_ERROR);
            		free(image->pixels);
            		free(image);
            		return;
        	}
    	}
    	
    	// Copiar el encabezado de la imagen
    	memcpy(&dataImage->header, &image->header, sizeof(BMP_Header));
    	dataImage->norm_height = image->norm_height;
    	dataImage->bytes_per_pixel = image->bytes_per_pixel;

    	// Alojar memoria para los pixeles de dataImage
    	dataImage->pixels = (Pixel **)malloc(dataImage->norm_height * sizeof(Pixel *));
    	if (!dataImage->pixels) {
        	printf("Error allocating memory for dataImage pixels\n");
        	printError(MEMORY_ERROR);
        	free(image->pixels);
        	free(image);
        	free(dataImage);
        	return;
    	}

    	for (int i = 0; i < dataImage->norm_height; i++) {
        	dataImage->pixels[i] = (Pixel *)malloc(row_size);
        	if (!dataImage->pixels[i]) {
            		printf("Error allocating memory for row of dataImage pixels\n");
            		printError(MEMORY_ERROR);
            		for (int j = 0; j < i; j++) {
                		free(dataImage->pixels[j]);
            		}
            		free(dataImage->pixels);
            		free(image->pixels);
            		free(image);
            		free(dataImage);
            		return;
        	}
        	memcpy(dataImage->pixels[i], image->pixels[i], row_size);
    	}

    	freeImage(image);
}

/* The input arguments are the destination file name, and BMP_Image pointer.
 * The function write the header and image data into the destination file.
*/
void writeImage(char* destFileName, BMP_Image* dataImage) {
	// abrir archivo de destino para escritura binaria
	FILE *destFile = fopen(destFileName, "wb");
	if (!destFile) {
		printf("error al abrir archivo de destino en modo escritura binaria\n");
		printError(FILE_ERROR);
		return;
	}

	// escribir el header de imagen en archivo de destino
	if (fwrite(&(dataImage->header), HEADER_SIZE, 1, destFile) != 1) {
		printf("error al escribir header en archivo destino\n");
		printError(FILE_ERROR);
		fclose(destFile);
		return;
	}

	int row_size = ((dataImage->header.width_px * dataImage->bytes_per_pixel + 3) / 4) * 4;
	fseek(destFile, dataImage->header.offset, SEEK_SET);

	// escribir los datos de pixeles en destino
	int height = dataImage->norm_height;

	if (dataImage->header.height_px > 0) {
        	// La imagen está almacenada de abajo hacia arriba
        	for (int i = height - 1; i >= 0; i--) {
            		if (fwrite(dataImage->pixels[i], row_size, 1, destFile) != 1) {
                		printf("error al escribir pixeles de abajo hacia arriba\n");
				printError(FILE_ERROR);
                		fclose(destFile);
                		return;
            		}
        	}
    	}
	else {
        	// La imagen está almacenada de arriba hacia abajo
        	for (int i = 0; i < height; i++) {
            		if (fwrite(dataImage->pixels[i], row_size, 1, destFile) != 1) {
                		printf("error al escribir pixeles de arriba hacia abajo\n");
				printError(FILE_ERROR);
                		fclose(destFile);
                		return;
            		}
        	}
    	}

	fclose(destFile);
}

/* The input argument is the BMP_Image pointer. The function frees memory of the BMP_Image.
*/
void freeImage(BMP_Image* image) {
	if (!image) return;
	
	// liberar la memoria de cada fila de la matriz de pixeles
	int height = image->norm_height;
	for (int i=0; i<height; i++) {
		free(image->pixels[i]);
	}
	
	// liberar la memoria de la matriz de punteros a filas
	free(image->pixels);

	// liberar la memoria de la estructura BMP_Image
	free(image);
}

/* The functions checks if the source image has a valid format.
 * It returns TRUE if the image is valid, and returns FASLE if the image is not valid.
 * DO NOT MODIFY THIS FUNCTION
*/
int checkBMPValid(BMP_Header* header) {
  // Make sure this is a BMP file
  if (header->type != 0x4d42) {
    return FALSE;
  }
  // Make sure we are getting 24 bits per pixel
  if (header->bits_per_pixel != 24) {
    return FALSE;
  }
  // Make sure there is only one image plane
  if (header->planes != 1) {
    return FALSE;
  }
  // Make sure there is no compression
  if (header->compression != 0) {
    return FALSE;
  }
  return TRUE;
}

/* The function prints all information of the BMP_Header.
   DO NOT MODIFY THIS FUNCTION
*/
void printBMPHeader(BMP_Header* header) {
  printf("file type (should be 0x4d42): %x\n", header->type);
  printf("file size: %d\n", header->size);
  printf("offset to image data: %d\n", header->offset);
  printf("header size: %d\n", header->header_size);
  printf("width_px: %d\n", header->width_px);
  printf("height_px: %d\n", header->height_px);
  printf("planes: %d\n", header->planes);
  printf("bits: %d\n", header->bits_per_pixel);
}

/* The function prints information of the BMP_Image.
   DO NOT MODIFY THIS FUNCTION
*/
void printBMPImage(BMP_Image* image) {
  printf("data size is %ld\n", sizeof(image->pixels));
  printf("norm_height size is %d\n", image->norm_height);
  printf("bytes per pixel is %d\n", image->bytes_per_pixel);
}
