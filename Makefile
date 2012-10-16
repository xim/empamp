.PHONY: all clean

all: mediaplayer

clean:
	rm mediaplayer

mediaplayer: mediaplayer.c
	gcc -std=c99 -Wall mediaplayer.c -o mediaplayer `pkg-config --cflags --libs gstreamer-0.10`
