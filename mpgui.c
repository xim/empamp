#include "mpgui.h"

/* static declarations. */
static void update_gst (void);
static void update_gui (void);
static int db_to_percent (int db);
static void volume_increase ();
static void volume_decrease ();
static void identify_key (int key);

static struct keydesc keymapping[] = {
  {KEY_PPAGE, playlist_go_previous},
  {KEY_NPAGE, playlist_go_next},
  {KEY_UP, volume_increase},
  {KEY_DOWN, volume_decrease},
  {'q', quit_empamp},
  {KEY_LEFT, seek_backwards},
  {KEY_RIGHT, seek_forwards},
  {' ', toggle_play_pause},
  {-1, NULL}
};

/* update gui after key event here. */
static void update_gui (void)
{
	/* format volume string. */
	mvprintw (term_height - 1, term_width - 6, "%d dB  ", volume);

	/* update display. */
	refresh();
}

/* update gst elements */
static void update_gst (void)
{
	set_volume(db_to_percent(volume));
}

void set_pos (char *position)
{
	mvprintw (term_height - 1, 0, "%s", position);
	refresh();
}
void set_status (char *message, ...)
{
	va_list argptr;
	char output[term_width + 1];
	va_start (argptr, message);
	vsnprintf (output, term_width + 1, message, argptr);
	int linelen = strlen(output);
	if (linelen != term_width) {
		output[linelen] = '\n';
		output[linelen + 1] = '\0';
	}
	va_end (argptr);
	wprintw (log_window, output);
	wrefresh (log_window);
}

static int db_to_percent (int db)
{
	return (int) 100 * ((float) pow (2.0, (float) db / 6));
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
	log_window = newwin (term_height - 4, term_width, 2, 0);
	scrollok(log_window, TRUE);

	/* spawn keyboard input listening thread. */
	pthread_t kbi_thread;

	int pret = pthread_create (&kbi_thread, NULL, key_listener, NULL);

	if (pret < 0)
		exit(EXIT_FAILURE);

	/* lastly, init control parameters. */
	volume = INITIAL_VOLUME;
	update_gui();
	update_gst();
	wrefresh (log_window);

	return EXIT_SUCCESS;
}
	
void kill_gui ()
{
	move (term_height - 1, 0);
	clrtoeol ();
	/* close up NCURSES window. */
	refresh();
	endwin();
}
