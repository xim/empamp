.PHONY: all clean

all: empamp

clean:
	rm empamp

empamp: mediaplayer.c mediaplayer.h mpgui.c mpgui.h
	gcc -std=c99 -Wall mpgui.c mediaplayer.c -o empamp `pkg-config --cflags --libs gstreamer-1.0 ncurses`
