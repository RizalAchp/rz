
CC=clang
CFLAGS=-DCB_TESTS -Wall -Wextra -Wpedantic 


all:
	$(CC) $(CFLAGS) cb.c -o cb 

unitest:
	$(CC) $(CFLAGS) -DUNIT_TEST cb.c -o $@
	./$@ --unittest

clean:
	rm -rf ./cb ./cb.old ./unitest
