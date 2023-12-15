#include <iostream>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <cstring>
#include <cstdlib>
#include <vector>
using namespace std;

// Función para redirigir la salida a un archivo
void output_redirection(const char* filename) {
    int fileDescriptor = open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0666);
    if (fileDescriptor < 0) {
        perror("Error abriendo redireccion de archivo");
        return;
    }
    dup2(fileDescriptor, STDOUT_FILENO); // Redirige la salida estándar al archivo
    close(fileDescriptor); // Cierra el descriptor del archivo
}

// Función para redirigir la entrada desde un archivo
void input_redirection(const char* filename) {
    int fileDescriptor = open(filename, O_RDONLY);
    if (fileDescriptor < 0){
        perror("Error al abrir el archivo para redireccionar la entrada");
        return;
    }
    dup2(fileDescriptor, STDIN_FILENO); // Redirige la entrada estándar desde el archivo
    close(fileDescriptor); // Cierra el descriptor del archivo
}

// Función para ejecutar comandos con posibles redirecciones de entrada/salida
void executeCommand(const char* command) {
    const char* redirect_out_Symbol = strchr(command, '>'); // Busca el símbolo '>'
    const char* redirect_in_Symbol = strchr(command, '<'); // Busca el símbolo '<'

    if (redirect_out_Symbol != nullptr) {
        output_redirection(redirect_out_Symbol + 1); // Redirige la salida si hay redirección de salida
    } else if (redirect_in_Symbol != nullptr){
        input_redirection(redirect_in_Symbol + 1); // Redirige la entrada si hay redirección de entrada
    }
    
    system(command); // Ejecuta el comando ingresado
}

// Función para ejecutar comandos con pipes
void executePipedCommands(const char* command) {
    vector<string> commands;
    char* token = strtok(const_cast<char*>(command), "|");

    while (token != nullptr) {
        commands.push_back(string(token));
        token = strtok(nullptr, "|");
    }

    int pipes[2 * (commands.size() - 1)];
    for (size_t i = 0; i < commands.size() - 1; ++i) {
        pipe(pipes + i * 2); // Crea los pipes necesarios para la comunicación entre comandos
    }

    size_t index = 0;
    for (const auto& cmd : commands) {
        char* subcommand = new char[cmd.size() + 1];
        strcpy(subcommand, cmd.c_str());

        bool last_command = (index == commands.size() - 1);
        pid_t pid = fork();

        if (pid < 0) {
            perror("Error del fork");
            exit(EXIT_FAILURE);
        } else if (pid == 0) {
            if (!last_command) {
                dup2(pipes[index * 2 + 1], STDOUT_FILENO); // Redirige la salida al siguiente comando
            }

            if (index != 0) {
                dup2(pipes[(index - 1) * 2], STDIN_FILENO); // Redirige la entrada desde el comando anterior
            }

            for (size_t i = 0; i < 2 * (commands.size() - 1); ++i) {
                close(pipes[i]);
            }

            executeCommand(subcommand); // Ejecuta el subcomando
            exit(EXIT_SUCCESS); // Sale del proceso hijo después de ejecutar el comando
        }

        delete[] subcommand;
        ++index;
    }

    for (size_t i = 0; i < 2 * (commands.size() - 1); ++i) {
        close(pipes[i]); // Cierra los pipes en el proceso padre
    }

    for (size_t i = 0; i < commands.size(); ++i) {
        wait(NULL); // Espera a que todos los procesos hijos terminen
    }
}

// Función principal
int main() {
    char command[256];

    char* currentDirectory = get_current_dir_name(); // Obtiene el directorio actual
    if (currentDirectory != nullptr) {
        string new_path = string(currentDirectory) + ":/bin"; // Agrega '/bin' al PATH
        setenv("PATH", new_path.c_str(), 1); // Actualiza el PATH
        free(currentDirectory); // Libera la memoria del directorio actual obtenido
    } else {
        cerr << "Error obtaining current directory." << endl;
        return -1;
    }

    while (1) {
        // Imprime el indicador de comandos en color y estilo
        cout << "\033[1;37m";
        cout << "\033[44m";
        cout << "\033[1m";
        cout << "Interprete de comandos > ";
        cout << "\033[0m";
        cout << " ";
        fflush(stdout); // Limpia el buffer de salida
        cin.getline(command, sizeof(command)); // Lee el comando ingresado por el usuario

        if (!strcmp(command, "salir")) { // Si el comando ingresado es "salir", termina el programa
            return 0;
        } else {
            if (strchr(command, '|') != nullptr) { // Si encuentra el símbolo "|"
                executePipedCommands(command); // Ejecuta comandos con pipes
            } else {
                pid_t returnedValue = fork();

                if (returnedValue < 0) {
                    perror("Error del fork");
                    return -1;
                } else if (returnedValue == 0) {
                    executeCommand(command); // Ejecuta un comando con posibles redirecciones
                    exit(EXIT_SUCCESS); // Sale del proceso hijo después de ejecutar el comando
                } else {
                    if (waitpid(returnedValue, 0, 0) < 0) {
                        perror("Error esperando proceso hijo");
                        return -1;
                    }
                }
            }
        }
    }

    return 0;
}