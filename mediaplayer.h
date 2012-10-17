
#ifndef MEDIAPLAYER_H
	
	#define MEDIAPLAYER_H

	#include <stdlib.h>
	#include <string.h>
	#include <gst/gst.h>
	#include <glib.h>

	void playlist_set_next (void);
	void playlist_set_previous (void);
	void set_volume (int);
	void toggle_play_pause (void);

#endif

