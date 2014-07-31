CC = gcc

.PHONY: all

all: jackrec

jackrec: main.o track.o
	$(CC) -o $@ $^ -lpthread `pkg-config --libs jack sndfile ncurses`

%.o: %.c track.h
	$(CC) -c `pkg-config --cflags jack sndfile ncurses` -o $@ $<
