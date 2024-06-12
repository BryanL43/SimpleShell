#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>

int main(int argc, char* argv[]) {
    //Check for custom prefix prompt, else use "> "

    char* prompt = "> ";

    //Apply custom RUNOPTIONS prompt
    if(argv[1] != NULL && strcmp(argv[1], "") != 0) {
        prompt = argv[1];
    }

    size_t bufferSize = 163;
    char* buffer = malloc(bufferSize * sizeof(char));

    if (buffer == NULL) {
        printf("Failed to create buffer!\n");
        return 1;
    }

    int exitCondition = 0;
    ssize_t lineSize; //ssize_t to allow for -1

    while (exitCondition == 0) {

        //Read command input and check for failure
        printf("%s", prompt);        
        if (fgets(buffer, bufferSize, stdin) == NULL) {
            //Check for EOF and end program
            if (feof(stdin)) {
                printf("End of file reached!\n");
                break;
            }

            //fgets failed
            printf("Error reading input!\n");
            continue;
        }

        //Check for empty inputs and trigger reprompt
        if (buffer[0] == '\n') {
            printf("Error: No input found!\n");
            continue;
        }

        //Remove newline character, if present
        char *lineBreak = strchr(buffer, '\n');
        if (lineBreak) {
            *lineBreak = '\0';
        }

        //Split input into commands by pipe
        char* cmds[82];
        int cmdsCount = 0;
        char* token = strtok(buffer, "|");

        while (token != NULL) {
            cmds[cmdsCount++] = token;
            token = strtok(NULL, "|");
        }
    
        //Check for exit input and finish program if so
        if (strcmp(cmds[0], "exit") == 0) {
            exitCondition = 1;
            break;
        }

        int pipefd[2];
        int fd_in = 0;
        
        for (int i = 0; i < cmdsCount; i++) {

            if (pipe(pipefd) == -1) {
                printf("Failed to create pipe\n");
                break;
            }

            char* argArray[82];
            int argArrayIndex = 0;

            char* token2 = strtok(cmds[i], " ");

            while (token2 != NULL) {
                argArray[argArrayIndex++] = token2;
                token2 = strtok(NULL, " ");
            }

            //Ensure NULL pointer to stop execvp()
            argArray[argArrayIndex] = NULL;

            //Fork a child process and save its PID
            pid_t pid = fork();

            switch (pid) {
                case -1: //Fork failed
                    printf("fork failed, aborting!\n");
                    exit(EXIT_FAILURE);
                case 0: //In the child process

                    //Redirect input
                    dup2(fd_in, 0);

                    //Redirect output back to stdout if we are in the last command
                    if (i < cmdsCount - 1) {
                        dup2(pipefd[1], 1);
                    }
                    close(pipefd[0]);

                    execvp(argArray[0], argArray);

                    //If execvp returns, an error occurred
                    printf("execvp failed!\n");
                    exit(EXIT_FAILURE);
                default: //Child terminated
                    int status;
                    waitpid(pid, &status, 0);

                    if (WIFEXITED(status)) {
                        printf("Child %d exited with status %d\n", pid, WEXITSTATUS(status));
                        close(pipefd[1]);
                        fd_in = pipefd[0]; //Save the input for the next command
                    } else {
                        printf("Child %d did not terminate normally\n", pid);
                    }
            }
        }
    }

    free(buffer);

    return 0;
}