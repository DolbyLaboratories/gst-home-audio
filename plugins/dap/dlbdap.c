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

/* Suppress warnings for GValueAraray */
#define GLIB_DISABLE_DEPRECATION_WARNINGS

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <string.h>
#include <gst/gst.h>

#include "dlbdap.h"
#include "dlbutils.h"

GST_DEBUG_CATEGORY_STATIC (dlb_dap_debug_category);
#define GST_CAT_DEFAULT dlb_dap_debug_category


#define DLB_DAP_SINK_CAPS                                               \
    "audio/x-raw, "                                                     \
    "format = (string) {"GST_AUDIO_NE (F32)", "GST_AUDIO_NE (F64)",     \
                        "GST_AUDIO_NE (S16)", "GST_AUDIO_NE (S32)" }, " \
    "channels = (int) { 2, 6, 8, 10, 12 }, "                            \
    "rate = (int) { 48000, 32000, 44100, 96000, 192000 }, "             \
    "layout = (string) { interleaved, non-interleaved }"

#define DLB_DAP_SRC_CAPS                                                \
    "audio/x-raw, "                                                     \
    "format = (string) {"GST_AUDIO_NE (F32)", "GST_AUDIO_NE (F64)",     \
                        "GST_AUDIO_NE (S16)", "GST_AUDIO_NE (S32)" }, " \
    "channels = (int) [2, 32], "                                        \
    "rate = (int) { 48000, 32000, 44100, 96000, 192000 }, "             \
    "layout = (string) { interleaved, non-interleaved }"

#define CHMASK(ch) (GST_AUDIO_CHANNEL_POSITION_MASK (ch))

static const guint64 allowed_input_channel_masks[] = {
  DLB_CHANNEL_MASK_2_0, DLB_CHANNEL_MASK_5_1, DLB_CHANNEL_MASK_5_1_2,
  DLB_CHANNEL_MASK_7_1, DLB_CHANNEL_MASK_5_1_4, DLB_CHANNEL_MASK_7_1_2,
  DLB_CHANNEL_MASK_7_1_4,
};

/* prototypes */
static void dlb_dap_set_property (GObject * object,
    guint property_id, const GValue * value, GParamSpec * pspec);
static void dlb_dap_get_property (GObject * object,
    guint property_id, GValue * value, GParamSpec * pspec);
static void dlb_dap_finalize (GObject * object);

static GstCaps *dlb_dap_transform_caps (GstBaseTransform * trans,
    GstPadDirection direction, GstCaps * caps, GstCaps * filter);
static gboolean dlb_dap_set_caps (GstBaseTransform * trans,
    GstCaps * incaps, GstCaps * outcaps);
static gboolean dlb_dap_query (GstBaseTransform * trans,
    GstPadDirection direction, GstQuery * query);
static gboolean dlb_dap_transform_size (GstBaseTransform * trans,
    GstPadDirection direction, GstCaps * caps, gsize size, GstCaps * othercaps,
    gsize * othersize);
static gboolean dlb_dap_get_unit_size (GstBaseTransform * trans,
    GstCaps * caps, gsize * size);
static void dlb_dap_close (DlbDap * dap);
static gboolean dlb_dap_open (DlbDap * dap);
static gboolean dlb_dap_start (GstBaseTransform * trans);
static gboolean dlb_dap_stop (GstBaseTransform * trans);
static gboolean dlb_dap_sink_event (GstBaseTransform * trans, GstEvent * event);
static gboolean dlb_dap_src_event (GstBaseTransform * trans, GstEvent * event);
static GstFlowReturn dlb_dap_push_drain (DlbDap * dap);
static GstFlowReturn dlb_dap_transform (GstBaseTransform * trans,
    GstBuffer * inbuf, GstBuffer * outbuf);

enum
{
  PROP_0,
  PROP_VIRTUALIZER_ENABLE,
  PROP_VIRT_FRONT_SPEAKER_ANGLE,
  PROP_VIRT_SURROUND_SPEAKER_ANGLE,
  PROP_VIRT_REAR_SURROUND_SPEAKER_ANGLE,
  PROP_VIRT_HEIGHT_SPEAKER_ANGLE,
  PROP_VIRT_REAR_HEIGHT_SPEAKER_ANGLE,
  PROP_HEIGHT_FILTER_ENABLE,
  PROP_BASS_ENHANCER_ENABLE,
  PROP_BASS_ENHANCER_BOOST,
  PROP_BASS_ENHANCER_CUTOFF_FREQ,
  PROP_BASS_ENHANCER_WIDTH,
  PROP_CALIBRATION_BOOST,
  PROP_DIALOG_ENHANCER_ENABLE,
  PROP_DIALOG_ENHANCER_AMOUNT,
  PROP_DIALOG_ENHANCER_DUCKING,
  PROP_GEQ_ENABLE,
  PROP_GEQ_FREQS,
  PROP_GEQ_GAINS,
  PROP_IEQ_ENABLE,
  PROP_IEQ_AMOUNT,
  PROP_IEQ_FREQS,
  PROP_IEQ_GAINS,
  PROP_MI_DIALOG_ENHANCER_STEERING_ENABLE,
  PROP_MI_DV_LEVELER_STEERING_ENABLE,
  PROP_MI_IEQ_STEERING_ENABLE,
  PROP_MI_SURROUND_COMPRESSOR_STEERING_ENABLE,
  PROP_PREGAIN,
  PROP_POSTGAIN,
  PROP_SYSGAIN,
  PROP_SURROUND_DECODER_ENABLE,
  PROP_SURROUND_DECODER_CENTER_SPREAD_ENABLE,
  PROP_SURROUND_BOOST,
  PROP_VOLMAX_BOOST,
  PROP_VOLUME_LEVELER_ENABLE,
  PROP_VOLUME_LEVELER_AMOUNT,
  PROP_DISCARD_LATENCY,
  PROP_FORCE_ORDER,
  PROP_JSON_CONFIG,
};

/* pad templates */

