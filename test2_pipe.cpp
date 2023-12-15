#include <iostream>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <cstring>
#include <cstdlib>
#include <vector>
using namespace std;

void executeCommand(const char* command) {
    const char* redirect_out_Symbol = strchr(command, '>');
    const char* redirect_in_Symbol = strchr(command, '<');

    if (redirect_out_Symbol != nullptr) {
    	const char* filename = redirect_out_Symbol + 1;
        int fileDescriptor = open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0666);
        if (fileDescriptor < 0) {
            perror("error abriendo redireccion de archivo");
            return;
        }
        dup2(fileDescriptor, STDOUT_FILENO);
        close(fileDescriptor);
        system(command);
        
    } else if (redirect_in_Symbol != nullptr){
    	const char* filename = redirect_in_Symbol + 1;
    	
    	int fileDescriptor = open(filename, O_RDONLY);
    	if (fileDescriptor < 0){
    		perror("Error al abrir el archivo para redireccionar la entrada");
    		return;
    	}
    	dup2(fileDescriptor, STDIN_FILENO);
    	close(fileDescriptor);
        system(command);
        
    }
    system(command);
}

int main() {
    char command[256];

    char* currentDirectory = get_current_dir_name();
    if (currentDirectory != nullptr) {
        string new_path = string(currentDirectory) + ":/bin";
        setenv("PATH", new_path.c_str(), 1);
        free(currentDirectory);
    } else {
        cerr << "Error obtaining current directory." << endl;
        return -1;
    }

    while (1) {
    	cout << "\033[1;37m";
    	cout << "\033[44m";
    	cout << "\033[1m";
    	cout << "Interprete de comandos > ";
        cout << "\033[0m";
        cout << " ";
        fflush(stdout);
        cin.getline(command, sizeof(command));

        if (!strcmp(command, "salir")) {
            return 0;
        } else {
            if (strchr(command, '|') != nullptr) { // Si encuentra el símbolo "|"
                vector<string> commands;
                char* token = strtok(command, "|");

                while (token != nullptr) {
                    commands.push_back(string(token));
                    token = strtok(nullptr, "|");
                }

                int pipes[2 * (commands.size() - 1)];
                for (size_t i = 0; i < commands.size() - 1; ++i) {
                    pipe(pipes + i * 2);
                }

                size_t index = 0;
                for (const auto& cmd : commands) {
                    char* subcommand = new char[cmd.size() + 1];
                    strcpy(subcommand, cmd.c_str());

                    bool last_command = (index == commands.size() - 1);
                    pid_t pid = fork();

                    if (pid < 0) {
                        perror("error del fork");
                        return -1;
                    } else if (pid == 0) {
                        if (!last_command) {
                            dup2(pipes[index * 2 + 1], STDOUT_FILENO);
                        }

                        if (index != 0) {
                            dup2(pipes[(index - 1) * 2], STDIN_FILENO);
                        }

                        for (size_t i = 0; i < 2 * (commands.size() - 1); ++i) {
                            close(pipes[i]);
                        }

                        executeCommand(subcommand);
                        return -1; // Salir del proceso hijo si hay error en la ejecución
                    }

                    delete[] subcommand;
                    ++index;
                }

                for (size_t i = 0; i < 2 * (commands.size() - 1); ++i) {
                    close(pipes[i]);
                }

                for (size_t i = 0; i < commands.size(); ++i) {
                    wait(NULL);
                }
            } else {
                pid_t returnedValue = fork();

                if (returnedValue < 0) {
                    perror("error del fork");
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
    }

    return 0;
}