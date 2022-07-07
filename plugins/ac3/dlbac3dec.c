/*******************************************************************************

 * Dolby Home Audio GStreamer Plugins
 * Copyright (C) 2020-2022, Dolby Laboratories

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

#include <gst/gst.h>

#include "dlbac3dec.h"
#include "dlbaudiometa.h"
#include "dlbutils.h"

GST_DEBUG_CATEGORY_STATIC (dlb_ac3dec_debug_category);
#define GST_CAT_DEFAULT dlb_ac3dec_debug_category

#define CHMASK(mask) (DLB_UDC_CHANNEL_MASK (mask))

/* public prototypes */
static void dlb_ac3dec_set_property (GObject * object,
    guint property_id, const GValue * value, GParamSpec * pspec);
static void dlb_ac3dec_get_property (GObject * object,
    guint property_id, GValue * value, GParamSpec * pspec);
static void dlb_ac3dec_dispose (GObject * object);
static void dlb_ac3dec_finalize (GObject * object);
static gboolean dlb_ac3dec_start (GstAudioDecoder * decoder);
static gboolean dlb_ac3dec_stop (GstAudioDecoder * decoder);
static gboolean dlb_ac3dec_set_format (GstAudioDecoder * decoder,
    GstCaps * caps);
static GstFlowReturn dlb_ac3dec_handle_frame (GstAudioDecoder * decoder,
    GstBuffer * inbuf);
static gboolean dlb_ac3dec_sink_event (GstAudioDecoder * dec, GstEvent * event);

/* private prototypes */
static gboolean restart (DlbAc3Dec * decoder);
static gboolean renegotiate (DlbAc3Dec * decoder,
    const dlb_udc_audio_info * info);
static gboolean update_static_params (DlbAc3Dec * decoder);
static gboolean update_dynamic_params (DlbAc3Dec * decoder);
static void evaluate_output_sample_format (DlbAc3Dec * decoder);
static void convert_dlb_udc_channel_mask_to_gst_pos (gint channel_mask,
    gint channels, GstAudioChannelPosition * pos, gboolean force_order);
static dlb_udc_output_mode get_udc_output_mode (DlbAudioDecoderOutMode outmode);

enum
{
  PROP_0,
  PROP_OUT_MODE,
  PROP_DRC_MODE,
  PROP_DRC_CUT,
  PROP_DRC_BOOST,
  PROP_DMX_ENABLE,
};

#define DLB_AC3DEC_SRC_CAPS                                             \
  "audio/x-raw, "                                                       \
    "format = (string) {"GST_AUDIO_NE (F32)", "GST_AUDIO_NE (F64)",     \
                        "GST_AUDIO_NE (S16)", "GST_AUDIO_NE (S32)" }, " \
    "channels = (int) [ 1, 16 ], "                                      \
    "rate = (int) { 32000, 44100, 48000 }, "                            \
    "layout = (string) { interleaved}; "                                \
  "audio/x-raw(" DLB_CAPS_FEATURE_META_OBJECT_AUDIO_META "),  "         \
    "format = (string) {"GST_AUDIO_NE (F32)", "GST_AUDIO_NE (F64)",     \
                        "GST_AUDIO_NE (S16)", "GST_AUDIO_NE (S32)" }, " \
    "channels = (int) [ 1, 16 ], "                                      \
    "rate = (int) { 32000, 44100, 48000 }, "                            \
    "layout = (string) { interleaved}; "                                \

#define DLB_AC3DEC_SINK_CAPS                                            \
  "audio/x-ac3, "                                                       \
    "framed = (boolean) true, "                                         \
    "rate = (int) [ 1, 655350 ]; "                                      \
  "audio/x-eac3, "                                                      \
    "framed = (boolean) true, "                                         \
    "rate = (int) [ 1, 655350 ]"                                        \

/* pad templates */
static GstStaticPadTemplate dlb_ac3dec_src_template =
GST_STATIC_PAD_TEMPLATE ("src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS (DLB_AC3DEC_SRC_CAPS)
    );

static GstStaticPadTemplate dlb_ac3dec_sink_template =
GST_STATIC_PAD_TEMPLATE ("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS (DLB_AC3DEC_SINK_CAPS)
    );

