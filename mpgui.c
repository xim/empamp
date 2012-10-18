#include "mpgui.h"
#include "mediaplayer.h"

struct keydesc {
	int code;
	void (*function)();
};

int volume;

int term_height, term_width;

void update_gui (void)
{
	/* update gui after key event here. */

	/* format volume string. */
	mvprintw (term_height - 1, term_width - 6, "%d dB  ", volume);

	/* update display. */
	refresh();
}

void set_pos (char *position) {
	mvprintw (term_height - 1, 0, "%s", position);
	refresh();
}

static int db_to_percent (int db)
{
	return (int) 100 * ((float) pow (2.0, (float) db / 6));
}

void update_gst (void)
{
	set_volume(db_to_percent(volume));
}


static void volume_increase ()
{
	if (volume <= (0 - VOLUME_STEP))
		volume += VOLUME_STEP;
}
static void volume_decrease ()
{
	if (volume > LOWEST_VOLUME)
		volume -= VOLUME_STEP;
}


/* identify recent key event and update everthing else accordingly. */
static void identify_key (int key)
{
	static struct keydesc keymapping[] = {
		{KEY_PPAGE, playlist_go_previous},
		{KEY_NPAGE, playlist_go_next},
		{KEY_UP, volume_increase},
		{KEY_DOWN, volume_decrease},
		{LOUDER_KEY, volume_increase},
		{QUIETER_KEY, volume_decrease},
		{QUIT_KEY, quit_empamp},
		{KEY_LEFT, seek_backwards},
		{KEY_RIGHT, seek_forwards},
		//{KEY_HOME, },
		//{KEY_END, },
		//{KEY_BACKSPACE, },
		//{KEY_IC, },
		//{KEY_DC, },
		//{KEY_F(1), },
		//{KEY_F(2), },
		//{KEY_F(3), },
		//{KEY_F(4), },
		//{KEY_F(5), },
		//{KEY_F(6), },
		//{KEY_F(7), },
		//{KEY_F(8), },
		//{KEY_F(9), },
		//{KEY_F(10), },
		//{KEY_F(11), },
		//{KEY_F(12), },
		{-1, NULL}
	};
	for (int i = 0 ; keymapping[i].code != -1 ; i++) {
		if (keymapping[i].code == key) {
			keymapping[i].function ();
			update_gst();
			update_gui();
			return;
		}
	}
}

/* listening for key events (blocking). */
void *key_listener ()
{
	int recent_key;

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
	if (initscr() == NULL) {
		fprintf(stderr, "Error initializing ncurses.\n");
		exit(EXIT_FAILURE);
	}

	/* no cursor. */
	curs_set(0);

	/* no newline or anything. */
	nonl();
	cbreak();

	/* Predictable keypad */
	keypad(stdscr, TRUE);

	/* no echo. */
	noecho();

	/* get window size. */
	getmaxyx(stdscr, term_height, term_width);

	/* spawn keyboard input listening thread. */
	pthread_t kbi_thread;

	int pret = pthread_create (&kbi_thread, NULL, key_listener, NULL);

	if (pret < 0)
		exit(EXIT_FAILURE);

	/* lastly, init control parameters. */
	volume = INITIAL_VOLUME;
	update_gst();

	return EXIT_SUCCESS;
}
	
void kill_gui ()
{
	/* close up NCURSES window. */
	curs_set(1);
	endwin();
	refresh();
}
