all: sbuffer.c sbuffer.h config.h main.c
	gcc -g -Wall main.c sbuffer.c sbuffer.h -o main.out

clean:
	rm -rf *.o

zip:
	zip milestone3.zip *.c *.h Makefile


gen: config.h file_creator.c
	gcc -g file_creator.c config.h -o gen.out

gendebug: config.h file_creator.c
	gcc -g file_creator.c config.h -o gendebug.out -DDEBUG