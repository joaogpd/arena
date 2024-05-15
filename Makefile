arena: arena.c
	gcc -Wall -g -c arena.o arena.c

all: arena.o main.c
	gcc -Wall -g -o main.out arena.c main.c