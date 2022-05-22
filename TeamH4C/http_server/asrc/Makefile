# Makefile

CC = gcc
EXENAME = http_server
SOURCES := $(wildcard *.c)
OBJS = ${SOURCES:.c=.o}

all: ${OBJS}
	$(CC) -g $^ -o $(EXENAME)

.c.o:
	$(CC) -g -c $^

clean:
	rm -f ${OBJS} $(EXENAME)