/* class initialization */
G_DEFINE_TYPE_WITH_CODE (DlbAc3Dec, dlb_ac3dec, GST_TYPE_AUDIO_DECODER,
    G_IMPLEMENT_INTERFACE (DLB_TYPE_AUDIO_DECODER, NULL));

static void
dlb_ac3dec_class_init (DlbAc3DecClass * klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  GstAudioDecoderClass *audio_decoder_class = GST_AUDIO_DECODER_CLASS (klass);

  gobject_class->set_property = dlb_ac3dec_set_property;
  gobject_class->get_property = dlb_ac3dec_get_property;
  gobject_class->dispose = dlb_ac3dec_dispose;
  gobject_class->finalize = dlb_ac3dec_finalize;
  audio_decoder_class->start = GST_DEBUG_FUNCPTR (dlb_ac3dec_start);
  audio_decoder_class->stop = GST_DEBUG_FUNCPTR (dlb_ac3dec_stop);
  audio_decoder_class->set_format = GST_DEBUG_FUNCPTR (dlb_ac3dec_set_format);
  audio_decoder_class->handle_frame =
      GST_DEBUG_FUNCPTR (dlb_ac3dec_handle_frame);
  audio_decoder_class->sink_event = GST_DEBUG_FUNCPTR (dlb_ac3dec_sink_event);

  /* Setting up pads and setting metadata should be moved to
     base_class_init if you intend to subclass this class. */
  gst_element_class_add_static_pad_template (GST_ELEMENT_CLASS (klass),
      &dlb_ac3dec_src_template);
  gst_element_class_add_static_pad_template (GST_ELEMENT_CLASS (klass),
      &dlb_ac3dec_sink_template);

  gst_element_class_set_static_metadata (GST_ELEMENT_CLASS (klass),
      "Dolby E-AC-3 and AC-3 Decoder", "Codec/Decoder/Audio",
      "Decode E-AC-3 and AC-3 audio stream",
      "Dolby Support <support@dolby.com>");

  /* install properties */
  g_object_class_override_property (gobject_class, PROP_OUT_MODE, "out-mode");
  g_object_class_override_property (gobject_class, PROP_DRC_MODE, "drc-mode");
  g_object_class_override_property (gobject_class, PROP_DRC_CUT, "drc-cut");
  g_object_class_override_property (gobject_class, PROP_DRC_BOOST, "drc-boost");
  g_object_class_override_property (gobject_class, PROP_DMX_ENABLE, "dmx-enable");

  gst_tag_register ("object-audio", GST_TAG_FLAG_META,
      G_TYPE_BOOLEAN, "object-audio tag",
      "a tag that indicates if object audio is present", NULL);
}

static void
dlb_ac3dec_init (DlbAc3Dec * ac3dec)
{
  ac3dec->outmode = DLB_AUDIO_DECODER_OUT_MODE_RAW;
  ac3dec->output_format = GST_AUDIO_FORMAT_F32LE;
  ac3dec->drc_mode = DLB_AUDIO_DECODER_DRC_MODE_DEFAULT;
  ac3dec->bps = 4;
  ac3dec->metadata_buffer = g_malloc (DLB_UDC_MAX_MD_SIZE);
  ac3dec->tags = gst_tag_list_new_empty ();
  ac3dec->dmx_enable = TRUE;

  dlb_udc_drc_settings_init (&ac3dec->drc);

  gst_audio_decoder_set_needs_format (GST_AUDIO_DECODER (ac3dec), TRUE);
  gst_audio_decoder_set_estimate_rate (GST_AUDIO_DECODER (ac3dec), TRUE);
  gst_audio_decoder_set_use_default_pad_acceptcaps (GST_AUDIO_DECODER_CAST
      (ac3dec), TRUE);

  GST_PAD_SET_ACCEPT_TEMPLATE (GST_AUDIO_DECODER_SINK_PAD (ac3dec));
}

