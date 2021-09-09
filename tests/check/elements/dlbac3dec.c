/*******************************************************************************

 * Dolby Home Audio GStreamer Plugins
 * Copyright (C) 2020-2021, Dolby Laboratories

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
#include <gst/check/gstharness.h>

#include "dlbaudiodecoder.h"

typedef struct TestFile_
{
  const gchar *name;
  gint samplerate;
  gint samples_per_frame;
  gint channels;
  gboolean lfe_present;
  gboolean atmos;
  gint frame_count;
  gint frames_per_timeslice;
} TestFile;

TestFile file_51_1kHz_dd =
    { "51_1kHz_dd.ac3", 48000, 6 * 256, 5, TRUE, FALSE, 10, 1 };
TestFile file_51_1kHz_ddp =
    { "51_1kHz_ddp.ec3", 48000, 6 * 256, 5, TRUE, FALSE, 10, 1 };


GST_START_TEST (test_dlbac3dec_drain_at_eos)
{
  GstFlowReturn ret;
  TestFile *file = &file_51_1kHz_ddp;

  GstHarness *h = gst_harness_new_parse ("dlbac3dec");
  GstHarness *hs = gst_harness_new_parse ("filesrc ! dlbac3parse");

  gchar *filename = g_build_filename (GST_TEST_FILES_PATH, file->name, NULL);
  gint n;

  gst_harness_add_src_harness (h, hs, TRUE);
  gst_harness_set (hs, "filesrc", "location", filename, NULL);
  g_free (filename);

  ret = gst_harness_src_crank_and_push_many (h, 0, file->frame_count);
  fail_unless_equals_int (ret, GST_FLOW_OK);

  gst_harness_push_event (h, gst_event_new_eos ());
  n = gst_harness_buffers_received (h);

  fail_unless_equals_int (n, file->frame_count * 6);
  gst_harness_teardown (h);
}

GST_END_TEST
GST_START_TEST (test_dlbac3dec_broken_data_ok)
{
  GstFlowReturn ret;
  TestFile *file = &file_51_1kHz_ddp;

  GstHarness *h = gst_harness_new ("dlbac3dec");
  GstHarness *hs =
      gst_harness_new_parse ("filesrc ! dlbac3parse ! breakmydata");
  gchar *filename = g_build_filename (GST_TEST_FILES_PATH, file->name, NULL);

  gst_harness_add_src_harness (h, hs, TRUE);
  /* gst_harness_set (h, "dlbac3dec", "max-errors", -1, NULL); */
  gst_harness_set (hs, "filesrc", "location", filename, NULL);
  gst_harness_set (hs, "breakmydata", "probability", 0.01, NULL);
  g_free (filename);

  ret = gst_harness_src_crank_and_push_many (h, 0, file->frame_count);
  fail_unless_equals_int (ret, GST_FLOW_OK);

  gst_harness_teardown (h);
}

GST_END_TEST
GST_START_TEST (test_dlbac3dec_property_drc_cut)
{
  GstHarness *h = gst_harness_new_parse ("fakesrc ! ac3parse ! dlbac3dec");
  gdouble val;

  /* init value 1.0 */
  gst_harness_get (h, "dlbac3dec", "drc-cut", &val, NULL);
  fail_unless_equals_float (val, 1.0);

  gst_harness_set (h, "dlbac3dec", "drc-cut", 0.1, NULL);
  gst_harness_get (h, "dlbac3dec", "drc-cut", &val, NULL);

  fail_unless_equals_float (val, 0.1);

  gst_harness_set (h, "dlbac3dec", "drc-cut", 0.5, NULL);
  gst_harness_get (h, "dlbac3dec", "drc-cut", &val, NULL);

  fail_unless_equals_float (val, 0.5);

  gst_harness_set (h, "dlbac3dec", "drc-cut", 1.0, NULL);
  gst_harness_get (h, "dlbac3dec", "drc-cut", &val, NULL);

  fail_unless_equals_float (val, 1.0);

  gst_harness_teardown (h);
}

GST_END_TEST
GST_START_TEST (test_dlbac3dec_property_drc_boost)
{
  GstHarness *h = gst_harness_new_parse ("fakesrc ! ac3parse ! dlbac3dec");
  gdouble val;

  /* init value 1.0 */
  gst_harness_get (h, "dlbac3dec", "drc-boost", &val, NULL);
  fail_unless_equals_float (val, 1.0);

  gst_harness_set (h, "dlbac3dec", "drc-boost", 0.1, NULL);
  gst_harness_get (h, "dlbac3dec", "drc-boost", &val, NULL);

  fail_unless_equals_float (val, 0.1);

  gst_harness_set (h, "dlbac3dec", "drc-boost", 0.5, NULL);
  gst_harness_get (h, "dlbac3dec", "drc-boost", &val, NULL);

  fail_unless_equals_float (val, 0.5);

  gst_harness_set (h, "dlbac3dec", "drc-boost", 1.0, NULL);
  gst_harness_get (h, "dlbac3dec", "drc-boost", &val, NULL);

  fail_unless_equals_float (val, 1.0);

  gst_harness_teardown (h);
}

GST_END_TEST static Suite *
dlbac3dec_suite (void)
{
  Suite *s = suite_create ("dlbac3dec");
  TCase *tc_general = tcase_create ("general");

  tcase_add_test (tc_general, test_dlbac3dec_drain_at_eos);
  tcase_add_test (tc_general, test_dlbac3dec_broken_data_ok);
  tcase_add_test (tc_general, test_dlbac3dec_property_drc_cut);
  tcase_add_test (tc_general, test_dlbac3dec_property_drc_boost);

  suite_add_tcase (s, tc_general);
  return s;
}

GST_CHECK_MAIN (dlbac3dec)
