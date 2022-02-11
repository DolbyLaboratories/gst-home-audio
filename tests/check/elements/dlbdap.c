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

static GstHarness *harness;

#define HARNESS_PAD_CAPS                                        \
  "audio/x-raw, "                                               \
  "format = (string) %s, "                                      \
  "channels = (int) %d, "                                       \
  "channel-mask = (bitmask) 0x%08x, "                           \
  "rate = (int) %d, "                                           \
  "layout = (string) interleaved"                               \

static void
dlb_dap_test_setup (void)
{
  /* setup harness & source */
  harness = gst_harness_new ("dlbdap");
}

static void
dlb_dap_test_teardown (void)
{
  /* harness could have been torn down explicitely at the end of a test */
  if (harness) {
    gst_harness_teardown (harness);
    harness = NULL;
  }
}

static void
init_buffer (GstBuffer *buf, GstClockTime start_ts, guint64 offset,
    gint samples, gint rate)
{
  gst_buffer_memset (buf, 0, 0, gst_buffer_get_size (buf));

  GST_BUFFER_TIMESTAMP (buf) = start_ts;
  GST_BUFFER_PTS (buf) = start_ts;
  GST_BUFFER_OFFSET (buf) = offset;
  GST_BUFFER_OFFSET_END (buf) = offset + samples;
  GST_BUFFER_DURATION (buf) =
      gst_util_uint64_scale_int (samples, GST_SECOND, rate);
}

GST_START_TEST (test_dap_src_caps_template)
{
  GstCaps * srccaps, *templatecaps;
  GstPad *dapsrcpad;

  dapsrcpad = gst_element_get_static_pad (harness->element, "src");
  fail_unless (dapsrcpad);

  srccaps = gst_pad_query_caps (dapsrcpad, NULL);
  fail_unless (srccaps);

  templatecaps = gst_pad_get_pad_template_caps(dapsrcpad);
  fail_unless (templatecaps);

  gst_check_caps_equal(srccaps, templatecaps);

  gst_caps_unref (srccaps);
  gst_caps_unref (templatecaps);
  gst_object_unref (dapsrcpad);
}
GST_END_TEST

GST_START_TEST (test_dap_sink_caps_template)
{
  GstCaps * sinkcaps, *templatecaps;
  GstPad *dapsinkpad;

  dapsinkpad = gst_element_get_static_pad (harness->element, "sink");
  fail_unless (dapsinkpad);

  sinkcaps = gst_pad_query_caps (dapsinkpad, NULL);
  fail_unless (sinkcaps);

  templatecaps = gst_pad_get_pad_template_caps(dapsinkpad);
  fail_unless (templatecaps);

  gst_check_caps_equal(sinkcaps, templatecaps);

  gst_caps_unref (sinkcaps);
  gst_caps_unref (templatecaps);
  gst_object_unref (dapsinkpad);
}
GST_END_TEST

/*
 * DAP test caps.
 */
typedef struct {
  gint channels;
  guint chmask;
} dap_test_caps;

/*
 * Test tuple for negotiation tests.
 */
typedef struct {
  dap_test_caps entry;
  dap_test_caps expected;
  gboolean virt_enable;
} dap_test_negotiation_tuple;

/*
 * Test cases for negotiation tests.
 */
