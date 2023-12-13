#include <iostream>
#include <unistd.h>
#include <sys/wait.h>
#include <cstring>
#include <vector>
using namespace std;


void executeCommand(const char* command) {
    system(command);
}

int main() {
    char command[256];

    while (1) {
        cout << "Command> ";
        fflush(stdout);
        cin.getline(command, sizeof(command));

        if (!strcmp(command, "exit")) {
            return 0;
        } else {
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
                    perror("error forking");
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
                    exit(0);
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
    }

    return 0;
}