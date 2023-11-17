/* Our own shell */

/*
WHAT EACH OF THE FOLLOWING DO:

fork(2) - creates a child process by duplicating the calling process

exec(3) - execute a file , return only if if an error has occurred , return value -1, 

wait(2) - wait for process to change state, and obtain info , see reuturn val 

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

*/

#include <unistd.h>
#include <sys/wait.h>
#include <string.h>

extern char **environ;

int main() {



}