static void
dlb_ac3dec_set_property (GObject * object, guint property_id,
    const GValue * value, GParamSpec * pspec)
{
  DlbAc3Dec *ac3dec = DLB_AC3DEC (object);
  gboolean update_static = FALSE, update_dynamic = FALSE;

  GST_OBJECT_LOCK (ac3dec);

  switch (property_id) {
    case PROP_OUT_MODE:
      ac3dec->outmode = g_value_get_enum (value);
      update_static = TRUE;
      break;
    case PROP_DRC_MODE:
      ac3dec->drc_mode = g_value_get_enum (value);
      update_dynamic = TRUE;
      break;
    case PROP_DRC_CUT:
      ac3dec->drc.cut = g_value_get_double (value);
      update_dynamic = TRUE;
      break;
    case PROP_DRC_BOOST:
      ac3dec->drc.boost = g_value_get_double (value);
      update_dynamic = TRUE;
      break;
    case PROP_DMX_ENABLE:
      ac3dec->dmx_enable = g_value_get_boolean (value);
      update_static = TRUE;
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }

  if (update_static && ac3dec->udc)
    update_static_params (ac3dec);

  if (update_dynamic && ac3dec->udc)
    update_dynamic_params (ac3dec);

  GST_OBJECT_UNLOCK (ac3dec);
}

static void
dlb_ac3dec_get_property (GObject * object,
    guint property_id, GValue * value, GParamSpec * pspec)
{
  DlbAc3Dec *ac3dec = DLB_AC3DEC (object);

  GST_DEBUG_OBJECT (ac3dec, "get_property");

  GST_OBJECT_LOCK (ac3dec);

  switch (property_id) {
    case PROP_OUT_MODE:
      g_value_set_enum (value, ac3dec->outmode);
      break;
    case PROP_DRC_MODE:
      g_value_set_enum (value, ac3dec->drc_mode);
      break;
    case PROP_DRC_CUT:
      g_value_set_double (value, ac3dec->drc.cut);
      break;
    case PROP_DRC_BOOST:
      g_value_set_double (value, ac3dec->drc.boost);
      break;
    case PROP_DMX_ENABLE:
      g_value_set_boolean (value, ac3dec->dmx_enable);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }

  GST_OBJECT_UNLOCK (ac3dec);
}

static void
dlb_ac3dec_dispose (GObject * object)
{
  G_OBJECT_CLASS (dlb_ac3dec_parent_class)->dispose (object);
}

static void
dlb_ac3dec_finalize (GObject * object)
{
  DlbAc3Dec *ac3dec = DLB_AC3DEC (object);

  GST_DEBUG_OBJECT (ac3dec, "Finalize");

  g_free (ac3dec->metadata_buffer);
  ac3dec->metadata_buffer = NULL;

  gst_tag_list_unref (ac3dec->tags);

  G_OBJECT_CLASS (dlb_ac3dec_parent_class)->finalize (object);
}

static dlb_udc_output_mode
get_udc_output_mode (DlbAudioDecoderOutMode outmode)
{
  switch (outmode) {
    case DLB_AUDIO_DECODER_OUT_MODE_2_0:
      return DLB_UDC_OUTPUT_MODE_2_0;
    case DLB_AUDIO_DECODER_OUT_MODE_2_1:
      return DLB_UDC_OUTPUT_MODE_2_1;
    case DLB_AUDIO_DECODER_OUT_MODE_3_0:
      return DLB_UDC_OUTPUT_MODE_3_0;
    case DLB_AUDIO_DECODER_OUT_MODE_3_1:
      return DLB_UDC_OUTPUT_MODE_3_1;
    case DLB_AUDIO_DECODER_OUT_MODE_4_0:
      return DLB_UDC_OUTPUT_MODE_4_0;
    case DLB_AUDIO_DECODER_OUT_MODE_4_1:
      return DLB_UDC_OUTPUT_MODE_4_1;
    case DLB_AUDIO_DECODER_OUT_MODE_5_0:
      return DLB_UDC_OUTPUT_MODE_5_0;
    case DLB_AUDIO_DECODER_OUT_MODE_5_1:
      return DLB_UDC_OUTPUT_MODE_5_1;
    case DLB_AUDIO_DECODER_OUT_MODE_6_0:
      return DLB_UDC_OUTPUT_MODE_6_0;
    case DLB_AUDIO_DECODER_OUT_MODE_6_1:
      return DLB_UDC_OUTPUT_MODE_6_1;
    case DLB_AUDIO_DECODER_OUT_MODE_7_0:
      return DLB_UDC_OUTPUT_MODE_7_0;
    case DLB_AUDIO_DECODER_OUT_MODE_7_1:
      return DLB_UDC_OUTPUT_MODE_7_1;
    case DLB_AUDIO_DECODER_OUT_MODE_CORE:
      return DLB_UDC_OUTPUT_MODE_CORE;
    case DLB_AUDIO_DECODER_OUT_MODE_RAW:
      return DLB_UDC_OUTPUT_MODE_RAW;
    default:
      return DLB_UDC_OUTPUT_MODE_2_0;
  }
}