static dap_test_negotiation_tuple dap_negotiation_test_table [] = {
    {{2,  0x3    }, {2,  0x3    }, FALSE}, // 2.0   -> 2.0  , virt disabled
    {{3,  0xb    }, {6,  0x3f   }, FALSE}, // 2.1   -> 5.1  , virt disabled
    {{3,  0x7    }, {6,  0x3f   }, FALSE}, // 3.0   -> 5.1  , virt disabled
    {{4,  0xf    }, {6,  0x3f   }, FALSE}, // 3.1   -> 5.1  , virt disabled
    {{4,  0x33   }, {6,  0x3f   }, FALSE}, // 4.0   -> 5.1  , virt disabled
    {{5,  0x3b   }, {6,  0x3f   }, FALSE}, // 4.1   -> 5.1  , virt disabled
    {{5,  0x37   }, {6,  0x3f   }, FALSE}, // 5.0   -> 5.1  , virt disabled
    {{6,  0x3f   }, {6,  0x3f   }, FALSE}, // 5.1   -> 5.1  , virt disabled
    {{6,  0xc33  }, {8,  0xc3f  }, FALSE}, // 6.0   -> 7.1  , virt disabled
    {{7,  0xc3b  }, {8,  0xc3f  }, FALSE}, // 6.1   -> 7.1  , virt disabled
    {{7,  0xc37  }, {8,  0xc3f  }, FALSE}, // 7.0   -> 7.1  , virt disabled
    {{8,  0xc3f  }, {8,  0xc3f  }, FALSE}, // 7.1   -> 7.1  , virt disabled
    {{3,  0x80003}, {8,  0xc003f}, FALSE}, // 2.0.1 -> 5.1.2, virt disabled
    {{4,  0xc0003}, {8,  0xc003f}, FALSE}, // 2.0.2 -> 5.1.2, virt disabled
    {{4,  0x8000b}, {8,  0xc003f}, FALSE}, // 2.1.1 -> 5.1.2, virt disabled
    {{5,  0xc000b}, {8,  0xc003f}, FALSE}, // 2.1.2 -> 5.1.2, virt disabled
    {{4,  0x80007}, {8,  0xc003f}, FALSE}, // 3.0.1 -> 5.1.2, virt disabled
    {{5,  0xc0007}, {8,  0xc003f}, FALSE}, // 3.0.2 -> 5.1.2, virt disabled
    {{5,  0xb000f}, {8,  0xc003f}, FALSE}, // 3.1.1 -> 5.1.2, virt disabled
    {{6,  0xc000f}, {8,  0xc003f}, FALSE}, // 3.1.2 -> 5.1.2, virt disabled
    {{5,  0xb0033}, {8,  0xc003f}, FALSE}, // 4.0.1 -> 5.1.2, virt disabled
    {{6,  0xc0033}, {8,  0xc003f}, FALSE}, // 4.0.2 -> 5.1.2, virt disabled
    {{6,  0xb003b}, {8,  0xc003f}, FALSE}, // 4.1.1 -> 5.1.2, virt disabled
    {{7,  0xc003b}, {8,  0xc003f}, FALSE}, // 4.1.2 -> 5.1.2, virt disabled
    {{8,  0x33033}, {10, 0x3303f}, FALSE}, // 4.0.4 -> 5.1.4, virt disabled
    {{8,  0xc3033}, {10, 0x3303f}, FALSE}, // 4.0.4 -> 5.1.4, virt disabled
    {{9,  0x3303b}, {10, 0x3303f}, FALSE}, // 4.1.4 -> 5.1.4, virt disabled
    {{6,  0xb0037}, {8,  0xc003f}, FALSE}, // 5.0.1 -> 5.1.2, virt disabled
    {{7,  0xc0037}, {8,  0xc003f}, FALSE}, // 5.0.2 -> 5.1.2, virt disabled
    {{7,  0xb003f}, {8,  0xc003f}, FALSE}, // 5.1.1 -> 5.1.2, virt disabled
    {{8,  0xc003f}, {8,  0xc003f}, FALSE}, // 5.1.2 -> 5.1.2, virt disabled
    {{9,  0x33037}, {10, 0x3303f}, FALSE}, // 5.0.4 -> 5.1.4, virt disabled
    {{10, 0x3303f}, {10, 0x3303f}, FALSE}, // 5.1.4 -> 5.1.4, virt disabled
    {{8,  0xc0c33}, {10, 0xc0c3f}, FALSE}, // 6.0.2 -> 7.1.2, virt disabled
    {{8,  0xc0c3b}, {10, 0xc0c3f}, FALSE}, // 6.1.2 -> 7.1.2, virt disabled
    {{9,  0xc0c37}, {10, 0xc0c3f}, FALSE}, // 7.0.2 -> 7.1.2, virt disabled
    {{10, 0xc0c3f}, {10, 0xc0c3f}, FALSE}, // 7.1.2 -> 7.1.2, virt disabled
    {{11, 0x33c37}, {12, 0x33c3f}, FALSE}, // 7.0.4 -> 7.1.4, virt disabled
    {{12, 0x33c3f}, {12, 0x33c3f}, FALSE}, // 7.1.4 -> 7.1.4, virt disabled
    {{2,  0x3    }, {8,  0xc003f}, TRUE},  // 2.0   -> 5.1.2, virt enabled
    {{3,  0xb    }, {8,  0xc003f}, TRUE},  // 2.1   -> 5.1.2, virt enabled
    {{3,  0x7    }, {8,  0xc003f}, TRUE},  // 3.0   -> 5.1.2, virt enabled
    {{4,  0xf    }, {8,  0xc003f}, TRUE},  // 3.1   -> 5.1.2, virt enabled
    {{4,  0x33   }, {10, 0x3303f}, TRUE},  // 4.0   -> 5.1.2, virt enabled
    {{5,  0x3b   }, {10, 0x3303f}, TRUE},  // 4.1   -> 5.1.2, virt enabled
    {{5,  0x37   }, {10, 0x3303f}, TRUE},  // 5.0   -> 5.1.2, virt enabled
    {{6,  0x3f   }, {10, 0x3303f}, TRUE},  // 5.1   -> 5.1.2, virt enabled
    {{6,  0xc37  }, {12, 0x33c3f}, TRUE},  // 6.0   -> 7.1.4, virt enabled
    {{7,  0xc3b  }, {12, 0x33c3f}, TRUE},  // 6.1   -> 7.1.4, virt enabled
    {{7,  0xc37  }, {12, 0x33c3f}, TRUE},  // 7.0   -> 7.1.4, virt enabled
    {{8,  0xc3f  }, {12, 0x33c3f}, TRUE},  // 7.1   -> 7.1.4, virt enabled
    {{3,  0xb0003}, {8,  0xc003f}, TRUE},  // 2.0.1 -> 5.1.2, virt enabled
    {{4,  0xc0003}, {8,  0xc003f}, TRUE},  // 2.0.2 -> 5.1.2, virt enabled
    {{4,  0x8000b}, {8,  0xc003f}, TRUE},  // 2.1.1 -> 5.1.2, virt enabled
    {{5,  0xc000b}, {8,  0xc003f}, TRUE},  // 2.1.2 -> 5.1.2, virt enabled
    {{4,  0x80007}, {8,  0xc003f}, TRUE},  // 3.0.1 -> 5.1.2, virt enabled
    {{5,  0xc0007}, {8,  0xc003f}, TRUE},  // 3.0.2 -> 5.1.2, virt enabled
    {{5,  0x8000f}, {8,  0xc003f}, TRUE},  // 3.1.1 -> 5.1.2, virt enabled
    {{6,  0xc000f}, {8,  0xc003f}, TRUE},  // 3.1.2 -> 5.1.2, virt enabled
    {{5,  0x80033}, {8,  0xc003f}, TRUE},  // 4.0.1 -> 5.1.2, virt enabled
    {{6,  0xc0033}, {8,  0xc003f}, TRUE},  // 4.0.2 -> 5.1.2, virt enabled
    {{6,  0x8003b}, {8,  0xc003f}, TRUE},  // 4.1.1 -> 5.1.2, virt enabled
    {{7,  0xc003b}, {8,  0xc003f}, TRUE},  // 4.1.2 -> 5.1.2, virt enabled
    {{8,  0x33033}, {10, 0x3303f}, TRUE},  // 4.0.4 -> 5.1.4, virt enabled
    {{9,  0x3303b}, {10, 0x3303f}, TRUE},  // 4.1.4 -> 5.1.4, virt enabled
    {{6,  0x80037}, {8,  0xc003f}, TRUE},  // 5.0.1 -> 5.1.2, virt enabled
    {{7,  0xc0037}, {8,  0xc003f}, TRUE},  // 5.0.2 -> 5.1.2, virt enabled
    {{7,  0x8003f}, {8,  0xc003f}, TRUE},  // 5.1.1 -> 5.1.2, virt enabled
    {{8,  0xc003f}, {8,  0xc003f}, TRUE},  // 5.1.2 -> 5.1.2, virt enabled
    {{9,  0x33037}, {10, 0x3303f}, TRUE},  // 5.0.4 -> 5.1.4, virt enabled
    {{10, 0x3303f}, {10, 0x3303f}, TRUE},  // 5.1.4 -> 5.1.4, virt enabled
    {{8,  0xc0c33}, {12, 0x33c3f}, TRUE},  // 6.0.2 -> 7.1.4, virt enabled
    {{8,  0xc0c3b}, {12, 0x33c3f}, TRUE},  // 6.1.2 -> 7.1.4, virt enabled
    {{9,  0xc0c37}, {12, 0x33c3f}, TRUE},  // 7.0.2 -> 7.1.4, virt enabled
    {{10, 0xc0c3f}, {12, 0x33c3f}, TRUE},  // 7.1.2 -> 7.1.4, virt enabled
    {{11, 0x33c37}, {12, 0x33c3f}, TRUE},  // 7.0.4 -> 7.1.4, virt enabled
    {{12, 0x33c3f}, {12, 0x33c3f}, TRUE},  // 7.1.4 -> 7.1.4, virt enabled
};
static const gint test_number = G_N_ELEMENTS (dap_negotiation_test_table);

