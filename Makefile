arena: arena.c
	gcc -Wall -g -c arena.o arena.c

test: arena.o test/test.c
	gcc -Wall -g -o main.out arena.c main.c
	./main.out
	rm arena.o
	rm main.out