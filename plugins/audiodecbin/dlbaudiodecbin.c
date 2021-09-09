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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gst/gstchildproxy.h>
#include <gst/gstvalue.h>
#include <gst/pbutils/pbutils.h>

#include "dlbaudiodecbin.h"
#include "dlbaudiodecoder.h"
#include "dlbaudiometa.h"
#include "dlbutils.h"

GST_DEBUG_CATEGORY_STATIC (dlb_audio_dec_bin_debug_category);
#define GST_CAT_DEFAULT dlb_audio_dec_bin_debug_category

static GstBinClass *parent_class = NULL;

#define DLB_AUDIO_DEC_BIN_SINK_CAPS                                            \
  "audio/ac3; "                                                                \
  "audio/eac3; "                                                               \
  "audio/x-ac3; "                                                              \
  "audio/x-eac3; "                                                             \
  "audio/x-ac4-raw; "                                                          \
  "audio/x-true-hd; "                                                          \
  "audio/x-mat"

#define DLB_AUDIO_DEC_BIN_SRC_CAPS                                             \
  "audio/x-raw; "                                                              \
  "audio/x-raw(" DLB_CAPS_FEATURE_META_OBJECT_AUDIO_META ")"                   \

/* generic templates */
static GstStaticPadTemplate sink_template = GST_STATIC_PAD_TEMPLATE ("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS (DLB_AUDIO_DEC_BIN_SINK_CAPS)
    );

static GstStaticPadTemplate src_template = GST_STATIC_PAD_TEMPLATE ("src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS (DLB_AUDIO_DEC_BIN_SRC_CAPS)
    );


G_DEFINE_TYPE_WITH_CODE (DlbAudioDecBin, dlb_audio_dec_bin,
    GST_TYPE_BIN, G_IMPLEMENT_INTERFACE (DLB_TYPE_AUDIO_DECODER, NULL));

/* properties */
enum
{
  PROP_0,
  PROP_OUT_MODE,
  PROP_DRC_MODE,
  PROP_DRC_CUT,
  PROP_DRC_BOOST,
  PROP_DMX_ENABLE,
};

#define CHMASK(ch) (GST_AUDIO_CHANNEL_POSITION_MASK (ch))

static const guint64 allowed_mixer_output_masks[4] = {
  DLB_CHANNEL_MASK_2_0,
  DLB_CHANNEL_MASK_5_1,
  DLB_CHANNEL_MASK_5_1_2,
  DLB_CHANNEL_MASK_7_1
};

/* Declarations of class functions */
static GstStateChangeReturn
dlb_audio_dec_bin_change_state (GstElement * element, GstStateChange trans);
static void dlb_audio_dec_bin_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec);
static void dlb_audio_dec_bin_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec);
static void dlb_audio_dec_bin_dispose (GObject * object);

static gboolean dlb_audio_dec_bin_add_children (DlbAudioDecBin * decbin);
static gboolean dlb_audio_dec_bin_add_decoder_chain (DlbAudioDecBin * decbin);
static void dlb_audio_dec_bin_sync_children_properties (DlbAudioDecBin *
    decbin);
static gboolean dlb_audio_dec_bin_start (DlbAudioDecBin * decbin);
static void dlb_audio_dec_bin_stop (DlbAudioDecBin * decbin);

static GstPadProbeReturn dlb_audio_dec_bin_on_add_oar (GstPad * pad,
    GstPadProbeInfo * info, gpointer user_data);
static GstPadProbeReturn dlb_audio_dec_bin_on_dec_finish_flush (GstPad * pad,
    GstPadProbeInfo * info, gpointer user_data);
static GstPadProbeReturn dlb_audio_dec_bin_on_dec_start_flush (GstPad * pad,
    GstPadProbeInfo * info, gpointer user_data);
static GstPadProbeReturn dlb_audio_dec_bin_on_dec_caps (GstPad * pad,
    GstPadProbeInfo * info, gpointer user_data);
static void dlb_audio_dec_bin_on_type_found (GstElement * typefind,
    guint probability, GstCaps * caps, DlbAudioDecBin * decbin);

