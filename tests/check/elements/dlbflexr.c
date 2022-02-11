/*******************************************************************************

 * Dolby Home Audio GStreamer Plugins
 * Copyright (C) 2021-2022, Dolby Laboratories

 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.

 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.

 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 ******************************************************************************/

#include <gst/check/gstcheck.h>
#include <gst/check/gstconsistencychecker.h>

static GMainLoop *main_loop;


static void
test_setup (void)
{
  main_loop = g_main_loop_new (NULL, FALSE);
}

static void
test_teardown (void)
{
  g_main_loop_unref (main_loop);
  main_loop = NULL;
}

static void
on_msg (GstBus * bus, GstMessage * message, GstPipeline * bin)
{
  switch (message->type) {
    case GST_MESSAGE_EOS:
      g_main_loop_quit (main_loop);
      break;
    default:
      g_assert_not_reached ();
      break;
  }
}

GST_START_TEST (test_dlb_flexr_data_consistency)
{
  GstElement *pipe, *src1, *src2, *flexr, *sink;
  GstBus *bus;
  gboolean ret;
  GstPad *srcpad, *sinkpad, *flexrpad1, *flexrpad2;
  GstStreamConsistency *chk_1, *chk_2, *chk_3;
  GstStateChangeReturn state_ret;

  gchar *device_conf = g_build_filename (GST_FGEN_FILES_PATH,
      "stereo.dconf", NULL);
  gchar *stream_conf = g_build_filename (GST_FGEN_FILES_PATH,
      "stereo.conf", NULL);

  /* build pipeline */
  pipe = gst_pipeline_new ("pipeline");
  bus = gst_element_get_bus (pipe);
  gst_bus_add_signal_watch_full (bus, G_PRIORITY_HIGH);

  src1 = gst_element_factory_make ("audiotestsrc", "src1");
  g_object_set (src1, "num-buffers", 10, "samplesperbuffer", 256, NULL);

  src2 = gst_element_factory_make ("audiotestsrc", "src2");
  g_object_set (src2, "num-buffers", 12, "samplesperbuffer", 256, NULL);

  flexr = gst_element_factory_make ("dlbflexr", "dlbflexr");
  g_object_set (flexr, "device-config", device_conf, NULL);

  sink = gst_element_factory_make ("fakesink", "sink");
  gst_bin_add_many (GST_BIN (pipe), src1, src2, flexr, sink, NULL);

  ret = gst_element_link (src1, flexr);
  fail_unless (ret == TRUE, "Failed to link src1 to dlbflexr");
  ret = gst_element_link (src2, flexr);
  fail_unless (ret == TRUE, "Failed to link src2 to dlbflexr");
  ret = gst_element_link (flexr, sink);
  fail_unless (ret == TRUE, "Failed to link dlbflexr to sink");

  flexrpad1 = gst_element_get_static_pad (flexr, "sink_0");
  g_object_set (flexrpad1, "stream-config", stream_conf, NULL);

  flexrpad2 = gst_element_get_static_pad (flexr, "sink_1");
  g_object_set (flexrpad2, "stream-config", stream_conf, NULL);

  srcpad = gst_element_get_static_pad (flexr, "src");
  chk_3 = gst_consistency_checker_new (srcpad);
  gst_object_unref (srcpad);

  /* create consistency checkers for the pads */
  srcpad = gst_element_get_static_pad (src1, "src");
  chk_1 = gst_consistency_checker_new (srcpad);
  sinkpad = gst_pad_get_peer (srcpad);
  gst_consistency_checker_add_pad (chk_3, sinkpad);
  gst_object_unref (sinkpad);
  gst_object_unref (srcpad);

  srcpad = gst_element_get_static_pad (src2, "src");
  chk_2 = gst_consistency_checker_new (srcpad);
  sinkpad = gst_pad_get_peer (srcpad);
  gst_consistency_checker_add_pad (chk_3, sinkpad);
  gst_object_unref (sinkpad);
  gst_object_unref (srcpad);

  g_signal_connect (bus, "message::eos", (GCallback) on_msg, pipe);

  state_ret = gst_element_set_state (pipe, GST_STATE_PAUSED);
  fail_unless (state_ret != GST_STATE_CHANGE_FAILURE);

  state_ret = gst_element_get_state (pipe, NULL, NULL, GST_CLOCK_TIME_NONE);
  fail_unless (state_ret != GST_STATE_CHANGE_FAILURE);

  state_ret = gst_element_set_state (pipe, GST_STATE_PLAYING);
  fail_unless (state_ret != GST_STATE_CHANGE_FAILURE);

  g_main_loop_run (main_loop);

  state_ret = gst_element_set_state (pipe, GST_STATE_NULL);
  fail_unless (state_ret != GST_STATE_CHANGE_FAILURE);

  gst_consistency_checker_free (chk_1);
  gst_consistency_checker_free (chk_2);
  gst_consistency_checker_free (chk_3);
  gst_object_unref (flexrpad1);
  gst_object_unref (flexrpad2);
  gst_object_unref (pipe);
  g_free (device_conf);
  g_free (stream_conf);
}

GST_END_TEST static Suite *
dlbflexr_suite (void)
{
  gchar *device_conf = g_build_filename (GST_FGEN_FILES_PATH,
      "stereo.dconf", NULL);
  gchar *stream_conf = g_build_filename (GST_FGEN_FILES_PATH,
      "stereo.conf", NULL);

  Suite *s = suite_create ("dlbflexr");

  /* add tests to the test case */
  if (g_file_test (device_conf, G_FILE_TEST_EXISTS)
      && g_file_test (stream_conf, G_FILE_TEST_EXISTS)) {
    TCase *tc_general = tcase_create ("general");

    tcase_add_test (tc_general, test_dlb_flexr_data_consistency);
    tcase_add_checked_fixture (tc_general, test_setup, test_teardown);

    /* add test case to the suite */
    suite_add_tcase (s, tc_general);
  }

  g_free (device_conf);
  g_free (stream_conf);

  return s;
}

GST_CHECK_MAIN (dlbflexr)
