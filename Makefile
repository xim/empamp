.PHONY: all clean

all: mediaplayer

clean:
	rm mediaplayer

mediaplayer: mediaplayer.c mediaplayer.h mpgui.c mpgui.h
	gcc -std=c99 -Wall mpgui.c mediaplayer.c -o mediaplayer `pkg-config --cflags --libs gstreamer-1.0 ncurses`