/*
 * Check if for given src caps DAP plugin proposes correct sink caps. Sink caps
 * should have preferred input format in first caps structure.
 */
GST_START_TEST (test_dap_transform_caps)
{
  GstCaps * caps, *harness_sink_caps;
  GstPad *dapsinkpad;
  GstStructure *s;
  gint inchannels;
  guint64 inchmask;
  const gint entry_channels =     dap_negotiation_test_table[__i__].entry.channels;
  const gint expected_channels =  dap_negotiation_test_table[__i__].expected.channels;
  const guint64 entry_chmask =    dap_negotiation_test_table[__i__].entry.chmask;
  const guint64 expected_chmask = dap_negotiation_test_table[__i__].expected.chmask;
  const gboolean virt_enable = dap_negotiation_test_table[__i__].virt_enable;

  GST_INFO ("iteration %d", __i__);

  gst_harness_set (harness, "dlbdap", "virtualizer-enable", virt_enable, NULL);

  harness_sink_caps = gst_caps_new_simple ("audio/x-raw", "channels",
      G_TYPE_INT, entry_channels, "channel-mask", GST_TYPE_BITMASK, entry_chmask,
      NULL);
  gst_harness_set_sink_caps (harness, harness_sink_caps);

  dapsinkpad = gst_element_get_static_pad (harness->element, "sink");
  fail_unless (dapsinkpad);

  caps = gst_pad_query_caps (dapsinkpad, NULL);
  fail_unless (caps);

  s = gst_caps_get_structure (caps, 0);
  fail_unless (s);

  gst_structure_get (s, "channels", G_TYPE_INT, &inchannels,
      "channel-mask", GST_TYPE_BITMASK, &inchmask, NULL);

  fail_unless_equals_int (expected_channels, inchannels);
  fail_unless_equals_int64_hex (expected_chmask, inchmask);

  gst_caps_unref (caps);
  g_object_unref (dapsinkpad);
}
GST_END_TEST

