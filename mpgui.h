
#ifndef MPGUI_H

	#define MPGUI_H

	#include <ctype.h>
	#include <math.h>
	#include <ncurses.h>
	#include <pthread.h>
	#include <stdarg.h>
	#include <stdlib.h>
	#include "mediaplayer.h"

	#define MSECS_IN_NSECS 1000000

	#define INITIAL_VOLUME 0
	#define LOWEST_VOLUME -99
	#define VOLUME_STEP 1

	int init_gui ();
	void *key_listener ();
	void kill_gui ();
	void set_pos (char *position);
	void set_status (char *message, ...);

	struct keydesc {
		int code;
		void (*function)();
	};

	int term_height, term_width;
	int current_logline, min_logline, max_logline_offset;
	int volume;

#endif
