
#ifndef MPGUI_H

	#define MPGUI_H

	#include <ctype.h>
	#include <math.h>
	#include <ncurses.h>
	#include <pthread.h>
	#include <stdlib.h>

	#define MSECS_IN_NSECS 1000000

	#define INITIAL_VOLUME 0
	#define LOWEST_VOLUME -99
	#define VOLUME_STEP 1

	#define LOUDER_KEY 'm'
	#define QUIETER_KEY 'n'
	#define QUIT_KEY 'q'

	int init_gui ();
	void *key_listener ();
	void kill_gui ();
	void update_gst (void);
	void update_gui (void);
	void set_pos (char *);

#endif