static void
convert_dlb_udc_channel_mask_to_gst_pos (gint channel_mask, gint channels,
    GstAudioChannelPosition * pos, gboolean force_order)
{
  gint channel = 0;

  if (0 == channel_mask && 1 == channels) {
    pos[0] = GST_AUDIO_CHANNEL_POSITION_MONO;
    return;
  }

  if (channel_mask & CHMASK (LEFT))
    pos[channel++] = GST_AUDIO_CHANNEL_POSITION_FRONT_LEFT;
  if (channel_mask & CHMASK (RIGHT))
    pos[channel++] = GST_AUDIO_CHANNEL_POSITION_FRONT_RIGHT;
  if (channel_mask & CHMASK (CENTER))
    pos[channel++] = GST_AUDIO_CHANNEL_POSITION_FRONT_CENTER;
  if (channel_mask & CHMASK (LFE))
    pos[channel++] = GST_AUDIO_CHANNEL_POSITION_LFE1;
  if (channel_mask & CHMASK (SIDE_LEFT))
    pos[channel++] = GST_AUDIO_CHANNEL_POSITION_SIDE_LEFT;
  if (channel_mask & CHMASK (SIDE_RIGHT))
    pos[channel++] = GST_AUDIO_CHANNEL_POSITION_SIDE_RIGHT;
  if (channel_mask & CHMASK (BACK_LEFT))
    pos[channel++] = GST_AUDIO_CHANNEL_POSITION_REAR_LEFT;
  if (channel_mask & CHMASK (BACK_RIGHT))
    pos[channel++] = GST_AUDIO_CHANNEL_POSITION_REAR_RIGHT;
  if (channel_mask & CHMASK (CENTER_FRONT_LEFT))
    pos[channel++] = GST_AUDIO_CHANNEL_POSITION_FRONT_LEFT_OF_CENTER;
  if (channel_mask & CHMASK (CENTER_FRONT_RIGHT))
    pos[channel++] = GST_AUDIO_CHANNEL_POSITION_FRONT_RIGHT_OF_CENTER;
  if (channel_mask & CHMASK (BACK_CENTER))
    pos[channel++] = GST_AUDIO_CHANNEL_POSITION_REAR_CENTER;
  if (channel_mask & CHMASK (TOP_SURROUND))
    pos[channel++] = GST_AUDIO_CHANNEL_POSITION_TOP_CENTER;
  if (channel_mask & CHMASK (SIDE_DIRECT_LEFT))
    pos[channel++] = GST_AUDIO_CHANNEL_POSITION_SIDE_LEFT;
  if (channel_mask & CHMASK (SIDE_DIRECT_RIGHT))
    pos[channel++] = GST_AUDIO_CHANNEL_POSITION_SIDE_RIGHT;
  if (channel_mask & CHMASK (WIDE_LEFT))
    pos[channel++] = GST_AUDIO_CHANNEL_POSITION_WIDE_LEFT;
  if (channel_mask & CHMASK (WIDE_RIGHT))
    pos[channel++] = GST_AUDIO_CHANNEL_POSITION_WIDE_RIGHT;
  if (channel_mask & CHMASK (VERTICAL_HEIGHT_LEFT))
    pos[channel++] = GST_AUDIO_CHANNEL_POSITION_TOP_FRONT_LEFT;
  if (channel_mask & CHMASK (VERTICAL_HEIGHT_RIGHT))
    pos[channel++] = GST_AUDIO_CHANNEL_POSITION_TOP_FRONT_RIGHT;
  if (channel_mask & CHMASK (VERTICAL_HEIGHT_CENTER))
    pos[channel++] = GST_AUDIO_CHANNEL_POSITION_TOP_FRONT_CENTER;
  if (channel_mask & CHMASK (TOP_SURROUND_LEFT))
    pos[channel++] = GST_AUDIO_CHANNEL_POSITION_TOP_SIDE_LEFT;
  if (channel_mask & CHMASK (TOP_SURROUND_RIGHT))
    pos[channel++] = GST_AUDIO_CHANNEL_POSITION_TOP_SIDE_RIGHT;
  if (channel_mask & CHMASK (LFE2))
    pos[channel++] = GST_AUDIO_CHANNEL_POSITION_LFE2;

  g_assert (channel == channels);

  if (force_order)
    gst_audio_channel_positions_to_valid_order (pos, channels);
}

