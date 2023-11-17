CFLAGS=-Wall -pedantic

mysh: mysh.c
	gcc $(CFLAGS) -o mysh mysh.c

clean:
	rm -f mysh
