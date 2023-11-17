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
create an array of file decriptors 0,1 for piping? pass output of one as input to the other
pipefd[0] refers to the read end of the pipe. pipefd[1] refers to the write end of the pipe.
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
        exit(EXIT_FAILURE);
    }

    token = strtok(command, " ");
    while (token != NULL && argCount < MAX_ARGS) {
        args[argCount++] = token;
        token = strtok(NULL, " ");
    }
    args[argCount] = NULL;

    // empty command
    if (argCount == 0) {
        return;
    }

    // Fork a child process
    if((cpid = fork()) == -1) {
        perror("fork");
        exit(EXIT_FAILURE);
    }

    // Child reads from pipe 
    if (cpid == 0) {
        // do child process stuff
        // Close write end of the pipe
        close(pipefd[1]);
    
    // Parent writes to pipe
    } else {
        // do parent stuff
        
        // Close read end of the pipe
        close(pipefd[0]);

        // Redirect standard output if there is a subsequent command in a pipeline
        if (argCount > 1) {
            dup2(pipefd[1], STDOUT_FILENO);
            close(pipefd[1]);
        }

        // Wait for the child to finish
        wait(NULL);
        exit(EXIT_SUCCESS);
    }
}