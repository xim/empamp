#include "mediaplayer.h"
#include "mpgui.h"

static gboolean empamp_verbose = FALSE;
static int playlist_index = -1, playlist_size = 0;
static char **playlist = NULL;
static char **playlist_uris = NULL;

static GstElement *playbin = NULL;
static GMainLoop *loop = NULL;

static void handle_eos (GstBus *bus, GstMessage *msg)
{
	if (empamp_verbose)
		set_status ("End of stream");
	g_main_loop_quit (loop);
}

static void handle_error (GstBus *bus, GstMessage *msg)
{
	gchar  *debug;
	GError *error;

	gst_message_parse_error (msg, &error, &debug);
	char errmsg[term_width];
	snprintf (errmsg, term_width, "ERROR: %s (%s)", error->message, (debug) ? debug : "no details");
	set_status (errmsg);
	g_free (debug);
	g_error_free (error);

	g_main_loop_quit (loop);
}

static void handle_warning (GstBus *bus, GstMessage *msg)
{
	gchar  *debug;
	GError *error;

	gst_message_parse_warning (msg, &error, &debug);
	char warnmsg[term_width];
	snprintf (warnmsg, term_width, "WARNING: %s (%s)", error->message, (debug) ? debug : "no details");
	set_status (warnmsg);
	g_free (debug);
	g_error_free (error);
}

static void handle_clock_loss (GstBus *bus, GstMessage *msg)
{
	GstClock *clock;

	gst_message_parse_clock_lost (msg, &clock);
	char warnmsg[term_width];
	snprintf (warnmsg, term_width, "CLOCK LOST: %s", GST_OBJECT_NAME (clock));
	set_status (warnmsg);

	gst_element_set_state (playbin, GST_STATE_PAUSED);
	gst_element_set_state (playbin, GST_STATE_PLAYING);
}

static gchar * try_force_to_uri (gchar *path)
{
	gchar *temp = NULL;
	temp = g_uri_parse_scheme (path);
	// If the path isn't a URI, try to make it into one
	if (temp == NULL) {
		if (! g_path_is_absolute (path)) {
			temp = g_get_current_dir ();
			path = g_build_filename (temp, path, NULL);
		}
		temp = g_filename_to_uri (path, NULL, NULL);
		// If the string can't be made into a URI, return the string
		if (temp != NULL)
			path = g_strdup (temp);
	}
	g_free (temp);
	return path;
}


static void playlist_set_next ()
{
	playlist_index = (playlist_index + 1) % playlist_size;
	char msg[term_width];
	snprintf (msg, term_width, "Queueing file: %s", playlist[playlist_index]);
	set_status (msg);
	g_object_set (G_OBJECT (playbin), "uri", playlist_uris[playlist_index], NULL);
}

void playlist_go_previous ()
{
	gst_element_set_state(playbin, GST_STATE_READY);
	playlist_index = (playlist_size + playlist_index - 1) % playlist_size;
	g_object_set (G_OBJECT (playbin), "uri", playlist_uris[playlist_index], NULL);
	gst_element_set_state(playbin, GST_STATE_PLAYING);
}
void playlist_go_next ()
{
	gst_element_set_state(playbin, GST_STATE_READY);
	playlist_set_next ();
	gst_element_set_state(playbin, GST_STATE_PLAYING);
}

void toggle_play_pause () {
	GstState state;
	gst_element_get_state (playbin, &state, NULL, GST_CLOCK_TIME_NONE);
	if (state == GST_STATE_PLAYING) {
		gst_element_set_state (playbin, GST_STATE_PAUSED);
	} else {
		gst_element_set_state (playbin, GST_STATE_PLAYING);
	}
}

static gint64 get_pos_nanosecs () {
	gint64 position = -1;
	GstFormat format = GST_FORMAT_TIME;
	gst_element_query_position (playbin, &format, &position);
	return position;
}
static gint64 get_dur_nanosecs () {
	gint64 duration = -1;
	GstFormat format = GST_FORMAT_TIME;
	gst_element_query_duration (playbin, &format, &duration);
	return duration;
}

void seek_backwards (void) {
	gint64 position = get_pos_nanosecs ();
	position += GST_SECOND * -10;
	if (position < 0)
		position = 0;
	gst_element_seek_simple (playbin, GST_FORMAT_TIME, GST_SEEK_FLAG_FLUSH, position);
}
void seek_forwards (void) {
	gint64 position = get_pos_nanosecs ();
	position += GST_SECOND * 10;
	if (position >= get_dur_nanosecs ()) {
		playlist_go_next ();
		return;
	}
	gst_element_seek_simple (playbin, GST_FORMAT_TIME, GST_SEEK_FLAG_FLUSH, position);
}

