
#ifndef MEDIAPLAYER_H

	#define MEDIAPLAYER_H

	#include <glib.h>
	#include <gst/gst.h>
	#include <stdlib.h>
	#include <string.h>

	void playlist_go_next (void);
	void playlist_go_previous (void);
	void quit_empamp (void);
	void set_volume (int);
	void toggle_play_pause (void);
	void seek_forwards (void);
	void seek_backwards (void);

#endif

