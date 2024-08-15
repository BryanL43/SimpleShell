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
#include <errno.h>

#define BUFFER_SIZE 163
#define READ_END 0
#define WRITE_END 1

int cmdCount; //Tracks the total commands i.e. cat Makefile | grep bryan | wc -l -w is **3**
int cmdArgc; //Index to track the current command's argument
char buffer[BUFFER_SIZE]; //Input buffer
char* commands[(BUFFER_SIZE / 2) + 1]; //Command array
char* cmdArgv[(BUFFER_SIZE / 2) + 1]; //Argument array

int main(int argc, char* argv[]) {
    char* buf;
    int waitstatus;

    //Set the user's prompt
    char* prompt = ">";
    if (argc > 1) {
        prompt = argv[1];
    }

    int exitCondition = 0;

    //Repeatedly prompts the user until exit command is inputted.
    //During each iteration, input will be received, parsed, and executed.
    while (exitCondition == 0) {
        cmdCount = 0;

        //Read the user's input and stores in buffer
        printf("%s", prompt);
        buf = fgets(buffer, BUFFER_SIZE, stdin);
        if (buf == NULL) {
            printf("\n"); //EOF
            exitCondition = 1;
            continue;
        }
        fflush(stdin); //flush extra input

        //Repeatedly acquire only the next character (ignore whitespace)
        if (buf[strlen(buf) - 1] != '\n') {
            while ((fgetc(stdin) != '\n') | feof(stdin));
        }

        //Fixes the messy output intruding on prompt
        if (isatty(0) == 0) {
            printf("\n");
        }

        //Seperate commands based on the pipe character
        char* token = strtok(buffer, "|\n");
        while (token != NULL) {
            commands[cmdCount] = token;
            cmdCount++;
            token = strtok(NULL, "|\n");
        }

        //Check for empty input
        if (cmdCount == 0) {
            printf("Empty string entered!\n");
            continue;
        }

        int pipefd[cmdCount - 1][2]; //declare the number of pipes

        //Create pipes for each commands
        if (cmdCount > 1) {
            for (int i = 0; i < cmdCount - 1; i++) {
                if (pipe(pipefd[i]) == -1) {
                    perror("Failed to create a pipe!\n");
                    exit(EXIT_FAILURE);
                }
            }
        }

        //Execute each command
        for (int i = 0; i < cmdCount; i++) {
            cmdArgc = 0;

            //Parse the commands for their arguments
            token = strtok(commands[i], " \t\n"); //seperate by whitespace delimiter
            while (token != NULL) {
                cmdArgv[cmdArgc] = token;
                cmdArgc++;
                token = strtok(NULL, " \t\n");
            }
            cmdArgv[cmdArgc] = NULL; //Null-terminator

            //Check for invalid inputs
            if (cmdArgc == 0) {
                break;
            }

            if (strcmp(cmdArgv[0], "exit") == 0) { //Exit command entered
                exitCondition = 1;
                break;
            } else { //Execute the command(s)

                //Create a child process to use exec
                pid_t child = fork();

                //Handle the values returned from forking a child process
                switch (child) {
                    case -1: //Fork failed
                        exit(EXIT_FAILURE);
                    case 0: //In the child process

                        //If not last command, copy the previous child's output
                        //to the current pipe's read end
                        if (i < cmdCount - 1) {
                            close(pipefd[i][READ_END]); //Close read end for this child
                            if (dup2(pipefd[i][WRITE_END], STDOUT_FILENO) == -1) {
                                perror("Error: not last command. Dup failed.");
                            }
                            close(pipefd[i][WRITE_END]);
                        }

                        //If not the first command, set the std input to prior command output
                        if (i > 0) {
                            if (dup2(pipefd[i - 1][READ_END], STDIN_FILENO) == -1) {
                                perror("Error: not first command. Dup failed");
                            }
                            close(pipefd[i - 1][READ_END]);
                        }

                        if (execvp(cmdArgv[0], cmdArgv) < 0) {
                            perror("Exec failed!\n");
                            return errno; //Exit child if exec failed
                        }
                    default: //Parent process waits for child process to terminate

                        pid_t childWait = wait(&waitstatus);
                        printf("Child %d exited with status %d\n", child, WEXITSTATUS(waitstatus));

                        //Close the read end of previous pipe & write end of current pipe
                        if (i > 0) {
                            close(pipefd[i - 1][READ_END]);
                        }
                        if (i < cmdCount - 1) {
                            close(pipefd[i][WRITE_END]);
                        }
                }
            }
        }
    }
    return 0;
}