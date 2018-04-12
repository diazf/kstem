# makefile for building Kstem and a routine to interactively test it
#   (test-kstem), and a routine to apply kstem to a file (kstem-file).
#
# CREATED: 5/3/01
#

#
#  Makefile variables.
#

#For Optimization
CFLAGS= -O2
#For debugging
#CFLAGS= -g

CC = g++


#
#  The default is to build everything.  (The first rule is the default rule.)
#

all:		test-kstem kstem-file kstem

kstem:  kstem.c public-kstem.o hash.o
	$(CC) $(CFLAGS) -o kstem $^ -lm

test-kstem:	test-kstem.c public-kstem.o hash.o 
	$(CC) $(CFLAGS) -o test-kstem $^ -lm

kstem-file:	kstem-file.c public-kstem.o hash.o 
	$(CC) $(CFLAGS) -o kstem-file $^ -lm

public-kstem.o: public-kstem-v0.8.c
	$(CC) -o public-kstem.o -c public-kstem-v0.8.c

hash.o:         hash.c hash.h
	$(CC) -c hash.c 

clean:	
	/bin/rm -f test-kstem kstem-file hash.o public-kstem.o kstem

# end makefile
