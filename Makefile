POPCL = popcl

POPCL_S = source/popcl.cpp

CC=g++
CFLAGS=-std=c++14

all: popcl 

popcl: source/popcl.h source/popcl.cpp
	$(CC) $(CFLAGS) $(POPCL_S) -o popcl

clean:
	rm popcl