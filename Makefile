######################################################################
# "Hello World" Makefile
######################################################################
# (C) 2022 Dr. Adam C. Champion
######################################################################
# Based on Prof. Neil Kirby's Systems I Makefile plus shell and awk
# scripts. Also, I used:
#
# https://stackoverflow.com/questions/54082715/how-to-add-lm-ldflags-
#   correctly-to-this-makefile ,
#
# which I retrieved 19 March 2022.
######################################################################
# FYI: I have a Emacs command-line config file (.emacs_term),
# which you can clone (using git) from:
#
# https://github.com/acchampion/dotfiles
#
# My .vimrc should also help you.
######################################################################


######################################################################
# Global variables.
######################################################################
# Lab 0 is NOT using any extra libraries that we're linking in to the
# final executable. *However*, future labs will require libraries such
# as Pthreads (POSIX threads). Define those libraries in the LIBS
# variable. Note: your C code *must* compile and link correctly
# following the C11 standard (with GNU extensions).
######################################################################
CC=gcc
LD=ld
WARNS=-Wall -pedantic -Wextra
CFLAGS=-g3 -std=gnu11 ${WARNS}
LIBS=


# hello is an executable I want to build, the rest are handy things
all: tags headers mysh


# This builds visual symbol (.vs) files and the header files.
headers: *.c tags
	./headers.sh

# Tags (for C code) are too handy not to keep up to date.
# This lets us use Control-] with vim (ctags command).
# Alternatively, we can use etags with emacs (etags command).
# Comment out the command that you're NOT using.
tags: *.c
	ctags -R .
	#etags -R .


# This is a link rule, we have a universal compile rule down below
# Output is the target of the rule : -o $@
# I want to link all of the dependencies: $^
mysh: shell.o 
	${CC} -g -o $@ $^ ${LIBS}

shell.o: shell.c
	${CC} -g -c $<

# This is our master compiler rule to generate .o files.
# It needs all 4 warnings (see WARNS variable defined above)
%.o:%.c *.h
	${CC} ${CFLAGS} -c $< -o $@