static const gint expected_ieq_gains [] = {157,164,219,218,204,188,192,192,212,214};
static const gint expected_ieq_bands [] = {20,100,200,300,400,500,600,700,800,900};
static const gint expected_geq_gains [] = {0,10,20,30,40,50,60,70,80,90};
static const gint expected_geq_bands [] = {201,1001,2002,3003,4004,5005,6006,7007,8008,9009};

static void
fail_unless_equal_gst_array_int (const GValue *value_array,
    const gint *expected_array)
{
  const guint size = gst_value_array_get_size (value_array);

  for (gint i = 0; i < size; ++i) {
    const GValue *row = gst_value_array_get_value (value_array, i);
    fail_unless_equals_int (g_value_get_int (row), expected_array[i]);
  }
}

static void
fail_unless_equal_gst_array_uint (const GValue *value_array,
    const gint *expected_array)
{
  const guint size = gst_value_array_get_size (value_array);

  for (gint i = 0; i < size; ++i) {
    const GValue *row = gst_value_array_get_value (value_array, i);
    fail_unless_equals_int (g_value_get_uint (row), expected_array[i]);
  }
}

GST_START_TEST (test_dap_json_parsing)
{
  gchar *json_filename = g_build_filename (GST_TEST_FILES_PATH,
      "default.json", NULL);

  gint front_speaker_angle, surr_speaker_angle, rear_surr_speaker_angle,
      height_speaker_angle, rear_height_speaker_angle, bass_enhancer_enable,
      bass_enhancer_boost, bass_enhancer_cutoff, bass_enhancer_width,
      calibration_boost, dialog_enhancer_enable, dialog_enhancer_amount,
      dialog_enhancer_ducking, graphic_equalizer_enable, ieq_enable,
      ieq_amount, mi_dialog_enhancer_steering_enable,
      mi_dv_leveler_steering_enable, mi_ieq_steering_enable,
      mi_surround_compressor_steering_enable, postgain, pregain,
      surround_boost, surround_decoder_center_spreading_enable,
      surround_decoder_enable, system_gain, volmax_boost,
      volume_leveler_enable, volume_leveler_amount, virtualizer_enable;

  GValue ieq_bands_val = G_VALUE_INIT, ieq_gains_val = G_VALUE_INIT, geq_bands_val = G_VALUE_INIT,
      geq_gains_val = G_VALUE_INIT;

  gst_harness_set (harness, "dlbdap", "json-config", json_filename, NULL);
  gst_harness_get (harness, "dlbdap",
    "virtualizer-enable", &virtualizer_enable,
    "virtualizer-front-speaker-angle", &front_speaker_angle,
    "virtualizer-surround-speaker-angle", &surr_speaker_angle,
    "virtualizer-rear-surround-speaker-angle", &rear_surr_speaker_angle,
    "virtualizer-height-speaker-angle", &height_speaker_angle,
    "virtualizer-rear-height-speaker-angle", &rear_height_speaker_angle,
    "bass-enhancer-enable", &bass_enhancer_enable,
    "bass-enhancer-boost", &bass_enhancer_boost,
    "bass-enhancer-cutoff-freq", &bass_enhancer_cutoff,
    "bass-enhancer-width", &bass_enhancer_width,
    "calibration-boost", &calibration_boost,
    "dialog-enhancer-enable", &dialog_enhancer_enable,
    "dialog-enhancer-amount", &dialog_enhancer_amount,
    "dialog-enhancer-ducking", &dialog_enhancer_ducking,
    "geq-enable", &graphic_equalizer_enable,
    "ieq-enable", &ieq_enable,
    "ieq-amount", &ieq_amount,
    "mi-dialog-enhancer-steering-enable", &mi_dialog_enhancer_steering_enable,
    "mi-dv-leveler-steering-enable", &mi_dv_leveler_steering_enable,
    "mi-ieq-steering-enable", &mi_ieq_steering_enable,
    "mi-surround-compressor-steering-enable", &mi_surround_compressor_steering_enable,
    "postgain", &postgain,
    "pregain", &pregain,
    "surround-boost", &surround_boost,
    "surround-decoder-cs-enable", &surround_decoder_center_spreading_enable,
    "surround-decoder-enable", &surround_decoder_enable,
    "sysgain", &system_gain,
    "volmax-boost", &volmax_boost,
    "volume-leveler-enable", &volume_leveler_enable,
    "volume-leveler-amount", &volume_leveler_amount,
    NULL);

  g_free (json_filename);

  fail_unless (virtualizer_enable);
  fail_unless_equals_int (front_speaker_angle,        5);
  fail_unless_equals_int (surr_speaker_angle,        10);
  fail_unless_equals_int (rear_surr_speaker_angle,   15);
  fail_unless_equals_int (height_speaker_angle,      20);
  fail_unless_equals_int (rear_height_speaker_angle, 25);
  fail_unless_equals_int (bass_enhancer_enable,   1);
  fail_unless_equals_int (bass_enhancer_boost,  100);
  fail_unless_equals_int (bass_enhancer_cutoff, 200);
  fail_unless_equals_int (bass_enhancer_width,   16);
  fail_unless_equals_int (calibration_boost, 4);
  fail_unless_equals_int (dialog_enhancer_enable, 1);
  fail_unless_equals_int (dialog_enhancer_amount, 3);
  fail_unless_equals_int (dialog_enhancer_ducking, 6);
  fail_unless_equals_int (graphic_equalizer_enable, 0);
  fail_unless_equals_int (ieq_enable, 1);
  fail_unless_equals_int (ieq_amount, 5);
  fail_unless_equals_int (mi_dialog_enhancer_steering_enable, 0);
  fail_unless_equals_int (mi_dv_leveler_steering_enable, 0);
  fail_unless_equals_int (mi_ieq_steering_enable, 0);
  fail_unless_equals_int (mi_surround_compressor_steering_enable, 1);
  fail_unless_equals_int (postgain, 20);
  fail_unless_equals_int (pregain, 30);
  fail_unless_equals_int (surround_boost, 296);
  fail_unless_equals_int (surround_decoder_center_spreading_enable, 0);
  fail_unless_equals_int (surround_decoder_enable, 1);
  fail_unless_equals_int (system_gain,  496);
  fail_unless_equals_int (volmax_boost, 200);
  fail_unless_equals_int (volume_leveler_enable, 1);
  fail_unless_equals_int (volume_leveler_amount, 3);

  g_object_get_property (G_OBJECT (harness->element), "ieq-freqs", &ieq_bands_val);
  fail_unless_equal_gst_array_uint (&ieq_bands_val, expected_ieq_bands);
  g_value_unset (&ieq_bands_val);

  g_object_get_property (G_OBJECT (harness->element), "ieq-gains", &ieq_gains_val);
  fail_unless_equal_gst_array_int (&ieq_gains_val, expected_ieq_gains);
  g_value_unset (&ieq_gains_val);

  g_object_get_property (G_OBJECT (harness->element), "geq-freqs", &geq_bands_val);
  fail_unless_equal_gst_array_uint (&geq_bands_val, expected_geq_bands);
  g_value_unset (&geq_bands_val);

  g_object_get_property (G_OBJECT (harness->element), "geq-gains", &geq_gains_val);
  fail_unless_equal_gst_array_int (&geq_gains_val, expected_geq_gains);
  g_value_unset (&geq_gains_val);
}
GST_END_TEST

