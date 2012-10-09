#include <stdlib.h>
#include <gst/gst.h>
#include <glib.h>

gboolean empamp_verbose = FALSE;

static gboolean bus_call (GstBus *bus, GstMessage *msg, gpointer data)
{
	GMainLoop *loop = (GMainLoop *) data;

	switch (GST_MESSAGE_TYPE (msg)) {

		case GST_MESSAGE_EOS:
			g_print ("End of stream\n");
			g_main_loop_quit (loop);
			break;

		case GST_MESSAGE_ERROR: {
			gchar  *debug;
			GError *error;

			gst_message_parse_error (msg, &error, &debug);
			g_free (debug);

			g_printerr ("Error: %s\n", error->message);
			g_error_free (error);

			g_main_loop_quit (loop);
			break;
		}
		default:
			break;
	}

	return TRUE;
}


static void on_pad_added (GstElement *element, GstPad *pad, gpointer data)
{
	GstPad *sinkpad;
	GstElement *decoder = (GstElement *) data;

	/* We can now link this pad with the vorbis-decoder sink pad */
	g_print ("Dynamic pad created.\n");

	sinkpad = gst_element_get_static_pad (decoder, "sink");

	gst_pad_link (pad, sinkpad);

	gst_object_unref (sinkpad);
}



int main (int argc, char *argv[])
{

	GMainLoop *loop;

	GstElement *pipeline, *source, *decoder, *conv, *sink;
	GstBus *bus;

	GOptionContext *ctx;
	GError *err = NULL;
	GOptionEntry entries[] = {
		{ "verbose", 'v', 0, G_OPTION_ARG_NONE, &empamp_verbose,
			"Output verbosely", NULL },
		{ NULL }
	};

	/* we must initialise the threading system before using any
	 *    * other GLib funtion, such as g_option_context_new() */
	if (!g_thread_supported ())
		g_thread_init (NULL);

	ctx = g_option_context_new ("filename [more filenames] - play media files");
	g_option_context_add_main_entries (ctx, entries, NULL);
	g_option_context_add_group (ctx, gst_init_get_option_group ());
	if (!g_option_context_parse (ctx, &argc, &argv, &err)) {
		g_print ("%s\n", err->message);
		g_error_free (err);
		return EXIT_FAILURE;
	}

	/* Initialisation */
	gst_init (&argc, &argv);

	loop = g_main_loop_new (NULL, FALSE);


	/* Check input arguments */
	if (argc < 2) {
		g_printerr (g_option_context_get_help (ctx, TRUE, NULL));
		return EXIT_FAILURE;
	}


	/* Create gstreamer elements */
	pipeline = gst_pipeline_new ("audio-player");
	source   = gst_element_factory_make ("filesrc",       "file-source");
	decoder  = gst_element_factory_make ("decodebin",     "decoder");

	conv     = gst_element_factory_make ("audioconvert",  "converter");
	sink     = gst_element_factory_make ("autoaudiosink", "audio-output");

	if (!pipeline || !source || !decoder || !conv || !sink) {
		g_printerr ("One element could not be created. Exiting.\n");
		return EXIT_FAILURE;
	}

	/* Set up the pipeline */

	/* we add a message handler */
	bus = gst_pipeline_get_bus (GST_PIPELINE (pipeline));
	gst_bus_add_watch (bus, bus_call, loop);
	gst_object_unref (bus);

	/* we add all elements into the pipeline */
	/* file-source | decodebin | converter | alsa-output */
	gst_bin_add_many (GST_BIN (pipeline),
			source, decoder, conv, sink, NULL);

	/* we link the elements together */
	/* file-source -> ogg-demuxer ~> vorbis-decoder -> converter -> alsa-output */
	gst_element_link (source, decoder);
	gst_element_link_many (conv, sink, NULL);
	g_signal_connect (decoder, "pad-added", G_CALLBACK (on_pad_added), conv);

	/* note that the decoder will be linked to the converter dynamically.
	The reason is that Ogg may contain various streams (for example
	audio and video). The source pad(s) will be created at run time,
	by the demuxer when it detects the amount and nature of streams.
	Therefore we connect a callback function which will be executed
	when the "pad-added" is emitted.*/

	for (int i = 1 ; i != argc ; i++) {

		/* we set the input filename to the source element */
		g_object_set (G_OBJECT (source), "location", argv[i], NULL);


		/* Set the pipeline to "playing" state*/
		g_print ("Now playing: %s\n", argv[i]);
		gst_element_set_state (pipeline, GST_STATE_PLAYING);


		/* Iterate */
		if (empamp_verbose)
			g_print ("Running...\n");
		g_main_loop_run (loop);


		/* Out of the main loop, clean up nicely */
		g_print ("Returned, stopping playback\n");
		gst_element_set_state (pipeline, GST_STATE_NULL);
	}

	if (empamp_verbose)
		g_print ("Deleting pipeline\n");
	gst_object_unref (GST_OBJECT (pipeline));

	return EXIT_SUCCESS;
}
