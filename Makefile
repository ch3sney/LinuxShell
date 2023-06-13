# Global variables.
CC=gcc
LD=ld
WARNS=-Wall -pedantic -Wextra
CFLAGS=-g3 -std=gnu11 ${WARNS}
LIBS=

all: tags headers mysh

# This builds visual symbol (.vs) files and the header files.
headers: *.c tags
	./headers.sh

tags: *.c
	ctags -R .
	#etags -R .
	
mysh: shell.o 
	${CC} -g -o $@ $^ ${LIBS}

shell.o: shell.c
	${CC} -g -c $<

%.o:%.c *.h
	${CC} ${CFLAGS} -c $< -o $@
