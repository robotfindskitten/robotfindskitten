INCLUDES = -I. -I../../src
CC = gcc
CFLAGS = -Wall -g -O2

count_messages: count_messages.o ../../src/messages.h
	$(CC) $(CFLAGS) -o count_messages count_messages.o

.c.o:
	$(CC) $(CFLAGS) $(INCLUDES) -c $<

clean:
	-rm -f *.o count_messages core
