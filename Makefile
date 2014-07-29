CC = gcc

.PHONY: all

all: jackrec

jackrec: main.o track.o
	$(CC) -o $@ $^ -ljack -lpthread -lncurses

%.o: %.c track.h
	$(CC) -c -o $@ $<