static gboolean
dlb_ac3dec_start (GstAudioDecoder * decoder)
{
  DlbAc3Dec *ac3dec = DLB_AC3DEC (decoder);
  GstAllocationParams alloc_params;

  GstAudioInfo audio_info;
  dlb_udc_init_info init_info;

  gint data_type;

  GST_DEBUG_OBJECT (ac3dec, "start");

  /* Base on src pad peer caps evaluates sample format and size */
  evaluate_output_sample_format (ac3dec);

  switch (ac3dec->output_format) {
    case GST_AUDIO_FORMAT_F32:
      data_type = DLB_BUFFER_FLOAT;
      break;
    case GST_AUDIO_FORMAT_F64:
      data_type = DLB_BUFFER_DOUBLE;
      break;
    case GST_AUDIO_FORMAT_S32:
      data_type = DLB_BUFFER_INT_LEFT;
      break;
    case GST_AUDIO_FORMAT_S16:
      data_type = DLB_BUFFER_SHORT_16;
      break;
    default:
      g_assert_not_reached ();
  }

  memset (&ac3dec->info, 0, sizeof (ac3dec->info));
  memset (&ac3dec->gstpos, 0, sizeof (ac3dec->gstpos));
  memset (&ac3dec->dlbpos, 0, sizeof (ac3dec->dlbpos));

  /* Get allocator from decoder base class and set memory alignment */
  gst_audio_decoder_get_allocator (&ac3dec->base_ac3dec, &ac3dec->alloc_dec,
      &alloc_params);
  ac3dec->alloc_params = gst_allocation_params_copy (&alloc_params);
  ac3dec->alloc_params->align = DLB_UDC_OUTBUF_MEMORY_ALIGNMENT - 1;

  init_info.outmode = get_udc_output_mode (ac3dec->outmode);
  init_info.dmx_enable = ac3dec->dmx_enable;

  ac3dec->udc = dlb_udc_new (&init_info);
  if (!ac3dec->udc)
    goto lib_error;

  ac3dec->max_channels = dlb_udc_query_max_output_channels (init_info.outmode);
  ac3dec->max_output_blocksz =
      dlb_udc_query_max_outbuf_size (init_info.outmode, data_type);

  gst_audio_info_init (&audio_info);
  gst_audio_info_set_format (&audio_info, ac3dec->output_format, 48000,
      ac3dec->max_channels, NULL);

  ac3dec->outbuf = dlb_buffer_new (&audio_info);
  if (!ac3dec->outbuf)
    goto buf_error;

  update_dynamic_params (ac3dec);
  return TRUE;

buf_error:
  dlb_udc_free (ac3dec->udc);
  ac3dec->udc = NULL;

lib_error:
  GST_ELEMENT_ERROR (ac3dec, LIBRARY, INIT, (NULL), ("Failed to open UDC"));
  return FALSE;
}

static gboolean
dlb_ac3dec_stop (GstAudioDecoder * decoder)
{
  DlbAc3Dec *ac3dec = DLB_AC3DEC (decoder);
  gboolean res = TRUE;

  GST_DEBUG_OBJECT (ac3dec, "stop");

  if (!ac3dec->udc)
    return TRUE;

  ac3dec->max_output_blocksz = 0;
  ac3dec->max_channels = 0;

  gst_allocation_params_free (ac3dec->alloc_params);
  ac3dec->alloc_params = NULL;

  dlb_buffer_free (ac3dec->outbuf);
  ac3dec->outbuf = NULL;

  dlb_udc_free (ac3dec->udc);
  ac3dec->udc = NULL;

  return res;
}

static gboolean
dlb_ac3dec_set_format (GstAudioDecoder * decoder, GstCaps * caps)
{
  DlbAc3Dec *ac3dec = DLB_AC3DEC (decoder);
  GstStructure *s;

  GST_DEBUG_OBJECT (ac3dec, "sink caps: %" GST_PTR_FORMAT, caps);

  s = gst_caps_get_structure (caps, 0);
  g_return_val_if_fail (s, FALSE);
  if (!gst_structure_has_name (s, "audio/x-ac3")
      && !gst_structure_has_name (s, "audio/x-eac3")) {
    GST_WARNING_OBJECT (ac3dec, "Unsupported stream type");
    g_return_val_if_reached (FALSE);
  }

  return TRUE;
}

