arena: arena.c
	gcc -Wall -g -c arena.o arena.c

test1: test/test.c
	gcc -Wall -g -o test1.out arena.c test/test.c
	valgrind ./test1.out &> test1.log
	rm test1.out

test2: test/test2.c
	gcc -Wall -g -o test2.out arena.c test/test2.c
	valgrind ./test2.out &> test2.log
	rm test2.out

tests: arena.o 
	make test1
	make test2
	rm arena.o