static GstStaticPadTemplate dlb_dap_src_template =
GST_STATIC_PAD_TEMPLATE ("src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS (DLB_DAP_SRC_CAPS));

static GstStaticPadTemplate dlb_dap_sink_template =
GST_STATIC_PAD_TEMPLATE ("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS (DLB_DAP_SINK_CAPS));


G_DEFINE_TYPE_WITH_CODE (DlbDap, dlb_dap, GST_TYPE_BASE_TRANSFORM,
    GST_DEBUG_CATEGORY_INIT (dlb_dap_debug_category, "dlbdap", 0,
        "debug category for dap element"));

typedef unsigned int uint;

#define G_TYPE_int G_TYPE_INT
#define G_TYPE_uint G_TYPE_UINT

#define MAKE_ARRAY_HELPERS(type)                                               \
static void                                                                    \
fill_gst_value_array_##type (GValue *array, type *data, guint size)            \
{                                                                              \
  GValue val = G_VALUE_INIT;                                                   \
  g_value_init (&val, G_TYPE_##type);                                          \
  for (gint i = 0; i < size; ++i) {                                            \
    g_value_set_##type (&val, data[i]);                                        \
    gst_value_array_append_value (array, &val);                                \
  }                                                                            \
  g_value_unset (&val);                                                        \
}                                                                              \
                                                                               \
static void                                                                    \
fill_data_array_##type (type *data, guint *size, const GValue *value_array)    \
{                                                                              \
  *size = gst_value_array_get_size (value_array);                              \
                                                                               \
  for (gint i = 0; i < *size; ++i) {                                           \
    const GValue *val = gst_value_array_get_value (value_array, i);            \
    data[i] = g_value_get_##type (val);                                        \
  }                                                                            \
}

MAKE_ARRAY_HELPERS (int)
MAKE_ARRAY_HELPERS (uint)
#undef G_TYPE_int
#undef G_TYPE_uint
/* check if format is valid */
     static gboolean is_format_valid (dlb_dap_channel_format * format)
{
  return (format->channels_floor > 0 && format->channels_floor <= 7
      && format->channels_top <= 4);
}

/* get number of channels from channel mask */
static gint
get_channels (guint64 channel_mask)
{
  gint channels = 0;

  while (channel_mask) {
    channels += channel_mask & G_GUINT64_CONSTANT (1);
    channel_mask >>= 1;
  }

  return channels;
}

static void
channel_mask_to_dap_format (guint64 channel_mask,
    dlb_dap_channel_format * format)
{
  gint floor = 0, top = 0, lfe = 0;

  if (channel_mask & CHMASK (FRONT_LEFT))
    floor++;
  if (channel_mask & CHMASK (FRONT_RIGHT))
    floor++;
  if (channel_mask & CHMASK (FRONT_CENTER))
    floor++;
  if (channel_mask & CHMASK (LFE1))
    lfe++;
  if (channel_mask & CHMASK (REAR_LEFT))
    floor++;
  if (channel_mask & CHMASK (REAR_RIGHT))
    floor++;
  if (channel_mask & CHMASK (SIDE_LEFT))
    floor++;
  if (channel_mask & CHMASK (SIDE_RIGHT))
    floor++;
  if (channel_mask & CHMASK (TOP_SIDE_LEFT))
    top++;
  if (channel_mask & CHMASK (TOP_SIDE_RIGHT))
    top++;
  if (channel_mask & CHMASK (TOP_FRONT_LEFT))
    top++;
  if (channel_mask & CHMASK (TOP_FRONT_RIGHT))
    top++;
  if (channel_mask & CHMASK (TOP_REAR_LEFT))
    top++;
  if (channel_mask & CHMASK (TOP_REAR_RIGHT))
    top++;
  if (channel_mask & CHMASK (TOP_CENTER))
    top++;

  format->channels_floor = floor;
  format->channels_top = top;
  format->lfe = lfe;
}

static void
dap_format_to_channel_mask (const dlb_dap_channel_format * format,
    guint64 * channel_mask)
{
  guint64 mask = DLB_CHANNEL_MASK_2_0;

  if (format->channels_floor == 3)
    mask |= CHMASK (FRONT_CENTER);
  else if (format->channels_floor == 4)
    mask |= (DLB_CHANNEL_MASK_5_1 & ~CHMASK (FRONT_CENTER));
  else if (format->channels_floor == 5)
    mask |= DLB_CHANNEL_MASK_5_1;
  else if (format->channels_floor == 6)
    mask |= (DLB_CHANNEL_MASK_7_1 & ~CHMASK (FRONT_CENTER));
  else if (format->channels_floor == 7)
    mask |= DLB_CHANNEL_MASK_7_1;

  if (format->channels_top == 1)
    mask |= CHMASK (TOP_CENTER);
  else if (format->channels_top == 2)
    mask |= (CHMASK (TOP_SIDE_LEFT) | CHMASK (TOP_SIDE_RIGHT));
  else if (format->channels_top == 4)
    mask |= (CHMASK (TOP_FRONT_LEFT) | CHMASK (TOP_FRONT_RIGHT) |
        CHMASK (TOP_REAR_LEFT) | CHMASK (TOP_REAR_RIGHT));

  mask = (format->lfe) ? mask | CHMASK (LFE1) : mask & ~CHMASK (LFE1);
  *channel_mask = mask;
}

/* class initialization */
static void
dlb_dap_class_init (DlbDapClass * klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  GstBaseTransformClass *base_transform_class =
      GST_BASE_TRANSFORM_CLASS (klass);

  /* Setting up pads and setting metadata should be moved to
     base_class_init if you intend to subclass this class. */
  gst_element_class_add_pad_template (GST_ELEMENT_CLASS (klass),
      gst_static_pad_template_get (&dlb_dap_src_template));
  gst_element_class_add_pad_template (GST_ELEMENT_CLASS (klass),
      gst_static_pad_template_get (&dlb_dap_sink_template));

  gst_element_class_set_static_metadata (GST_ELEMENT_CLASS (klass),
      "Dolby Audio Processing plugin", "Filter/Effect/Audio",
      "This is a swiss army knife gstreamer plugin for audio processing",
      "Dolby Support <support@dolby.com>");

  gobject_class->set_property = dlb_dap_set_property;
  gobject_class->get_property = dlb_dap_get_property;
  gobject_class->finalize = dlb_dap_finalize;
  base_transform_class->transform_caps =
      GST_DEBUG_FUNCPTR (dlb_dap_transform_caps);
  base_transform_class->set_caps = GST_DEBUG_FUNCPTR (dlb_dap_set_caps);
  base_transform_class->query = GST_DEBUG_FUNCPTR (dlb_dap_query);
  base_transform_class->transform_size =
      GST_DEBUG_FUNCPTR (dlb_dap_transform_size);
  base_transform_class->get_unit_size =
      GST_DEBUG_FUNCPTR (dlb_dap_get_unit_size);
  base_transform_class->start = GST_DEBUG_FUNCPTR (dlb_dap_start);
  base_transform_class->stop = GST_DEBUG_FUNCPTR (dlb_dap_stop);
  base_transform_class->sink_event = GST_DEBUG_FUNCPTR (dlb_dap_sink_event);
  base_transform_class->src_event = GST_DEBUG_FUNCPTR (dlb_dap_src_event);
  base_transform_class->transform = GST_DEBUG_FUNCPTR (dlb_dap_transform);

  g_object_class_install_property (gobject_class,
      PROP_VIRTUALIZER_ENABLE,
      g_param_spec_boolean ("virtualizer-enable", "Virtualizer enable",
          "Enables or disables the speaker virtualizer", FALSE,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_VIRT_FRONT_SPEAKER_ANGLE,
      g_param_spec_int ("virtualizer-front-speaker-angle",
          "Speaker virtualizer front angle",
          "The absolute horizontal angle of front loudspeakers from the "
          "central listening position",
          0, 30, 0, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class,
      PROP_VIRT_SURROUND_SPEAKER_ANGLE,
      g_param_spec_int ("virtualizer-surround-speaker-angle",
          "Speaker virtualizer surround angle",
          "The absolute horizontal angle of surround loudspeakers from the "
          "central listening position",
          0, 30, 0, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class,
      PROP_VIRT_REAR_SURROUND_SPEAKER_ANGLE,
      g_param_spec_int ("virtualizer-rear-surround-speaker-angle",
          "Speaker virtualizer rear surround angle",
          "The absolute horizontal angle of surround loudspeakers from the "
          "central listening position",
          0, 30, 0, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class,
      PROP_VIRT_HEIGHT_SPEAKER_ANGLE,
      g_param_spec_int ("virtualizer-height-speaker-angle",
          "Speaker virtualizer height angle",
          "The absolute horizontal angle of height loudspeakers from the "
          "central listening position",
          0, 30, 0, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class,
      PROP_VIRT_REAR_HEIGHT_SPEAKER_ANGLE,
      g_param_spec_int ("virtualizer-rear-height-speaker-angle",
          "Speaker virtualizer rear height angle",
          "The absolute horizontal angle of height loudspeakers from the "
          "central listening position",
          0, 30, 0, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_HEIGHT_FILTER_ENABLE,
      g_param_spec_boolean ("height-filter-enable", "Height filter enable",
          "Enable the perceptual height filter", FALSE,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_BASS_ENHANCER_ENABLE,
      g_param_spec_boolean ("bass-enhancer-enable", "Bass enhancer enable",
          "Enable bass enhancer.", FALSE,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_BASS_ENHANCER_BOOST,
      g_param_spec_int ("bass-enhancer-boost",
          "Bass enhancer boost",
          "The amount of bass enhancement boost applied by Bass Enhancer "
          "represented as a fixed point number with 4 fractional bits "
          "[0.0, 24.0] dB",
          0, 384, 192, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class,
      PROP_BASS_ENHANCER_CUTOFF_FREQ,
      g_param_spec_int ("bass-enhancer-cutoff-freq",
          "Bass enhancer cutoff frequency",
          "Bass enhancement cutoff frequency used by Bass Enhancer",
          20, 2000, 200, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_BASS_ENHANCER_WIDTH,
      g_param_spec_int ("bass-enhancer-width", "Bass enhancer width",
          "The width of the bass enhancement boost curve used by Bass Enhancer "
          "represented as a fixed point number with 4 fractional bits "
          "[0.125, 4.0] Octaves",
          2, 64, 16, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_CALIBRATION_BOOST,
      g_param_spec_int ("calibration-boost", "Calibration boost",
          "Calibration Boost is an extra gain which is applied to the signal "
          "represented as a fixed point number with 4 fractional bits "
          "[0.0, 12.0] dB",
          0, 192, 0, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_DIALOG_ENHANCER_ENABLE,
      g_param_spec_boolean ("dialog-enhancer-enable", "Dialog enhancer enable",
          "Enable dialog enhancer.", FALSE,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_DIALOG_ENHANCER_AMOUNT,
      g_param_spec_int ("dialog-enhancer-amount", "Dialog enhancer amount",
          "The strength of the Dialog Enhancer effect represented as a fixed "
          "point number with 4 fractional bits [0.0, 1.0]",
          0, 16, 0, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_DIALOG_ENHANCER_DUCKING,
      g_param_spec_int ("dialog-enhancer-ducking",
          "Dialog enhancer ducking",
          "The degree of suppression of channels that don't contain dialog, "
          "represented as a fixed point number with 4 fractional bits "
          "[0.00, 1.00]",
          0, 16, 0, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_GEQ_ENABLE,
      g_param_spec_boolean ("geq-enable", "GEQ enable",
          "Enable graphic equalizer.", FALSE,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_GEQ_FREQS,
      gst_param_spec_array ("geq-freqs",
          "Graphic EQ band center frequencies",
          "An array of values which contain center frequencies at which the "
          "gain values should be applied.",
          g_param_spec_uint ("band-freq-element",
              "Center frequencies array element",
              "Element of the center frequencies array.",
              20, 20000, 20,
              G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS),
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_GEQ_GAINS,
      gst_param_spec_array ("geq-gains", "Graphic EQ band gains",
          "An array of values which contain the gains for each of the "
          "processing channels, represented as a fixed point numbers with "
          "4 fractional bits [-36.0, 36.0]",
          g_param_spec_int ("band-gain-element",
              "Gains array element", "Element of the gains array.",
              -576, 576, 0,
              G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS),
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_IEQ_ENABLE,
      g_param_spec_boolean ("ieq-enable", "IEQ enable",
          "Enable inelligent equalizer.", FALSE,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_IEQ_AMOUNT,
      g_param_spec_int ("ieq-amount", "Intelligent equalizer amount",
          "Specifies the strength of the Intelligent Equalizer effect to apply, "
          "represented as a fixed point number with 4 fractional bits "
          "[0.00, 1.00]",
          0, 16, 10, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_IEQ_FREQS,
      gst_param_spec_array ("ieq-freqs",
          "Intelligent EQ band center frequencies",
          "An array of values which contain center frequencies at which the "
          "gain values should be applied.",
          g_param_spec_uint ("band-freq-element",
              "Center frequencies array element",
              "Element of the center frequencies array.",
              20, 20000, 20,
              G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS),
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_IEQ_GAINS,
      gst_param_spec_array ("ieq-gains",
          "Intelligent EQ band gains",
          "An array of values which contain the gains for each of the "
          "processing channels.",
          g_param_spec_int ("band-gain-element", "Gains array element",
              "Element of the gains array represented as a fixed point number "
              "with 4 fractional bits [-30.0, 30.00] dB",
              -480, 480, 0,
              G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS),
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class,
      PROP_MI_IEQ_STEERING_ENABLE,
      g_param_spec_boolean ("mi-ieq-steering-enable",
          "MI intelligent equalizer steering enable",
          "If enabled, the parameters in Intelligent Equalizer will be updated "
          "based on the information from Media Intelligence.", FALSE,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class,
      PROP_MI_DV_LEVELER_STEERING_ENABLE,
      g_param_spec_boolean ("mi-dv-leveler-steering-enable",
          "MI volume leveler steering enable",
          "If enabled, the parameters in the Volume Leveler will be updated "
          "based on the information from Media Intelligence.", FALSE,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class,
      PROP_MI_DIALOG_ENHANCER_STEERING_ENABLE,
      g_param_spec_boolean ("mi-dialog-enhancer-steering-enable",
          "MI dialog enhancer steering enable",
          "If enabled, the parameters in the Dialog Enhancer will be updated "
          "based on the information from Media Intelligence.", FALSE,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class,
      PROP_MI_SURROUND_COMPRESSOR_STEERING_ENABLE,
      g_param_spec_boolean ("mi-surround-compressor-steering-enable",
          "MI surround compressor steering enable",
          "If enabled, the parameters in the Surround Compressor will be updated "
          "based on the information from Media Intelligence.", FALSE,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_PREGAIN,
      g_param_spec_int ("pregain", "Pregain",
          "Pre-gain specifies the amount of gain which has been applied to the "
          "signal before entering the signal chain, represented as a fixed "
          "point number with 4 fractional bits [-130.0, 30.0] dB",
          -2080, 480, 0, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_POSTGAIN,
      g_param_spec_int ("postgain", "Postgain",
          "Post-gain specifies the amount of gain which will be applied to the "
          "signal externally after leaving the signal chain, represented as a "
          "fixed point number with 4 fractional bits [-130.0, 30.0] dB",
          -2080, 480, 0, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_SYSGAIN,
      g_param_spec_int ("sysgain", "System gain",
          "System gain specifies the amount of gain which be applied by the "
          "signal chain represented as a fixed point number with 4 fractional "
          "bits [-130.0, 30.0] dB",
          -2080, 480, 0, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class,
      PROP_SURROUND_DECODER_ENABLE,
      g_param_spec_boolean ("surround-decoder-enable",
          "Surround decoder enable",
          "Enable surround decoder.", TRUE,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class,
      PROP_SURROUND_DECODER_CENTER_SPREAD_ENABLE,
      g_param_spec_boolean ("surround-decoder-cs-enable",
          "Surround decoder center spread enable",
          "Enable surround decoder center spreading.", TRUE,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_SURROUND_BOOST,
      g_param_spec_int ("surround-boost", "Surround boost",
          "Surround Compressor boost to be used. This boost is applied only to "
          "signals passing through the Speaker Virtualizer, represented as a fixed "
          "point number with 4 fractional bits [0.00, 6.00] dB",
          0, 96, 0, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_VOLMAX_BOOST,
      g_param_spec_int ("volmax-boost",
          "Volume maximizer boost",
          "The boost gain applied to the signal in the signal chain. Volume "
          "maximization will be performed only if Volume Leveler is enabled, "
          "this is represented as a fixed point number with 4 fractional bits "
          "[0.0, 12.0] dB, default 9.0 dB",
          0, 192, 144, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class,
      PROP_VOLUME_LEVELER_ENABLE,
      g_param_spec_boolean ("volume-leveler-enable",
          "Volume leveler enable",
          "Volume leveler enable or disable.", FALSE,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_VOLUME_LEVELER_AMOUNT,
      g_param_spec_int ("volume-leveler-amount", "Volume leveler amount",
          "Specifies how aggressive the leveler is in attempting to reach the "
          "output target level",
          0, 10, 7, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class,
      PROP_DISCARD_LATENCY,
      g_param_spec_boolean ("discard-latency",
          "Discard latency",
          "Discard initial latency zeros from the output", FALSE,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class,
      PROP_FORCE_ORDER,
      g_param_spec_boolean ("force-order",
          "Force order",
          "Force Dolby specific channel order at the output", FALSE,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_JSON_CONFIG,
      g_param_spec_string ("json-config",
          "Json config path",
          "Path to json configuration file.",
          NULL, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  gst_tag_register ("surround-decoder-enable", GST_TAG_FLAG_META,
      G_TYPE_BOOLEAN, "surround-decoder-enable tag",
      "a tag that indicates if surround-decoder is enabled", NULL);
}

static void
dlb_dap_init (DlbDap * dap)
{
  GstBaseTransform *trans = GST_BASE_TRANSFORM (dap);
  gst_base_transform_set_prefer_passthrough (trans, FALSE);
  gst_base_transform_set_gap_aware (trans, TRUE);

  g_mutex_init (&dap->lock);

  gst_audio_info_init (&dap->ininfo);
  gst_audio_info_init (&dap->outinfo);

  dlb_dap_virtualizer_settings_init (&dap->virt_conf);
  dlb_dap_profile_settings_init (&dap->profile);

  dap->adapter = gst_adapter_new ();
  dap->transform_blocks = 0;
  dap->inbufsz = 0;
  dap->outbufsz = 0;
  dap->latency = 0;
  dap->prefill = 0;
  dap->latency_samples = 0;
  dap->latency_time = GST_CLOCK_TIME_NONE;
  dap->discard_latency = FALSE;
  dap->force_order = FALSE;

  dap->serialized_config = NULL;
  dap->json_config_path = NULL;
  dap->global_conf.profile = NULL;
}

static void
dlb_dap_update_state_unlocked (DlbDap * dap)
{
  if (dap->dap_instance) {
    dlb_dap_set_profile_settings (dap->dap_instance, &dap->profile);
    dlb_dap_set_gain_settings (dap->dap_instance, &dap->gains);

    if (!dap->serialized_config ||
        dap->global_conf.override_virtualizer_settings)
      dlb_dap_set_virtualizer_settings (dap->dap_instance, &dap->virt_conf);
  }
}

static void
dlb_dap_update_state (DlbDap * dap)
{
  g_mutex_lock (&dap->lock);
  dlb_dap_update_state_unlocked (dap);
  g_mutex_unlock (&dap->lock);
}

static void
dlb_dap_post_serialized_info_message (DlbDap * dap, gint channels,
    guint64 mask, gboolean virtualizer_enable)
{
  GstStructure *s;

  GST_DEBUG_OBJECT (dap, "Posting serialized config info");

  s = gst_structure_new ("dap-serialized-info",
      "output-channels", G_TYPE_INT, channels,
      "processing-format", G_TYPE_UINT64, mask,
      "virtualizer-enable", G_TYPE_BOOLEAN, virtualizer_enable, NULL);

  gst_element_post_message (GST_ELEMENT_CAST (dap),
      gst_message_new_element (GST_OBJECT_CAST (dap), s));
}

static void
dlb_dap_post_stream_info_message (DlbDap * dap, gchar * audio_codec,
    gboolean object_audio, gboolean surround_decoder_enable,
    GstClockTime timestamp)
{
  GstStructure *stream_info = gst_structure_new ("stream-info", "audio-codec",
      G_TYPE_STRING, audio_codec, "object-audio", G_TYPE_BOOLEAN, object_audio,
      "surround-decoder-enable",
      G_TYPE_BOOLEAN, surround_decoder_enable, NULL);
  GstMessage *msg = gst_message_new_application (GST_OBJECT_CAST (dap),
      stream_info);

  GST_MESSAGE_TIMESTAMP (msg) = timestamp;

  gst_element_post_message (GST_ELEMENT_CAST (dap), msg);
}

static gboolean
dlb_dap_get_serialized_config_from_json (DlbDap * dap, int rate,
    gboolean virtualizer_enable, gboolean force)
{
  GError *error = NULL;

  GST_DEBUG_OBJECT (dap, "Parsing serialized config from %s",
      dap->json_config_path);

  if (dap->serialized_config && !force)
    return TRUE;

  if (dap->serialized_config)
    g_free (dap->serialized_config);

  dap->serialized_config =
      dlb_dap_json_parse_serialized_config (dap->json_config_path,
      rate, virtualizer_enable, &error);

  if (error) {
    GST_ELEMENT_WARNING (dap, LIBRARY, SETTINGS, ("%s: %d", error->message,
            error->code), ("JSON parsing failed"));

    g_error_free (error);
    return FALSE;
  }

  return TRUE;
}

static gboolean
dlb_dap_get_config_from_json (DlbDap * dap)
{
  GError *error = NULL;
  gint virtualizer_enable = 0;

  GST_DEBUG_OBJECT (dap, "Parsing config from %s", dap->json_config_path);

  g_free (dap->global_conf.profile);
  dap->global_conf.profile = NULL;

  dlb_dap_json_parse_config (dap->json_config_path, &dap->global_conf,
      &dap->virt_conf, &dap->gains, &dap->profile, &error);

  if (error)
    goto parsing_error;

  if (dap->global_conf.use_serialized_settings) {
    gint channels, ret;
    guint64 channel_mask;
    dlb_dap_channel_format fmt;

    ret = dlb_dap_get_serialized_config_from_json (dap, 48000,
        dap->global_conf.virtualizer_enable, FALSE);

    if (!ret)
      goto config_error;

    dlb_dap_preprocess_serialized_config (dap->serialized_config,
        &fmt, &channels, &virtualizer_enable);

    dap_format_to_channel_mask (&fmt, &channel_mask);

    dlb_dap_post_serialized_info_message (dap, channels, channel_mask,
        virtualizer_enable);
  }

  dap->virtualizer_enable = dap->global_conf.virtualizer_enable;
  dlb_dap_update_state (dap);

  return TRUE;

parsing_error:
  GST_ELEMENT_WARNING (dap, LIBRARY, SETTINGS, ("%s: %d", error->message,
          error->code), ("JSON parsing failed"));

  g_error_free (error);
  return FALSE;

config_error:
  GST_ELEMENT_WARNING (dap, LIBRARY, SETTINGS, (NULL),
      ("Could not find serialized config for given settings: rate %d, virt: %d",
          48000, virtualizer_enable));

  return FALSE;
}

void
dlb_dap_set_property (GObject * object, guint property_id,
    const GValue * value, GParamSpec * pspec)
{
  DlbDap *dap = DLB_DAP (object);

  switch (property_id) {
    case PROP_VIRTUALIZER_ENABLE:
      dap->virtualizer_enable = g_value_get_boolean (value);
      break;
    case PROP_VIRT_FRONT_SPEAKER_ANGLE:
      dap->virt_conf.front_speaker_angle = g_value_get_int (value);
      break;
    case PROP_VIRT_SURROUND_SPEAKER_ANGLE:
      dap->virt_conf.surround_speaker_angle = g_value_get_int (value);
      break;
    case PROP_VIRT_REAR_SURROUND_SPEAKER_ANGLE:
      dap->virt_conf.rear_surround_speaker_angle = g_value_get_int (value);
      break;
    case PROP_VIRT_HEIGHT_SPEAKER_ANGLE:
      dap->virt_conf.height_speaker_angle = g_value_get_int (value);
      break;
    case PROP_VIRT_REAR_HEIGHT_SPEAKER_ANGLE:
      dap->virt_conf.rear_height_speaker_angle = g_value_get_int (value);
      break;
    case PROP_HEIGHT_FILTER_ENABLE:
      dap->virt_conf.height_filter_enable = g_value_get_boolean (value);
      break;
    case PROP_BASS_ENHANCER_ENABLE:
      dap->profile.bass_enhancer_enable = g_value_get_boolean (value);
      break;
    case PROP_BASS_ENHANCER_BOOST:
      dap->profile.bass_enhancer_boost = g_value_get_int (value);
      break;
    case PROP_BASS_ENHANCER_CUTOFF_FREQ:
      dap->profile.bass_enhancer_cutoff_frequency = g_value_get_int (value);
      break;
    case PROP_BASS_ENHANCER_WIDTH:
      dap->profile.bass_enhancer_width = g_value_get_int (value);
      break;
    case PROP_CALIBRATION_BOOST:
      dap->profile.calibration_boost = g_value_get_int (value);
      break;
    case PROP_DIALOG_ENHANCER_ENABLE:
      dap->profile.dialog_enhancer_enable = g_value_get_boolean (value);
      break;
    case PROP_DIALOG_ENHANCER_AMOUNT:
      dap->profile.dialog_enhancer_amount = g_value_get_int (value);
      break;
    case PROP_DIALOG_ENHANCER_DUCKING:
      dap->profile.dialog_enhancer_ducking = g_value_get_int (value);
      break;
    case PROP_GEQ_ENABLE:
      dap->profile.graphic_equalizer_enable = g_value_get_boolean (value);
      break;
    case PROP_GEQ_FREQS:
      fill_data_array_uint (dap->profile.graphic_equalizer_bands,
          &dap->profile.graphic_equalizer_bands_num, value);
      break;
    case PROP_GEQ_GAINS:
      fill_data_array_int (dap->profile.graphic_equalizer_gains,
          &dap->profile.graphic_equalizer_bands_num, value);
      break;
    case PROP_IEQ_ENABLE:
      dap->profile.ieq_enable = g_value_get_boolean (value);
      break;
    case PROP_IEQ_AMOUNT:
      dap->profile.ieq_amount = g_value_get_int (value);
      break;
    case PROP_IEQ_FREQS:
      fill_data_array_uint (dap->profile.ieq_bands, &dap->profile.ieq_bands_num,
          value);
      break;
    case PROP_IEQ_GAINS:
      fill_data_array_int (dap->profile.ieq_gains, &dap->profile.ieq_bands_num,
          value);
      break;
    case PROP_MI_DIALOG_ENHANCER_STEERING_ENABLE:
      dap->profile.mi_dialog_enhancer_steering_enable =
          g_value_get_boolean (value);
      break;
    case PROP_MI_DV_LEVELER_STEERING_ENABLE:
      dap->profile.mi_dv_leveler_steering_enable = g_value_get_boolean (value);
      break;
    case PROP_MI_IEQ_STEERING_ENABLE:
      dap->profile.mi_ieq_steering_enable = g_value_get_boolean (value);
      break;
    case PROP_MI_SURROUND_COMPRESSOR_STEERING_ENABLE:
      dap->profile.mi_surround_compressor_steering_enable =
          g_value_get_boolean (value);
      break;
    case PROP_PREGAIN:
      dap->gains.pregain = g_value_get_int (value);
      break;
    case PROP_POSTGAIN:
      dap->gains.postgain = g_value_get_int (value);
      break;
    case PROP_SYSGAIN:
      dap->gains.system_gain = g_value_get_int (value);
      break;
    case PROP_SURROUND_DECODER_ENABLE:
      dap->profile.surround_decoder_enable = g_value_get_boolean (value);
      break;
    case PROP_SURROUND_DECODER_CENTER_SPREAD_ENABLE:
      dap->profile.surround_decoder_center_spreading_enable =
          g_value_get_boolean (value);
      break;
    case PROP_SURROUND_BOOST:
      dap->profile.surround_boost = g_value_get_int (value);
      break;
    case PROP_VOLMAX_BOOST:
      dap->profile.volmax_boost = g_value_get_int (value);
      break;
    case PROP_VOLUME_LEVELER_ENABLE:
      dap->profile.volume_leveler_enable = g_value_get_boolean (value);
      break;
    case PROP_VOLUME_LEVELER_AMOUNT:
      dap->profile.volume_leveler_amount = g_value_get_int (value);
      break;
    case PROP_JSON_CONFIG:
      g_free (dap->json_config_path);
      dap->json_config_path = g_strdup (g_value_get_string (value));
      dlb_dap_get_config_from_json (dap);
      break;
    case PROP_DISCARD_LATENCY:
      dap->discard_latency = g_value_get_boolean (value);
      break;
    case PROP_FORCE_ORDER:
      dap->force_order = g_value_get_boolean (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }

  dlb_dap_update_state (dap);
}

void
dlb_dap_get_property (GObject * object, guint property_id,
    GValue * value, GParamSpec * pspec)
{
  DlbDap *dap = DLB_DAP (object);

  switch (property_id) {
    case PROP_VIRTUALIZER_ENABLE:
      g_value_set_boolean (value, dap->virtualizer_enable);
      break;
    case PROP_VIRT_FRONT_SPEAKER_ANGLE:
      g_value_set_int (value, dap->virt_conf.front_speaker_angle);
      break;
    case PROP_VIRT_SURROUND_SPEAKER_ANGLE:
      g_value_set_int (value, dap->virt_conf.surround_speaker_angle);
      break;
    case PROP_VIRT_REAR_SURROUND_SPEAKER_ANGLE:
      g_value_set_int (value, dap->virt_conf.rear_surround_speaker_angle);
      break;
    case PROP_VIRT_HEIGHT_SPEAKER_ANGLE:
      g_value_set_int (value, dap->virt_conf.height_speaker_angle);
      break;
    case PROP_VIRT_REAR_HEIGHT_SPEAKER_ANGLE:
      g_value_set_int (value, dap->virt_conf.rear_height_speaker_angle);
      break;
    case PROP_HEIGHT_FILTER_ENABLE:
      g_value_set_boolean (value, dap->virt_conf.height_filter_enable);
      break;
    case PROP_BASS_ENHANCER_ENABLE:
      g_value_set_boolean (value, dap->profile.bass_enhancer_enable);
      break;
    case PROP_BASS_ENHANCER_BOOST:
      g_value_set_int (value, dap->profile.bass_enhancer_boost);
      break;
    case PROP_BASS_ENHANCER_CUTOFF_FREQ:
      g_value_set_int (value, dap->profile.bass_enhancer_cutoff_frequency);
      break;
    case PROP_BASS_ENHANCER_WIDTH:
      g_value_set_int (value, dap->profile.bass_enhancer_width);
      break;
    case PROP_CALIBRATION_BOOST:
      g_value_set_int (value, dap->profile.calibration_boost);
      break;
    case PROP_DIALOG_ENHANCER_ENABLE:
      g_value_set_boolean (value, dap->profile.dialog_enhancer_enable);
      break;
    case PROP_DIALOG_ENHANCER_AMOUNT:
      g_value_set_int (value, dap->profile.dialog_enhancer_amount);
      break;
    case PROP_DIALOG_ENHANCER_DUCKING:
      g_value_set_int (value, dap->profile.dialog_enhancer_ducking);
      break;
    case PROP_GEQ_ENABLE:
      g_value_set_boolean (value, dap->profile.graphic_equalizer_enable);
      break;
    case PROP_GEQ_FREQS:
      fill_gst_value_array_uint (value, dap->profile.graphic_equalizer_bands,
          dap->profile.graphic_equalizer_bands_num);
      break;
    case PROP_GEQ_GAINS:
      fill_gst_value_array_int (value, dap->profile.graphic_equalizer_gains,
          dap->profile.graphic_equalizer_bands_num);
      break;
    case PROP_IEQ_ENABLE:
      g_value_set_boolean (value, dap->profile.ieq_enable);
      break;
    case PROP_IEQ_AMOUNT:
      g_value_set_int (value, dap->profile.ieq_amount);
      break;
    case PROP_IEQ_FREQS:
      fill_gst_value_array_uint (value, dap->profile.ieq_bands,
          dap->profile.ieq_bands_num);
      break;
    case PROP_IEQ_GAINS:
      fill_gst_value_array_int (value, dap->profile.ieq_gains,
          dap->profile.ieq_bands_num);
      break;
    case PROP_MI_DIALOG_ENHANCER_STEERING_ENABLE:
      g_value_set_boolean (value,
          dap->profile.mi_dialog_enhancer_steering_enable);
      break;
    case PROP_MI_DV_LEVELER_STEERING_ENABLE:
      g_value_set_boolean (value, dap->profile.mi_dv_leveler_steering_enable);
      break;
    case PROP_MI_IEQ_STEERING_ENABLE:
      g_value_set_boolean (value, dap->profile.mi_ieq_steering_enable);
      break;
    case PROP_MI_SURROUND_COMPRESSOR_STEERING_ENABLE:
      g_value_set_boolean (value,
          dap->profile.mi_surround_compressor_steering_enable);
      break;
    case PROP_PREGAIN:
      g_value_set_int (value, dap->gains.pregain);
      break;
    case PROP_POSTGAIN:
      g_value_set_int (value, dap->gains.postgain);
      break;
    case PROP_SYSGAIN:
      g_value_set_int (value, dap->gains.system_gain);
      break;
    case PROP_SURROUND_DECODER_ENABLE:
      g_value_set_boolean (value, dap->profile.surround_decoder_enable);
      break;
    case PROP_SURROUND_DECODER_CENTER_SPREAD_ENABLE:
      g_value_set_boolean (value,
          dap->profile.surround_decoder_center_spreading_enable);
      break;
    case PROP_SURROUND_BOOST:
      g_value_set_int (value, dap->profile.surround_boost);
      break;
    case PROP_VOLMAX_BOOST:
      g_value_set_int (value, dap->profile.volmax_boost);
      break;
    case PROP_VOLUME_LEVELER_ENABLE:
      g_value_set_boolean (value, dap->profile.volume_leveler_enable);
      break;
    case PROP_VOLUME_LEVELER_AMOUNT:
      g_value_set_int (value, dap->profile.volume_leveler_amount);
      break;
    case PROP_JSON_CONFIG:
      g_value_set_string (value, dap->json_config_path);
      break;
    case PROP_DISCARD_LATENCY:
      g_value_set_boolean (value, dap->discard_latency);
      break;
    case PROP_FORCE_ORDER:
      g_value_set_boolean (value, dap->force_order);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}

void
dlb_dap_finalize (GObject * object)
{
  DlbDap *dap = DLB_DAP (object);

  g_free (dap->json_config_path);
  g_free (dap->serialized_config);
  g_free (dap->global_conf.profile);
  g_mutex_clear (&dap->lock);
  g_object_unref (dap->adapter);

  G_OBJECT_CLASS (dlb_dap_parent_class)->finalize (object);
}

static gboolean
add_format_to_structure (GstCapsFeatures * features,
    GstStructure * other, gpointer user_data)
{
  GstStructure *s = (GstStructure *) user_data;
  const GValue *format;

  if ((format = gst_structure_get_value (s, "format")))
    gst_structure_set_value (other, "format", format);

  return TRUE;
}

static gboolean
add_rate_to_structure (GstCapsFeatures * features,
    GstStructure * other, gpointer user_data)
{
  GstStructure *s = (GstStructure *) user_data;
  const GValue *rate;

  if ((rate = gst_structure_get_value (s, "rate")))
    gst_structure_set_value (other, "rate", rate);

  return TRUE;
}

static GstCaps *
caps_add_channel_configuration (GstCaps * in_caps, GstStructure * in_structure,
    guint64 channel_mask)
{
  GArray *arr = NULL;
  gint channels;
  gint i, N = G_N_ELEMENTS (allowed_input_channel_masks);

  GstCaps *caps = gst_caps_new_empty ();

  arr = g_array_new (FALSE, FALSE, sizeof (guint64));
  g_array_append_vals (arr, allowed_input_channel_masks, N);

  /* First move preferred channel mask to the begin */
  for (i = 0; i < N; ++i) {
    if (g_array_index (arr, guint64, i) == channel_mask)
      break;
  }

  g_array_prepend_val (arr, channel_mask);
  g_array_remove_index (arr, ++i);

  for (i = 0; i < N; ++i) {
    GstStructure *s = gst_structure_copy (in_structure);
    channel_mask = g_array_index (arr, guint64, i);
    channels = get_channels (channel_mask);

    gst_structure_remove_fields (s, "channels", "channel-mask", NULL);
    gst_structure_set (s, "channels", G_TYPE_INT, channels, "channel-mask",
        GST_TYPE_BITMASK, channel_mask, NULL);

    caps = gst_caps_merge_structure (caps, s);
  }

  g_array_free (arr, TRUE);
  gst_caps_unref (in_caps);

  return caps;
}

static GstCaps *
dlb_dap_transform_caps (GstBaseTransform * trans, GstPadDirection direction,
    GstCaps * caps, GstCaps * filter)
{
  DlbDap *dap = DLB_DAP (trans);
  GstCaps *othercaps;
  GstStructure *s;

  dlb_dap_channel_format infmt, outfmt;
  guint64 outchmask = 0, inchmask = 0;
  int channels, rate;

  GST_DEBUG_OBJECT (dap, "Transform caps (direction %d)", direction);

  if (!(s = gst_caps_get_structure (caps, 0)))
    return NULL;

  if (direction == GST_PAD_SRC) {
    /* transform caps going upstream */
    othercaps = gst_static_pad_template_get_caps (&dlb_dap_sink_template);
    othercaps = gst_caps_make_writable (othercaps);

    gst_structure_get (s, "channel-mask", GST_TYPE_BITMASK, &outchmask, NULL);
    channel_mask_to_dap_format (outchmask, &outfmt);

    /* If we use serialized config, output is determined by json, so we ignore
     * channel-mask on src pad */
    if (dap->global_conf.use_serialized_settings)
      outfmt = dap->outfmt;

    if (is_format_valid (&outfmt)) {
      dlb_dap_propose_input_format (&outfmt, dap->virtualizer_enable, &infmt);
      dap_format_to_channel_mask (&infmt, &inchmask);

      othercaps = caps_add_channel_configuration (othercaps, s, inchmask);
    }
  } else {
    /* transform caps going downstream */
    othercaps = gst_static_pad_template_get_caps (&dlb_dap_src_template);
    othercaps = gst_caps_make_writable (othercaps);

    if (dap->global_conf.use_serialized_settings
        && gst_structure_get (s, "rate", G_TYPE_INT, &rate, NULL)) {

      GstStructure *other = gst_caps_get_structure (othercaps, 0);

      dlb_dap_get_serialized_config_from_json (dap, rate,
          dap->virtualizer_enable, TRUE);

      if (!dap->serialized_config)
        goto config_error;

      dlb_dap_preprocess_serialized_config (dap->serialized_config,
          &dap->outfmt, &channels, &dap->virtualizer_enable);

      gst_structure_set (other, "channels", G_TYPE_INT, channels,
          "channel-mask", GST_TYPE_BITMASK, 0, NULL);
    }
  }

  gst_caps_map_in_place (othercaps, add_format_to_structure, s);
  gst_caps_map_in_place (othercaps, add_rate_to_structure, s);

  GST_DEBUG_OBJECT (dap,
      "transformed %" GST_PTR_FORMAT " into %" GST_PTR_FORMAT, caps, othercaps);

  if (filter) {
    GstCaps *intersect;

    GST_DEBUG_OBJECT (dap, "Using filter caps %" GST_PTR_FORMAT, filter);
    intersect =
        gst_caps_intersect_full (othercaps, filter, GST_CAPS_INTERSECT_FIRST);
    gst_caps_unref (othercaps);
    GST_DEBUG_OBJECT (dap, "Intersection %" GST_PTR_FORMAT, intersect);

    return intersect;
  } else {
    return othercaps;
  }

config_error:
  GST_ERROR_OBJECT (dap, "Could not find serialized config for given settings: "
      "rate %d, virt: %d", rate, dap->virtualizer_enable);

  gst_caps_unref (othercaps);
  return NULL;
}

static gboolean
dlb_dap_open (DlbDap * dap)
{
  dlb_dap_init_info info;

  info.virtualizer_enable = dap->virtualizer_enable;
  info.sample_rate = dap->ininfo.rate;
  info.output_format = dap->outfmt;
  info.serialized_config = dap->serialized_config;

  if (!dap->dap_instance) {
    dap->dap_instance = dlb_dap_new (&info);

    if (!dap->dap_instance) {
      GST_ELEMENT_ERROR (dap, LIBRARY, INIT, (NULL), ("Failed to open DAP"));
      return FALSE;
    }
  }

  dlb_dap_update_state (dap);
  return TRUE;
}

static void
dlb_dap_close (DlbDap * dap)
{
  if (dap->dap_instance)
    dlb_dap_free (dap->dap_instance);

  dap->dap_instance = NULL;
}

static gboolean
dlb_dap_set_caps (GstBaseTransform * trans, GstCaps * incaps, GstCaps * outcaps)
{
  DlbDap *dap = DLB_DAP (trans);
  GstAudioInfo in, out;
  gsize blocksz, latency;
  guint64 inchmask, outchmask;

  GST_DEBUG_OBJECT (dap, "incaps %" GST_PTR_FORMAT ", outcaps %" GST_PTR_FORMAT,
      incaps, outcaps);

  if (!gst_audio_info_from_caps (&in, incaps))
    goto incaps_error;
  if (!gst_audio_info_from_caps (&out, outcaps))
    goto outcaps_error;

  dlb_dap_push_drain (dap);

  gst_audio_channel_positions_to_mask (in.position, in.channels,
      FALSE, &inchmask);
  gst_audio_channel_positions_to_mask (out.position, out.channels,
      FALSE, &outchmask);

  channel_mask_to_dap_format (inchmask, &dap->infmt);
  channel_mask_to_dap_format (outchmask, &dap->outfmt);
  dap->ininfo = in;
  dap->outinfo = out;

  if (!dlb_dap_open (dap))
    return FALSE;

  blocksz = dlb_dap_query_block_samples (dap->dap_instance);
  latency = dlb_dap_query_latency (dap->dap_instance);

  if (!dap->discard_latency)
    latency = 0;

  dap->inbufsz = blocksz * GST_AUDIO_INFO_BPF (&in);
  dap->outbufsz = blocksz * GST_AUDIO_INFO_BPF (&out);
  dap->latency = latency * GST_AUDIO_INFO_BPF (&out);
  dap->prefill = latency * GST_AUDIO_INFO_BPF (&in);

  dap->latency_samples = latency;
  dap->latency_time = gst_util_uint64_scale_int (latency, GST_SECOND, in.rate);

  GST_DEBUG_OBJECT (dap,
      "input buff size: %" G_GSIZE_FORMAT " output buff size: %" G_GSIZE_FORMAT
      " latency size: %" G_GSIZE_FORMAT, dap->inbufsz, dap->outbufsz,
      dap->latency);

  return TRUE;

  /* ERROR */
incaps_error:
  GST_ERROR_OBJECT (trans, "invalid incaps");
  return FALSE;

outcaps_error:
  GST_ERROR_OBJECT (trans, "invalid outcaps");
  return FALSE;
}

static gboolean
dlb_dap_query (GstBaseTransform * trans, GstPadDirection direction,
    GstQuery * query)
{
  DlbDap *dap = DLB_DAP (trans);

  switch (GST_QUERY_TYPE (query)) {
    case GST_QUERY_LATENCY:{
      if ((gst_pad_peer_query (GST_BASE_TRANSFORM_SINK_PAD (trans), query))) {
        GstClockTime min, max;
        gboolean live;
        guint64 latency = dap->latency_samples;
        gint rate = dap->ininfo.rate;

        if (!rate)
          return FALSE;

        if (gst_base_transform_is_passthrough (trans))
          latency = 0;

        latency = gst_util_uint64_scale_round (latency, GST_SECOND, rate);
        gst_query_parse_latency (query, &live, &min, &max);

        GST_DEBUG_OBJECT (dap, "Peer latency: min %"
            GST_TIME_FORMAT " max %" GST_TIME_FORMAT,
            GST_TIME_ARGS (min), GST_TIME_ARGS (max));

        /* add our own latency */
        GST_DEBUG_OBJECT (dap, "Our latency: %" GST_TIME_FORMAT,
            GST_TIME_ARGS (latency));

        min += latency;
        if (GST_CLOCK_TIME_IS_VALID (max))
          max += latency;

        GST_DEBUG_OBJECT (dap, "Calculated total latency : min %"
            GST_TIME_FORMAT " max %" GST_TIME_FORMAT,
            GST_TIME_ARGS (min), GST_TIME_ARGS (max));

        gst_query_set_latency (query, live, min, max);
      }

      return TRUE;
    }
    default:
      return GST_BASE_TRANSFORM_CLASS (dlb_dap_parent_class)->query (trans,
          direction, query);
  }
}

static gboolean
dlb_dap_transform_size (GstBaseTransform * trans, GstPadDirection direction,
    GstCaps * caps, gsize size, GstCaps * othercaps, gsize * othersize)
{
  DlbDap *dap = DLB_DAP (trans);

  if (direction == GST_PAD_SRC) {
    *othersize = dap->inbufsz;
  } else {
    gsize adapter_size = gst_adapter_available (dap->adapter);

    dap->transform_blocks = (size + adapter_size) / dap->inbufsz;
    *othersize = dap->transform_blocks * dap->outbufsz;

    GST_LOG_OBJECT (dap,
        "adapter size: %" G_GSIZE_FORMAT ", input size: %" G_GSIZE_FORMAT,
        adapter_size, size);
  }

  GST_LOG_OBJECT (dap, "transform_size: %" G_GSIZE_FORMAT, *othersize);

  return TRUE;
}

static gboolean
dlb_dap_get_unit_size (GstBaseTransform * trans, GstCaps * caps, gsize * size)
{
  GstAudioInfo info;

  if (!gst_audio_info_from_caps (&info, caps))
    goto caps_error;

  *size = GST_AUDIO_INFO_BPF (&info);

  return TRUE;

  /* Error */
caps_error:
  GST_ERROR_OBJECT (trans, "invalid caps");
  return FALSE;
}

static gboolean
dlb_dap_start (GstBaseTransform * trans)
{
  DlbDap *dap = DLB_DAP (trans);

  if (dap->json_config_path)
    dlb_dap_get_config_from_json (dap);

  return TRUE;
}

static gboolean
dlb_dap_stop (GstBaseTransform * trans)
{
  DlbDap *dap = DLB_DAP (trans);

  dlb_dap_close (dap);
  gst_adapter_clear (dap->adapter);

  gst_audio_info_init (&dap->ininfo);
  gst_audio_info_init (&dap->outinfo);

  g_free (dap->global_conf.profile);

  memset(&dap->infmt, 0, sizeof(dap->infmt));
  memset(&dap->outfmt, 0, sizeof(dap->outfmt));
  memset(&dap->global_conf, 0, sizeof(dap->global_conf));

  dap->transform_blocks = 0;
  dap->inbufsz = 0;
  dap->outbufsz = 0;
  dap->latency = 0;
  dap->prefill = 0;
  dap->latency_samples = 0;
  dap->latency_time = GST_CLOCK_TIME_NONE;

  return TRUE;
}

static void
dlb_dap_get_input_timing (DlbDap * dap, GstClockTime * timestamp,
    guint64 * offset)
{
  GstClockTime ts;
  guint64 off, distance;

  ts = gst_adapter_prev_pts (dap->adapter, &distance);
  if (ts != GST_CLOCK_TIME_NONE) {
    distance /= dap->ininfo.bpf;
    ts += gst_util_uint64_scale_int (distance, GST_SECOND, dap->ininfo.rate);
  }

  off = gst_adapter_prev_offset (dap->adapter, &distance);
  if (off != GST_CLOCK_TIME_NONE) {
    distance /= dap->ininfo.bpf;
    off += distance;
  }

  *timestamp = ts;
  *offset = off;
}

static void
dlb_dap_set_output_timing (DlbDap * dap, GstBuffer * outbuf,
    GstClockTime timestamp, guint64 offset)
{
  gsize outsize = gst_buffer_get_size (outbuf);
  gint outsamples = outsize / dap->outinfo.bpf;

  GST_BUFFER_PTS (outbuf) = timestamp;
  GST_BUFFER_DURATION (outbuf) =
      gst_util_uint64_scale_int (outsamples, GST_SECOND, dap->outinfo.rate);

  GST_BUFFER_OFFSET (outbuf) = offset;
  GST_BUFFER_OFFSET_END (outbuf) = offset + outsamples;
}

static void
dlb_dap_handle_tag_list (DlbDap * dap, GstTagList * taglist)
{
  GstClockTime timestamp;
  guint64 offset;
  gchar *audio_codec = NULL;
  gboolean object_audio = FALSE;

  if (gst_tag_list_get_string (taglist, "audio-codec", &audio_codec)) {
    gst_tag_list_get_boolean (taglist, "object-audio", &object_audio);

    GST_DEBUG_OBJECT (dap, "audio_codec %s, object-audio %d", audio_codec,
        object_audio);

    if (strstr (audio_codec, "E-AC-3") || strstr (audio_codec, "AC-3")) {
      dap->profile.volume_leveler_in_target = -496;
      gst_tag_list_add (taglist, GST_TAG_MERGE_REPLACE,
          "surround-decoder-enable", dap->profile.surround_decoder_enable,
          NULL);
    } else if (dap->ininfo.channels > 2) {
      dap->profile.volume_leveler_in_target = -432;
    } else {
      dap->profile.volume_leveler_in_target = -320;
    }

    dlb_dap_get_input_timing (dap, &timestamp, &offset);

    if (dap->ininfo.bpf && dap->ininfo.rate) {
      timestamp +=
          gst_util_uint64_scale_int (gst_adapter_available (dap->adapter) /
          dap->ininfo.bpf, GST_SECOND, dap->ininfo.rate);
    }

    dlb_dap_post_stream_info_message (dap, audio_codec, object_audio,
        dap->profile.surround_decoder_enable, timestamp);

    dlb_dap_update_state (dap);
    g_free (audio_codec);
  }
}

/* sink and src pad event handlers */
static gboolean
dlb_dap_sink_event (GstBaseTransform * trans, GstEvent * event)
{
  DlbDap *dap = DLB_DAP (trans);
  GstTagList *taglist;

  GST_LOG_OBJECT (dap, "sink_event");

  switch (GST_EVENT_TYPE (event)) {
    case GST_EVENT_FLUSH_STOP:
    case GST_EVENT_EOS:
      if (dap->dap_instance)
        dlb_dap_push_drain (dap);
      break;
    case GST_EVENT_TAG:
      GST_DEBUG_OBJECT (dap, "Handle tag event");
      gst_event_parse_tag (event, &taglist);
      dlb_dap_handle_tag_list (dap, taglist);
      break;
    default:
      break;
  }

  return GST_BASE_TRANSFORM_CLASS (dlb_dap_parent_class)->sink_event (trans,
      event);
}

static gboolean
dlb_dap_src_event (GstBaseTransform * trans, GstEvent * event)
{
  DlbDap *dap = DLB_DAP (trans);

  GST_LOG_OBJECT (dap, "src_event");

  return GST_BASE_TRANSFORM_CLASS (dlb_dap_parent_class)->src_event (trans,
      event);
}

static GstFlowReturn
dlb_dap_push_drain (DlbDap * dap)
{
  GstFlowReturn ret;
  GstBuffer *inbuf, *outbuf;
  GstClockTime timestamp;
  guint64 offset;

  GstAllocator *allocator;
  GstAllocationParams params;

  gsize outsize, insize, residuesize, adaptersize;
  gint blocks_num;

  GstBaseTransform *trans = GST_BASE_TRANSFORM_CAST (dap);
  gst_base_transform_get_allocator (trans, &allocator, &params);

  adaptersize = gst_adapter_available (dap->adapter);

  if (G_UNLIKELY (dap->ininfo.bpf * dap->outinfo.bpf == 0))
    return GST_FLOW_OK;

  outsize = dap->latency + (adaptersize / dap->ininfo.bpf * dap->outinfo.bpf);

  adaptersize %= dap->inbufsz;
  residuesize = adaptersize;
  residuesize += dap->latency_samples * dap->ininfo.bpf;

  /* nothing to do */
  if (!residuesize)
    return GST_FLOW_OK;

  blocks_num = ((residuesize + dap->inbufsz - 1) / dap->inbufsz);
  insize = blocks_num * dap->inbufsz - adaptersize;

  inbuf = gst_buffer_new_allocate (allocator, insize, &params);
  gst_buffer_memset (inbuf, 0, 0, insize);

  if (allocator)
    gst_object_unref (allocator);

  dlb_dap_get_input_timing (dap, &timestamp, &offset);
  dlb_dap_set_output_timing (dap, inbuf, timestamp, offset);

  ret =
      GST_BASE_TRANSFORM_CLASS (dlb_dap_parent_class)->prepare_output_buffer
      (trans, inbuf, &outbuf);

  if (ret != GST_FLOW_OK)
    goto outbuf_error;

  ret = dlb_dap_transform (trans, inbuf, outbuf);
  if (ret != GST_FLOW_OK)
    goto transform_error;

  gst_buffer_unref (inbuf);
  gst_buffer_resize (outbuf, 0, outsize);
  GST_DEBUG_OBJECT (dap, "Flushing buff of %" G_GSIZE_FORMAT " bytes", outsize);

  ret = gst_pad_push (GST_BASE_TRANSFORM_SRC_PAD (dap), outbuf);
  if (ret != GST_FLOW_OK)
    GST_WARNING_OBJECT (dap, "Pushing failed: %s", gst_flow_get_name (ret));

  return ret;

transform_error:
  gst_buffer_unref (inbuf);
  gst_buffer_unref (outbuf);

outbuf_error:
  gst_buffer_unref (inbuf);

  GST_WARNING_OBJECT (dap, "Draining failed: %s", gst_flow_get_name (ret));
  return ret;
}

static GstFlowReturn
dlb_dap_transform (GstBaseTransform * trans, GstBuffer * inbuf,
    GstBuffer * outbuf)
{
  DlbDap *dap = DLB_DAP (trans);
  GstMapInfo outmap;
  gint i;

  GstClockTime timestamp;
  guint64 offset;
  gsize outsize;

  g_mutex_lock (&dap->lock);

  gst_buffer_ref (inbuf);
  gst_adapter_push (dap->adapter, inbuf);

  if (G_UNLIKELY (gst_adapter_available (dap->adapter) <= dap->prefill) ||
      dap->transform_blocks == 0)
    goto no_output;

  dlb_dap_get_input_timing (dap, &timestamp, &offset);
  gst_buffer_map (outbuf, &outmap, GST_MAP_WRITE);

  for (i = 0; i < dap->transform_blocks; ++i) {
    dlb_buffer *in, *out;
    const guint8 *indata = gst_adapter_map (dap->adapter, dap->inbufsz);
    const guint8 *outdata = outmap.data + i * dap->outbufsz;

    in = dlb_buffer_new_wrapped (indata, &dap->ininfo, TRUE);
    out = dlb_buffer_new_wrapped (outdata, &dap->outinfo, !dap->force_order);

    dlb_dap_process (dap->dap_instance, &dap->infmt, in, out);

    gst_adapter_unmap (dap->adapter);
    gst_adapter_flush (dap->adapter, dap->inbufsz);
    dlb_buffer_free (in);
    dlb_buffer_free (out);
  }

  gst_buffer_unmap (outbuf, &outmap);

  if (G_UNLIKELY (dap->prefill)) {
    outsize = gst_buffer_get_size (outbuf);
    outsize -= dap->latency;
    GST_DEBUG_OBJECT (dap, "Trimming latency from the output");

    gst_buffer_resize (outbuf, dap->latency, outsize);
    dap->prefill = 0;
  } else {
    timestamp -= dap->latency_time;
    offset -= dap->latency_samples;
  }

  dlb_dap_set_output_timing (dap, outbuf, timestamp, offset);

  GST_LOG_OBJECT (dap, "inbuf %" GST_PTR_FORMAT ", outbuf %" GST_PTR_FORMAT,
      inbuf, outbuf);

  g_mutex_unlock (&dap->lock);
  return GST_FLOW_OK;

no_output:
  g_mutex_unlock (&dap->lock);
  return GST_BASE_TRANSFORM_FLOW_DROPPED;
}


static gboolean
plugin_init (GstPlugin * plugin)
{
#ifdef DLB_DAP_OPEN_DYNLIB
  if (dlb_dap_try_open_dynlib ())
    return FALSE;
#endif

  if (!gst_element_register (plugin, "dlbdap", GST_RANK_PRIMARY, GST_TYPE_DAP))
    return FALSE;

  return TRUE;
}

GST_PLUGIN_DEFINE (GST_VERSION_MAJOR,
    GST_VERSION_MINOR,
    dlbdap,
    "Dolby Audio Processing", plugin_init, VERSION, LICENSE, PACKAGE, ORIGIN)
