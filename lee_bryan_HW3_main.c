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
    size_t bufferSize = 163;
    char* buffer = malloc(bufferSize);

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
        if (fgets(buffer, bufferSize, stdin) == NULL) {
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

        char* cmds[(BUFFER_SIZE / 2) + 1];
        int cmdsCount = 0;
        
        //Split and tokenize the pipeline's commands
        char* token = strtok(buffer, "|");

        //Iterate through the token and store the seperated commands in a vector
        while (token != NULL) {
            cmds[cmdsCount++] = token;
            token = strtok(NULL, "|");
        }
    
        //Exit the program if the command input is "exit"
        if (strcmp(cmds[0], "exit") == 0) {
            exitCondition = 1;
            break;
        }

        int pipefd[2]; //Pipe file descriptor
        int fd_in = 0; //Tracks the read end of a pipe
        
        //Iterate through each command to parse it into individual commands and arguments.
        //After parsing, fork a child process for each command and execute them.
        for (int i = 0; i < cmdsCount; i++) {
            
            //Creates a pipe with pipe file descriptor and checks for errors
            if (pipe(pipefd) < 0) {
                perror("Failed to create a pipe!\n");
                exit(EXIT_FAILURE);
            }

            //Note: The first index of the vector will be the command,
            //the rest will be arguments.
            char* argArray[(BUFFER_SIZE / 2) + 1];
            int argArrayIndex = 0;

            //Split and tokenize the individual parts of the commands
            char* token2 = strtok(cmds[i], " ");

            //Iterate through the token and store the seperated components in a vector
            while (token2 != NULL) {
                argArray[argArrayIndex++] = token2;
                token2 = strtok(NULL, " ");
            }

            //Include a NULL pointer in the array after the last substring
            argArray[argArrayIndex++] = NULL;

            //Fork a child process and save its PID
            pid_t pid = fork();

            //Handle different outcomes of the child process creation operation
            switch (pid) {
                case -1: //Fork failed
                    perror("fork failed!\n");
                    exit(EXIT_FAILURE);
                case 0: //In the child process that was successfully created
                    //Close the unused read end of the pipe
                    close(pipefd[0]);

                    //Redirect the current child process to read from the previous command's output
                    if (dup2(fd_in, 0) == -1) {
                        perror("dup2 for stdin failed!\n");
                        exit(EXIT_FAILURE);
                    }
                    
                    //Redirect the last command output back to the write end of the pipe.
                    //This output will be displayed on the shell.
                    if (i < cmdsCount - 1) {
                        if (dup2(pipefd[1], 1) == -1) {
                            perror("dup2 for stdout failed!\n");
                            exit(EXIT_FAILURE);
                        }
                    }

                    execvp(argArray[0], argArray);

                    printf("execvp failed!\n");
                    exit(EXIT_FAILURE);
                default: //In the parent process after child process exits
                    //Parent needs to wait for the termination of the child process
                    int status;
                    waitpid(pid, &status, 0);

                    //Check if the child process exited normally
                    if (WIFEXITED(status)) {
                        printf("Child %d exited with status %d\n", pid, WEXITSTATUS(status));

                        //Close the write end of the pipe
                        close(pipefd[1]);

                        //Set the read end of the pipe to receive the previous command's output
                        //for the next command in the pipeline
                        fd_in = pipefd[0];
                    } else {
                        printf("Child %d did not terminate normally\n", pid);
                    }
            }
        }
        close(fd_in);
    }

    free(buffer);
    buffer = NULL;

    return 0;
}