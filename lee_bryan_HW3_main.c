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

    //Instantiate a buffer of BUFFER_SIZE
    char* buffer = malloc(BUFFER_SIZE);

    //End the program if instantiation for buffer fails
    if (buffer == NULL) {
        printf("Failed to create buffer!\n");
        return 1;
    }

    int exitCondition = 0;

    //Repeatedly prompts the user until exit command is inputted.
    //During each iteration, input will be received, parsed, and executed.
    while (exitCondition == 0) {

        printf("%s", prompt);

        //Read the user's input and stores in buffer
        if (fgets(buffer, BUFFER_SIZE, stdin) == NULL) {
            //Check if end-of-file (EOF) was reached and exit the prompt
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

        //Remove line break character from the buffer
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

        //Iterates through the tokenized user input, splitting by pipes and spaces,
        //before storing the individual arguments.
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
            cmds[i][j] = NULL;
            i++;
            token = strtok_r(NULL, "|", &saveptr1);
        }
        cmdsCount = i;
    
        if (cmdsCount == 1) { //Handle commands without pipes
            //Create a child process and store its PID
            pid_t pid = fork();

            //Handle the values returned by forking a child process
            switch (pid) {
                case -1: //Fork failed
                    perror("fork failed!\n");
                    exit(EXIT_FAILURE);
                case 0: //Child process executes the command
                    execvp(cmds[0][0], cmds[0]);

                    perror("execvp failed!\n");
                    exit(EXIT_FAILURE);
                default: //Parent process waits for child process to terminate
                    int status;
                    waitpid(pid, &status, 0);

                    //Check if the child process exited normally
                    if (WIFEXITED(status)) {
                        printf("Child %d exited with status %d\n", pid, WEXITSTATUS(status));
                    } else {
                        printf("Child %d did not terminate normally\n", pid);
                    }
            }
        } else if (cmdsCount > 1) { //Handle commands with pipe(s)
            int pipefd[2];
            int oldFileDesc = READ_END; //Tracks the read end of a pipe

            //Iterate through the commands and execute them sequentially in a pipeline
            for (int i = 0; i < cmdsCount; i++) {
                //Instantiate a pipe with the file descriptor
                if (i < cmdsCount - 1) {
                    if (pipe(pipefd) < 0) {
                        perror("Failed to create a pipe!\n");
                        exit(EXIT_FAILURE);
                    }
                }

                //Create a child process and store its PID
                pid_t pid = fork();

                //Handle the values returned by forking a child process
                switch (pid) {
                    case -1: //Fork failed
                        perror("fork failed!\n");
                        exit(EXIT_FAILURE);
                    case 0: //Child process executes the command and redirects
                            //output for the next child process

                        //Copy the previous child's output to the current pipe's read end,
                        //which will be used for merging two commands dependent on each other.
                        if (i > 0) {
                            if (dup2(oldFileDesc, READ_END) == -1) {
                                perror("dup2 for read end failed!\n");
                                exit(EXIT_FAILURE);
                            }
                            close(oldFileDesc);
                        }

                        //Redirect the pipe's write end to the child process's output.
                        //Note: Ignores the operation for the last command
                        //since the write end of the pipe is not needed.
                        if (i < cmdsCount - 1) {
                            if (dup2(pipefd[WRITE_END], WRITE_END) == -1) {
                                perror("dup2 for write end failed!\n");
                                exit(EXIT_FAILURE);
                            }
                        }

                        //Close both ends of the pipe that is no longer needed
                        close(pipefd[READ_END]);
                        close(pipefd[WRITE_END]);

                        execvp(cmds[i][0], cmds[i]);

                        perror("execvp failed!\n");
                        exit(EXIT_FAILURE);
                    default: //In the parent process after child process exits
                        close(pipefd[WRITE_END]);

                        //Close the previous read end of the pipe that is no longer needed
                        if (oldFileDesc != READ_END) {
                            close(oldFileDesc);
                        }
                        
                        //Save the read end of the current pipe for the following child process
                        oldFileDesc = pipefd[READ_END];

                        //Parent process waits for child process to terminate
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