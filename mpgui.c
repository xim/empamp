#include "mpgui.h"
#include "mediaplayer.h"

int volume;

int term_height, term_width;

void update_gui (void)
{
	/* update gui after key event here. */

	/* format volume string. */
	char *vol_string = malloc (4 * sizeof(char));
	sprintf(vol_string, "%d dB", volume);

	/* clear printing area, then print. */
	mvaddstr (term_height - 1, 0, "    ");
	mvaddstr (term_height - 1, 0, vol_string);

	/* update display. */
	refresh();

	/* free that string. */
	free (vol_string);
}

static int db_to_percent (int db)
{
  return (int) 100 * ((float) pow (2.0, (float) db / 6));
}

void update_gst (void)
{
	/* TODO: update GST after key event here. */
	set_volume(db_to_percent(volume));
}

/* identify recent key event and update everthing else accordingly. */
void identify_key (char key)
{
	switch (key) {

		case LOUDER_KEY:
			if (volume <= (0 - VOLUME_STEP))
				volume += VOLUME_STEP;
			break;

		case QUIETER_KEY:
		  volume -= VOLUME_STEP;
			break;
	}

	update_gst();
	update_gui();
}	

/* listening for key events (blocking). */
void *key_listener ()
{
	char recent_key;

	while(1)
	{
		/* get new input. */
		recent_key = getch();

		identify_key (recent_key);
	}
}

int init_gui ()
{

	/* setup ncurses window. */
	initscr();

	/* no cursor. */
	curs_set(0);

	/* no newline or anything. */
	nonl();
	cbreak();

	/* no echo. */
	noecho();

	/* get window size. */
 	getmaxyx(stdscr, term_height, term_width);

	/* spawn keyboard input listening thread. */
    pthread_t kbi_thread;

	int pret = pthread_create (&kbi_thread, NULL, key_listener, NULL);

    if (pret < 0)
        return EXIT_FAILURE;

	/* lastly, init control parameters. */
	volume = INITIAL_VOLUME;

	return EXIT_SUCCESS;
}
	
void kill_gui ()
{
	/* close up NCURSES window. */
	endwin();
}
