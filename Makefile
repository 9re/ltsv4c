CC = gcc
CFLAGS = -O0 -g -Wall -Wextra -std=c89 -pedantic-errors

all: test

.PHONY: test
test: test.c ltsv4c.c
	$(CC) $(CFLAGS) -o $@ test.c ltsv4c.c
	./$@

clean:
	rm -f test *.o