/* Definitions of class functions */
static void
dlb_audio_dec_bin_class_init (DlbAudioDecBinClass * klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GstElementClass *element_class = GST_ELEMENT_CLASS (klass);

  parent_class = (GstBinClass *) g_type_class_peek_parent (klass);

  /* define virtual function pointers */
  element_class->change_state =
      GST_DEBUG_FUNCPTR (dlb_audio_dec_bin_change_state);
  object_class->set_property =
      GST_DEBUG_FUNCPTR (dlb_audio_dec_bin_set_property);
  object_class->get_property =
      GST_DEBUG_FUNCPTR (dlb_audio_dec_bin_get_property);
  object_class->dispose = GST_DEBUG_FUNCPTR (dlb_audio_dec_bin_dispose);

  gst_element_class_add_static_pad_template (element_class, &sink_template);
  gst_element_class_add_static_pad_template (element_class, &src_template);

  gst_element_class_set_static_metadata (element_class,
      "Dolby Audio Decoder Bin", "Decoder/Audio/Bin",
      "Decoding bin for Dolby audio formats",
      "Dolby Support <support@dolby.com>");

  g_object_class_override_property (object_class, PROP_OUT_MODE, "out-mode");
  g_object_class_override_property (object_class, PROP_DRC_MODE, "drc-mode");
  g_object_class_override_property (object_class, PROP_DRC_CUT, "drc-cut");
  g_object_class_override_property (object_class, PROP_DRC_BOOST, "drc-boost");
  g_object_class_override_property (object_class, PROP_DMX_ENABLE, "dmx-enable");
}

static void
dlb_audio_dec_bin_init (DlbAudioDecBin * decbin)
{
  GstPadTemplate *tmpl;

  decbin->stream = NULL;
  decbin->typefind = NULL;
  decbin->parser = NULL;
  decbin->dec = NULL;
  decbin->oar = NULL;
  decbin->conv = NULL;
  decbin->capsfilter = NULL;

  decbin->caps_seqnum = 0;
  decbin->dec_probe_id = 0;
  decbin->have_type_id = 0;
  decbin->have_type = FALSE;

  decbin->outmode = DLB_AUDIO_DECODER_OUT_MODE_RAW;
  decbin->drcboost = 1.0;
  decbin->drccut = 1.0;
  decbin->dmxenable = TRUE;

  /* create ghost pads for the bin */
  tmpl = gst_static_pad_template_get (&sink_template);
  decbin->sink = gst_ghost_pad_new_no_target_from_template ("sink", tmpl);
  gst_object_unref (tmpl);
  gst_element_add_pad (GST_ELEMENT_CAST (decbin), decbin->sink);

  tmpl = gst_static_pad_template_get (&src_template);
  decbin->src = gst_ghost_pad_new_no_target_from_template ("src", tmpl);
  gst_object_unref (tmpl);
  gst_element_add_pad (GST_ELEMENT_CAST (decbin), decbin->src);

  dlb_audio_dec_bin_add_children (decbin);
}

static GstStateChangeReturn
dlb_audio_dec_bin_change_state (GstElement * element, GstStateChange trans)
{
  GstStateChangeReturn ret = GST_STATE_CHANGE_SUCCESS;
  DlbAudioDecBin *decbin = DLB_AUDIO_DEC_BIN (element);

  GST_DEBUG_OBJECT (decbin, "Changing state: %s => %s",
      gst_element_state_get_name (GST_STATE_TRANSITION_CURRENT (trans)),
      gst_element_state_get_name (GST_STATE_TRANSITION_NEXT (trans)));

  switch (trans) {
    case GST_STATE_CHANGE_NULL_TO_READY:
      if (dlb_audio_dec_bin_start (decbin) == FALSE)
        return GST_STATE_CHANGE_FAILURE;
      break;
    default:
      break;
  }

  ret = GST_ELEMENT_CLASS (parent_class)->change_state (element, trans);

  switch (trans) {
    case GST_STATE_CHANGE_READY_TO_NULL:
      dlb_audio_dec_bin_stop (decbin);
    default:
      break;
  }

  return ret;
}