void set_volume (int volume) {
	gdouble vol = ((gdouble) volume) / 100;
	g_object_set (G_OBJECT (playbin), "volume", vol, NULL);
}

void quit_empamp (void) {
	g_main_loop_quit (loop);
}

static gboolean print_progress (void)
{
	gint64 duration = get_dur_nanosecs ();
	gint64 position = get_pos_nanosecs ();
	char dur_str[15], pos_str[15], progress[32];

	if (position != -1) {
		snprintf (pos_str, 15, "%" GST_TIME_FORMAT, GST_TIME_ARGS (position));
	} else {
		snprintf (pos_str, 15, "-:--:--.-----");
	}

	if (duration != -1) {
		snprintf (dur_str, 15, "%" GST_TIME_FORMAT, GST_TIME_ARGS (duration));
	} else {
		snprintf (dur_str, 15, "-:--:--.-----");
	}

	sprintf (progress, "%14s / %14s", pos_str, dur_str);
	set_pos (progress);

	return TRUE;
}

int main (int argc, char *argv[])
{
	GstBus *bus;

	GOptionContext *ctx;
	GError *err = NULL;
	GOptionEntry entries[] = {
		{ "verbose", 'v', 0, G_OPTION_ARG_NONE, &empamp_verbose,
			"Output verbosely", NULL },
		{ G_OPTION_REMAINING, 0, 0, G_OPTION_ARG_FILENAME_ARRAY, &playlist,
			"Filename(s)" },
		{ NULL }
	};

	/* we must initialise the threading system before using any
	 * other GLib funtion, such as g_option_context_new() */
	if (!g_thread_supported ())
		g_thread_init (NULL);
	gst_init (&argc, &argv);

	ctx = g_option_context_new ("filename [more filenames] - play media files");
	g_option_context_add_main_entries (ctx, entries, NULL);
	g_option_context_add_group (ctx, gst_init_get_option_group ());
	if (!g_option_context_parse (ctx, &argc, &argv, &err)) {
		g_printerr ("%s\n", err->message);
		g_error_free (err);
		return EXIT_FAILURE;
	}

	/* Initialisation */

	loop = g_main_loop_new (NULL, FALSE);

	playlist_size = g_strv_length (playlist);
	playlist_uris = malloc(sizeof(char*) * playlist_size);
	for (int i = 0 ; i != playlist_size ; i++)
		playlist_uris[i] = try_force_to_uri (playlist[i]);

	/* Check input arguments */
	if (playlist_size < 1) {
		g_printerr (g_option_context_get_help (ctx, TRUE, NULL));
		return EXIT_FAILURE;
	}

	/* Create gstreamer elements */
	playbin = gst_element_factory_make ("playbin2", "playbin");

	if (!playbin) {
		g_printerr ("Player could not be created. Exiting.\n");
		return EXIT_FAILURE;
	}

	init_gui ();

	/* Set up the pipeline */

	/* we add a message handler */
	bus = gst_pipeline_get_bus (GST_PIPELINE (playbin));
	gst_bus_add_signal_watch (bus);

	g_signal_connect (bus, "message::eos", G_CALLBACK (handle_eos), NULL);
	g_signal_connect (bus, "message::error", G_CALLBACK (handle_error), NULL);
	g_signal_connect (bus, "message::warning", G_CALLBACK (handle_warning), NULL);
	// TODO Does this ever happen?
	g_signal_connect (bus, "message::clock-lost", G_CALLBACK (handle_clock_loss), playbin);


	/* Handle adding the next element in the playlist. This makes us gapless */
	g_signal_connect (playbin, "about-to-finish", G_CALLBACK (playlist_set_next), NULL);

	playlist_set_next ();
	toggle_play_pause ();

	guint progressor = g_timeout_add (33, (GSourceFunc) print_progress, NULL);

	/* Iterate */
	if (empamp_verbose)
		set_status ("Running...");

	g_main_loop_run (loop);
	g_source_remove (progressor);


	/* Out of the main loop, clean up nicely */
	set_status ("Quitting...");
	gst_element_set_state (playbin, GST_STATE_NULL);

	if (empamp_verbose)
		set_status ("Deleting pipeline");
	gst_object_unref (playbin);
	gst_object_unref (bus);
	g_main_loop_unref (loop);

	kill_gui ();
	return EXIT_SUCCESS;
}
