#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

void run_process(const char *program, const char *arg1, const char *arg2, const char *arg3) {
    pid_t pid = fork();
    if (pid == 0) {
        execl(program, program, arg1, arg2, arg3, NULL);
        perror("execl");
        exit(EXIT_FAILURE);
    } else if (pid < 0) {
        perror("fork");
        exit(EXIT_FAILURE);
    }
    waitpid(pid, NULL, 0);
}

int main(int argc, char *argv[]) {
    if (argc != 4) {
        fprintf(stderr, "Uso: %s <ruta_imagen> <ruta_imagen_salida> <numero_hilos>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    printf("Iniciando el proceso...\n");

    // Ejecutar Publicador
    run_process("./publicador", argv[1], NULL, NULL);

    // Ejecutar Desenfocador
    run_process("./desenfocador", argv[3], NULL, NULL);

    // Ejecutar Realzador
    run_process("./realzador", argv[3], NULL, NULL);

    // Ejecutar Combinador
    run_process("./combinador", argv[2], NULL, NULL);

    printf("Proceso terminado.\n");

    exit(EXIT_SUCCESS);
}

