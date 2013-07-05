/*
 * automuxerbin.c - gst-kurento-plugins
 *
 * Copyright (C) 2013 Kurento
 * Contact: José Antonio Santos Cadenas <santoscadenas@kurento.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <gst/check/gstcheck.h>
#include <gst/gst.h>
#include <glib.h>

GstElement *pipeline;

static void
bus_msg (GstBus * bus, GstMessage * msg, gpointer pipe)
{

  switch (msg->type) {
    case GST_MESSAGE_ERROR:{
      GST_ERROR ("Error: %P", msg);
      GST_DEBUG_BIN_TO_DOT_FILE_WITH_TS (GST_BIN (pipe),
          GST_DEBUG_GRAPH_SHOW_ALL, "bus_error");
      fail ("Error received on bus");
      break;
    }
    case GST_MESSAGE_WARNING:{
      GST_WARNING ("Warning: %P", msg);
      GST_DEBUG_BIN_TO_DOT_FILE_WITH_TS (GST_BIN (pipe),
          GST_DEBUG_GRAPH_SHOW_ALL, "warning");
      break;
    }
    case GST_MESSAGE_STATE_CHANGED:
    {
      if (g_str_has_prefix (GST_OBJECT_NAME (msg->src), "automuxerbin")) {
        GST_INFO ("Event: %P", msg);
      }
    }
      break;
    default:
      break;
  }
}

static void
fakesink_hand_off (GstElement * fakesink, GstBuffer * buf, GstPad * pad,
    gpointer data)
{
  GMainLoop *loop = (GMainLoop *) data;

  GST_DEBUG ("buffer created");
  g_main_loop_quit (loop);

}

static gboolean test_timeout = FALSE;

static gboolean
timeout (gpointer data)
{
  GMainLoop *loop = (GMainLoop *) data;

  GST_DEBUG ("finished timeout");

  test_timeout = TRUE;
  GST_DEBUG_BIN_TO_DOT_FILE_WITH_TS (GST_BIN (pipeline),
      GST_DEBUG_GRAPH_SHOW_ALL, "finished timeout");
  g_main_loop_quit (loop);

  return FALSE;
}

void
pad_added (GstElement * element, GstPad * pad, gpointer data)
{
  GstPad *sinkpad;
  GstElement *fakesink = (GstElement *) data;

  GST_DEBUG ("pad_added callback\n");
  if (GST_PAD_IS_SRC (pad)) {
    sinkpad = gst_element_get_static_pad (fakesink, "sink");
    gst_pad_link (pad, sinkpad);
    gst_object_unref (sinkpad);
  }

}

GST_START_TEST (videoraw)
{
  GMainLoop *loop = g_main_loop_new (NULL, TRUE);

  pipeline = gst_pipeline_new (NULL);
  GstElement *videotestsrc = gst_element_factory_make ("videotestsrc", NULL);
  GstElement *automuxerbin = gst_element_factory_make ("automuxerbin", NULL);
  GstElement *outputfakesink = gst_element_factory_make ("fakesink", NULL);

  GstBus *bus = gst_pipeline_get_bus (GST_PIPELINE (pipeline));

  gst_bus_add_watch (bus, gst_bus_async_signal_func, NULL);
  g_signal_connect (bus, "message", G_CALLBACK (bus_msg), pipeline);
  g_object_unref (bus);

  g_object_set (G_OBJECT (outputfakesink), "signal-handoffs", TRUE, NULL);
  g_signal_connect (G_OBJECT (outputfakesink), "handoff",
      G_CALLBACK (fakesink_hand_off), loop);

  mark_point ();
  gst_bin_add_many (GST_BIN (pipeline), videotestsrc, automuxerbin,
      outputfakesink, NULL);

  g_signal_connect (automuxerbin, "pad-added", G_CALLBACK (pad_added),
      outputfakesink);

  gst_element_link (videotestsrc, automuxerbin);
  mark_point ();
  gst_element_set_state (pipeline, GST_STATE_PLAYING);
  mark_point ();

  GST_DEBUG_BIN_TO_DOT_FILE_WITH_TS (GST_BIN (pipeline),
      GST_DEBUG_GRAPH_SHOW_ALL, "before_entering_loop_videoraw");
  g_timeout_add (10000, (GSourceFunc) timeout, loop);

  mark_point ();
  g_main_loop_run (loop);
  mark_point ();

  fail_unless (test_timeout == FALSE);

  GST_DEBUG_BIN_TO_DOT_FILE_WITH_TS (GST_BIN (pipeline),
      GST_DEBUG_GRAPH_SHOW_ALL, "create_buffer_test_end_videoraw");

  gst_element_set_state (pipeline, GST_STATE_NULL);
  g_object_unref (pipeline);
}

GST_END_TEST
GST_START_TEST (vp8enc)
{
  GMainLoop *loop = g_main_loop_new (NULL, TRUE);

  pipeline = gst_pipeline_new (NULL);
  GstElement *videotestsrc = gst_element_factory_make ("videotestsrc", NULL);
  GstElement *vp8enc = gst_element_factory_make ("vp8enc", NULL);
  GstElement *automuxerbin = gst_element_factory_make ("automuxerbin", NULL);
  GstElement *outputfakesink = gst_element_factory_make ("fakesink", NULL);

  GstBus *bus = gst_pipeline_get_bus (GST_PIPELINE (pipeline));

  gst_bus_add_watch (bus, gst_bus_async_signal_func, NULL);
  g_signal_connect (bus, "message", G_CALLBACK (bus_msg), pipeline);
  g_object_unref (bus);

  g_object_set (G_OBJECT (outputfakesink), "signal-handoffs", TRUE, NULL);
  g_signal_connect (G_OBJECT (outputfakesink), "handoff",
      G_CALLBACK (fakesink_hand_off), loop);

  mark_point ();
  gst_bin_add_many (GST_BIN (pipeline), videotestsrc, vp8enc, automuxerbin,
      outputfakesink, NULL);

  g_signal_connect (automuxerbin, "pad-added", G_CALLBACK (pad_added),
      outputfakesink);

  gst_element_link_many (videotestsrc, vp8enc, automuxerbin, NULL);
  mark_point ();
  gst_element_set_state (pipeline, GST_STATE_PLAYING);
  mark_point ();

  GST_DEBUG_BIN_TO_DOT_FILE_WITH_TS (GST_BIN (pipeline),
      GST_DEBUG_GRAPH_SHOW_ALL, "before_entering_loop_vp8enc");
  g_timeout_add (10000, (GSourceFunc) timeout, loop);

  mark_point ();
  g_main_loop_run (loop);
  mark_point ();

  fail_unless (test_timeout == FALSE);

  GST_DEBUG_BIN_TO_DOT_FILE_WITH_TS (GST_BIN (pipeline),
      GST_DEBUG_GRAPH_SHOW_ALL, "create_buffer_test_end_vp8enc");

  gst_element_set_state (pipeline, GST_STATE_NULL);
  g_object_unref (pipeline);
}

GST_END_TEST
GST_START_TEST (x264enc)
{
  GMainLoop *loop = g_main_loop_new (NULL, TRUE);

  pipeline = gst_pipeline_new (NULL);
  GstElement *videotestsrc = gst_element_factory_make ("videotestsrc", NULL);
  GstElement *x264enc = gst_element_factory_make ("x264enc", NULL);
  GstElement *automuxerbin = gst_element_factory_make ("automuxerbin", NULL);
  GstElement *outputfakesink = gst_element_factory_make ("fakesink", NULL);

  GstBus *bus = gst_pipeline_get_bus (GST_PIPELINE (pipeline));

  gst_bus_add_watch (bus, gst_bus_async_signal_func, NULL);
  g_signal_connect (bus, "message", G_CALLBACK (bus_msg), pipeline);
  g_object_unref (bus);

  g_object_set (G_OBJECT (outputfakesink), "signal-handoffs", TRUE, NULL);
  g_signal_connect (G_OBJECT (outputfakesink), "handoff",
      G_CALLBACK (fakesink_hand_off), loop);

  mark_point ();
  gst_bin_add_many (GST_BIN (pipeline), videotestsrc, x264enc,
      automuxerbin, outputfakesink, NULL);

  g_signal_connect (automuxerbin, "pad-added", G_CALLBACK (pad_added),
      outputfakesink);

  gst_element_link_many (videotestsrc, x264enc, automuxerbin, NULL);
  mark_point ();
  gst_element_set_state (pipeline, GST_STATE_PLAYING);
  mark_point ();

  GST_DEBUG_BIN_TO_DOT_FILE_WITH_TS (GST_BIN (pipeline),
      GST_DEBUG_GRAPH_SHOW_ALL, "before_entering_loop_x264enc");
  g_timeout_add (10000, (GSourceFunc) timeout, loop);

  mark_point ();
  g_main_loop_run (loop);
  mark_point ();

  fail_unless (test_timeout == FALSE);

  GST_DEBUG_BIN_TO_DOT_FILE_WITH_TS (GST_BIN (pipeline),
      GST_DEBUG_GRAPH_SHOW_ALL, "create_buffer_test_end_x264enc");

  gst_element_set_state (pipeline, GST_STATE_NULL);
  g_object_unref (pipeline);
}

GST_END_TEST
GST_START_TEST (avenc_wmv2)
{
  GMainLoop *loop = g_main_loop_new (NULL, TRUE);

  pipeline = gst_pipeline_new (NULL);
  GstElement *videotestsrc = gst_element_factory_make ("videotestsrc", NULL);
  GstElement *avenc_wmv2 = gst_element_factory_make ("avenc_wmv2", NULL);
  GstElement *automuxerbin = gst_element_factory_make ("automuxerbin", NULL);
  GstElement *outputfakesink = gst_element_factory_make ("fakesink", NULL);

  GstBus *bus = gst_pipeline_get_bus (GST_PIPELINE (pipeline));

  gst_bus_add_watch (bus, gst_bus_async_signal_func, NULL);
  g_signal_connect (bus, "message", G_CALLBACK (bus_msg), pipeline);
  g_object_unref (bus);

  g_object_set (G_OBJECT (outputfakesink), "signal-handoffs", TRUE, NULL);
  g_signal_connect (G_OBJECT (outputfakesink), "handoff",
      G_CALLBACK (fakesink_hand_off), loop);

  mark_point ();
  gst_bin_add_many (GST_BIN (pipeline), videotestsrc, avenc_wmv2, automuxerbin,
      outputfakesink, NULL);

  g_signal_connect (automuxerbin, "pad-added", G_CALLBACK (pad_added),
      outputfakesink);

  gst_element_link_many (videotestsrc, avenc_wmv2, automuxerbin, NULL);
  mark_point ();
  gst_element_set_state (pipeline, GST_STATE_PLAYING);
  mark_point ();

  GST_DEBUG_BIN_TO_DOT_FILE_WITH_TS (GST_BIN (pipeline),
      GST_DEBUG_GRAPH_SHOW_ALL, "before_entering_loop_avenc_wmv2");
  g_timeout_add (10000, (GSourceFunc) timeout, loop);

  mark_point ();
  g_main_loop_run (loop);
  mark_point ();

  fail_unless (test_timeout == FALSE);

  GST_DEBUG_BIN_TO_DOT_FILE_WITH_TS (GST_BIN (pipeline),
      GST_DEBUG_GRAPH_SHOW_ALL, "create_buffer_test_end_avenc_wmv2");

  gst_element_set_state (pipeline, GST_STATE_NULL);
  g_object_unref (pipeline);
}

GST_END_TEST
/*
 * End of test cases
 */
static Suite *
sdp_suite (void)
{
  Suite *s = suite_create ("automuxerbin");
  TCase *tc_chain = tcase_create ("element");

  suite_add_tcase (s, tc_chain);
  tcase_add_test (tc_chain, videoraw);
  tcase_add_test (tc_chain, x264enc);
  tcase_add_test (tc_chain, avenc_wmv2);
  tcase_add_test (tc_chain, vp8enc);

  return s;
}

GST_CHECK_MAIN (sdp);