GST_START_TEST (test_dap_serialized_settings)
{
  gchar *json_filename = g_build_filename (GST_TEST_FILES_PATH,
      "default-serialized-config.json", NULL);

  gchar *src_pad_caps_str = g_strdup_printf (
      HARNESS_PAD_CAPS, "S32LE", 2, 0x3, 48000);

  GstCaps *caps;
  GstPad *srcpad;
  GstStructure *s;

  gint channels;
  guint64 channel_mask;

  gst_harness_set (harness, "dlbdap", "json-config", json_filename, NULL);
  gst_harness_set_src_caps_str (harness, src_pad_caps_str);

  srcpad = gst_element_get_static_pad (harness->element, "src");
  fail_unless (srcpad);

  caps = gst_pad_query_caps (srcpad, NULL);
  fail_unless (caps);

  s = gst_caps_get_structure (caps, 0);
  fail_unless (s);

  gst_structure_get (s, "channels", G_TYPE_INT, &channels,
      "channel-mask", GST_TYPE_BITMASK, &channel_mask, NULL);

  fail_unless_equals_int (channels, 3);
  fail_unless_equals_int (channel_mask, 0);

  gst_caps_unref (caps);
  g_object_unref (srcpad);
  g_free (src_pad_caps_str);
  g_free (json_filename);
}
GST_END_TEST

