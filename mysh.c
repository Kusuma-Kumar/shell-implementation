#include <unistd.h>
#include <sys/wait.h>
#include <string.h>
#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>

#define MAX_INPUT_SIZE 4096
#define MAX_ARGS 10
// to flush and return to terminal when execvp fails
#define EXECVP_FFLUSH 3 

void executeCommands(char *command);
void forkAndExec(char *args[]);
int handleIORedirection(char *fileName, int flags);
int handlePiping(char *args[]);
char *tokenize(char *input);
void printArguments(char *args[], int argCount);

// starts the shell by printing prompt. Checks for EOF (Ctrl-D), exit command. 
int main() {
    char input[MAX_INPUT_SIZE];
    while (1) {
        printf("my-shell$ ");
        fflush(stdout);

        // Read input
        if(fgets(input, sizeof(input), stdin) == NULL || strcmp(input, "exit\n") == 0) {
            // input indicates EOF (Ctrl-D)
            break; 
        }

        // user entered exit
        if(strcmp(input, "exit\n") == 0) {
            break; 
        }

        executeCommands(input);
    }
    printf("\n");
    return 0;
}

// receives input given by the user, formats it by calling tokenize.
// duplicate file descriptors STDIN_FILENO, STDOUT_FILENO
// Loop through tokenized input and call functions based on special characters provided
// reset file descriptors back to original (in terminal)
void executeCommands(char *command) {
    char *args[MAX_ARGS + 1];
    char *tokens;
    // Save the current standard output file descriptor
    int originalStdout; 
    // Save the current standard input file descriptor
    int originalStdin; 
    // Keep track of child process PIDs
    pid_t childPIDs[MAX_ARGS];
    int childCount = 0;
    int failFlag = 0;

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
    
    // while loop fill args with all of the argument right upto a special character is found
    // after executing special charachter command args resets and continues collectiong all tokens until next special character is found
    // args always contain only the program to be executed and its respective arguments
    int i = 0;
    while (arg) {
        if(strcmp(arg, "<") == 0) {
            if(handleIORedirection(strtok(NULL, " "), O_RDONLY) == -1) {
                failFlag = 1;
                break;
            }
        
        } else if(strcmp(arg, ">>") == 0) {
            if(handleIORedirection(strtok(NULL, " "), O_WRONLY | O_CREAT | O_APPEND) == -1) {
                failFlag = 1;
                break;
            }
        
        } else if(strcmp(arg, ">") == 0) {
            if(handleIORedirection(strtok(NULL, " "), O_WRONLY | O_CREAT | O_TRUNC) == -1) {
                failFlag = 1;
                break;
            }
        
        } else if(strcmp(arg, "|") == 0) {
            args[i] = NULL;
            // After executing commands, collect child PIDs
            int childPID = handlePiping(args);
            if (childPID != -1) {
                childPIDs[childCount++] = childPID;
            } else {
                failFlag = 1;
                break;
            }
            i = 0;
        
        } else {
            args[i] = arg;
            i++;
        }
        arg = strtok(NULL, " ");
    }
    args[i] = NULL;

    if(!failFlag) {
        forkAndExec(args);
    }
    
    // Wait for all child processes to finish before exiting
    for (int i = 0; i < childCount; i++) {
        waitpid(childPIDs[i], NULL, 0);
    }

    // Restore the original standard output file descriptor so it can go back to receiving input and printing output in the terminal
    if(dup2(originalStdout, STDOUT_FILENO) == -1) {
        perror("dup2");
        return;
    }
    if(dup2(originalStdin, STDIN_FILENO) == -1) {
        perror("dup2");
        return;
    }

    free(tokens);  // Free the memory allocated for tokens
}

// perform commands that are not explictly included in piping like ls and cat
void forkAndExec(char *args[]) {
    pid_t cpid;
    FILE *execFailure = (FILE *)EXECVP_FFLUSH;

    // Fork a child process
    if((cpid = fork()) == -1) {
        perror("fork");
        return;
    }

    if(cpid == 0) { // do child stuff - reads from pipe
        // if (dup2(STDOUT_FILENO, STDERR_FILENO) == -1) {
        //     perror("dup2");
        //     return;
        // }
        
        execvp(args[0], args);
        perror("execvp");
        fflush(execFailure);
        return;

    } else { 
        // do parent stuff - write to pipe
        // Wait for the specific child process to finish so 
        // when child is done exectuting, print prompt again and wait for user input
        waitpid(cpid, NULL, 0);
    }
}

// opens files based on flags provide and directs it to STDIN_FILENO or STDOUT_FILENO respectively
int handleIORedirection(char *fileName, int flags) {
    int fd;
    if((fd = open(fileName, flags)) == -1) {
        perror("open");
        return -1;
    }
    
    if (flags == O_RDONLY) {
        if (dup2(fd, STDIN_FILENO) == -1) {
            close(fd);  // Close the file descriptor on failure
            perror("dup2");
            return -1;
        }
    } else {
        if (dup2(fd, STDOUT_FILENO) == -1) {
            close(fd);  // Close the file descriptor on failure
            perror("dup2");
            return -1;
        }
    }
    
    if(close(fd) == -1) {
        perror("close");
        return -1;
    }
    return 0;
}

int handlePiping(char *args[]) {
    // initialize default file descriptors
    int pipefd[2]; 
    pid_t cpid;
    FILE *execFailure = (FILE *)EXECVP_FFLUSH;

    if(pipe(pipefd) == -1) {
        perror("pipe");
        return -1; 
    }

    if((cpid = fork()) == -1) {
        perror("fork");
        return -1; 
    }

    if(cpid == 0) { 
        // Close read end of the pipe in the child process
        if(close(pipefd[0]) == -1) { 
            perror("close");
            return -1;
        }
        if(dup2(pipefd[1], STDOUT_FILENO) == -1) {
            perror("dup2");
            return -1;
        }
        if(close(pipefd[1]) == -1) { // Close write end of the pipe
            perror("close");
            return -1;
        }

        // Execute the command in the child process
        execvp(args[0], args);
        perror("execvp");
        fflush(execFailure);
        return -1;

    } else { 
        // Close write end of the pipe in the parent process
        if(close(pipefd[1]) == -1) { 
            perror("close");
            return -1;
        } 
        if(dup2(pipefd[0], STDIN_FILENO) == -1) {
            perror("dup2");
            return -1;
        }
         // Close read end of the pipe iin the parent process
        if(close(pipefd[0]) == -1) {
            perror("close");
            return -1;
        }

        // Return the PID of the child process
        return cpid; 
    }
    return 0;
}

// add spaces around special characters(|,<,>,>>) in input for fomatting
char *tokenize(char *input) {
    int i;
    int j = 0;
    char *tokenized = (char *)malloc((MAX_INPUT_SIZE * 2) * sizeof(char));

    // add spaces around special characters
    for (i = 0; i < strlen(input); i++) {
        if(input[i] != '>' && input[i] != '<' && input[i] != '|') {
            tokenized[j++] = input[i];
        } else {
            tokenized[j++] = ' ';
            if((input[i] == '>') && (input[i+1] == '>')) {
                tokenized[j++] = input[i];
                tokenized[j++] = input[i+1];
                // skip the second >
                i++; 
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