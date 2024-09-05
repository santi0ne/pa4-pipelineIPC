#include <stdio.h>
#include <stdlib.h>
#include "bmp.h"

// Función para imprimir errores
void printError(int error) {
    switch (error) {
        case ARGUMENT_ERROR:
            printf("Argument Error\n");
            break;
        case FILE_ERROR:
            printf("File Error\n");
            break;
        case MEMORY_ERROR:
            printf("Memory Error\n");
            break;
        case VALID_ERROR:
            printf("BMP file not valid!\n");
            break;
        default:
            printf("Unknown Error\n");
            break;
    }
}

// Crear estructura BMP_Image
BMP_Image* createBMPImage() {
    BMP_Image* newImage = (BMP_Image*) malloc(sizeof(BMP_Image));
    if (newImage == NULL) {
        printError(MEMORY_ERROR);
        return NULL;
    }
    newImage->pixels = NULL;
    return newImage;
}

// Leer datos de la imagen
void readImageData(FILE *srcFile, BMP_Image *dataImage, int dataSize) {
    dataImage->pixels = (Pixel**) malloc(sizeof(Pixel*) * dataImage->norm_height);
    if (dataImage->pixels == NULL) {
        printError(MEMORY_ERROR);
        return;
    }

    for (int i = 0; i < dataImage->norm_height; i++) {
        dataImage->pixels[i] = (Pixel*) malloc(sizeof(Pixel) * dataImage->header.width_px);
        if (dataImage->pixels[i] == NULL) {
            printError(MEMORY_ERROR);
            return;
        }
    }

    fseek(srcFile, dataImage->header.offset, SEEK_SET);
    for (int i = 0; i < dataImage->norm_height; i++) {
        fread(dataImage->pixels[i], sizeof(Pixel), dataImage->header.width_px, srcFile);
    }
}

// Leer el encabezado y la imagen completa
void readImage(FILE *srcFile, BMP_Image *dataImage) {
    // Verificar si el archivo está abierto correctamente
    if (srcFile == NULL) {
        printError(FILE_ERROR);
        return;
    }

    //printf("Iniciando lectura de imagen en formato BMP\n");

    // Leer encabezado del archivo
    if (fread(&(dataImage->header), sizeof(BMP_Header), 1, srcFile) != 1) {
        printError(FILE_ERROR);
        return;
    }

    // Validar el BMP
    //printf("Validando BMP header...\n");
    if (checkBMPValid(&(dataImage->header)) == FALSE) {
        printError(VALID_ERROR);
        return;
    }

    // Calcular altura normalizada y bytes por píxel
    dataImage->norm_height = abs(dataImage->header.height_px);
    dataImage->bytes_per_pixel = dataImage->header.bits_per_pixel / 8;

    // Imprimir información del BMP
    printf("Tipo de archivo: %x\n", dataImage->header.type);
    printf("Bits por pixel: %u\n", dataImage->header.bits_per_pixel);
    printf("Planos: %u\n", dataImage->header.planes);
    printf("Compresión: %u\n", dataImage->header.compression);

    // Leer datos de la imagen
    int dataSize = dataImage->header.width_px * dataImage->norm_height * dataImage->bytes_per_pixel;
    readImageData(srcFile, dataImage, dataSize);
}

// Escribir la imagen en un archivo
void writeImage(char* destFileName, BMP_Image* dataImage) {
    FILE *destFile = fopen(destFileName, "wb");
    if (destFile == NULL) {
        printError(FILE_ERROR);
        return;
    }

    // Escribir encabezado
    fwrite(&(dataImage->header), sizeof(BMP_Header), 1, destFile);

    // Escribir datos de imagen
    for (int i = 0; i < dataImage->norm_height; i++) {
        fwrite(dataImage->pixels[i], sizeof(Pixel), dataImage->header.width_px, destFile);
    }

    fclose(destFile);
}

// Liberar la memoria de la imagen
void freeImage(BMP_Image* image) {
    for (int i = 0; i < image->norm_height; i++) {
        free(image->pixels[i]);
    }
    free(image->pixels);
    free(image);
}

// Validar el encabezado BMP
int checkBMPValid(BMP_Header* header) {
    printf("Validando BMP header...\n");

    // Verificar el tipo de archivo
    if (header->type != 0x4D42) {
        printf("El tipo de archivo no es BMP (esperado 0x4D42, recibido: 0x%x)\n", header->type);
        return FALSE;
    }

    // Verificar bits por píxel (24 bits)
    if (header->bits_per_pixel != 24) {
        printf("El archivo no tiene 24 bits por píxel (esperado 24, recibido: %u)\n", header->bits_per_pixel);
        return FALSE;
    }

    // Verificar que no haya compresión (debe ser 0)
    if (header->compression != 0) {
        printf("El archivo BMP está comprimido (esperado 0, recibido: %u)\n", header->compression);
        return FALSE;
    }

    // Verificar que haya un solo plano
    if (header->planes != 1) {
        printf("El archivo BMP tiene más de un plano (esperado 1, recibido: %u)\n", header->planes);
        return FALSE;
    }

    return TRUE;
}

// Imprimir el encabezado BMP
void printBMPHeader(BMP_Header* header) {
    printf("BMP Header Information:\n");
    printf("Tipo de archivo: %x\n", header->type);
    printf("Tamaño del archivo: %u bytes\n", header->size);
    printf("Ancho de la imagen: %d px\n", header->width_px);
    printf("Alto de la imagen: %d px\n", header->height_px);
    printf("Bits por pixel: %u\n", header->bits_per_pixel);
    printf("Compresión: %u\n", header->compression);
}

// Imprimir la información de la imagen BMP
void printBMPImage(BMP_Image* image) {
    printBMPHeader(&(image->header));
    printf("Altura normalizada: %d\n", image->norm_height);
    printf("Bytes por píxel: %d\n", image->bytes_per_pixel);
}