GST_START_TEST (test_dap_message_post)
{
  GstElement *dap;
  GstBus *bus;
  GstMessage *message;

  gint channels = 0, virtualizer_enable = 0;
  guint64 channel_mask = 0;

  const GstStructure *s;

  gchar *json_filename = g_build_filename (GST_TEST_FILES_PATH,
      "default-serialized-config.json", NULL);

  dap = gst_harness_find_element (harness, "dlbdap");
  fail_unless (dap != NULL);

  bus = gst_bus_new ();
  gst_element_set_bus (dap, bus);

  g_object_set (dap, "json-config", json_filename, NULL);

  message = gst_bus_pop_filtered (bus, GST_MESSAGE_ELEMENT);
  fail_unless (message != NULL);
  fail_unless (GST_MESSAGE_SRC (message) == GST_OBJECT (dap));
  fail_unless (GST_MESSAGE_TYPE (message) == GST_MESSAGE_ELEMENT);

  s = gst_message_get_structure (message);
  fail_if (s == NULL);

  fail_unless_equals_string ((char *) gst_structure_get_name (s),
      "dap-serialized-info");

  gst_structure_get (s, "output-channels", G_TYPE_INT, &channels,
      "processing-format", G_TYPE_UINT64, &channel_mask,
      "virtualizer-enable", G_TYPE_BOOLEAN, &virtualizer_enable, NULL);

  fail_unless (virtualizer_enable);
  fail_unless_equals_int (channels, 3);
  fail_unless_equals_int64_hex (channel_mask, 0xc003f);

  gst_message_unref (message);
  gst_bus_set_flushing (bus, TRUE);
  gst_object_unref (bus);
  gst_object_unref (dap);
  g_free (json_filename);
}
GST_END_TEST

GST_START_TEST (test_dlb_dap_timestamps_latency_off)
{
  gint channels = 2, bps = 4;
  gint samples = 1000;
  gint64 delta_ts, delta_pts;
  GstClockTime duration, start_ts;
  GstBuffer *inbuf, *outbuf = NULL;

  gchar *sink_pad_caps_str = g_strdup_printf (
      HARNESS_PAD_CAPS, "S32LE", 8, 0xc003f, 48000);
  gchar *src_pad_caps_str = g_strdup_printf (
      HARNESS_PAD_CAPS, "S32LE", 2, 0x3, 48000);

  gst_harness_set (harness, "dlbdap", "discard-latency", FALSE, NULL);
  gst_harness_set_sink_caps_str (harness, sink_pad_caps_str);
  gst_harness_set_src_caps_str (harness, src_pad_caps_str);

  start_ts = gst_util_uint64_scale_int (48000, GST_SECOND, 48000);
  duration = gst_util_uint64_scale_int (samples, GST_SECOND, 48000);

  inbuf = gst_harness_create_buffer (harness, samples * channels * bps);
  init_buffer (inbuf, start_ts, 0, samples, 48000);

  outbuf = gst_harness_push_and_pull (harness, inbuf);

  /* 1000 samples = 3 full processing blocks, this gives us 768 processed,
   * 232 remains in the adapter */
  fail_unless_equals_clocktime (GST_BUFFER_TIMESTAMP (outbuf), start_ts);
  fail_unless_equals_clocktime (GST_BUFFER_PTS (outbuf), start_ts);
  fail_unless_equals_uint64 (GST_BUFFER_OFFSET (outbuf), 0);
  fail_unless_equals_uint64 (GST_BUFFER_OFFSET_END (outbuf), 768);
  fail_unless_equals_clocktime (GST_BUFFER_DURATION (outbuf),
      gst_util_uint64_scale_int (768, GST_SECOND, 48000));

  gst_buffer_unref (outbuf);
  inbuf = gst_harness_create_buffer (harness, samples * channels * bps);
  init_buffer (inbuf, start_ts + duration, samples, samples, 48000);

  outbuf = gst_harness_push_and_pull (harness, inbuf);

  /* another 1000 samples + 232 cached, gives us 4 full processing blocks */
  delta_ts = ABS (GST_CLOCK_DIFF (GST_BUFFER_TIMESTAMP (outbuf),
      start_ts + gst_util_uint64_scale_int (768, GST_SECOND, 48000)));
  delta_pts = ABS (GST_CLOCK_DIFF (GST_BUFFER_PTS (outbuf),
      start_ts + gst_util_uint64_scale_int (768, GST_SECOND, 48000)));

  /* fail if delta is more than one sample */
  fail_unless (delta_ts < gst_util_uint64_scale_int (1, GST_SECOND, 48000));
  fail_unless (delta_pts < gst_util_uint64_scale_int (1, GST_SECOND, 48000));

  fail_unless_equals_uint64 (GST_BUFFER_OFFSET (outbuf), 768);
  fail_unless_equals_uint64 (GST_BUFFER_OFFSET_END (outbuf), 1792);
  fail_unless_equals_clocktime (GST_BUFFER_DURATION (outbuf),
      gst_util_uint64_scale_int (1024, GST_SECOND, 48000));

  g_free (sink_pad_caps_str);
  g_free (src_pad_caps_str);
  gst_buffer_unref (outbuf);
}

