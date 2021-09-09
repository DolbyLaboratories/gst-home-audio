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

#include "dlbaudiometa.h"

static GstHarness *harness;

#define HARNESS_SRC_PAD_CAPS                                    \
  "audio/x-raw(" DLB_CAPS_FEATURE_META_OBJECT_AUDIO_META "), "  \
  "format = (string) %s, "                                      \
  "channels = (int) %d, "                                       \
  "channel-mask = (bitmask) 0x0, "                              \
  "rate = (int) %d, "                                           \
  "layout = (string) interleaved, "                             \
  "processing-block-size = (int) %d"                            \

#define HARNESS_SINK_PAD_CAPS                                   \
  "audio/x-raw, "                                               \
  "format = (string) %s, "                                      \
  "channels = (int) %d, "                                       \
  "channel-mask = (bitmask) 0x%08x, "                           \
  "rate = (int) %d, "                                           \
  "layout = (string) interleaved"                               \

static void
dlb_oar_test_setup (void)
{
  harness = gst_harness_new ("dlboar");
}

static void
dlb_oar_test_teardown (void)
{
  /* harness could have been torn down explicitely at the end of a test */
  if (harness) {
    gst_harness_teardown (harness);
    harness = NULL;
  }
}

static void
init_buffer_ts (GstBuffer *buf, GstClockTime start_ts, guint64 offset,
    gint samples, gint rate)
{
  GST_BUFFER_TIMESTAMP (buf) = start_ts;
  GST_BUFFER_PTS (buf) = start_ts;
  GST_BUFFER_OFFSET (buf) = offset;
  GST_BUFFER_OFFSET_END (buf) = offset + samples;
  GST_BUFFER_DURATION (buf) =
      gst_util_uint64_scale_int (samples, GST_SECOND, rate);
}

GST_START_TEST (test_dlb_oar_timestamps_latency_off)
{
  gint channels = 32, bps = 4;
  gint samples = 1000;
  gint64 delta_ts, delta_pts;
  GstClockTime duration, start_ts;
  GstBuffer *inbuf, *outbuf = NULL;

  gchar *sink_pad_caps_str = g_strdup_printf (
      HARNESS_SINK_PAD_CAPS, "S32LE", 8, 0xc003f, 48000);
  gchar *src_pad_caps_str = g_strdup_printf (
      HARNESS_SRC_PAD_CAPS, "S32LE", 32, 48000, 32);

  gst_harness_set_sink_caps_str (harness, sink_pad_caps_str);
  gst_harness_set_src_caps_str (harness, src_pad_caps_str);

  start_ts = gst_util_uint64_scale_int (48000, GST_SECOND, 48000);
  duration = gst_util_uint64_scale_int (samples, GST_SECOND, 48000);

  inbuf = gst_harness_create_buffer (harness, samples * channels * bps);
  init_buffer_ts (inbuf, start_ts, 0, samples, 48000);

  outbuf = gst_harness_push_and_pull (harness, inbuf);

  /* from 1000 samples OAR can process 31 x 32 sample blocks = 992 */
  fail_unless_equals_clocktime (GST_BUFFER_TIMESTAMP (outbuf), start_ts);
  fail_unless_equals_clocktime (GST_BUFFER_PTS (outbuf), start_ts);
  fail_unless_equals_uint64 (GST_BUFFER_OFFSET (outbuf), 0);
  fail_unless_equals_uint64 (GST_BUFFER_OFFSET_END (outbuf), 992);
  fail_unless_equals_clocktime (GST_BUFFER_DURATION (outbuf),
      gst_util_uint64_scale_int (992, GST_SECOND, 48000));

  gst_buffer_unref (outbuf);
  inbuf = gst_harness_create_buffer (harness, samples * channels * bps);
  init_buffer_ts (inbuf, start_ts + duration, samples, samples, 48000);

  outbuf = gst_harness_push_and_pull (harness, inbuf);

  /* another 1000 samples + 8 cached, gives us 31 full processing blocks */
  delta_ts = ABS (GST_CLOCK_DIFF (GST_BUFFER_TIMESTAMP (outbuf),
      start_ts + gst_util_uint64_scale_int (992, GST_SECOND, 48000)));
  delta_pts = ABS (GST_CLOCK_DIFF (GST_BUFFER_PTS (outbuf),
      start_ts + gst_util_uint64_scale_int (992, GST_SECOND, 48000)));

  /* fail if delta is more than one sample */
  fail_unless (delta_ts < gst_util_uint64_scale_int (1, GST_SECOND, 48000));
  fail_unless (delta_pts < gst_util_uint64_scale_int (1, GST_SECOND, 48000));

  fail_unless_equals_uint64 (GST_BUFFER_OFFSET (outbuf), 992);
  fail_unless_equals_uint64 (GST_BUFFER_OFFSET_END (outbuf), 1984);
  fail_unless_equals_clocktime (GST_BUFFER_DURATION (outbuf),
      gst_util_uint64_scale_int (992, GST_SECOND, 48000));

  gst_buffer_unref (outbuf);
  g_free (sink_pad_caps_str);
  g_free (src_pad_caps_str);
}