static void
dlb_audio_dec_bin_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec)
{
  DlbAudioDecBin *decbin = DLB_AUDIO_DEC_BIN (object);

  switch (prop_id) {
    case PROP_OUT_MODE:
      decbin->outmode = g_value_get_enum (value);
      break;
    case PROP_DRC_MODE:
      decbin->drcmode = g_value_get_enum (value);
      break;
    case PROP_DRC_CUT:
      decbin->drccut = g_value_get_double (value);
      break;
    case PROP_DRC_BOOST:
      decbin->drcboost = g_value_get_double (value);
      break;
    case PROP_DMX_ENABLE:
      decbin->dmxenable = g_value_get_boolean (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }

  dlb_audio_dec_bin_sync_children_properties (decbin);
}

static void
dlb_audio_dec_bin_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec)
{
  DlbAudioDecBin *decbin = DLB_AUDIO_DEC_BIN (object);

  switch (prop_id) {
    case PROP_OUT_MODE:
      g_value_set_enum (value, decbin->outmode);
      break;
    case PROP_DRC_MODE:
      g_value_set_enum (value, decbin->drcmode);
      break;
    case PROP_DRC_CUT:
      g_value_set_double (value, decbin->drccut);
      break;
    case PROP_DRC_BOOST:
      g_value_set_double (value, decbin->drcboost);
      break;
    case PROP_DMX_ENABLE:
      g_value_set_boolean (value, decbin->dmxenable);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
dlb_audio_dec_bin_dispose (GObject * object)
{
  DlbAudioDecBin *decbin = DLB_AUDIO_DEC_BIN (object);
  g_free (decbin->stream);

  G_OBJECT_CLASS (dlb_audio_dec_bin_parent_class)->dispose (object);
}

static void
get_mixer_config (const guint64 in_channel_mask,
    const gint in_channels, guint64 * out_channel_mask, gint * out_channels)
{
  gint i;

  *out_channel_mask = in_channel_mask;
  *out_channels = in_channels;

  for (i = 0; i < G_N_ELEMENTS (allowed_mixer_output_masks); ++i)
    if (in_channel_mask == allowed_mixer_output_masks[i])
      return;

  if (in_channel_mask & (CHMASK (SIDE_RIGHT) | CHMASK (SIDE_LEFT))) {
    *out_channel_mask = DLB_CHANNEL_MASK_7_1;
    *out_channels = 8;
  } else if (in_channel_mask == CHMASK (FRONT_CENTER)) {
    *out_channel_mask = DLB_CHANNEL_MASK_2_0;
    *out_channels = 2;
  } else {
    *out_channel_mask = DLB_CHANNEL_MASK_5_1;
    *out_channels = 6;
  }
}

static GValue *
generate_mix_matrix (const gint in_channels,
    const GstAudioChannelPosition * in_position, const gint out_channels,
    const GstAudioChannelPosition * out_position)
{
  gint i, j;
  GValue row = G_VALUE_INIT, val0 = G_VALUE_INIT, val1 = G_VALUE_INIT;
  GValue *matrix = g_new0 (GValue, 1);

  g_value_init (matrix, GST_TYPE_ARRAY);
  g_value_init (&row, GST_TYPE_ARRAY);
  g_value_init (&val0, G_TYPE_FLOAT);
  g_value_init (&val1, G_TYPE_FLOAT);

  if (in_channels == 1) {
    /* Mono, -3dB in L,R */
    g_value_set_float (&val0, 0.708);
    gst_value_array_append_value (&row, &val0);
    gst_value_array_append_value (matrix, &row);
    gst_value_array_append_value (matrix, &row);
  } else {
    g_value_set_float (&val0, 0.0);
    g_value_set_float (&val1, 1.0);

    for (i = 0; i < out_channels; i++) {
      for (j = 0; j < in_channels; j++) {
        if (out_position[i] == in_position[j])
          gst_value_array_append_value (&row, &val1);
        else
          gst_value_array_append_value (&row, &val0);
      }
      gst_value_array_append_value (matrix, &row);
      g_value_unset (&row);
      g_value_init (&row, GST_TYPE_ARRAY);
    }
  }

  g_value_unset (&row);
  return matrix;
}

static void
reset_mixer (DlbAudioDecBin * decbin)
{
  GValue *matrix = g_new0 (GValue, 1);
  g_value_init (matrix, GST_TYPE_ARRAY);

  g_object_set_property (G_OBJECT (decbin->conv), "mix-matrix", matrix);
  g_object_set (decbin->capsfilter, "caps", NULL, NULL);

  g_value_unset (matrix);
  g_free (matrix);
}

static void
setup_mixer (DlbAudioDecBin * decbin, const GstCaps * caps)
{
  GstAudioChannelPosition position[8];
  GstAudioInfo in;
  GstCaps *filter_caps;

  guint64 inchmask, outchmask;
  gint channels;
  GValue *matrix;
  gchar *tmp1, *tmp2;

  reset_mixer (decbin);

  if (decbin->outmode != DLB_AUDIO_DECODER_OUT_MODE_RAW &&
      decbin->outmode != DLB_AUDIO_DECODER_OUT_MODE_CORE)
    return;

  if (!gst_audio_info_from_caps (&in, caps))
    goto caps_error;

  gst_audio_channel_positions_to_mask (in.position, in.channels, TRUE,
      &inchmask);

  get_mixer_config (inchmask, in.channels, &outchmask, &channels);
  gst_audio_channel_positions_from_mask (channels, outchmask, position);

  tmp1 = gst_audio_channel_positions_to_string (in.position, in.channels);
  tmp2 = gst_audio_channel_positions_to_string (position, channels);

  GST_INFO_OBJECT (decbin, "setting mix matrix from %s to %s", tmp1, tmp2);

  matrix = generate_mix_matrix (in.channels, in.position, channels, position);

  filter_caps = gst_caps_new_simple ("audio/x-raw",
      "channels", G_TYPE_INT, channels,
      "channel-mask", GST_TYPE_BITMASK, outchmask, NULL);

  g_object_set (decbin->capsfilter, "caps", filter_caps, NULL);
  g_object_set_property (G_OBJECT (decbin->conv), "mix-matrix", matrix);

  gst_caps_unref (filter_caps);

  g_value_unset (matrix);
  g_free (matrix);
  g_free (tmp1);
  g_free (tmp2);

  return;

caps_error:
  GST_ERROR_OBJECT (decbin, "parsing caps failed");
}

static void
dlb_audio_dec_bin_sync_children_properties (DlbAudioDecBin * decbin)
{
  GstChildProxy *proxy = GST_CHILD_PROXY (GST_BIN (decbin));
  GValue val = G_VALUE_INIT;

  if (!decbin->dec)
    return;

  g_value_init (&val, DLB_TYPE_AUDIO_DECODER_OUT_MODE);
  g_value_set_enum (&val, decbin->outmode);
  gst_child_proxy_set_property (proxy, "decoder0::out-mode", &val);

  g_value_unset (&val);
  g_value_init (&val, DLB_TYPE_AUDIO_DECODER_DRC_MODE);
  g_value_set_enum (&val, decbin->drcmode);
  gst_child_proxy_set_property (proxy, "decoder0::drc-mode", &val);

  g_value_unset (&val);
  g_value_init (&val, G_TYPE_BOOLEAN);

  g_value_set_boolean (&val, decbin->dmxenable);
  gst_child_proxy_set_property (proxy, "decoder0::dmx-enable", &val);

  g_value_unset (&val);
  g_value_init (&val, G_TYPE_DOUBLE);

  g_value_set_double (&val, decbin->drccut);
  gst_child_proxy_set_property (proxy, "decoder0::drc-cut", &val);

  g_value_set_double (&val, decbin->drcboost);
  gst_child_proxy_set_property (proxy, "decoder0::drc-boost", &val);
}

static void
missing_element_print_info (DlbAudioDecBin * decbin, const gchar * factory)
{
  gst_element_post_message (GST_ELEMENT_CAST (decbin),
      gst_missing_element_message_new (GST_ELEMENT_CAST (decbin), factory));

  GST_ELEMENT_ERROR (decbin, CORE, MISSING_PLUGIN,
      ("Missing element '%s' - check your GStreamer installation.", factory),
      (NULL));
  GST_WARNING_OBJECT (decbin, "Creating internal elements failed");
}

static gboolean
dlb_audio_dec_bin_add_children (DlbAudioDecBin * decbin)
{
  GstPad *pad;
  const gchar *factory = NULL;

  factory = "typefind";
  if ((decbin->typefind = gst_element_factory_make (factory, NULL)) == NULL)
    goto missing_element;

  factory = "audioconvert";
  if ((decbin->conv = gst_element_factory_make (factory, NULL)) == NULL)
    goto missing_element;

  factory = "capsfilter";
  if ((decbin->capsfilter = gst_element_factory_make (factory, NULL)) == NULL)
    goto missing_element;

  gst_bin_add_many (GST_BIN (decbin), decbin->typefind, decbin->conv,
      decbin->capsfilter, NULL);

  if (!gst_element_link (decbin->conv, decbin->capsfilter))
    GST_ERROR_OBJECT (decbin, "couldn't link elements");

  pad = gst_element_get_static_pad (decbin->typefind, "sink");
  if (!gst_ghost_pad_set_target (GST_GHOST_PAD (decbin->sink), pad))
    GST_ERROR_OBJECT (decbin->src, "couldn't set sinkpad target");
  gst_object_unref (pad);

  pad = gst_element_get_static_pad (decbin->capsfilter, "src");
  if (!gst_ghost_pad_set_target (GST_GHOST_PAD (decbin->src), pad))
    GST_ERROR_OBJECT (decbin->src, "couldn't set srcpad target");
  gst_object_unref (pad);

  return TRUE;

missing_element:
  missing_element_print_info (decbin, factory);
  return FALSE;
}

static gboolean
dlb_audio_dec_bin_add_decoder_chain (DlbAudioDecBin * decbin)
{
  GstPad *srcpad;
  const gchar *decoder = NULL;
  const gchar *parser = NULL;

  if (!g_strcmp0 (decbin->stream, "audio/x-ac3") ||
      !g_strcmp0 (decbin->stream, "audio/x-eac3")) {
    decoder = "dlbac3dec";
    parser = "dlbac3parse";
  } else if (!g_strcmp0 (decbin->stream, "audio/x-ac4-raw")) {
    decoder = "dlbac4dec";
    parser = "dlbac4parse";
  } else if (!g_strcmp0 (decbin->stream, "audio/x-true-hd")) {
    decoder = "dlbtruehddec";
    parser = "dlbtruehdparse";
  } else if (!g_strcmp0 (decbin->stream, "audio/x-mat")) {
    decoder = "dlbmatdec";
    parser = "dlbmatparse";
  } else {
    g_assert_not_reached ();
  }

  decbin->parser = gst_element_factory_make (parser, "parser0");
  if (decbin->parser == NULL)
    goto missing_parser;

  decbin->dec = gst_element_factory_make (decoder, "decoder0");
  if (decbin->dec == NULL)
    goto missing_decoder;

  gst_bin_add_many (GST_BIN (decbin), decbin->parser, decbin->dec, NULL);
  gst_element_link_many (decbin->typefind, decbin->parser, decbin->dec, NULL);

  dlb_audio_dec_bin_sync_children_properties (decbin);

  srcpad = gst_element_get_static_pad (decbin->dec, "src");
  decbin->dec_probe_id = gst_pad_add_probe (srcpad,
      GST_PAD_PROBE_TYPE_EVENT_DOWNSTREAM,
      dlb_audio_dec_bin_on_dec_caps, decbin, NULL);
  gst_object_unref (srcpad);

  if (!gst_element_sync_state_with_parent (decbin->dec) ||
      !gst_element_sync_state_with_parent (decbin->parser)) {
    GST_ERROR_OBJECT (decbin, "Unable to sync children state with decbin");
    return FALSE;
  }

  return TRUE;

missing_parser:
  missing_element_print_info (decbin, parser);
  return FALSE;

missing_decoder:
  missing_element_print_info (decbin, decoder);
  return FALSE;
}

static GstPadProbeReturn
dlb_audio_dec_bin_on_oar_finish_flush (GstPad * pad, GstPadProbeInfo * info,
    gpointer user_data)
{
  DlbAudioDecBin *decbin = DLB_AUDIO_DEC_BIN_CAST (user_data);
  GstPad *srcpad;
  GstCaps *caps;

  if (GST_EVENT_TYPE (GST_PAD_PROBE_INFO_DATA (info)) != GST_EVENT_EOS)
    return GST_PAD_PROBE_PASS;

  gst_pad_remove_probe (pad, GST_PAD_PROBE_INFO_ID (info));
  gst_element_set_state (decbin->oar, GST_STATE_NULL);

  /* remove unlinks automatically */
  GST_DEBUG_OBJECT (decbin, "removing %" GST_PTR_FORMAT, decbin->oar);
  gst_bin_remove (GST_BIN (decbin), decbin->oar);
  decbin->oar = NULL;

  gst_element_link_many (decbin->dec, decbin->conv, decbin->capsfilter, NULL);
  gst_element_sync_state_with_parent (decbin->conv);

  srcpad = gst_element_get_static_pad (decbin->dec, "src");
  caps = gst_pad_get_current_caps (srcpad);

  setup_mixer (decbin, caps);

  gst_object_unref (srcpad);
  gst_caps_unref (caps);

  return GST_PAD_PROBE_DROP;
}

static GstPadProbeReturn
dlb_audio_dec_bin_on_oar_start_flush (GstPad * pad, GstPadProbeInfo * info,
    gpointer user_data)
{
  DlbAudioDecBin *decbin = DLB_AUDIO_DEC_BIN_CAST (user_data);
  GstPad *srcpad, *sinkpad;

  GST_DEBUG_OBJECT (decbin, "start flushing, pad is blocked");

  /* remove the probe first */
  gst_pad_remove_probe (pad, GST_PAD_PROBE_INFO_ID (info));

  /* install new probe for EOS */
  srcpad = gst_element_get_static_pad (decbin->oar, "src");
  gst_pad_add_probe (srcpad,
      GST_PAD_PROBE_TYPE_BLOCK | GST_PAD_PROBE_TYPE_EVENT_DOWNSTREAM,
      dlb_audio_dec_bin_on_oar_finish_flush, user_data, NULL);

  /* push EOS into the element, the probe will be fired when the
   * EOS leaves the effect and it has thus drained all of its data */
  sinkpad = gst_element_get_static_pad (decbin->oar, "sink");
  gst_pad_send_event (sinkpad, gst_event_new_eos ());

  gst_object_unref (srcpad);
  gst_object_unref (sinkpad);

  return GST_PAD_PROBE_OK;
}

static GstPadProbeReturn
dlb_audio_dec_bin_on_add_oar (GstPad * pad,
    GstPadProbeInfo * info, gpointer user_data)
{
  DlbAudioDecBin *decbin = DLB_AUDIO_DEC_BIN_CAST (user_data);

  GST_DEBUG_OBJECT (decbin, "adding oar, pad is blocked");

  /* remove the probe first, unlink elements */
  gst_pad_remove_probe (pad, GST_PAD_PROBE_INFO_ID (info));
  gst_element_unlink_many (decbin->dec, decbin->conv, decbin->capsfilter, NULL);

  decbin->oar = gst_element_factory_make ("dlboar", NULL);
  if (decbin->oar == NULL)
    goto missing_element;

  reset_mixer (decbin);

  gst_bin_add (GST_BIN (decbin), decbin->oar);
  gst_element_link_many (decbin->dec, decbin->oar, decbin->capsfilter, NULL);
  gst_element_sync_state_with_parent (decbin->oar);

  return GST_PAD_PROBE_DROP;

missing_element:
  missing_element_print_info (decbin, "dlboar");
  return GST_PAD_PROBE_DROP;
}

static GstPadProbeReturn
dlb_audio_dec_bin_on_dec_finish_flush (GstPad * pad, GstPadProbeInfo * info,
    gpointer user_data)
{
  DlbAudioDecBin *decbin = DLB_AUDIO_DEC_BIN_CAST (user_data);
  GstElement *peer;

  if (GST_EVENT_TYPE (GST_PAD_PROBE_INFO_DATA (info)) != GST_EVENT_EOS)
    return GST_PAD_PROBE_PASS;

  gst_pad_remove_probe (pad, GST_PAD_PROBE_INFO_ID (info));
  gst_element_set_state (decbin->parser, GST_STATE_NULL);
  gst_element_set_state (decbin->dec, GST_STATE_NULL);

  /* remove unlinks automatically */
  GST_DEBUG_OBJECT (decbin, "removing %" GST_PTR_FORMAT, decbin->dec);
  gst_bin_remove (GST_BIN (decbin), decbin->parser);
  gst_bin_remove (GST_BIN (decbin), decbin->dec);

  dlb_audio_dec_bin_add_decoder_chain (decbin);
  GST_DEBUG_OBJECT (decbin, "added  %" GST_PTR_FORMAT, decbin->dec);

  peer = decbin->oar ? decbin->oar : decbin->conv;
  gst_element_link (decbin->dec, peer);
  gst_element_set_state (decbin->dec, GST_STATE_PLAYING);

  return GST_PAD_PROBE_DROP;
}

static GstPadProbeReturn
dlb_audio_dec_bin_on_dec_start_flush (GstPad * pad, GstPadProbeInfo * info,
    gpointer user_data)
{
  DlbAudioDecBin *decbin = DLB_AUDIO_DEC_BIN_CAST (user_data);
  GstPad *srcpad, *sinkpad;

  GST_DEBUG_OBJECT (decbin, "start flushing, pad is blocked");

  /* remove the probe first */
  gst_pad_remove_probe (pad, GST_PAD_PROBE_INFO_ID (info));

  /* install new probe for EOS */
  srcpad = gst_element_get_static_pad (decbin->dec, "src");
  gst_pad_add_probe (srcpad,
      GST_PAD_PROBE_TYPE_BLOCK | GST_PAD_PROBE_TYPE_EVENT_DOWNSTREAM,
      dlb_audio_dec_bin_on_dec_finish_flush, user_data, NULL);

  /* push EOS into the element, the probe will be fired when the
   * EOS leaves the effect and it has thus drained all of its data */
  sinkpad = gst_element_get_static_pad (decbin->dec, "sink");
  gst_pad_send_event (sinkpad, gst_event_new_eos ());

  gst_object_unref (srcpad);
  gst_object_unref (sinkpad);

  return GST_PAD_PROBE_OK;
}

static GstPadProbeReturn
dlb_audio_dec_bin_on_dec_caps (GstPad * pad,
    GstPadProbeInfo * info, gpointer user_data)
{
  GstEvent *event = GST_PAD_PROBE_INFO_EVENT (info);
  DlbAudioDecBin *decbin = DLB_AUDIO_DEC_BIN_CAST (user_data);
  GstCaps *caps = NULL;
  GstCaps *peercaps = NULL;
  GstCapsFeatures *features = NULL;
  gboolean upstream_meta = FALSE;
  gboolean downstream_meta = FALSE;
  guint32 seqnum = gst_event_get_seqnum (event);

  if (GST_EVENT_TYPE (event) != GST_EVENT_CAPS)
    return GST_PAD_PROBE_OK;

  if (seqnum == decbin->caps_seqnum) {
    GST_DEBUG_OBJECT (pad, "CAPS event already handled");
    return GST_PAD_PROBE_OK;
  }

  decbin->caps_seqnum = seqnum;
  gst_event_parse_caps (event, &caps);

  if ((features = gst_caps_get_features (caps, 0))) {
    upstream_meta =
        gst_caps_features_contains (features,
        DLB_CAPS_FEATURE_META_OBJECT_AUDIO_META);
  }

  if ((peercaps = gst_pad_peer_query_caps (decbin->src, NULL))) {
    guint i, size = gst_caps_get_size (peercaps);

    for (i = 0; i < size; ++i) {
      if ((features = gst_caps_get_features (peercaps, i))) {
        downstream_meta =
            gst_caps_features_contains (features,
            DLB_CAPS_FEATURE_META_OBJECT_AUDIO_META);
      }
    }

    gst_caps_unref (peercaps);
  }

  /* downstream element can decode metadata we should pass it through */
  if (upstream_meta && downstream_meta) {
    gst_element_unlink (decbin->conv, decbin->capsfilter);
    gst_element_link (decbin->dec, decbin->capsfilter);
  } else if (upstream_meta && decbin->oar == NULL) {
    /* object based audio, there is no oar yet */
    gst_pad_add_probe (pad, GST_PAD_PROBE_TYPE_BLOCK_DOWNSTREAM,
        dlb_audio_dec_bin_on_add_oar, decbin, NULL);
  } else if (!upstream_meta && decbin->oar != NULL) {
    /* channel based audio, remove oar */
    gst_pad_add_probe (pad, GST_PAD_PROBE_TYPE_BLOCK_DOWNSTREAM,
        dlb_audio_dec_bin_on_oar_start_flush, decbin, NULL);
  } else if (!upstream_meta) {
    /* channel based audio */
    gst_element_link (decbin->dec, decbin->conv);
    setup_mixer (decbin, caps);
  }

  return GST_PAD_PROBE_OK;
}

static void
dlb_audio_dec_bin_on_type_found (GstElement * typefind, guint probability,
    GstCaps * caps, DlbAudioDecBin * decbin)
{
  gboolean reconf = FALSE;
  const gchar *stream;

  GstPadTemplate *dec_tmpl = gst_static_pad_template_get (&sink_template);
  GstCaps *dec_caps = gst_pad_template_get_caps (dec_tmpl);

  GstPad *sinkpad = gst_element_get_static_pad (typefind, "sink");
  GstPad *srcpad = gst_element_get_static_pad (typefind, "src");

  stream = gst_structure_get_name (gst_caps_get_structure (caps, 0));
  reconf = g_strcmp0 (stream, decbin->stream);

  GST_DEBUG_OBJECT (decbin, "typefind found caps %" GST_PTR_FORMAT, caps);

  if (!gst_caps_can_intersect (caps, dec_caps)) {
    GST_WARNING_OBJECT (decbin, "cannot handle type %s", stream);
    goto exit;
  }

  g_free (decbin->stream);
  decbin->stream = g_strdup (stream);

  GST_PAD_STREAM_LOCK (sinkpad);

  if (decbin->dec == NULL) {
    dlb_audio_dec_bin_add_decoder_chain (decbin);
  } else if (reconf) {
    gst_pad_add_probe (srcpad, GST_PAD_PROBE_TYPE_BLOCK_DOWNSTREAM,
        dlb_audio_dec_bin_on_dec_start_flush, decbin, NULL);
  }

  GST_PAD_STREAM_UNLOCK (sinkpad);

exit:
  gst_object_unref (srcpad);
  gst_object_unref (sinkpad);
  gst_object_unref (dec_tmpl);
  gst_caps_unref (dec_caps);
}

static gboolean
dlb_audio_dec_bin_start (DlbAudioDecBin * decbin)
{
  GST_DEBUG_OBJECT (decbin, "start");

  decbin->have_type_id = g_signal_connect (decbin->typefind, "have-type",
      G_CALLBACK (dlb_audio_dec_bin_on_type_found), decbin);

  return decbin->have_type_id ? TRUE : FALSE;
}

static void
dlb_audio_dec_bin_stop (DlbAudioDecBin * decbin)
{
  GST_DEBUG_OBJECT (decbin, "stop");

  if (decbin->have_type_id)
    g_signal_handler_disconnect (decbin->typefind, decbin->have_type_id);

  decbin->caps_seqnum = 0;
  decbin->have_type_id = 0;
  decbin->have_type = FALSE;
}

static gboolean
plugin_init (GstPlugin * plugin)
{
  guint rank = GST_RANK_PRIMARY + 2;

  GST_DEBUG_CATEGORY_INIT (dlb_audio_dec_bin_debug_category, "dlbaudiodecbin",
      0, "debug category for Dolby audio decoder bin");

  if (!gst_element_register (plugin, "dlbaudiodecbin", rank,
          DLB_TYPE_AUDIO_DEC_BIN))
    return FALSE;

  return TRUE;
}

GST_PLUGIN_DEFINE (GST_VERSION_MAJOR, GST_VERSION_MINOR, dlbaudiodecbin,
    "Decoder bin for Dolby audio formats", plugin_init, VERSION,
    LICENSE, PACKAGE, ORIGIN)
