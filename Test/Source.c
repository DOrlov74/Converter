#include <gst/gst.h>
#include <glib.h>

static void
on_pad_added(GstElement* element,
  GstPad* pad,
  gpointer    data)
{
  GstPad* sinkpad;
  GstElement* queue = (GstElement*)data;

  g_print("Dynamic pad created, linking source and queue.\n");

  sinkpad = gst_element_get_static_pad(queue, "sink");

  gst_pad_link(pad, sinkpad);

  gst_object_unref(sinkpad);
}

static gboolean
bus_call(GstBus* bus,
  GstMessage* msg,
  gpointer    data)
{
  GMainLoop* loop = (GMainLoop*)data;

  switch (GST_MESSAGE_TYPE(msg)) {

  case GST_MESSAGE_EOS:
    g_print("End of stream\n");
    g_main_loop_quit(loop);
    break;

  case GST_MESSAGE_ERROR: {
    gchar* debug;
    GError* error;

    gst_message_parse_error(msg, &error, &debug);
    g_free(debug);

    g_printerr("Error: %s\n", error->message);
    g_error_free(error);

    g_main_loop_quit(loop);
    break;
  }
  default:
    break;
  }

  return TRUE;
}

int
main(int   argc,
  char* argv[])
{
  GMainLoop* loop;

  GstMessage* msg;
  GError* error;


  GstElement* pipeline, * source, * queue, * depayer, * muxer, * sink;
  GstBus* bus;
  guint bus_watch_id;

  /* Initialisation */
  gst_init(&argc, &argv);

  loop = g_main_loop_new(NULL, FALSE);


  /* Check input arguments */
  /* 


  /* Create gstreamer elements */
  /* Working command line example: gst-launch-1.0 rtspsrc location='rtsp://admin:admin@192.168.27.103:554/cam/realmonitor' ! rtph264depay ! mpegtsmux ! filesink location=file.mp4 */
  pipeline = gst_pipeline_new("rtsp_capture");
  source = gst_element_factory_make("souphttpsrc", "source");
  //source = gst_element_factory_make("rtspsrc", "rtsp-source");
  queue = gst_element_factory_make("queue", "queue"); // Do we need it?
  depayer = gst_element_factory_make("matroskademux", "matroskademux");
  muxer = gst_element_factory_make("matroskamux", "matroskamux");
  sink = gst_element_factory_make("filesink", "file-output");

  if (!pipeline || !source || !queue || !depayer || !muxer || !sink) {
    g_printerr("One element could not be created. Exiting.\n");
    return -1;
  }

  /* Set up the pipeline */

  g_object_set(source, "location",
    "https://gstreamer.freedesktop.org/data/media/sintel_trailer-480p.webm",
    NULL);

  /* we set the input filename to the source element */
  //g_object_set(source, "location", argv[1], NULL);
  g_object_set(sink, "location", "testout.mkv", NULL);

  /* we add a message handler */
  bus = gst_pipeline_get_bus(GST_PIPELINE(pipeline));
  bus_watch_id = gst_bus_add_watch(bus, bus_call, loop);
  gst_object_unref(bus);

  /* we add all elements into the pipeline */
  gst_bin_add_many(GST_BIN(pipeline),
    source, queue, depayer, muxer, sink, NULL);

  /* we link the elements together */
  /* file-source -> h264-depayer ~> mpeg-ts-muxer -> file-output */

  g_signal_connect(source, "pad-added", G_CALLBACK(on_pad_added), queue);
  gst_element_link(queue, depayer);
  gst_element_link(depayer, muxer);
  gst_element_link(muxer, sink);

  /* Set the pipeline to "playing" state*/
  g_print("Now playing: %s\n", argv[1]);
  gst_element_set_state(pipeline, GST_STATE_PLAYING);


  /* Iterate */
  g_print("Running...\n");
  g_main_loop_run(loop);


  /* Out of the main loop, clean up nicely */
  g_print("Returned, stopping playback\n");
  gst_element_set_state(pipeline, GST_STATE_NULL);

  g_print("Deleting pipeline\n");
  gst_object_unref(GST_OBJECT(pipeline));
  g_source_remove(bus_watch_id);
  g_main_loop_unref(loop);

  return 0;
}