#include <gst/gst.h>
#include <stdio.h>
#include <string.h>

#ifdef __APPLE__
#include <TargetConditionals.h>
#endif

/* Structure to contain all our information, so we can pass it to callbacks */
typedef struct _CustomData
{
  GstElement* pipeline;
  GstElement* source;
  GstElement* audio_convert;
  GstElement* audio_resample;
  GstElement* audio_tee;
  GstElement* video_tee;
  GstElement* audio_auto_queue;
  GstElement* audio_file_queue;
  GstElement* vp8dec;
  GstElement* video_convert;
  GstElement* video_auto_queue;
  GstElement* video_file_queue;
  GstElement* audio_sink;
  GstElement* video_sink;
  GstElement* vorbisdec;
  GstElement* vorbisparse;
  GstElement* matroskademux;
  GstElement* audio_matroskamux;
  GstElement* video_matroskamux;
  GstElement* audio_file_sink;
  GstElement* video_file_sink;
} CustomData;

/* Handler for the pad-added signal */
static void pad_added_handler(GstElement* src, GstPad* pad,
  CustomData* data);

int
tutorial_main(int argc, char* argv[])
{
  CustomData data;
  GstBus* bus;
  GstMessage* msg;
  GstStateChangeReturn ret;
  gboolean terminate = FALSE;
  GstPad* tee_audio_auto_pad;
  GstPad* tee_audio_file_pad;
  GstPad* tee_video_auto_pad;
  GstPad* tee_video_file_pad;
  GstPad* queue_audio_auto_pad;
  GstPad* queue_audio_file_pad;
  GstPad* queue_video_auto_pad;
  GstPad* queue_video_file_pad;
  GstPad* queue_video_matroska_pad;
  GstPad* matroska_video_pad;
  GstPad* matroska_audio_pad;
  GstPad* vorbis_matroska_pad;

  /* Initialize GStreamer */
  gst_init(&argc, &argv);

  /* Check input arguments */
  if (argc != 2) {
    g_printerr("Usage: %s <input filename>\n", argv[0]);
    return -1;
  }

  /* Create the elements */
  data.source = gst_element_factory_make("filesrc", "source");
  data.matroskademux = gst_element_factory_make("matroskademux", "matroskademux");
  data.audio_tee = gst_element_factory_make("tee", "audio_tee");
  data.video_tee = gst_element_factory_make("tee", "video_tee");
  data.audio_file_queue = gst_element_factory_make("queue", "audio_file_queue");
  data.video_file_queue = gst_element_factory_make("queue", "video_file_queue");
  data.audio_auto_queue = gst_element_factory_make("queue", "audio_auto_queue");
  data.video_auto_queue = gst_element_factory_make("queue", "video_auto_queue");
  data.vorbisdec = gst_element_factory_make("vorbisdec", "vorbisdec");
  data.audio_convert = gst_element_factory_make("audioconvert", "audio_convert");
  data.audio_resample = gst_element_factory_make("audioresample", "audio_resample");
  data.vp8dec = gst_element_factory_make("vp8dec", "vp8dec");
  data.video_convert = gst_element_factory_make("videoconvert", "video_convert");
  data.audio_sink = gst_element_factory_make("autoaudiosink", "audio_sink");
  data.video_sink = gst_element_factory_make("autovideosink", "video_sink");
  data.vorbisparse = gst_element_factory_make("vorbisparse", "vorbisparse");
  data.audio_matroskamux = gst_element_factory_make("matroskamux", "audio_matroskamux");
  data.video_matroskamux = gst_element_factory_make("matroskamux", "video_matroskamux");
  data.audio_file_sink = gst_element_factory_make("filesink", "audio_file_sink");
  data.video_file_sink = gst_element_factory_make("filesink", "video_file_sink");

  /* Create the empty pipeline */
  data.pipeline = gst_pipeline_new("test-pipeline");

  if (!data.pipeline || !data.matroskademux || !data.audio_tee || !data.video_tee || !data.source
    || !data.audio_file_queue || !data.video_file_queue || !data.audio_auto_queue || !data.video_auto_queue
    || !data.vorbisdec || !data.audio_convert || !data.audio_resample || !data.vp8dec
    || !data.video_convert || !data.audio_sink || !data.video_sink || !data.vorbisparse
    || !data.audio_matroskamux || !data.video_matroskamux || !data.audio_file_sink || !data.video_file_sink) {
    g_printerr("Not all elements could be created.\n");
    return -1;
  }

  /* Build the pipeline. Note that we are NOT linking the source at this
   * point. We will do it later. */
  gst_bin_add_many(GST_BIN(data.pipeline), data.source, data.matroskademux, data.audio_tee, data.video_tee,
    data.audio_file_queue, data.video_file_queue, data.audio_auto_queue, data.video_auto_queue,
    data.audio_convert, data.audio_resample, data.vp8dec, data.audio_sink, data.video_convert, data.video_sink,
    data.vorbisdec, data.vorbisparse, data.audio_matroskamux, data.video_matroskamux, data.audio_file_sink, data.video_file_sink, NULL);
  if (!gst_element_link_many(data.source, data.matroskademux, NULL)) {
    g_printerr("Elements %s, %s could not be linked.\n", gst_pad_get_name(data.source), gst_pad_get_name(data.matroskademux));
    gst_object_unref(data.pipeline);
    return -1;
  }
  if (!gst_element_link_many(data.audio_auto_queue, data.vorbisdec, data.audio_convert,
    data.audio_resample, data.audio_sink, NULL)) {
    g_printerr("Elements %s, %s, %s, %s, %s could not be linked.\n", gst_pad_get_name(data.audio_auto_queue),
      gst_pad_get_name(data.vorbisdec), gst_pad_get_name(data.audio_convert), gst_pad_get_name(data.audio_resample),
      gst_pad_get_name(data.audio_sink));
    gst_object_unref(data.pipeline);
    return -1;
  }
  if (!gst_element_link_many(data.video_auto_queue, data.vp8dec, data.video_convert, data.video_sink, NULL)) {
    g_printerr("Elements %s, %s, %s, %s could not be linked.\n", gst_pad_get_name(data.video_auto_queue),
      gst_pad_get_name(data.vp8dec), gst_pad_get_name(data.video_convert), gst_pad_get_name(data.video_sink));
    gst_object_unref(data.pipeline);
    return -1;
  }
  if (!gst_element_link_many(data.audio_file_queue, data.vorbisparse, NULL)) {
    g_printerr("Elements %s, %s could not be linked.\n", gst_pad_get_name(data.audio_file_queue),
      gst_pad_get_name(data.vorbisparse));
    gst_object_unref(data.pipeline);
    return -1;
  }
  if (!gst_element_link_many(data.audio_matroskamux, data.audio_file_sink, NULL)) {
    g_printerr("Elements %s, %s could not be linked.\n", gst_pad_get_name(data.audio_matroskamux),
      gst_pad_get_name(data.audio_file_sink));
    gst_object_unref(data.pipeline);
    return -1;
  }
  if (!gst_element_link_many(data.video_matroskamux, data.video_file_sink, NULL)) {
    g_printerr("Elements %s, %s could not be linked.\n", gst_pad_get_name(data.video_matroskamux),
      gst_pad_get_name(data.video_file_sink));
    gst_object_unref(data.pipeline);
    return -1;
  }

  /* Manually link the Tee, which has "Request" pads */
  tee_audio_auto_pad = gst_element_request_pad_simple(data.audio_tee, "src_%u");
  g_print("Obtained request pad %s for audio auto branch.\n",
    gst_pad_get_name(tee_audio_auto_pad));
  queue_audio_auto_pad = gst_element_get_static_pad(data.audio_auto_queue, "sink");
  tee_video_auto_pad = gst_element_request_pad_simple(data.video_tee, "src_%u");
  g_print("Obtained request pad %s for video auto branch.\n",
    gst_pad_get_name(tee_video_auto_pad));
  queue_video_auto_pad = gst_element_get_static_pad(data.video_auto_queue, "sink");
  tee_audio_file_pad = gst_element_request_pad_simple(data.audio_tee, "src_%u");
  g_print("Obtained request pad %s for audio file branch.\n",
    gst_pad_get_name(tee_audio_file_pad));
  queue_audio_file_pad = gst_element_get_static_pad(data.audio_file_queue, "sink");
  tee_video_file_pad = gst_element_request_pad_simple(data.video_tee, "src_%u");
  g_print("Obtained request pad %s for video file branch.\n",
    gst_pad_get_name(tee_video_file_pad));
  queue_video_file_pad = gst_element_get_static_pad(data.video_file_queue, "sink");
  if (gst_pad_link(tee_audio_auto_pad, queue_audio_auto_pad) != GST_PAD_LINK_OK ||
    gst_pad_link(tee_video_auto_pad, queue_video_auto_pad) != GST_PAD_LINK_OK ||
    gst_pad_link(tee_audio_file_pad, queue_audio_file_pad) != GST_PAD_LINK_OK ||
    gst_pad_link(tee_video_file_pad, queue_video_file_pad) != GST_PAD_LINK_OK) {
    g_printerr("Tee could not be linked.\n");
    gst_object_unref(data.pipeline);
    return -1;
  }
  gst_object_unref(queue_audio_auto_pad);
  gst_object_unref(queue_video_auto_pad);
  gst_object_unref(queue_audio_file_pad);
  gst_object_unref(queue_video_file_pad);

  /* Manually link the matroskamux, which has "Request" pads */
  matroska_video_pad = gst_element_request_pad_simple(data.video_matroskamux, "video_%u");
  g_print("Obtained request pad %s for video matroskamux branch.\n",
    gst_pad_get_name(matroska_video_pad));
  queue_video_matroska_pad = gst_element_get_static_pad(data.video_file_queue, "src");
  matroska_audio_pad = gst_element_request_pad_simple(data.audio_matroskamux, "audio_%u");
  g_print("Obtained request pad %s for audio matroskamux branch.\n",
    gst_pad_get_name(matroska_audio_pad));
  vorbis_matroska_pad = gst_element_get_static_pad(data.vorbisparse, "src");
  if (gst_pad_link(queue_video_matroska_pad, matroska_video_pad) != GST_PAD_LINK_OK ||
    gst_pad_link(vorbis_matroska_pad, matroska_audio_pad) != GST_PAD_LINK_OK) {
    g_printerr("Matroskamux could not be linked.\n");
    gst_object_unref(data.pipeline);
    return -1;
  }
  gst_object_unref(queue_video_matroska_pad);
  gst_object_unref(vorbis_matroska_pad);

  /* Set the URI to play */
  g_object_set(data.source, "location",
    argv[1],
    NULL);

  char* name = argv[1];
  int pos = strstr(argv[1], ".") - argv[1];
  if (pos != 0) {
    strncpy_s(name, strlen(name) + 1, argv[1], pos);
  }
  char* filename = malloc(strlen(name) + 5);
  sprintf_s(filename, strlen(name) + 5, "%s.mka", name);

  /* Set the output location */
  g_object_set(data.audio_file_sink, "location",
    filename,
    NULL);
  sprintf_s(filename, strlen(filename) + 1, "%s.mkv", name);
  g_object_set(data.video_file_sink, "location",
    filename,
    NULL);

  /* Connect to the pad-added signal */
  g_signal_connect(data.matroskademux, "pad-added", G_CALLBACK(pad_added_handler),
    &data);

  /* Start playing */
  ret = gst_element_set_state(data.pipeline, GST_STATE_PLAYING);
  if (ret == GST_STATE_CHANGE_FAILURE) {
    g_printerr("Unable to set the pipeline to the playing state.\n");
    gst_object_unref(data.pipeline);
    return -1;
  }

  /* Listen to the bus */
  bus = gst_element_get_bus(data.pipeline);
  do {
    msg = gst_bus_timed_pop_filtered(bus, GST_CLOCK_TIME_NONE,
      GST_MESSAGE_STATE_CHANGED | GST_MESSAGE_ERROR | GST_MESSAGE_EOS);

    /* Parse message */
    if (msg != NULL) {
      GError* err;
      gchar* debug_info;

      switch (GST_MESSAGE_TYPE(msg)) {
      case GST_MESSAGE_ERROR:
        gst_message_parse_error(msg, &err, &debug_info);
        g_printerr("Error received from element %s: %s\n",
          GST_OBJECT_NAME(msg->src), err->message);
        g_printerr("Debugging information: %s\n",
          debug_info ? debug_info : "none");
        g_clear_error(&err);
        g_free(debug_info);
        terminate = TRUE;
        break;
      case GST_MESSAGE_EOS:
        g_print("End-Of-Stream reached.\n");
        terminate = TRUE;
        break;
      case GST_MESSAGE_STATE_CHANGED:
        /* We are only interested in state-changed messages from the pipeline */
        if (GST_MESSAGE_SRC(msg) == GST_OBJECT(data.pipeline)) {
          GstState old_state, new_state, pending_state;
          gst_message_parse_state_changed(msg, &old_state, &new_state,
            &pending_state);
          g_print("Pipeline state changed from %s to %s:\n",
            gst_element_state_get_name(old_state),
            gst_element_state_get_name(new_state));
        }
        break;
      default:
        /* We should not reach here */
        g_printerr("Unexpected message received.\n");
        break;
      }
      gst_message_unref(msg);
    }
  } while (!terminate);

  /* Free resources */
  gst_object_unref(bus);
  gst_element_set_state(data.pipeline, GST_STATE_NULL);
  gst_object_unref(data.pipeline);
  return 0;
}

