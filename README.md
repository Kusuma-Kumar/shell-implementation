Authors: Kusuma Kumar
Known Bugs: execvp error for output. It empties file contents when execvp fails (>) or mispell cat command. This is what the real bash does however. 
Resources: https://www.geeksforgeeks.org/removing-trailing-newline-character-from-fgets-input/#, https://stackoverflow.com/questions/32547540/why-is-printf-before-exevp-not-running

Project description: 

This project involves the development of a basic shell named mysh that mimics the behavior of a typical command-line shell. The primary functionalities include handling user commands, supporting input/output redirection, and implementing pipelines.

Features
Prompt Display: The shell displays a simple prompt to indicate that it is ready to accept commands.

Command Execution: Upon entering a command and pressing Enter, the shell executes the command and waits for its completion before displaying a new prompt.

Input Redirection: Users can redirect input from a file using the < syntax, allowing the command to read from the specified file instead of standard input.

bash
Copy code
$ ./mysh < input-file
Output Redirection: Output from a command can be redirected to a file using > or >> syntax, allowing users to create or append to files.

bash
Copy code
$ ./mysh > output-file   # truncate and write
$ ./mysh >> output-file2 # append to file
Pipelines: The shell supports pipelines, enabling the output of one command to be directly fed as input to another using the | symbol.

bash
Copy code
$ ./program1 | ./program2
Arbitrary Number of Pipes: The shell handles an arbitrary number of pipes in a single command.

User Termination: The shell continues to read and execute commands until it encounters end-of-file (Ctrl-D) or the user enters the "exit" command.

Mechanics
The implementation leverages various system calls and functions, including fork, exec, wait, pipe, dup, fgets, and strtok.

Usage
Clone the repository:
Build the shell using the provided Makefile:

bash
Copy code
make
Run the shell:

bash
Copy code
./mysh
Enter commands at the prompt and observe the shell's behavior.

Files:
mysh.c: Implementation of the shell.
Makefile: Build script.
README.md: Documentation with authors, known bugs, and resources consulted.