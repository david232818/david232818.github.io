# Makefile

CC = gcc
EXENAME = ca
SOURCES := $(wildcard *.c)
OBJS = ${SOURCES:.c=.o}

all: ${OBJS}
	$(CC) $^ -o $(EXENAME)
.c.o:
	$(CC) -c $^
clean:
	rm -f ${OBJS} $(EXENAME)
