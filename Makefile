CC = gcc

.PHONY: all

all: jackrec

jackrec: main.o track.o ui.o
	$(CC) -Wall -o $@ $^ -lm -lpthread `pkg-config --libs jack sndfile ncurses`

%.o: %.c track.h ui.h
	$(CC) -Wall -c `pkg-config --cflags jack sndfile ncurses` -o $@ $<