GST_END_TEST
GST_START_TEST (test_dlb_oar_timestamps_latency_on)
{
  gint channels = 32, bps = 4;
  gint samples = 3000;
  gint64 delta_ts, delta_pts;
  GstClockTime duration, start_ts;
  GstBuffer *inbuf, *outbuf = NULL;

  gchar *sink_pad_caps_str = g_strdup_printf (
      HARNESS_SINK_PAD_CAPS, "S32LE", 8, 0xc003f, 48000);
  gchar *src_pad_caps_str = g_strdup_printf (
      HARNESS_SRC_PAD_CAPS, "S32LE", 32, 48000, 32);

  gst_harness_set (harness, "dlboar", "discard-latency", TRUE, NULL);
  gst_harness_set_sink_caps_str (harness, sink_pad_caps_str);
  gst_harness_set_src_caps_str (harness, src_pad_caps_str);

  start_ts = gst_util_uint64_scale_int (48000, GST_SECOND, 48000);
  duration = gst_util_uint64_scale_int (samples, GST_SECOND, 48000);

  inbuf = gst_harness_create_buffer (harness, samples * channels * bps);
  init_buffer_ts (inbuf, start_ts, 0, samples, 48000);

  outbuf = gst_harness_push_and_pull (harness, inbuf);

  /* from 3000 samples OAR can process 93 x 32 sample blocks = 2976,
   * minus latency = 2944 */
  fail_unless_equals_clocktime (GST_BUFFER_TIMESTAMP (outbuf), start_ts);
  fail_unless_equals_clocktime (GST_BUFFER_PTS (outbuf), start_ts);
  fail_unless_equals_uint64 (GST_BUFFER_OFFSET (outbuf), 0);
  fail_unless_equals_uint64 (GST_BUFFER_OFFSET_END (outbuf), 2944);
  fail_unless_equals_clocktime (GST_BUFFER_DURATION (outbuf),
      gst_util_uint64_scale_int (2944, GST_SECOND, 48000));

  gst_buffer_unref (outbuf);
  inbuf = gst_harness_create_buffer (harness, samples * channels * bps);
  init_buffer_ts (inbuf, start_ts + duration, samples, samples, 48000);

  outbuf = gst_harness_push_and_pull (harness, inbuf);

  /* another 3000 samples + 24 cached, gives us 94 full processing blocks */
  delta_ts = ABS (GST_CLOCK_DIFF (GST_BUFFER_TIMESTAMP (outbuf),
      start_ts + gst_util_uint64_scale_int (2944, GST_SECOND, 48000)));
  delta_pts = ABS (GST_CLOCK_DIFF (GST_BUFFER_PTS (outbuf),
      start_ts + gst_util_uint64_scale_int (2944, GST_SECOND, 48000)));

  /* fail if delta is more than one sample */
  fail_unless (delta_ts < gst_util_uint64_scale_int (1, GST_SECOND, 48000));
  fail_unless (delta_pts < gst_util_uint64_scale_int (1, GST_SECOND, 48000));

  fail_unless_equals_uint64 (GST_BUFFER_OFFSET (outbuf), 2944);
  fail_unless_equals_uint64 (GST_BUFFER_OFFSET_END (outbuf), 5952);
  fail_unless_equals_clocktime (GST_BUFFER_DURATION (outbuf),
      gst_util_uint64_scale_int (3008, GST_SECOND, 48000));

  gst_buffer_unref (outbuf);
  g_free (sink_pad_caps_str);
  g_free (src_pad_caps_str);
}
GST_END_TEST

