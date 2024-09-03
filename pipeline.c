#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <semaphore.h>

#define SHM_READY "/img_ready"
#define SHM_BLUR "/blur_done"
#define SHM_EDGE "/edge_done"

int main(int argc, char *argv[]) {

	// control de error
	if (argc != 3) {
		fprintf(stderr, "Uso: %s <ruta_imagen> <ruta_salida>\n", argv[0]);
		exit(EXIT_FAILURE);
	}

	// rutas
	const char *image_path = argv[1];
	const char *output_path = argv[2];

	// inicializar semaforos
	sem_t *sem_img_ready = sem_open(SHM_READY, O_CREAT, 0666, 0);
	sem_t *sem_blur_done = sem_open(SHM_BLUR, O_CREAT, 0666, 0);
	sem_t *sem_edge_done = sem_open(SHM_EDGE, O_CREAT, 0666, 0);

	if (sem_img_ready == SEM_FAILED || sem_blur_done == SEM_FAILED || sem_edge_done == SEM_FAILED) {
		perror("Pipeline: Error al crear semaforos\n");
		exit(EXIT_FAILURE);
	}

	// ejecutar publicador
	pid_t pid_publicador = fork();
	if (pid_publicador == 0) {
		execl("./publicador", "publicador", image_path, NULL);
		perror("Pipeline: Error al ejecutar publicador\n");
		exit(EXIT_FAILURE);
	}

	// ejecutar desenfocador
	pid_t pid_desenfocador = fork();
    	if (pid_desenfocador == 0) {
        	execl("./desenfocador", "desenfocador", NULL);
        	perror("Pipeline: Error al ejecutar desenfocador\n");
        	exit(EXIT_FAILURE);
    	}

	// ejecutar realzador
	pid_t pid_realzador = fork();
    	if (pid_realzador == 0) {
        	execl("./realzador", "realzador", NULL);
        	perror("Pipeline: Error al ejecutar realzador\n");
        	exit(EXIT_FAILURE);
    	}

	// Esperar a que los procesos hijos terminen
    	wait(NULL); // Esperar a publicador
    	wait(NULL); // Esperar a desenfocador
    	wait(NULL); // Esperar a realzador

    	// Ejecutar Combinador
    	execl("./combinador", "combinador", output_path, NULL);
    	perror("Pipeline: Error al ejecutar combinador\n");
    	exit(EXIT_FAILURE);
}
