/*******************************************************************************

 * Dolby Home Audio GStreamer Plugins
 * Copyright (C) 2020, Dolby Laboratories

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
#include <gst/audio/gstaudiodecoder.h>
#include <gst/audio/audio.h>

#include <stdio.h>

#include "dlbaudiodecoder.h"

GstHarness *harness;
GstHarness *src_harness;

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

typedef struct DlbAc3DecoderParamValues_
{
  gint dmx_mode;
  DlbAudioDecoderOutMode out_ch_config;
  DlbAudioDecoderDrcMode drc_mode;
  gint drc_cut;
  gint drc_boost;
  gboolean drc_suppress;
} DlbAc3DecoderParamValues;

typedef struct DlbAc3DecoderParamNames_
{
  const gchar *out_ch_config;
  const gchar *drc_mode;
  const gchar *drc_cut;
  const gchar *drc_boost;
  const gchar *drc_suppress;
  const gchar *dmx_mode;
} DlbAc3DecoerParamNames;

typedef struct DlbAc3Decoder_
{
  const gchar *name;

  GstAudioFormat output_format;
  DlbAc3DecoerParamNames names;
  DlbAc3DecoderParamValues values;
} DlbAc3Decoder;

static DlbAc3Decoder DEFAULT_SETTINGS = {
  .name = "dlbac3dec",
  .output_format = GST_AUDIO_FORMAT_F32LE,
  .names.out_ch_config = "out-mode",
  .names.drc_mode = "drc-mode",
  .names.drc_cut = "drc-cut",
  .names.drc_boost = "drc-boost",
  .names.drc_suppress = "drc-suppress",
  .names.dmx_mode = "dmx-mode",
  .values.out_ch_config = DLB_AUDIO_DECODER_OUT_MODE_2_0,
  .values.drc_mode = DLB_AUDIO_DECODER_DRC_MODE_AUTO,
  .values.drc_cut = 0,
  .values.drc_boost = 0,
  .values.drc_suppress = FALSE,
  .values.dmx_mode = 0
};

TestFile file_51_1kHz_dd =
    { "51_1kHz_dd.ac3", 48000, 6 * 256, 5, TRUE, FALSE, 10, 1 };
TestFile file_51_1kHz_ddp =
    { "51_1kHz_ddp.ec3", 48000, 6 * 256, 5, TRUE, FALSE, 10, 1 };

static void dlb_ac3dec_test_setup (void);
static void dlb_ac3dec_test_setup_with_static_config (const TestFile * file,
    const DlbAc3Decoder * settings);
static void dlb_ac3dec_test_teardown (void);
static void dlb_ac3dec_test_set_input_file (const gchar * filename);

static const gchar *
output_format_str (const GstAudioFormat format)
{
  const gchar *s_format;

  if (GST_AUDIO_FORMAT_F32LE == format) {
    s_format = "F32LE";
  } else if (GST_AUDIO_FORMAT_S32LE == format) {
    s_format = "S32LE";
  } else if (GST_AUDIO_FORMAT_S16LE == format) {
    s_format = "S16LE";
  } else {
    fail_unless (FALSE, "Unsupported sample format=%d", format);
  }

  return s_format;
}

static void
dlb_ac3dec_test_setup (void)
{
  dlb_ac3dec_test_setup_with_static_config (NULL, &DEFAULT_SETTINGS);
}

static void
dlb_ac3dec_test_setup_with_static_config (const TestFile * file,
    const DlbAc3Decoder * settings)
{
  gchar *harness_pipeline;

  if ((DLB_AUDIO_DECODER_OUT_MODE_RAW == settings->values.out_ch_config)
      && file->atmos) {
    harness_pipeline =
        g_strdup_printf
        ("%s ! capsfilter caps=audio/x-raw(meta:DlbObjectAudioMeta),format=(string)%s,channels=(int)16",
        settings->name, output_format_str (settings->output_format));
  } else {
    harness_pipeline =
        g_strdup_printf
        ("%s ! capsfilter caps=audio/x-raw,format=(string)%s",
        settings->name, output_format_str (settings->output_format));
  }

  harness = gst_harness_new_parse (harness_pipeline);

  src_harness = gst_harness_new_parse ("filesrc ! dlbac3parse");
  gst_harness_add_src_harness (harness, src_harness, FALSE);

  g_free (harness_pipeline);
}

static void
dlb_ac3dec_test_teardown (void)
{
  // harness could have been torn down explicitely at the end of a test
  if (harness) {
    gst_harness_teardown (harness);
    harness = NULL;
    src_harness = NULL;
  }
}

static void
dlb_ac3dec_test_set_input_file (const gchar * filename)
{
  gchar *full_filename = g_build_filename (GST_TEST_FILES_PATH, filename, NULL);
  gst_harness_set (src_harness, "filesrc", "location", full_filename, NULL);
  g_free (full_filename);
}

static void
dlb_ac3dec_test_property_double (const gchar * property_name, gdouble min,
    gdouble max)
{
  DlbAc3Decoder settings = DEFAULT_SETTINGS;
  gdouble value;

  for (gdouble i = min; i <= max; ++i) {
    gst_harness_set (harness, settings.name, property_name, i, NULL);
    gst_harness_get (harness, settings.name, property_name, &value, NULL);
    fail_unless (value == i, "Unable to set double property %s to %f",
        property_name, i);
  }
}

GST_START_TEST (test_dlbac3dec_drain_at_eos)
{
  GstFlowReturn ret;
  TestFile *file = &file_51_1kHz_ddp;

  dlb_ac3dec_test_set_input_file (file->name);

  // push all ten available frames
  ret = gst_harness_src_crank_and_push_many (harness, 0, file->frame_count);
  fail_unless_equals_int (ret, GST_FLOW_OK);

  gst_harness_push_event (harness, gst_event_new_eos ());

  fail_unless_equals_int (gst_harness_buffers_received (harness),
      file->frame_count * 6);

  // teardown has to be run explicitely at the end of a test, because
  // gst_task_cleanup_all() is called before fixture's teardown function
  // and it will wait forever for the harness
  dlb_ac3dec_test_teardown ();
}

GST_END_TEST
GST_START_TEST (test_dlbac3dec_broken_data)
{
  GstFlowReturn ret;
  TestFile *file = &file_51_1kHz_ddp;

  src_harness = gst_harness_new_parse ("filesrc ! ac3parse ! breakmydata");
  gst_harness_set (src_harness, "breakmydata", "probability", 0.0001, NULL);
  gst_harness_add_src_harness (harness, src_harness, FALSE);

  dlb_ac3dec_test_set_input_file (file->name);

  // push whole file through
  ret = gst_harness_src_crank_and_push_many (harness, 0, file->frame_count);
  fail_unless_equals_int (ret, GST_FLOW_ERROR);

  gst_harness_push_event (harness, gst_event_new_eos ());

  // teardown has to be run explicitely at the end of a test, because
  // gst_task_cleanup_all() is called before fixture's teardown function
  // and it will wait forever for the harness
  dlb_ac3dec_test_teardown ();
}

GST_END_TEST
GST_START_TEST (test_dlbac3dec_property_drc_cut)
{
  dlb_ac3dec_test_property_double (DEFAULT_SETTINGS.names.drc_cut, 0.0, 1.0);
}

GST_END_TEST
GST_START_TEST (test_dlbac3dec_property_drc_boost)
{
  dlb_ac3dec_test_property_double (DEFAULT_SETTINGS.names.drc_boost, 0.0, 1.0);
}
GST_END_TEST

static Suite *
dlbac3dec_suite (void)
{
  Suite *s = suite_create ("dlbac3dec");
  TCase *tc_general = tcase_create ("general");
  // set setup and teardown functions for each test in the test case
  tcase_add_checked_fixture (tc_general, dlb_ac3dec_test_setup,
      dlb_ac3dec_test_teardown);
  // add tests to the test case
  tcase_add_test (tc_general, test_dlbac3dec_drain_at_eos);
  tcase_add_test (tc_general, test_dlbac3dec_broken_data);
  tcase_add_test (tc_general, test_dlbac3dec_property_drc_cut);
  tcase_add_test (tc_general, test_dlbac3dec_property_drc_boost);

  // add test case to the suite
  suite_add_tcase (s, tc_general);
  return s;
}

GST_CHECK_MAIN (dlbac3dec)