/* This function will be called by the pad-added signal */
static void
pad_added_handler(GstElement* src, GstPad* new_pad, CustomData* data)
{
  GstPad* audio_sink_pad = gst_element_get_static_pad(data->audio_tee, "sink");
  GstPad* video_sink_pad = gst_element_get_static_pad(data->video_tee, "sink");
  GstPadLinkReturn ret;
  GstCaps* new_pad_caps = NULL;
  GstStructure* new_pad_struct = NULL;
  const gchar* new_pad_type = NULL;

  g_print("Received new pad '%s' from '%s':\n", GST_PAD_NAME(new_pad),
    GST_ELEMENT_NAME(src));

  /* If our converter is already linked, we have nothing to do here */
  if (gst_pad_is_linked(audio_sink_pad) && gst_pad_is_linked(video_sink_pad)) {
    g_print("We are already linked. Ignoring.\n");
    goto exit;
  }

  /* Check the new pad's type */
  new_pad_caps = gst_pad_get_current_caps(new_pad);
  new_pad_struct = gst_caps_get_structure(new_pad_caps, 0);
  new_pad_type = gst_structure_get_name(new_pad_struct);
  if (strstr(new_pad_type, "audio")) {
    /* Attempt the link audio pad*/
    ret = gst_pad_link(new_pad, audio_sink_pad);
    if (GST_PAD_LINK_FAILED(ret)) {
      g_print("Type is '%s' but audio link failed.\n", new_pad_type);
    }
    else {
      g_print("Audio Link succeeded (type '%s').\n", new_pad_type);
    }
  }
  else {
    /* Attempt the link video pad*/
    ret = gst_pad_link(new_pad, video_sink_pad);
    if (GST_PAD_LINK_FAILED(ret)) {
      g_print("Type is '%s' but video link failed.\n", new_pad_type);
    }
    else {
      g_print("Video Link succeeded (type '%s').\n", new_pad_type);
    }
  }

exit:
  /* Unreference the new pad's caps, if we got them */
  if (new_pad_caps != NULL)
    gst_caps_unref(new_pad_caps);

  /* Unreference the sink pad */
  gst_object_unref(audio_sink_pad);
  gst_object_unref(video_sink_pad);
}

int
main(int argc, char* argv[])
{
#if defined(__APPLE__) && TARGET_OS_MAC && !TARGET_OS_IPHONE
  return gst_macos_main(tutorial_main, argc, argv, NULL);
#else
  return tutorial_main(argc, argv);
#endif
}
