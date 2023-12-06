#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

#define MAX_INPUT_SIZE 1024
#define MAX_ARG_SIZE 64

void run_command(char *command);

int main() {
    char input[MAX_INPUT_SIZE];

    while (1) {
        // Mostrar el prompt personalizado
        printf("MiShell> ");
        
        // Obtener la entrada del usuario
        fgets(input, sizeof(input), stdin);

        // Eliminar el salto de línea al final de la entrada
        input[strcspn(input, "\n")] = '\0';

        // Verificar si se ingresó "salir"
        if (strcmp(input, "salir") == 0) {
            break;
        }

        // Tokenizar la entrada para obtener el comando y sus argumentos
        char *token;
        char *args[MAX_ARG_SIZE];
        int i = 0;

        token = strtok(input, " ");
        while (token != NULL) {
            args[i++] = token;
            token = strtok(NULL, " ");
        }
        args[i] = NULL;

        // Ejecutar el comando
        run_command(args[0]);
    }

    return 0;
}

void run_command(char *command) {
    pid_t pid = fork();

    if (pid == -1) {
        perror("Error en fork");
        exit(EXIT_FAILURE);
    } else if (pid == 0) {
        // Este bloque se ejecuta en el proceso hijo
        // Ejecutar el comando
        execvp(command, args);
        
        // Si execvp falla, imprimir un mensaje de error
        perror("Error al ejecutar el comando");
        exit(EXIT_FAILURE);
    } else {
        // Este bloque se ejecuta en el proceso padre
        // Esperar a que el proceso hijo termine
        wait(NULL);
    }
}
