/* Our own shell */

/*
WHAT EACH OF THE FOLLOWING DO:

fork(2) - creates a child process by duplicating the calling process

exec(3) - execute a file , return only if if an error has occurred , return value -1, 

wait(2) - wait for process to change state, and obtain info , see return val 

pipe(2)- creates pipe, undirectional data channel--> used for interprocess communication, return val 0 if error -1 and pipefd 
         should be left unchanged 

         pipefd is used to return 2 fd (referencing to the end of pipes)
         pipefd[0] --> read end of the pipe
         pipefd[1] --> write end of the pipe 

dup(2)- duplicated a fd , meh i think this is one of the ones we might not have to use but look at later in case

fgets(3)- reads characters from stream and stores into buffer pointed to by s 
          return the character read as an unsigned char cast to an int or blah 

strtok(3)- breaks a string into a sequence of zero or more nonempty tokens. First call, the string to be parsed should be specified in str.
           each subsequent call that should parse the  same  string,  str must be NULL.
           

STUFF I THINK IS IMPORTANT/STUFF TO LOOK FURTHER INTO:
Pipe should prob be called before fork ?

stdin (fd 0) is where a program receives its input from (eg, typing at the keyboard)
stdout (fd 1) is where a program’s output goes by default (eg, printf)
stderr (fd 2) is where errors printed by a program go by default (eg, perror)
*/

#include <unistd.h>
#include <sys/wait.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

extern char **environ;

#define MAX_INPUT_SIZE 4096
#define MAX_ARGS 10

void executeCommands(char *command);
void handleInputRedirection(char *args[], int *argCount);
void handleOutputRedirection(char *args[], int *argCount);
void printArguments(char *args[], int argCount);

int main() {
    char input[MAX_INPUT_SIZE];
    
    while (1) {
        // Print prompt
        printf("my-shell$ ");
        fflush(stdout);

        // Read input
        if (fgets(input, sizeof(input), stdin) == NULL) {
            // if it is EOF (Ctrl-D)
            printf("\n");
            break;
        }

        // Check for "exit" command
        if (strcmp(input, "exit\n") == 0) {
            break;
        }

        // Remove newline character
        input[strcspn(input, "\n")] = '\0';

        executeCommands(input);
    }
    
    return 0;
}

void executeCommands(char *command){
    // create tokens from given command as arguments will be separated by a single space
    char *args[MAX_ARGS + 1];
    char *token;
    int argCount = 0;
    pid_t cpid;
    int pipefd[2]; // initialize default file descriptors
    
    if (pipe(pipefd) == -1) {
        perror("pipe");
        return; // print error message and prompt user for next command
    }

    token = strtok(command, " ");
    while (token != NULL && argCount < MAX_ARGS) {
        args[argCount++] = token;
        token = strtok(NULL, " ");
    }
    args[argCount] = NULL;

    printArguments(args, argCount);

    // empty command
    if (argCount == 0) {
        return;
    }

    // Fork a child process
    if((cpid = fork()) == -1) {
        perror("fork");
        return;
    }

    // Child reads from pipe 
    if (cpid == 0) {
        // do child process stuff

        handleInputRedirection(args, &argCount);
        handleOutputRedirection(args, &argCount);

        // Close write end of the pipe
        close(pipefd[1]);

        // Redirect standard input if there is a previous command in a pipeline
        if (argCount > 1) {
            dup2(pipefd[0], STDIN_FILENO);
            close(pipefd[0]);
        }

        // Execute the command
        execvp(args[0], args);
        perror("execvp");
        return;
    

    } else {   // do parent stuff - write to pipe
        
        // Close read end of the pipe
        close(pipefd[0]);

        // Redirect standard output if there is a subsequent command in a pipeline
        if (argCount > 1) {
            dup2(pipefd[1], STDOUT_FILENO);
            close(pipefd[1]);
        }

        // Wait for the specific child process to finish so 
        // when child is done exectuting, primt prompt again and wait for user input
        waitpid(cpid, NULL, 0);
    }
}

void handleInputRedirection(char *args[], int *argCount) {
    // input redirection (<)
    if (*argCount > 2 && strcmp(args[*argCount - 2], "<") == 0) {
        printf("%s\n", args[*argCount - 1]);
        FILE *inputFile = fopen(args[*argCount - 1], "r");
        if (inputFile == NULL) {
            perror("fopen");
            return;
        }

        dup2(fileno(inputFile), STDIN_FILENO);
        fclose(inputFile);

        // Remove the input redirection tokens
        args[*argCount - 2] = NULL;
        args[*argCount - 1] = NULL;
    }
}

void handleOutputRedirection(char *args[], int *argCount) {
    // output redirection (> and >>)
    if (*argCount > 2 && (strcmp(args[*argCount - 2], ">") == 0 || strcmp(args[*argCount - 2], ">>") == 0)) {
        FILE *outputFile;

        if (strcmp(args[*argCount - 2], ">") == 0) {
            // Truncate file to zero length
            outputFile = fopen(args[*argCount - 1], "w");
        } else {
            // Open for appending (writing at the end of the file).
            outputFile = fopen(args[*argCount - 1], "a");
        }

        if (outputFile == NULL) {
            perror("fopen");
            return;
        }
        dup2(fileno(outputFile), STDOUT_FILENO);
        fclose(outputFile);

        // Remove the output redirection tokens
        args[*argCount - 2] = NULL;
        args[*argCount - 1] = NULL;
        printArguments(args, *argCount);
    } 
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