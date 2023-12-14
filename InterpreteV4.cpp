#include <iostream>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <cstring>
#include <cstdlib>
using namespace std;

// Función para ejecutar un comando con posible redirección de salida
void executeCommand(const char* command) {
    // Buscar el carácter de redirección ">"
    const char* redirectSymbol = strchr(command, '>');

    if (redirectSymbol != nullptr) {
        // Se encontró el símbolo de redirección ">"
        // Obtener el nombre del archivo de destino para la redirección
        const char* filename = redirectSymbol + 1;

        // Crear un descriptor de archivo para el archivo de destino
        int fileDescriptor = open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0666);
        if (fileDescriptor < 0) {
            perror("error abriendo redireccion de archivo");
            return;
        }

        // Redirigir la salida estándar al archivo
        dup2(fileDescriptor, STDOUT_FILENO);

        // Cerrar el descriptor de archivo que ya no se necesita
        close(fileDescriptor);

        // Ejecutar el comando
        system(command);
    } else {
        // No se encontró el símbolo de redirección, ejecutar el comando normalmente
        system(command);
    }
}

int main() {
    char command[256];

    // Obtener el directorio actual
    char* currentDirectory = get_current_dir_name();
	if (currentDirectory != nullptr) {
	    // Agregar el directorio actual y /bin a la variable de entorno PATH
	    string new_path = string(currentDirectory) + ":/bin";
	    setenv("PATH", new_path.c_str(), 1);
	    free(currentDirectory);
	} else {
	    cerr << "Error obtaining current directory." << endl;
	    return -1;
	}

    while (1) { // loop until return
        cout << "Interprete de comandos > ";
        fflush(stdout);
        cin.getline(command, sizeof(command));

        if (!strcmp(command, "salir")) {
            return 0;
        } else {
            pid_t returnedValue = fork();

            if (returnedValue < 0) {
                perror("error  del fork");
                return -1;
            } else if (returnedValue == 0) {
                // En el proceso hijo, ejecutar el comando con posible redirección
                executeCommand(command);

                // Salir del proceso hijo si hubo un error en la ejecución
                return -1;
            } else {
                // En el proceso padre, esperar al proceso hijo
                if (waitpid(returnedValue, 0, 0) < 0) {
                    perror("error esperando proceso hijo");
                    return -1;
                }
            }
        }
    }

    return 0;
}