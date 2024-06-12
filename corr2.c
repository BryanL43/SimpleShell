#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>

#define MAX_ARGS 82
#define MAX_COMMANDS 10

int main(int argc, char* argv[]) {
    // Check for custom prefix prompt, else use "> "
    char* prompt = "> ";

    // Apply custom RUNOPTIONS prompt
    if (argc > 1 && strcmp(argv[1], "") != 0) {
        prompt = argv[1];
    }

    size_t bufferSize = 163;
    char* buffer = malloc(bufferSize * sizeof(char));

    if (buffer == NULL) {
        printf("Failed to create buffer!\n");
        return 1;
    }

    int exitCondition = 0;

    while (exitCondition == 0) {
        // Initialize array for holding command-line args
        char* argArray[MAX_ARGS];
        int argArrayIndex = 0;

        // Read command input
        printf("%s", prompt);
        if (fgets(buffer, bufferSize, stdin) == NULL) {
            if (feof(stdin)) {
                printf("End of file reached!\n");
                break;
            } else {
                printf("Error reading input!\n");
                continue;
            }
        }

        // Check for empty inputs and trigger reprompt
        if (buffer[0] == '\n') {
            printf("Error: No input found!\n");
            continue;
        }

        // Remove newline character, if present
        char *lineBreak = strchr(buffer, '\n');
        if (lineBreak) {
            *lineBreak = '\0';
        }

        // Split input into commands by pipe
        char* commands[MAX_COMMANDS];
        int commandCount = 0;
        char* command = strtok(buffer, "|");
        while (command != NULL) {
            commands[commandCount++] = command;
            command = strtok(NULL, "|");
        }

        int pipefd[2];
        pid_t pid;
        int fd_in = 0; // Input file descriptor for the current command

        for (int i = 0; i < commandCount; i++) {
            pipe(pipefd);

            if ((pid = fork()) == -1) {
                perror("fork");
                exit(EXIT_FAILURE);
            } else if (pid == 0) {
                // In the child process

                // Redirect input
                dup2(fd_in, 0);
                if (i < commandCount - 1) {
                    // Redirect output to pipe
                    dup2(pipefd[1], 1);
                }
                close(pipefd[0]);

                // Split the command into arguments
                argArrayIndex = 0;
                char* token = strtok(commands[i], " ");
                while (token != NULL) {
                    argArray[argArrayIndex++] = token;
                    token = strtok(NULL, " ");
                }
                argArray[argArrayIndex] = NULL;

                // Check for exit input and finish program if so
                if (strcmp(argArray[0], "exit") == 0) {
                    exitCondition = 1;
                    exit(EXIT_SUCCESS);
                }

                execvp(argArray[0], argArray);
                perror("execvp");
                exit(EXIT_FAILURE);
            } else {
                // In the parent process
                wait(NULL); // Wait for the child process to finish
                close(pipefd[1]);
                fd_in = pipefd[0]; // Save the input for the next command
            }
        }
    }

    free(buffer);

    return 0;
}
