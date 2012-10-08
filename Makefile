mediaplayer:
	gcc -Wall mediaplayer.c -o mediaplayer $(pkg-config --cflags --libs gstreamer-0.10)