static GstFlowReturn
dlb_ac3dec_handle_frame (GstAudioDecoder * decoder, GstBuffer * inbuf)
{
  DlbAc3Dec *ac3dec = DLB_AC3DEC (decoder);
  GstFlowReturn ret = GST_FLOW_OK;
  GstMapInfo inmap, outmap;
  GstBuffer *outbuf;

  dlb_udc_audio_info info;
  gint status;
  gsize blocksz = 0;
  gboolean update = FALSE;

  if (G_UNLIKELY (!inbuf)) {
    return GST_FLOW_OK;
  }

  /* calculate available input data size */
  GST_LOG_OBJECT (decoder, "handling input buffer %" GST_PTR_FORMAT, inbuf);
  gst_buffer_map (inbuf, &inmap, GST_MAP_READ);

  status =
      dlb_udc_push_timeslice (ac3dec->udc, (gchar *) inmap.data, inmap.size);
  if (status)
    goto push_error;

  for (gint i = 0; i < DLB_UDC_MAX_BLOCKS_PER_FRAME; ++i) {
    dlb_evo_payload md = {.data = ac3dec->metadata_buffer, };

    /* allocate output buffer */
    outbuf =
        gst_buffer_new_allocate (ac3dec->alloc_dec, ac3dec->max_output_blocksz,
        ac3dec->alloc_params);

    gst_buffer_map (outbuf, &outmap, GST_MAP_READWRITE);
    dlb_buffer_map_memory (ac3dec->outbuf, outmap.data);

    status =
        dlb_udc_process_block (ac3dec->udc, ac3dec->outbuf, &blocksz, &md,
        &info);
    if (G_UNLIKELY (status))
      goto decode_error;

    if (G_UNLIKELY (!blocksz)) {
      gst_buffer_unmap (outbuf, &outmap);
      gst_buffer_unref (outbuf);
      goto cleanup;
    }

    if (md.id == DLB_EVODEC_METADATA_ID_OAMD) {
      gsize bpf = ac3dec->bps * info.channels;
      dlb_audio_object_meta_add (outbuf, md.data, md.size, md.offset, bpf);
    }

    update = memcmp (&info, &ac3dec->info, sizeof (info));
    if (update) {
      renegotiate (ac3dec, &info);
    }

    if (!ac3dec->info.object_audio) {
      gst_audio_reorder_channels (outmap.data, blocksz, ac3dec->output_format,
          ac3dec->info.channels, ac3dec->dlbpos, ac3dec->gstpos);
    }

    gst_buffer_resize (outbuf, 0, blocksz);

    GST_LOG_OBJECT (decoder, "finish subframe %d", i);
    gst_buffer_unmap (outbuf, &outmap);
    ret = gst_audio_decoder_finish_subframe (decoder, outbuf);

    if (ret == GST_FLOW_ERROR) {
      GST_ERROR_OBJECT (ac3dec, "Finish subframe returned error %d", ret);
      goto cleanup;
    }
  }

cleanup:
  {
    gst_buffer_unmap (inbuf, &inmap);

    if (ret != GST_FLOW_ERROR) {
      GST_LOG_OBJECT (decoder, "finish frame");
      ret = gst_audio_decoder_finish_subframe (decoder, NULL);
    }

    return ret;
  }

push_error:
  {
    gst_buffer_unmap (inbuf, &inmap);

    GST_AUDIO_DECODER_ERROR (ac3dec, 1, STREAM, DECODE, (NULL),
        ("push timeslice error: %d", status), ret);

    return ret;
  }

decode_error:
  {
    gst_buffer_unmap (inbuf, &inmap);
    gst_buffer_unmap (outbuf, &outmap);
    gst_buffer_unref (outbuf);

    GST_AUDIO_DECODER_ERROR (ac3dec, 1, STREAM, DECODE, (NULL),
        ("process block error: %d", status), ret);

    return ret;
  }
}

