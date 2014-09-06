CC = gcc

.PHONY: all

all: portrec

portrec: main.o audio.o track.o ui.o
	$(CC) -Wall -o $@ $^ -lm -lpthread `pkg-config --libs portaudio-2.0 sndfile ncurses`

%.o: %.c audio.h track.h ui.h
	$(CC) -Wall -c `pkg-config --cflags portaudio-2.0 sndfile ncurses` -o $@ $<
