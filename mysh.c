/*
 work on when passing anything in double quotes to grep
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
int handleInputRedirection(char *fileName, char *mode);
int handleOutputRedirection(char *fileName, char *mode);
int handlePiping(char *args[]);
char * tokenize(char *input);
void printArguments(char *args[], int argCount);

// starts the shell by printing prompt. Checks for EOF (Ctrl-D), exit command. 
int main() {
    char input[MAX_INPUT_SIZE];
    while (1) {
        printf("my-shell$ ");
        fflush(stdout);

        // Read input
        // mocking the bash terminal when you typed exit or click EOF, it automatically clear terminal
        if (fgets(input, sizeof(input), stdin) == NULL || strcmp(input, "exit\n") == 0) {
            printf("exit\n");
            executeCommands("clear\n");
            break; // if it is EOF (Ctrl-D) or exit terminal
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
    int originalStdout; // Save the current standard output file descriptor
    int originalStdin; // Save the current standard input file descriptor
    
    if((originalStdout = dup(STDOUT_FILENO)) == -1) {
        perror("dup");
        return;
    }
    if((originalStdin = dup(STDIN_FILENO)) == -1) {
        perror("dup");
        return;
    }
    
    tokens = tokenize(command);
    char *arg = strtok(tokens, " ");
    
    // while loop fill args with all of the argument right upto a speacial character is found
    // after executing special charachter command args resets and continues collectiong all tokens until next special character is found
    // args always contain only the program to be executed and its respective arguments
    int i = 0;
    while (arg) {
        if (strcmp(arg, "<") == 0) {
            if (handleInputRedirection(strtok(NULL, " "), "r") == -1) {
                return;
            }
        
        } else if (strcmp(arg, ">>") == 0) {
            if (handleOutputRedirection(strtok(NULL, " "), "a") == -1) {
                return;
            }
        
        } else if (strcmp(arg, ">") == 0) {
            if (handleOutputRedirection(strtok(NULL, " "), "w") == -1) {
                return;
            }
        
        } else if (strcmp(arg, "|") == 0) {
            args[i] = NULL;
            if (handlePiping(args) == -1) {
                return;
            }
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
    if(dup2(originalStdout, STDOUT_FILENO) == -1) {
        perror("dup2");
        return;
    }
    if(dup2(originalStdin, STDIN_FILENO) == -1) {
        perror("dup2");
        return;
    }
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
int handleInputRedirection(char *fileName, char *mode) {
    FILE *inputFile;
    if ((inputFile = fopen(fileName, mode)) == NULL) {
        perror("fopen");
        return -1;
    }

    if(dup2(fileno(inputFile), STDIN_FILENO) == -1) {
        perror("dup2");
        return -1;
    }
    if(fclose(inputFile) == -1) {
        perror("fclose");
        return -1;
    }
    return 0;
}

// mode a - append(>>), mode w - truncate/create and write(>)
int handleOutputRedirection(char *fileName, char *mode) {
    FILE *outputFile;
    if ((outputFile = fopen(fileName, mode)) == NULL) {
        perror("fopen");
        return -1;
    }
    
    if(dup2(fileno(outputFile), STDOUT_FILENO) == -1) {
        perror("dup2");
        return -1;
    }
    if(fclose(outputFile) == -1) {
        perror("fclose");
        return -1;
    }
    return 0;
}

int handlePiping(char *args[]) {
    int pipefd[2]; // initialize default file descriptors
    pid_t cpid;

    if (pipe(pipefd) == -1) {
        perror("pipe");
        return -1; 
    }

    if ((cpid = fork()) == -1) {
        perror("fork");
        return -1; 
    }

    if (cpid == 0) { 
        close(pipefd[0]); // Close read end of the pipe in the child process
        if(dup2(pipefd[1], STDOUT_FILENO) == -1) {
            perror("dup2");
            return -1;
        }
        close(pipefd[1]); // Close write end of the pipe

        // Execute the command in the child process
        execvp(args[0], args);
        perror("execvp");
        // return;

    } else { 
        close(pipefd[1]); // Close write end of the pipe in the parent process
        if(dup2(pipefd[0], STDIN_FILENO) == -1) {
            perror("dup2");
            return -1;
        }
        close(pipefd[0]); // Close read end of the pipe

        // Wait for the child process to finish
        waitpid(cpid, NULL, 0);
    }
    return 0;
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