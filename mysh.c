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
void forkAndExec(char *args[]);
void handleInputRedirection(char *fileName);
void handleOutputRedirection(char *fileName);
void handlePiping(char *args[]);
char * tokenize(char *input);
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

void executeCommands(char *command) {
    // create tokens from given command as arguments will be separated by a single space
    char *args[MAX_ARGS + 1];
    char *token;
    int argCount = 0;
    
    token = tokenize(command);
    
    char *arg = strtok(token, " ");
    int i = 0;
    while (arg) {
        printf("Arg is%s\n", arg);
        if (*arg == '<') {
            handleInputRedirection(strtok(NULL, " "));
        } else if (*arg == '>') {
            handleOutputRedirection(strtok(NULL, " "));
        } else if (*arg == '|') {
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
    
    return;
}

void forkAndExec(char *args[]) {
    pid_t cpid;

    // Fork a child process
    if((cpid = fork()) == -1) {
        perror("fork");
        return;
    }

    // Child reads from pipe 
    if (cpid == 0) {
        
        // do child process stuff
        // Print the command being executed
        printf("Command: ");
        for (int i = 0; args[i] != NULL; ++i) {
            printf("%s ", args[i]);
        }
        printf("\n");

        execvp(args[0], args);
        perror("execvp");

    } else {   // do parent stuff - write to pipe
        printf("Back in parent\n");

        // Wait for the specific child process to finish so 
        // when child is done exectuting, primt prompt again and wait for user input
        waitpid(cpid, NULL, 0);
    }
    // i want to redirect my inputs and output to the terminal 
    // instead of in the files i was currently receiving or writing to
    handleInputRedirection("/dev/tty");
    handleOutputRedirection("/dev/tty");
}

void handleInputRedirection(char *fileName) {
    // input redirection (<)
    FILE *inputFile = fopen(fileName, "r");
    if (inputFile == NULL) {
        perror("fopen");
        return;
    }

    dup2(fileno(inputFile), STDIN_FILENO);
    fclose(inputFile);
}

void handleOutputRedirection(char *fileName) {
    // output redirection (> and >>)
    printf("handling output\n");
    FILE *outputFile;

    // if (strcmp(args[*argCount - 2], ">") == 0) {
    //     // Truncate file to zero length
    //     outputFile = fopen(fileName, "w");
    // } else {
    //     // Open for appending (writing at the end of the file).
    //     outputFile = fopen(fileName, "a");
    // }
    outputFile = fopen(fileName, "w");
    
    if (outputFile == NULL) {
        perror("fopen");
        return;
    }
    dup2(fileno(outputFile), STDOUT_FILENO);
    fclose(outputFile);
}

void handlePiping(char *args[]) {
    int pipefd[2]; // initialize default file descriptors
    if (pipe(pipefd) == -1) {
        perror("pipe");
        return; // print error message and prompt user for next command
    }

    dup2(pipefd[1], STDOUT_FILENO);
    close(pipefd[1]); // Close write end of the pipe

    printf("args = %s\n", *args);

    forkAndExec(args);

    dup2(pipefd[0], STDIN_FILENO);
    close(pipefd[0]); // Close read end of the pipe
}

char * tokenize(char *input) {
    printf("in tokenize\n");
    int i;
    int j = 0;
    char *tokenized = (char *)malloc((MAX_INPUT_SIZE * 2) * sizeof(char));

    // add spaces around special characters
    for (i = 0; i < strlen(input); i++) {
        if (input[i] != '>' && input[i] != '<' && input[i] != '|') {
            tokenized[j++] = input[i];
        } else {
            tokenized[j++] = ' ';
            tokenized[j++] = input[i];
            tokenized[j++] = ' ';
        }
    }
    tokenized[j++] = '\0';

    // add null to the end
    char *end;
    end = tokenized + strlen(tokenized) - 1;
    end--;
    *(end + 1) = '\0';
    printf("%s\n", tokenized);

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