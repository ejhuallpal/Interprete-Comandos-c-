#include <iostream>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <cstring>
#include <cstdlib>
#include <vector>
using namespace std;

void output_redirection(const char* filename) {
    int fileDescriptor = open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0666);
    if (fileDescriptor < 0) {
        perror("Error abriendo redireccion de archivo");
        return;
    }
    dup2(fileDescriptor, STDOUT_FILENO);
    close(fileDescriptor);
}

void input_redirection(const char* filename) {
    int fileDescriptor = open(filename, O_RDONLY);
    if (fileDescriptor < 0){
        perror("Error al abrir el archivo para redireccionar la entrada");
        return;
    }
    dup2(fileDescriptor, STDIN_FILENO);
    close(fileDescriptor);
}

void executeCommand(const char* command) {
    const char* redirect_out_Symbol = strchr(command, '>');
    const char* redirect_in_Symbol = strchr(command, '<');

    if (redirect_out_Symbol != nullptr) {
        output_redirection(redirect_out_Symbol + 1);
    } else if (redirect_in_Symbol != nullptr){
        input_redirection(redirect_in_Symbol + 1);
    }
    
    system(command);
}

void executePipedCommands(const char* command) {
    vector<string> commands;
    char* token = strtok(const_cast<char*>(command), "|");

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
            perror("Error del fork");
            exit(EXIT_FAILURE);
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
            exit(EXIT_SUCCESS);
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
            if (strchr(command, '|') != nullptr) {
                executePipedCommands(command);
            } else {
                pid_t returnedValue = fork();

                if (returnedValue < 0) {
                    perror("error del fork");
                    return -1;
                } else if (returnedValue == 0) {
                    executeCommand(command);
                    exit(EXIT_SUCCESS);
                } else {
                    if (waitpid(returnedValue, 0, 0) < 0) {
                        perror("error esperando proceso hijo");
                        return -1;
                    }
                }
            }
        }
    }

    return 0;
}