GST_END_TEST
GST_START_TEST (test_dlb_dap_timestamps_latency_on)
{
  gint channels = 2, bps = 4;
  gint samples = 1000;
  gint64 delta_ts, delta_pts;
  GstClockTime duration, start_ts;
  GstBuffer *inbuf, *outbuf = NULL;

  gchar *sink_pad_caps_str = g_strdup_printf (
      HARNESS_PAD_CAPS, "S32LE", 8, 0xc003f, 48000);
  gchar *src_pad_caps_str = g_strdup_printf (
      HARNESS_PAD_CAPS, "S32LE", 2, 0x3, 48000);

  gst_harness_set (harness, "dlbdap", "discard-latency", TRUE, NULL);
  gst_harness_set_sink_caps_str (harness, sink_pad_caps_str);
  gst_harness_set_src_caps_str (harness, src_pad_caps_str);

  start_ts = gst_util_uint64_scale_int (48000, GST_SECOND, 48000);
  duration = gst_util_uint64_scale_int (samples, GST_SECOND, 48000);

  inbuf = gst_harness_create_buffer (harness, samples * channels * bps);
  init_buffer (inbuf, start_ts, 0, samples, 48000);
  outbuf = gst_harness_push_and_pull (harness, inbuf);

  /* 1000 samples = 3 full processing blocks of which 2 will be dropped,
   * this gives us 512 dropped, 256 processed, 232 remains in the adapter */
  fail_unless_equals_clocktime (GST_BUFFER_TIMESTAMP (outbuf), start_ts);
  fail_unless_equals_clocktime (GST_BUFFER_PTS (outbuf), start_ts);
  fail_unless_equals_uint64 (GST_BUFFER_OFFSET (outbuf), 0);
  fail_unless_equals_uint64 (GST_BUFFER_OFFSET_END (outbuf), 256);
  fail_unless_equals_clocktime (GST_BUFFER_DURATION (outbuf),
      gst_util_uint64_scale_int (256, GST_SECOND, 48000));

  gst_buffer_unref (outbuf);
  inbuf = gst_harness_create_buffer (harness, samples * channels * bps);
  init_buffer (inbuf, start_ts + duration, samples, samples, 48000);

  outbuf = gst_harness_push_and_pull (harness, inbuf);

  /* another 1000 samples + 232 cached, gives us 4 full processing blocks */
  delta_ts = ABS (GST_CLOCK_DIFF (GST_BUFFER_TIMESTAMP (outbuf),
      start_ts + gst_util_uint64_scale_int (256, GST_SECOND, 48000)));
  delta_pts = ABS (GST_CLOCK_DIFF (GST_BUFFER_PTS (outbuf),
      start_ts + gst_util_uint64_scale_int (256, GST_SECOND, 48000)));

  /* fail if delta is more than one sample */
  fail_unless (delta_ts < gst_util_uint64_scale_int (1, GST_SECOND, 48000));
  fail_unless (delta_pts < gst_util_uint64_scale_int (1, GST_SECOND, 48000));

  fail_unless_equals_uint64 (GST_BUFFER_OFFSET (outbuf), 256);
  fail_unless_equals_uint64 (GST_BUFFER_OFFSET_END (outbuf), 1280);
  fail_unless_equals_clocktime (GST_BUFFER_DURATION (outbuf),
      gst_util_uint64_scale_int (1024, GST_SECOND, 48000));

  gst_buffer_unref (outbuf);
  g_free (sink_pad_caps_str);
  g_free (src_pad_caps_str);
}
GST_END_TEST

GST_START_TEST (test_dlb_dap_drain_on_flush_event)
{
  gint samples = 1000;
  GstBuffer *inbuf, *outbuf = NULL;

  gchar *sink_pad_caps_str = g_strdup_printf (
      HARNESS_PAD_CAPS, "F32LE", 8, 0xc003f, 48000);
  gchar *src_pad_caps_str = g_strdup_printf (
      HARNESS_PAD_CAPS, "F32LE", 6, 0x3f, 48000);

  gst_harness_set (harness, "dlbdap", "discard-latency", TRUE, NULL);
  gst_harness_set_sink_caps_str (harness, sink_pad_caps_str);
  gst_harness_set_src_caps_str (harness, src_pad_caps_str);

  inbuf = gst_harness_create_buffer (harness, samples * 6 * 4);
  init_buffer (inbuf, 0, 0, samples, 48000);

  outbuf = gst_harness_push_and_pull (harness, inbuf);

  /* 3 full blocks x channels x sample size */
  fail_unless_equals_int (gst_buffer_get_size (outbuf), (3*256 - 512) * 8 * 4);
  gst_buffer_unref (outbuf);

  gst_harness_push_event (harness, gst_event_new_flush_stop (TRUE));
  outbuf = gst_harness_pull (harness);

  /* (adapter + latency) x channels x sample size */
  fail_unless_equals_int (gst_buffer_get_size (outbuf), (232 + 512) * 8 * 4);

  gst_buffer_unref (outbuf);
  g_free (sink_pad_caps_str);
  g_free (src_pad_caps_str);
}
GST_END_TEST

