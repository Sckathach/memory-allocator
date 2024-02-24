build:
	gcc memory_alloc.c -o memory_alloc -g -O0 -Wall -L. -lm -lcmocka

test: build
	./memory_alloc
