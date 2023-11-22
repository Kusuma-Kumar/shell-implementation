/*
 work on when passing anything in double quotes to grep
 work on when receiving a fopen error, it return but not back to main function as it should
*/

#include <unistd.h>
#include <sys/wait.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#define MAX_INPUT_SIZE 4096
#define MAX_ARGS 10

void executeCommands(char *command);
void forkAndExec(char *args[]);
void handleInputRedirection(char *fileName, char *mode);
void handleOutputRedirection(char *fileName, char *mode);
void handlePiping(char *args[]);
char * tokenize(char *input);
void printArguments(char *args[], int argCount);

// starts the shell by printing prompt. Checks for EOF (Ctrl-D), exit command. 
int main() {
    char input[MAX_INPUT_SIZE];
    
    while (1) {
        printf("my-shell$ ");
        fflush(stdout);

        // Read input
        if (fgets(input, sizeof(input), stdin) == NULL) {
            printf("\n");
            break; // if it is EOF (Ctrl-D)
        }

        if (strcmp(input, "exit\n") == 0) {
            break; // "exit" command
        }

        executeCommands(input);
    }
    
    return 0;
}

// receives input given by the user, formats it by calling tokenize.
// duplicate file descriptors STDIN_FILENO, STDOUT_FILENO
// Loop through tokenized input and call functions based on special characters provided
// reset file descriptors back to original (in terminal)
void executeCommands(char *command) {
    char *args[MAX_ARGS + 1];
    char *tokens;
    int originalStdout = dup(STDOUT_FILENO); // Save the current standard output file descriptor
    int originalStdin = dup(STDIN_FILENO); // Save the current standard input file descriptor
    
    tokens = tokenize(command);
    char *arg = strtok(tokens, " ");
    
    // while loop fill args with all of the argument right upto a speacial character is found
    // after executing special charachter command args resets and continues collectiong all tokens until next special character is found
    // args always contain only the program to be executed and its respective arguments
    int i = 0;
    while (arg) {
        if (strcmp(arg, "<") == 0) {
            handleInputRedirection(strtok(NULL, " "), "r");
        
        } else if (strcmp(arg, ">>") == 0) {
            handleOutputRedirection(strtok(NULL, " "), "a");
        
        } else if (strcmp(arg, ">") == 0) {
            handleOutputRedirection(strtok(NULL, " "), "w");
        
        } else if (strcmp(arg, "|") == 0) {
            args[i] = NULL;
            handlePiping(args);
            i = 0;
        
        } else {
            args[i] = arg;
            i++;
        }
        arg = strtok(NULL, " ");
    }
    args[i] = NULL;

    forkAndExec(args);
    // Restore the original standard output file descriptor so it can go back to receiving input and printing output in the terminal
    dup2(originalStdout, STDOUT_FILENO);
    dup2(originalStdin, STDIN_FILENO);
}

// more for individual commands that do not need piping like ls
void forkAndExec(char *args[]) {
    pid_t cpid;

    // Fork a child process
    if((cpid = fork()) == -1) {
        perror("fork");
        return;
    }

    // do child stuff - reads from pipe
    if (cpid == 0) {
        execvp(args[0], args);
        perror("execvp");
        return;

    // do parent stuff - write to pipe
    } else {
        // Wait for the specific child process to finish so 
        // when child is done exectuting, print prompt again and wait for user input
        waitpid(cpid, NULL, 0);
    }
}

// mode r - open for reading(<)
void handleInputRedirection(char *fileName, char *mode) {
    FILE *inputFile;
    if ((inputFile = fopen(fileName, mode)) == NULL) {
        perror("fopen");
        return;
    }

    dup2(fileno(inputFile), STDIN_FILENO);
    fclose(inputFile);
}

// mode a - append(>>), mode w - truncate/create and write(>)
void handleOutputRedirection(char *fileName, char *mode) {
    FILE *outputFile;
    if ((outputFile = fopen(fileName, mode)) == NULL) {
        perror("fopen");
        return;
    }
    
    dup2(fileno(outputFile), STDOUT_FILENO);
    fclose(outputFile);
}

void handlePiping(char *args[]) {
    int pipefd[2]; // initialize default file descriptors
    pid_t cpid;

    if (pipe(pipefd) == -1) {
        perror("pipe");
        return; // print error message and prompt user for the next command
    }

    if ((cpid = fork()) == -1) {
        perror("fork");
        return; // print error message and prompt user for the next command
    }

    if (cpid == 0) { 
        close(pipefd[0]); // Close read end of the pipe in the child process
        dup2(pipefd[1], STDOUT_FILENO);
        close(pipefd[1]); // Close write end of the pipe

        // Execute the command in the child process
        execvp(args[0], args);
        perror("execvp");
        // return;

    } else { 
        close(pipefd[1]); // Close write end of the pipe in the parent process
        dup2(pipefd[0], STDIN_FILENO);
        close(pipefd[0]); // Close read end of the pipe

        // Wait for the child process to finish
        waitpid(cpid, NULL, 0);
    }
}


// add spaces around special characters(|,<,>,>>) in input for fomatting
char * tokenize(char *input) {
    int i;
    int j = 0;
    char *tokenized = (char *)malloc((MAX_INPUT_SIZE * 2) * sizeof(char));

    // add spaces around special characters
    for (i = 0; i < strlen(input); i++) {
        if (input[i] != '>' && input[i] != '<' && input[i] != '|') {
            tokenized[j++] = input[i];
        } else {
            tokenized[j++] = ' ';
            if ((input[i] == '>') && (input[i+1] == '>')) {
                tokenized[j++] = input[i];
                tokenized[j++] = input[i+1];
                i++; // skip the second >
            } else {
                tokenized[j++] = input[i];
            }
            tokenized[j++] = ' ';
        }
    }
    tokenized[j++] = '\0';

    // add null to the end, for commands that do not have any special characters
    char *end;
    end = tokenized + strlen(tokenized) - 1;
    end--;
    *(end + 1) = '\0';

    return tokenized;
}

void printArguments(char *args[], int argCount) {
    // Print the arguments
    printf("Arguments: ");
    for (int i = 0; i < argCount; i++) {
        printf("%s ", args[i]);
    }
    printf("\n");

    // Print the argument count
    printf("Argument Count: %d\n", argCount);
}