gboolean
dlb_ac3dec_sink_event (GstAudioDecoder * dec, GstEvent * event)
{
  DlbAc3Dec *ac3dec = DLB_AC3DEC (dec);
  GstTagList *tags, *new_tags, *old_tags;

  GST_LOG_OBJECT (ac3dec, "sink_event");

  switch (GST_EVENT_TYPE (event)) {
    case GST_EVENT_TAG:
      GST_DEBUG_OBJECT (ac3dec, "Handle tag event");
      old_tags = ac3dec->tags;
      gst_event_parse_tag (event, &tags);
      new_tags = gst_tag_list_merge (old_tags, tags, GST_TAG_MERGE_REPLACE);
      ac3dec->tags = new_tags;
      if (old_tags)
        gst_tag_list_unref (old_tags);
      gst_event_unref (event);
      return TRUE;
    default:
      break;
  }

  return GST_AUDIO_DECODER_CLASS (dlb_ac3dec_parent_class)->sink_event (dec,
      event);
}

static gboolean
renegotiate (DlbAc3Dec * ac3dec, const dlb_udc_audio_info * info)
{
  GstAudioInfo audio_info;
  GstCaps *caps;
  GstCapsFeatures *features;
  GstEvent *event;

  GST_DEBUG_OBJECT (ac3dec, "output changed - generate new caps");

  if (!info->object_audio) {
    GST_DEBUG_OBJECT (ac3dec,
        "Channel-based decoding, channel-mask %" G_GUINT64_FORMAT
        ", channels = %d", info->channel_mask, info->channels);

    for (gint i = 0; i < DLB_UDC_MAX_RAW_OUTPUT_CHANNELS; ++i) {
      ac3dec->dlbpos[i] = GST_AUDIO_CHANNEL_POSITION_INVALID;
      ac3dec->gstpos[i] = GST_AUDIO_CHANNEL_POSITION_INVALID;
    }

    convert_dlb_udc_channel_mask_to_gst_pos (info->channel_mask,
        info->channels, ac3dec->dlbpos, FALSE);
    convert_dlb_udc_channel_mask_to_gst_pos (info->channel_mask,
        info->channels, ac3dec->gstpos, TRUE);

    {
      gchar *gstpos, *dlbpos;
      gstpos =
          gst_audio_channel_positions_to_string (ac3dec->gstpos,
          info->channels);
      dlbpos =
          gst_audio_channel_positions_to_string (ac3dec->dlbpos,
          info->channels);

      GST_DEBUG_OBJECT (ac3dec, "gstpos = %s, dlbpos = %s ", gstpos, dlbpos);

      g_free (gstpos);
      g_free (dlbpos);
    }
  } else {
    GST_DEBUG_OBJECT (ac3dec, "Atmos decoding");

    for (gint i = 0; i < DLB_UDC_MAX_RAW_OUTPUT_CHANNELS; ++i)
      ac3dec->gstpos[i] = GST_AUDIO_CHANNEL_POSITION_NONE;
  }

  gst_tag_list_add (ac3dec->tags, GST_TAG_MERGE_REPLACE, "object-audio",
      info->object_audio, NULL);

  /* Reconfigure audio decoder output format */
  gst_audio_info_init (&audio_info);
  gst_audio_info_set_format (&audio_info, ac3dec->output_format,
      info->rate, info->channels, ac3dec->gstpos);

  caps = gst_audio_info_to_caps (&audio_info);

  if (info->object_audio) {
    features =
        gst_caps_features_from_string (DLB_CAPS_FEATURE_META_OBJECT_AUDIO_META);
    gst_caps_set_features_simple (caps, features);
  }

  if (!gst_audio_decoder_set_output_caps (GST_AUDIO_DECODER (ac3dec), caps)) {
    GST_ERROR_OBJECT (ac3dec, "Audio decoder output format set failed");

    gst_caps_unref (caps);
    return FALSE;
  }

  event = gst_event_new_tag (gst_tag_list_copy (ac3dec->tags));
  gst_pad_push_event (GST_AUDIO_DECODER (ac3dec)->srcpad, event);

  ac3dec->info = *info;
  gst_caps_unref (caps);

  return TRUE;
}

static gboolean
restart (DlbAc3Dec * ac3dec)
{
  gboolean res = TRUE;

  GST_DEBUG_OBJECT (ac3dec, "restart");

  res = res & dlb_ac3dec_stop (GST_AUDIO_DECODER (ac3dec));
  res = res & dlb_ac3dec_start (GST_AUDIO_DECODER (ac3dec));

  return res;
}

