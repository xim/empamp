
#ifndef MPGUI_H
	
	#define MPGUI_H

	#include <stdlib.h>
	#include <ncurses.h>
	#include <pthread.h>

	#define MSECS_IN_NSECS 1000000

	#define INITIAL_VOLUME 100
	#define VOLUME_STEP 5
	#define LOUDER_KEY 'm'
	#define QUIETER_KEY 'n'

	void *key_listener ();
	void update_gui (void);
	void update_gst (void);
	void identify_key (char key);
	int init_gui ();
	void kill_gui ();

#endif
