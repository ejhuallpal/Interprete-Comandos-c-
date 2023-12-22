#include <iostream>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <cstring>
#include <cstdlib>
#include <vector>
using namespace std;

// Función para redireccionar la salida a un archivo si hay símbolo '>'
void output_redirection(const char* command) {
    const char* redirect_out_Symbol = strchr(command, '>');
    if (redirect_out_Symbol != nullptr) {
        const char* filename = redirect_out_Symbol + 1;
        while (*filename && isspace(*filename)){
            ++filename;
        }
        int fileDescriptor = open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0666);
        if (fileDescriptor < 0) {
            perror("error abriendo redireccion de archivo");
            return;
        }
        dup2(fileDescriptor, STDOUT_FILENO);
        close(fileDescriptor);
    }
}

// Función para redireccionar la entrada desde un archivo si hay símbolo '<'
void input_redirection(const char* command) {
    const char* redirect_in_Symbol = strchr(command, '<');
    if (redirect_in_Symbol != nullptr){
        const char* filename = redirect_in_Symbol + 1;
        while (*filename && isspace(*filename)){
            ++filename;
        }
        int fileDescriptor = open(filename, O_RDONLY);
        if (fileDescriptor < 0){
            perror("Error al abrir el archivo para redireccionar la entrada");
            return;
        }
        dup2(fileDescriptor, STDIN_FILENO);
        close(fileDescriptor);
    }
}

// Función para ejecutar comandos con posibles redirecciones de entrada/salida
void executeCommand(const char* command) {
    output_redirection(command); // Redirige la salida si hay redirección de salida
    input_redirection(command); // Redirige la entrada si hay redirección de entrada
    system(command); // Ejecuta el comando ingresado
}

// Función para ejecutar comandos con pipes
void executePipedCommands(const char* command) {
    vector<string> commands;
    char* token = strtok(const_cast<char*>(command), "|");

    while (token != nullptr) {
        string cmd = string(token);
        size_t start = cmd.find_first_not_of(" \t");
        size_t end = cmd.find_last_not_of(" \t");

        if (start != string::npos && end != string::npos)
            commands.push_back(cmd.substr(start, end - start + 1));

        token = strtok(nullptr, "|");
    }

    if (!commands.empty()) {
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
                perror("error del fork");
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
}

//Funcion para recononcer el comando "cd"
void change_directory(const char* command) {
    char* path = strtok(const_cast<char*>(command), " "); // Obtener el argumento de la ruta
    path = strtok(NULL, " "); // Obtener el siguiente token
    if (path != nullptr) {
    	chdir(path);
    } else {
        cerr << "cd: Falta argumento de directorio" <<endl;
    }
}

int main() {
    char command[256];

    // Obtener el directorio actual y agregarlo al PATH del sistema
    char* currentDirectory = get_current_dir_name();
    if (currentDirectory != nullptr) {
        string new_path = string(currentDirectory) + ":/bin";
        setenv("PATH", new_path.c_str(), 1);
        free(currentDirectory);
    } else {
        cerr << "Error obtaining current directory." << endl; // Error si no se puede obtener el directorio actual
        return -1;
    }

    // Bucle principal del intérprete de comandos
    while (1) {
        // Mostrar el indicador del intérprete de comandos
        cout << "\033[1;37m";
        cout << "\033[44m";
        cout << "\033[1m";
        cout << "Interprete de comandos > ";
        cout << "\033[0m";
        cout << " ";
        fflush(stdout);

        // Leer la entrada del usuario
        cin.getline(command, sizeof(command));

        // Comprobar si el comando es "salir" para terminar el intérprete
        if (!strcmp(command, "salir")) {
            return 0; // Finalizar el programa si se ingresa "salir"
        } else {
            // Verificar si se ingreso el comando "cd" para efectuar el cambio de directorio
            if(!strncmp(command, "cd", 2)){
                change_directory(command);
            }
            // Verificar si hay pipes en el comando ingresado
            if (strchr(command, '|') != nullptr) { // Si hay símbolo "|", se ejecutan comandos con pipes
                executePipedCommands(command);
            } else {
                pid_t returnedValue = fork();

                if (returnedValue < 0) {
                    perror("error del fork"); // Error si el fork falla
                    return -1;
                } else if (returnedValue == 0) {
                    executeCommand(command); // Ejecutar el comando ingresado
                    exit(EXIT_SUCCESS); // Salir del proceso hijo después de ejecutar el comando
                } else {
                    // En el proceso padre, esperar al proceso hijo para completar la ejecución
                    if (waitpid(returnedValue, 0, 0) < 0) {
                        perror("error esperando proceso hijo"); // Error si hay problemas al esperar al proceso hijo
                        return -1;
                    }
                }
            }
        }
    }

    return 0;
}