GST_START_TEST (test_dlb_oar_drain_on_flush_event)
{
  gint samples = 100;
  GstBuffer *inbuf, *outbuf = NULL;

  gchar *sink_pad_caps_str = g_strdup_printf (
      HARNESS_SINK_PAD_CAPS, "F32LE", 8, 0xc003f, 48000);
  gchar *src_pad_caps_str = g_strdup_printf (
      HARNESS_SRC_PAD_CAPS, "F32LE", 16, 48000, 32);

  gst_harness_set (harness, "dlboar", "discard-latency", TRUE, NULL);
  gst_harness_set_sink_caps_str (harness, sink_pad_caps_str);
  gst_harness_set_src_caps_str (harness, src_pad_caps_str);

  inbuf = gst_harness_create_buffer (harness, samples * 16 * 4);
  init_buffer_ts (inbuf, 0, 0, samples, 48000);

  outbuf = gst_harness_push_and_pull (harness, inbuf);

  /* 3 full blocks minus latency x channels x sample size */
  fail_unless_equals_int (gst_buffer_get_size (outbuf), (96 - 32) * 8 * 4);
  gst_buffer_unref (outbuf);

  gst_harness_push_event (harness, gst_event_new_flush_stop (TRUE));
  outbuf = gst_harness_pull (harness);

  /* (adapter + latency) x channels x sample size */
  fail_unless_equals_int (gst_buffer_get_size (outbuf), (4 + 32) * 8 * 4);

  gst_buffer_unref (outbuf);
  g_free (sink_pad_caps_str);
  g_free (src_pad_caps_str);
}
GST_END_TEST

GST_START_TEST (test_dlb_oar_drain_on_eos_event)
{
  gint samples = 555;
  GstBuffer *inbuf, *outbuf = NULL;

  gchar *sink_pad_caps_str = g_strdup_printf (
      HARNESS_SINK_PAD_CAPS, "F32LE", 2, 0x3, 48000);
  gchar *src_pad_caps_str = g_strdup_printf (
      HARNESS_SRC_PAD_CAPS, "F32LE", 16, 48000, 32);

  gst_harness_set (harness, "dlboar", "discard-latency", TRUE, NULL);
  gst_harness_set_sink_caps_str (harness, sink_pad_caps_str);
  gst_harness_set_src_caps_str (harness, src_pad_caps_str);

  inbuf = gst_harness_create_buffer (harness, samples * 16 * 4);
  init_buffer_ts (inbuf, 0, 0, samples, 48000);

  outbuf = gst_harness_push_and_pull (harness, inbuf);

  /* 17 full blocks minus latency x channels x sample size */
  fail_unless_equals_int (gst_buffer_get_size (outbuf), (17*32 - 32) * 2 * 4);
  gst_buffer_unref (outbuf);

  gst_harness_push_event (harness, gst_event_new_eos ());
  outbuf = gst_harness_pull (harness);

  /* (adapter + latency) x channels x sample size */
  fail_unless_equals_int (gst_buffer_get_size (outbuf), (11 + 32) * 2 * 4);

  gst_buffer_unref (outbuf);
  g_free (sink_pad_caps_str);
  g_free (src_pad_caps_str);
}
GST_END_TEST

GST_START_TEST (test_dlb_oar_drain_adapter_only)
{
  gint samples = 16;
  GstBuffer *inbuf, *outbuf = NULL;

  gchar *sink_pad_caps_str = g_strdup_printf (
      HARNESS_SINK_PAD_CAPS, "F32LE", 6, 0x3f, 48000);
  gchar *src_pad_caps_str = g_strdup_printf (
      HARNESS_SRC_PAD_CAPS, "F32LE", 16, 48000, 32);

  gst_harness_set (harness, "dlboar", "discard-latency", FALSE, NULL);
  gst_harness_set_sink_caps_str (harness, sink_pad_caps_str);
  gst_harness_set_src_caps_str (harness, src_pad_caps_str);

  inbuf = gst_harness_create_buffer (harness, samples * 16 * 4);
  init_buffer_ts (inbuf, 0, 0, samples, 48000);

  gst_harness_push (harness, inbuf);
  gst_harness_push_event (harness, gst_event_new_eos ());
  outbuf = gst_harness_pull (harness);

  /* adapter x channels x sample size */
  fail_unless_equals_int (gst_buffer_get_size (outbuf), 16 * 6 * 4);

  gst_buffer_unref (outbuf);
  g_free (sink_pad_caps_str);
  g_free (src_pad_caps_str);
}
GST_END_TEST
static Suite *
dlboar_suite (void)
{
  Suite *s = suite_create ("dlboar");
  TCase *tc_general = tcase_create ("general");

  /* set setup and teardown functions for each test in the test case */
  tcase_add_checked_fixture (tc_general, dlb_oar_test_setup,
      dlb_oar_test_teardown);

  /* add tests to the test case */
  tcase_add_test (tc_general, test_dlb_oar_timestamps_latency_off);
  tcase_add_test (tc_general, test_dlb_oar_timestamps_latency_on);
  tcase_add_test (tc_general, test_dlb_oar_drain_on_flush_event);
  tcase_add_test (tc_general, test_dlb_oar_drain_on_eos_event);
  tcase_add_test (tc_general, test_dlb_oar_drain_adapter_only);

  /* add test case to the suite */
  suite_add_tcase (s, tc_general);

  return s;
}

GST_CHECK_MAIN (dlboar)