static gboolean
update_static_params (DlbAc3Dec * ac3dec)
{
  return restart (ac3dec);
}

static gboolean
update_dynamic_params (DlbAc3Dec * ac3dec)
{
  dlb_udc_drc_settings drc;

  if (!ac3dec->udc) {
    GST_DEBUG_OBJECT (ac3dec, "UDC handle not created");
    return FALSE;
  }

  if (DLB_AUDIO_DECODER_DRC_MODE_DISABLE == ac3dec->drc_mode) {
    drc.cut = 0;
    drc.boost = 0;
  } else {
    drc.cut = ac3dec->drc.cut;
    drc.boost = ac3dec->drc.boost;
  }

  GST_DEBUG_OBJECT (ac3dec, "Dynamic settings: drc_boost %.2f, drc_cut %.2f",
      drc.boost, drc.cut);

  if (dlb_udc_drc_settings_set (ac3dec->udc, &drc)) {
    GST_WARNING_OBJECT (ac3dec, "UDC DRC settings failed.");
    return FALSE;
  }

  return TRUE;
}

void
evaluate_output_sample_format (DlbAc3Dec * ac3dec)
{
  GstCaps *filter_all, *filter_f32, *filter_f64, *filter_s32, *filter_s16;
  GstCaps *down_caps;

  filter_all =
      gst_caps_from_string ("audio/x-raw,format = (string) {" GST_AUDIO_NE (F32)
      ", " GST_AUDIO_NE (F64) ", " GST_AUDIO_NE (S16) ", " GST_AUDIO_NE (S32)
      " }");

  filter_f32 =
      gst_caps_from_string ("audio/x-raw,format=(string) " GST_AUDIO_NE (F32)
      "");
  filter_f64 =
      gst_caps_from_string ("audio/x-raw,format=(string) " GST_AUDIO_NE (F64)
      "");
  filter_s32 =
      gst_caps_from_string ("audio/x-raw,format=(string) " GST_AUDIO_NE (S32)
      "");
  filter_s16 =
      gst_caps_from_string ("audio/x-raw,format=(string) " GST_AUDIO_NE (S16)
      "");

  down_caps =
      gst_pad_peer_query_caps (GST_AUDIO_DECODER_SRC_PAD (ac3dec), filter_all);

  // default format and bps
  ac3dec->output_format = GST_AUDIO_FORMAT_F32;
  ac3dec->bps = 4;

  if (down_caps) {
    if (gst_caps_is_subset (down_caps, filter_f32)) {
      ac3dec->output_format = GST_AUDIO_FORMAT_F32;
      ac3dec->bps = 4;
    } else if (gst_caps_is_subset (down_caps, filter_f64)) {
      ac3dec->output_format = GST_AUDIO_FORMAT_F64;
      ac3dec->bps = 8;
    } else if (gst_caps_is_subset (down_caps, filter_s32)) {
      ac3dec->output_format = GST_AUDIO_FORMAT_S32;
      ac3dec->bps = 4;
    } else if (gst_caps_is_subset (down_caps, filter_s16)) {
      ac3dec->output_format = GST_AUDIO_FORMAT_S16;
      ac3dec->bps = 2;
    }
    gst_caps_unref (down_caps);
  }

  gst_caps_unref (filter_all);
  gst_caps_unref (filter_f32);
  gst_caps_unref (filter_f64);
  gst_caps_unref (filter_s32);
  gst_caps_unref (filter_s16);
}

static gboolean
plugin_init (GstPlugin * plugin)
{
#ifdef DLB_UDC_OPEN_DYNLIB
  if (dlb_udc_try_open_dynlib ())
    return FALSE;
#endif

  GST_DEBUG_CATEGORY_INIT (dlb_ac3dec_debug_category, "dlbac3dec", 0,
      "debug category for AC3 decoder element");

  if (!gst_element_register (plugin, "dlbac3dec", GST_RANK_PRIMARY,
          DLB_TYPE_AC3DEC))
    return FALSE;

  return TRUE;
}

GST_PLUGIN_DEFINE (GST_VERSION_MAJOR,
    GST_VERSION_MINOR,
    dlbac3dec,
    "Dolby AC-3 and E-AC-3 Decoder",
    plugin_init, VERSION, LICENSE, PACKAGE, ORIGIN)
