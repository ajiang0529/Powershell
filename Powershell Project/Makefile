CFLAGS = -ansi -Wall -g -O0 -Werror -Wshadow -Wwrite-strings -pedantic-errors -fstack-protector-all
all: d8sh
d8sh: d8sh.o parser.tab.o lexer.o executor.o
	gcc -lreadline d8sh.o parser.tab.o lexer.o executor.o -o d8sh

d8sh.o: d8sh.c lexer.h executor.h
	gcc $(CFLAGS) -c d8sh.c

executor.o: executor.c executor.h command.h
	gcc $(CFLAGS) -c executor.c

parser.tab.o: parser.tab.c command.h
	gcc $(CFLAGS) -c parser.tab.c

lexer.o: lexer.c
	gcc $(CFLAGS) -c lexer.c

clean:
	rm d8sh.o
	rm d8sh
