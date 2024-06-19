/**************************************************************
* Class::  CSC-415-01 Summer 2024
* Name:: Bryan Lee
* Student ID:: 922649673
* GitHub-Name:: BryanL43
* Project:: Assignment 3 – Simple Shell with Pipes
*
* File:: lee_bryan_HW3_main.c
*
* Description:: The program is a simple shell that runs on top of the regular
* command-line interpreter for Linux and supports piping. It takes the user's input,
* parses it into commands and arguments, and executes them within the shell.
*
**************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>

#define BUFFER_SIZE 163
#define READ_END 0
#define WRITE_END 1

int main(int argc, char* argv[]) {

    //Verify if a custom prefix prompt is set; otherwise, use "> "
    char* prompt = "> ";
    if(argv[1] != NULL && strcmp(argv[1], "") != 0) {
        prompt = argv[1];
    }

    //Instantiate a buffer of 163 Bytes
    char* buffer = malloc(BUFFER_SIZE);

    //End the program if instantiation for buffer fails
    if (buffer == NULL) {
        printf("Failed to create buffer!\n");
        return 1;
    }

    int exitCondition = 0;

    //Repeatedly prompts the user until exit command is inputted
    //During each iteration of the loop, input will be
    //received, parsed, and the command executed.
    while (exitCondition == 0) {

        printf("%s", prompt);

        //Read the command line the user inputted and check for failure
        if (fgets(buffer, BUFFER_SIZE, stdin) == NULL) {
            //Check if end-of-file (EOF) was reached and exit the prompt loop
            if (feof(stdin)) {
                printf("\n");
                break;
            }

            printf("Error reading input!\n");
            continue;
        }

        //Check for empty inputs and prompt the user again
        if (buffer[0] == '\n') {
            printf("Error: No input found!\n");
            continue;
        }

        //Remove newline character from the buffer
        char *lineBreak = strchr(buffer, '\n');
        if (lineBreak) {
            *lineBreak = '\0';
        }

        //Exit the program if the command input is "exit"
        if (strcmp(buffer, "exit") == 0) {
            exitCondition = 1;
            break;
        }

        char* cmds[(BUFFER_SIZE / 2) + 1][(BUFFER_SIZE / 2) + 1];
        int cmdsCount = 0;

        char* saveptr1;
        char* saveptr2;
        int i = 0, j;

        char* token = strtok_r(buffer, "|", &saveptr1);
        char* subtoken;

        while (token != NULL) {
            j = 0;
            subtoken = strtok_r(token, " ", &saveptr2);
            while(subtoken != NULL) {
                cmds[i][j] = subtoken;
                j++;
                subtoken = strtok_r(NULL, " ", &saveptr2);
            }
            cmds[i][j] = NULL; //NULL terminate the argument array
            i++;
            token = strtok_r(NULL, "|", &saveptr1);
        }
        cmdsCount = i;
    
        if (cmdsCount == 1) { //Handles normal commands without pipes
            //Fork a child process and save its PID
            pid_t pid = fork();

            //Handle different outcomes of the child process creation operation
            switch (pid) {
                case -1: //Fork failed
                    perror("fork failed!\n");
                    exit(EXIT_FAILURE);
                case 0: //In the child process that was successfully created
                    execvp(cmds[0][0], cmds[0]);

                    perror("execvp failed!\n");
                    exit(EXIT_FAILURE);
                default: //In the parent process after child process exits
                    //Parent needs to wait for the termination of the child process
                    int status;
                    waitpid(pid, &status, 0);

                    //Check if the child process exited normally
                    if (WIFEXITED(status)) {
                        printf("Child %d exited with status %d\n", pid, WEXITSTATUS(status));
                    } else {
                        printf("Child %d did not terminate normally\n", pid);
                    }
            }
        } else if (cmdsCount > 1) { //Handles commands with pipes
            int pipefd[2]; //Pipe file descriptor
            int oldFileDesc = READ_END; //Tracks the read end of a pipe

            for (int i = 0; i < cmdsCount; i++) {
                //Creates a pipe with pipe file descriptor and checks for errors.
                //Do not create a pipe for last command
                if (i < cmdsCount - 1) {
                    if (pipe(pipefd) < 0) {
                        perror("Failed to create a pipe!\n");
                        exit(EXIT_FAILURE);
                    }
                }

                //Fork a child process and save its PID
                pid_t pid = fork();

                //Handle different outcomes of the child process creation operation
                switch (pid) {
                    case -1: //Fork failed
                        perror("fork failed!\n");
                        exit(EXIT_FAILURE);
                    case 0: //In the child process that was successfully created
                        if (i > 0) {
                            if (dup2(oldFileDesc, READ_END) == -1) {
                                perror("dup2 for read end failed!\n");
                                exit(EXIT_FAILURE);
                            }
                            close(oldFileDesc);
                        }

                        if (cmds[i + 1][0] != NULL) {
                            if (dup2(pipefd[WRITE_END], WRITE_END) == -1) {
                                perror("dup2 for write end failed!\n");
                                exit(EXIT_FAILURE);
                            }
                        }

                        close(pipefd[READ_END]);
                        close(pipefd[WRITE_END]);

                        execvp(cmds[i][0], cmds[i]);

                        perror("execvp failed!\n");
                        exit(EXIT_FAILURE);
                    default: //In the parent process after child process exits
                        close(pipefd[WRITE_END]);

                        if (oldFileDesc != READ_END) {
                            close(oldFileDesc);
                        }
                        
                        oldFileDesc = pipefd[READ_END];

                        //Parent needs to wait for the termination of the child process
                        int status;
                        waitpid(pid, &status, 0);

                        //Check if the child process exited normally
                        if (WIFEXITED(status)) {
                            printf("Child %d exited with status %d\n", pid, WEXITSTATUS(status));
                        } else {
                            printf("Child %d did not terminate normally\n", pid);
                        }
                }
            }
        }
    }

    free(buffer);
    buffer = NULL;

    return 0;
}