GST_START_TEST (test_dlb_dap_drain_on_eos_event)
{
  gint samples = 1555;
  GstBuffer *inbuf, *outbuf = NULL;

  gchar *sink_pad_caps_str = g_strdup_printf (
      HARNESS_PAD_CAPS, "F32LE", 2, 0x3, 48000);
  gchar *src_pad_caps_str = g_strdup_printf (
      HARNESS_PAD_CAPS, "F32LE", 6, 0x3f, 48000);

  gst_harness_set (harness, "dlbdap", "discard-latency", TRUE, NULL);
  gst_harness_set_sink_caps_str (harness, sink_pad_caps_str);
  gst_harness_set_src_caps_str (harness, src_pad_caps_str);

  inbuf = gst_harness_create_buffer (harness, samples * 6 * 4);
  init_buffer (inbuf, 0, 0, samples, 48000);

  outbuf = gst_harness_push_and_pull (harness, inbuf);

  /* 6 full blocks minus latency x channels x sample size */
  fail_unless_equals_int (gst_buffer_get_size (outbuf), (6*256 - 512) * 2 * 4);
  gst_buffer_unref (outbuf);

  gst_harness_push_event (harness, gst_event_new_eos ());
  outbuf = gst_harness_pull (harness);

  /* (adapter + latency) x channels x sample size */
  fail_unless_equals_int (gst_buffer_get_size (outbuf), (19 + 512) * 2 * 4);

  gst_buffer_unref (outbuf);
  g_free (sink_pad_caps_str);
  g_free (src_pad_caps_str);
}
GST_END_TEST

GST_START_TEST (test_dlb_dap_drain_adapter_only)
{
  gint samples = 12;
  GstBuffer *inbuf, *outbuf = NULL;

  gchar *sink_pad_caps_str = g_strdup_printf (
      HARNESS_PAD_CAPS, "F32LE", 6, 0x3f, 48000);
  gchar *src_pad_caps_str = g_strdup_printf (
      HARNESS_PAD_CAPS, "F32LE", 6, 0x3f, 48000);

  gst_harness_set (harness, "dlbdap", "discard-latency", FALSE, NULL);
  gst_harness_set_sink_caps_str (harness, sink_pad_caps_str);
  gst_harness_set_src_caps_str (harness, src_pad_caps_str);

  inbuf = gst_harness_create_buffer (harness, samples * 6 * 4);
  init_buffer (inbuf, 0, 0, samples, 48000);

  gst_harness_push (harness, inbuf);
  gst_harness_push_event (harness, gst_event_new_eos ());
  outbuf = gst_harness_pull (harness);

  /* adapter x channels x sample size */
  fail_unless_equals_int (gst_buffer_get_size (outbuf), 12 * 6 * 4);

  gst_buffer_unref (outbuf);
  g_free (sink_pad_caps_str);
  g_free (src_pad_caps_str);
}
GST_END_TEST

static Suite *
dlbdap_suite (void)
{
  Suite *s = suite_create ("dlbdap");
  TCase *tc_general = tcase_create ("general");

  /* set setup and teardown functions for each test in the test case */
  tcase_add_checked_fixture (tc_general, dlb_dap_test_setup,
      dlb_dap_test_teardown);

  /* add tests to the test case */
  tcase_add_test (tc_general, test_dap_src_caps_template);
  tcase_add_test (tc_general, test_dap_sink_caps_template);
  tcase_add_loop_test (tc_general, test_dap_transform_caps, 0, test_number);
  tcase_add_test (tc_general, test_dap_json_parsing);
  tcase_add_test (tc_general, test_dap_serialized_settings);
  tcase_add_test (tc_general, test_dap_message_post);
  tcase_add_test (tc_general, test_dlb_dap_timestamps_latency_off);
  tcase_add_test (tc_general, test_dlb_dap_timestamps_latency_on);
  tcase_add_test (tc_general, test_dlb_dap_drain_on_flush_event);
  tcase_add_test (tc_general, test_dlb_dap_drain_on_eos_event);
  tcase_add_test (tc_general, test_dlb_dap_drain_adapter_only);

  /* add test case to the suite */
  suite_add_tcase (s, tc_general);

  return s;
}

GST_CHECK_MAIN (dlbdap)
