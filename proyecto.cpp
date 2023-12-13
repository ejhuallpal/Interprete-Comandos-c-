#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>

#define MAX_INPUT_SIZE 1024
#define MAX_ARG_SIZE 64

void run_command(char *command, char *args[]);

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
        run_command(args[0], args);
    }

    return 0;
}

void run_command(char *command, char *args[]) {
    pid_t pid = fork();

    if (pid == -1) {
        perror("Error en fork");
        exit(EXIT_FAILURE);
    } else if (pid == 0) {
        // Este bloque se ejecuta en el proceso hijo

        // Buscar el símbolo de redirección '>'
        int i = 0;
        while (args[i] != NULL) {
            if (strcmp(args[i], ">") == 0) {
                // Redirección detectada, abrir el archivo para escritura
                int fd = open(args[i + 1], O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
                if (fd == -1) {
                    perror("Error al abrir el archivo de salida");
                    exit(EXIT_FAILURE);
                }

                // Redirigir la salida estándar al archivo
                dup2(fd, STDOUT_FILENO);

                // Cerrar el descriptor de archivo después de la redirección
                close(fd);

                // Eliminar los elementos de redirección de los argumentos
                args[i] = NULL;
                break;
            }
            i++;
        }

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