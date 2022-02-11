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

#include <gst/gst.h>
#include <gst/audio/audio.h>
#include <gst/base/gstbasetransform.h>

#include "dlboar.h"
#include "dlbaudiometa.h"
#include "dlbutils.h"

GST_DEBUG_CATEGORY_STATIC (dlb_oar_debug_category);
#define GST_CAT_DEFAULT dlb_oar_debug_category

#define ALLOWED_SRC_CAPS                                                \
  "audio/x-raw, "                                                       \
  "format = (string) {"GST_AUDIO_NE (F32)", "GST_AUDIO_NE (F64)",       \
                      "GST_AUDIO_NE (S16)", "GST_AUDIO_NE (S32)" }, "   \
  "channels = (int) [ 2, 35 ], "                                        \
  "rate = (int) { 48000, 32000, 44100, 88200, 96000 }, "                \
  "layout = (string) interleaved"


#define ALLOWED_SINK_CAPS                                               \
  "audio/x-raw(" DLB_CAPS_FEATURE_META_OBJECT_AUDIO_META "), "          \
  "format = (string) {"GST_AUDIO_NE (F32)", "GST_AUDIO_NE (F64)",       \
                      "GST_AUDIO_NE (S16)", "GST_AUDIO_NE (S32)" }, "   \
  "channels = (int) [ 1, 32 ], "                                        \
  "rate = (int) { 48000, 32000, 44100, 88200, 96000 }, "                \
  "layout = (string) interleaved"                                       \

/**
 * OARMSK:
 * @ch: Dolby Audio Speaker config mask sufix (like: L_R)
 * return: Full Dolby Audio Speaker config mask
 */
#define OARMSK(ch) (DLB_OAR_SPEAKER_CONFIG_MASK (ch))
#define GSTMSK(ch) (GST_AUDIO_CHANNEL_POSITION_MASK (ch))

/* prototypes */
static void dlb_oar_set_property (GObject * object,
    guint property_id, const GValue * value, GParamSpec * pspec);
static void dlb_oar_get_property (GObject * object,
    guint property_id, GValue * value, GParamSpec * pspec);
static GstCaps *dlb_oar_transform_caps (GstBaseTransform * trans,
    GstPadDirection direction, GstCaps * caps, GstCaps * filter);
static gboolean dlb_oar_set_caps (GstBaseTransform * trans,
    GstCaps * incaps, GstCaps * outcaps);
static gboolean dlb_oar_query (GstBaseTransform * trans,
    GstPadDirection direction, GstQuery * query);
static gboolean dlb_oar_transform_size (GstBaseTransform * trans,
    GstPadDirection direction, GstCaps * caps, gsize size, GstCaps * othercaps,
    gsize * othersize);
static gboolean dlb_oar_start (GstBaseTransform * trans);
static gboolean dlb_oar_stop (GstBaseTransform * trans);
static gboolean dlb_oar_sink_event (GstBaseTransform * trans, GstEvent * event);
static gboolean dlb_oar_src_event (GstBaseTransform * trans, GstEvent * event);
static GstFlowReturn dlb_oar_push_drain (DlbOar * oar);
static GstFlowReturn dlb_oar_transform (GstBaseTransform * trans,
    GstBuffer * inbuf, GstBuffer * outbuf);

/* helper functions definitions */
static gboolean oar_open (DlbOar * oar);
static void oar_close (DlbOar * oar);
static gboolean oar_restart (DlbOar * oar);
static gboolean oar_is_opened (DlbOar * oar);
static gint gst_buffer_get_oamd (DlbOar * oar, GstBuffer * buffer);
static void transform_data_block (DlbOar * oar, dlb_buffer * inbuf,
    dlb_buffer * outbuf, gint samples);

enum
{
  PROP_0,
  PROP_LIMITER_ENABLE,
  PROP_DISCARD_LATENCY,
};

/* pad templates */
static GstStaticPadTemplate dlb_oar_src_template =
GST_STATIC_PAD_TEMPLATE ("src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS (ALLOWED_SRC_CAPS)
    );

static GstStaticPadTemplate dlb_oar_sink_template =
GST_STATIC_PAD_TEMPLATE ("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS (ALLOWED_SINK_CAPS)
    );

/* class initialization */

G_DEFINE_TYPE_WITH_CODE (DlbOar, dlb_oar, GST_TYPE_BASE_TRANSFORM,
    GST_DEBUG_CATEGORY_INIT (dlb_oar_debug_category, "dlboar", 0,
        "debug category for oar element"));

static void
dlb_oar_class_init (DlbOarClass * klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  GstBaseTransformClass *base_transform_class =
      GST_BASE_TRANSFORM_CLASS (klass);

  /* Setting up pads and setting metadata should be moved to
     base_class_init if you intend to subclass this class. */
  gst_element_class_add_static_pad_template (GST_ELEMENT_CLASS (klass),
      &dlb_oar_src_template);
  gst_element_class_add_static_pad_template (GST_ELEMENT_CLASS (klass),
      &dlb_oar_sink_template);

  gst_element_class_set_static_metadata (GST_ELEMENT_CLASS (klass),
      "Object Audio Renderer plugin", "Audio",
      "Plugin for rendering Dolby Object Audio",
      "Dolby Support <support@dolby.com>");

  gobject_class->set_property = GST_DEBUG_FUNCPTR (dlb_oar_set_property);
  gobject_class->get_property = GST_DEBUG_FUNCPTR (dlb_oar_get_property);
  base_transform_class->transform_caps =
      GST_DEBUG_FUNCPTR (dlb_oar_transform_caps);
  base_transform_class->set_caps = GST_DEBUG_FUNCPTR (dlb_oar_set_caps);
  base_transform_class->query = GST_DEBUG_FUNCPTR (dlb_oar_query);
  base_transform_class->transform_size =
      GST_DEBUG_FUNCPTR (dlb_oar_transform_size);
  base_transform_class->start = GST_DEBUG_FUNCPTR (dlb_oar_start);
  base_transform_class->stop = GST_DEBUG_FUNCPTR (dlb_oar_stop);
  base_transform_class->sink_event = GST_DEBUG_FUNCPTR (dlb_oar_sink_event);
  base_transform_class->src_event = GST_DEBUG_FUNCPTR (dlb_oar_src_event);
  base_transform_class->transform = GST_DEBUG_FUNCPTR (dlb_oar_transform);

  /* install properties */
  g_object_class_install_property (gobject_class, PROP_LIMITER_ENABLE,
      g_param_spec_boolean ("limiter-enable",
          "Object audio renderer limiter",
          "Enable object audio renderer limiter", TRUE,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class,
      PROP_DISCARD_LATENCY,
      g_param_spec_boolean ("discard-latency",
          "Discard latency",
          "Discard initial latency zeros from the output", FALSE,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
}

static void
dlb_oar_init (DlbOar * oar)
{
  oar->oar_instance = NULL;
  oar->oamd_payloads = NULL;
  oar->adapter = NULL;
  oar->max_payloads = 0;
  oar->max_block_size = 0;
  oar->min_block_size = 0;
  oar->latency = 0;
  oar->prefill = 0;
  oar->latency_samples = 0;
  oar->latency_time = GST_CLOCK_TIME_NONE;
  oar->discard_latency = FALSE;

  oar->oar_config.speaker_mask = 0;
  oar->oar_config.sample_rate = 0;
  oar->oar_config.limiter_enable = 1;
}

void
dlb_oar_set_property (GObject * object, guint property_id,
    const GValue * value, GParamSpec * pspec)
{
  DlbOar *oar = DLB_OAR (object);
  gboolean reneg = FALSE;

  GST_DEBUG_OBJECT (oar, "set_property");
  GST_OBJECT_LOCK (oar);

  switch (property_id) {
    case PROP_LIMITER_ENABLE:
      oar->oar_config.limiter_enable = g_value_get_boolean (value);
      reneg = TRUE;
      break;
    case PROP_DISCARD_LATENCY:
      oar->discard_latency = g_value_get_boolean (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }

  GST_OBJECT_UNLOCK (oar);

  if (reneg && oar_is_opened (oar)) {
    oar_restart (oar);
    gst_base_transform_reconfigure_src (GST_BASE_TRANSFORM_CAST (oar));
  }
}

void
dlb_oar_get_property (GObject * object, guint property_id,
    GValue * value, GParamSpec * pspec)
{
  DlbOar *oar = DLB_OAR (object);

  GST_DEBUG_OBJECT (oar, "get_property");
  GST_OBJECT_LOCK (oar);

  switch (property_id) {
    case PROP_LIMITER_ENABLE:
      g_value_set_boolean (value, oar->oar_config.limiter_enable);
      break;
    case PROP_DISCARD_LATENCY:
      g_value_set_boolean (value, oar->discard_latency);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }

  GST_OBJECT_UNLOCK (oar);
}

static guint64
get_channel_mask_from_caps (GstCaps * caps)
{
  GstStructure *s;
  guint64 mask = 0;

  if ((s = gst_caps_get_structure (caps, 0)))
    gst_structure_get (s, "channel-mask", GST_TYPE_BITMASK, &mask, NULL);

  return mask;
}

static gboolean
oar_is_opened (DlbOar * oar)
{
  return (oar->oar_instance != NULL);
}

static guint64
channel_mask_to_oar_speaker_config (const guint64 gst_chmask)
{
  guint64 msk = 0;

  if (gst_chmask & GSTMSK (FRONT_CENTER))
    msk |= OARMSK (C);

  if (gst_chmask & GSTMSK (LFE1))
    msk |= OARMSK (LFE);

  if ((gst_chmask & GSTMSK (FRONT_LEFT))
      && (gst_chmask & GSTMSK (FRONT_RIGHT)))
    msk |= OARMSK (L_R);

  if ((gst_chmask & GSTMSK (SIDE_LEFT)) && (gst_chmask & GSTMSK (SIDE_RIGHT)))
    msk |= OARMSK (LS_RS);

  if ((gst_chmask & GSTMSK (REAR_LEFT)) && (gst_chmask & GSTMSK (REAR_RIGHT)))
    msk |= (msk & OARMSK (LS_RS)) ? OARMSK (LRS_RRS) : OARMSK (LS_RS);

  if ((gst_chmask & GSTMSK (TOP_FRONT_LEFT)) &&
      (gst_chmask & GSTMSK (TOP_FRONT_RIGHT)))
    msk |= OARMSK (LTF_RTF);

  if ((gst_chmask & GSTMSK (TOP_SIDE_LEFT))
      && (gst_chmask & GSTMSK (TOP_SIDE_RIGHT)))
    msk |= OARMSK (LTM_RTM);

  if ((gst_chmask & GSTMSK (TOP_REAR_LEFT)) &&
      (gst_chmask & GSTMSK (TOP_REAR_RIGHT)))
    msk |= OARMSK (LTR_RTR);

  if ((gst_chmask & GSTMSK (WIDE_LEFT)) && (gst_chmask & GSTMSK (WIDE_RIGHT)))
    msk |= OARMSK (LW_RW);

  if ((gst_chmask & GSTMSK (FRONT_LEFT_OF_CENTER)) &&
      (gst_chmask & GSTMSK (FRONT_RIGHT_OF_CENTER)))
    msk |= OARMSK (LC_RC);

  if ((gst_chmask & GSTMSK (SURROUND_LEFT))
      && (gst_chmask & GSTMSK (SURROUND_RIGHT)))
    msk |= OARMSK (LRS1_RRS1);

  if (gst_chmask & GSTMSK (REAR_CENTER))
    msk |= OARMSK (CS);

  return msk;
}

static GstCaps *
dlb_oar_transform_caps (GstBaseTransform * trans, GstPadDirection direction,
    GstCaps * caps, GstCaps * filter)
{
  DlbOar *oar = DLB_OAR (trans);
  GstCaps *othercaps;
  GstStructure *base, *s;

  /* Simply return pad template for given direction */
  if (direction == GST_PAD_SRC) {
    /* transform caps going upstream */
    othercaps = gst_static_pad_template_get_caps (&dlb_oar_sink_template);
  } else {
    /* transform caps going downstream */
    othercaps = gst_static_pad_template_get_caps (&dlb_oar_src_template);
  }

  othercaps = gst_caps_make_writable (othercaps);

  if (!gst_caps_is_empty (caps) && (s = gst_caps_get_structure (caps, 0))) {
    const GValue *format, *rate;
    base = gst_caps_get_structure (othercaps, 0);

    if ((format = gst_structure_get_value (s, "format")))
      gst_structure_set_value (base, "format", format);

    if ((rate = gst_structure_get_value (s, "rate")))
      gst_structure_set_value (base, "rate", rate);
  }

  GST_DEBUG_OBJECT (oar,
      "transformed %" GST_PTR_FORMAT " into %" GST_PTR_FORMAT, caps, othercaps);

  if (filter) {
    GstCaps *intersect;

    GST_DEBUG_OBJECT (oar, "Using filter caps %" GST_PTR_FORMAT, filter);
    intersect = gst_caps_intersect (othercaps, filter);
    gst_caps_unref (othercaps);
    GST_DEBUG_OBJECT (oar, "Intersection %" GST_PTR_FORMAT, intersect);

    return intersect;
  } else {
    return othercaps;
  }
}

static gboolean
dlb_oar_set_caps (GstBaseTransform * trans, GstCaps * incaps, GstCaps * outcaps)
{
  DlbOar *oar = DLB_OAR (trans);
  GstAudioInfo in, out;

  gboolean ret;
  gint rate, channels;
  gsize latency;
  guint64 channel_mask, oar_mask;
  gchar *tmp;

  GST_DEBUG_OBJECT (oar, "incaps %" GST_PTR_FORMAT ", outcaps %" GST_PTR_FORMAT,
      incaps, outcaps);

  gst_audio_info_init (&in);
  gst_audio_info_init (&out);

  if (!gst_audio_info_from_caps (&in, incaps))
    goto incaps_error;
  if (!gst_audio_info_from_caps (&out, outcaps))
    goto outcaps_error;

  rate = GST_AUDIO_INFO_RATE (&in);
  channels = GST_AUDIO_INFO_CHANNELS (&out);
  channel_mask = get_channel_mask_from_caps (outcaps);

  if (!channel_mask)
    channel_mask = gst_audio_channel_get_fallback_mask (channels);

  oar_mask = channel_mask_to_oar_speaker_config (channel_mask);

  tmp = gst_audio_channel_positions_to_string (out.position, channels);
  GST_INFO_OBJECT (oar, "oar output configuration: %s", tmp);
  g_free (tmp);

  if (oar_is_opened (oar)) {
    if (oar->oar_config.sample_rate != rate) {
      oar->oar_config.sample_rate = rate;
      dlb_oar_reset (oar->oar_instance, rate);
    }

    if (oar->oar_config.speaker_mask != oar_mask) {
      oar->oar_config.speaker_mask = oar_mask;

      if ((ret = oar_restart (oar)) == FALSE)
        goto open_error;
    }
  } else {
    oar->oar_config.sample_rate = rate;
    oar->oar_config.speaker_mask = oar_mask;

    if ((ret = oar_open (oar)) == FALSE)
      goto open_error;
  }

  oar->max_block_size =
      dlb_oar_query_max_block_samples (oar->oar_instance) *
      GST_AUDIO_INFO_BPF (&in);
  oar->min_block_size =
      dlb_oar_query_min_block_samples (oar->oar_instance) *
      GST_AUDIO_INFO_BPF (&in);

  latency = dlb_oar_query_latency (oar->oar_instance);

  if (!oar->discard_latency)
    latency = 0;

  oar->latency = latency * GST_AUDIO_INFO_BPF (&out);
  oar->prefill = latency * GST_AUDIO_INFO_BPF (&in);
  oar->latency_samples = latency;
  oar->latency_time = gst_util_uint64_scale_int (latency, GST_SECOND, in.rate);

  oar->ininfo = in;
  oar->outinfo = out;

  return TRUE;

  /* ERROR */
incaps_error:
  GST_ERROR_OBJECT (trans, "invalid incaps");
  return FALSE;

outcaps_error:
  GST_ERROR_OBJECT (trans, "invalid outcaps");
  return FALSE;

open_error:
  oar_close (oar);
  return FALSE;
}

static gboolean
dlb_oar_query (GstBaseTransform * trans, GstPadDirection direction,
    GstQuery * query)
{
  DlbOar *oar = DLB_OAR (trans);

  switch (GST_QUERY_TYPE (query)) {
    case GST_QUERY_LATENCY:{
      if ((gst_pad_peer_query (GST_BASE_TRANSFORM_SINK_PAD (trans), query))) {
        GstClockTime min, max;
        gboolean live;
        guint64 latency = oar->latency_samples;
        gint rate = oar->ininfo.rate;

        if (!rate)
          return FALSE;

        if (gst_base_transform_is_passthrough (trans))
          latency = 0;

        latency = gst_util_uint64_scale_round (latency, GST_SECOND, rate);
        gst_query_parse_latency (query, &live, &min, &max);

        GST_DEBUG_OBJECT (oar, "Peer latency: min %"
            GST_TIME_FORMAT " max %" GST_TIME_FORMAT,
            GST_TIME_ARGS (min), GST_TIME_ARGS (max));

        /* add our own latency */
        GST_DEBUG_OBJECT (oar, "Our latency: %" GST_TIME_FORMAT,
            GST_TIME_ARGS (latency));
        min += latency;
        if (GST_CLOCK_TIME_IS_VALID (max))
          max += latency;

        GST_DEBUG_OBJECT (oar, "Calculated total latency : min %"
            GST_TIME_FORMAT " max %" GST_TIME_FORMAT,
            GST_TIME_ARGS (min), GST_TIME_ARGS (max));
        gst_query_set_latency (query, live, min, max);
      }

      return TRUE;
    }
    default:{
      return GST_BASE_TRANSFORM_CLASS (dlb_oar_parent_class)->query (trans,
          direction, query);
    }
  }
}

/* transform size */
static gboolean
dlb_oar_transform_size (GstBaseTransform * trans, GstPadDirection direction,
    GstCaps * caps, gsize size, GstCaps * othercaps, gsize * othersize)
{
  DlbOar *oar = DLB_OAR (trans);
  gint blocks;

  if (GST_PAD_SRC == direction) {
    *othersize = oar->max_block_size;
  } else {
    gsize adapter_size = gst_adapter_available (oar->adapter);

    blocks = (size + adapter_size) / oar->min_block_size;
    *othersize = blocks * oar->min_block_size;

    GST_LOG_OBJECT (oar,
        "adapter size: %" G_GSIZE_FORMAT ", input size: %" G_GSIZE_FORMAT,
        adapter_size, size);
  }

  GST_LOG_OBJECT (oar, "transform_size: %" G_GSIZE_FORMAT, *othersize);

  return TRUE;
}

/* states */
static gboolean
dlb_oar_start (GstBaseTransform * trans)
{
  DlbOar *oar = DLB_OAR (trans);
  GST_DEBUG_OBJECT (oar, "start");

  return TRUE;
}

static gboolean
dlb_oar_stop (GstBaseTransform * trans)
{
  DlbOar *oar = DLB_OAR (trans);
  GST_DEBUG_OBJECT (oar, "stop");

  oar_close (oar);

  oar->max_payloads = 0;
  oar->max_block_size = 0;
  oar->min_block_size = 0;
  oar->latency = 0;
  oar->prefill = 0;
  oar->latency_samples = 0;
  oar->latency_time = GST_CLOCK_TIME_NONE;

  oar->oar_config.speaker_mask = 0;
  oar->oar_config.sample_rate = 0;

  return TRUE;
}

/* sink and src pad event handlers */
static gboolean
dlb_oar_sink_event (GstBaseTransform * trans, GstEvent * event)
{
  DlbOar *oar = DLB_OAR (trans);

  GST_DEBUG_OBJECT (oar, "sink_event");

  switch (GST_EVENT_TYPE (event)) {
    case GST_EVENT_FLUSH_STOP:
    case GST_EVENT_EOS:
      if (oar_is_opened (oar))
        dlb_oar_push_drain (oar);
      break;
    default:
      break;
  }

  return GST_BASE_TRANSFORM_CLASS (dlb_oar_parent_class)->sink_event (trans,
      event);
}

static gboolean
dlb_oar_src_event (GstBaseTransform * trans, GstEvent * event)
{
  DlbOar *oar = DLB_OAR (trans);

  GST_LOG_OBJECT (oar, "src_event");

  return GST_BASE_TRANSFORM_CLASS (dlb_oar_parent_class)->src_event (trans,
      event);
}

static gint
gst_buffer_get_oamd (DlbOar * oar, GstBuffer * buf)
{
  DlbObjectAudioMeta *meta;

  GList *list = NULL;
  gpointer it = NULL;

  guint num_payloads = 0;
  gint scale = GST_AUDIO_INFO_RATE (&oar->ininfo) > 48000 ? 2 : 1;

  /* clear OAMD metadata tables */
  memset (oar->oamd_payloads, 0, sizeof (dlb_oar_payload) * oar->max_payloads);

  num_payloads = dlb_audio_object_meta_get_n (buf);
  GST_LOG_OBJECT (oar, "num_payloads=%d", num_payloads);

  if (num_payloads == 0)
    return 0;

  if (num_payloads > oar->max_payloads) {
    GST_WARNING_OBJECT (oar, "Discarding OAMD");
    num_payloads = oar->max_payloads;
  }

  while ((meta = dlb_audio_object_meta_iterate (buf, &it))) {
    list = g_list_prepend (list, meta);
  }

  list = g_list_sort (list, (GCompareFunc) gst_meta_compare_seqnum);

  for (gint i = 0; i < num_payloads; ++i) {
    meta = (DlbObjectAudioMeta *) g_list_nth_data (list, i);

    oar->oamd_payloads[i].data = meta->payload;
    oar->oamd_payloads[i].size = meta->size;
    oar->oamd_payloads[i].sample_offset = meta->offset * scale;
  }

  g_list_free (list);
  return num_payloads;
}

static void
transform_data_block (DlbOar * oar, dlb_buffer * inbuf, dlb_buffer * outbuf,
    gint samples)
{
  GST_LOG_OBJECT (oar,
      "transform in_ch=%d, out_ch=%d, block_size=%d",
      GST_AUDIO_INFO_CHANNELS (&oar->ininfo),
      GST_AUDIO_INFO_CHANNELS (&oar->outinfo), samples);

  dlb_oar_process (oar->oar_instance, inbuf, outbuf, samples);
}

static gsize
get_next_block_size (DlbOar * oar)
{
  gsize size = MIN (oar->max_block_size, gst_adapter_available (oar->adapter));

  size /= oar->min_block_size;
  size *= oar->min_block_size;
  return size;
}

static void
dlb_oar_get_input_timing (DlbOar * oar, GstClockTime * timestamp,
    guint64 * offset)
{
  GstClockTime ts;
  guint64 off, distance;

  ts = gst_adapter_prev_pts (oar->adapter, &distance);
  if (ts != GST_CLOCK_TIME_NONE) {
    distance /= oar->ininfo.bpf;
    ts += gst_util_uint64_scale_int (distance, GST_SECOND, oar->ininfo.rate);
  }

  off = gst_adapter_prev_offset (oar->adapter, &distance);
  if (off != GST_CLOCK_TIME_NONE) {
    distance /= oar->ininfo.bpf;
    off += distance;
  }

  *timestamp = ts;
  *offset = off;
}

static void
dlb_oar_set_output_timing (DlbOar * oar, GstBuffer * outbuf,
    GstClockTime timestamp, guint64 offset)
{
  gsize outsize = gst_buffer_get_size (outbuf);
  gint outsamples = outsize / oar->outinfo.bpf;

  GST_BUFFER_PTS (outbuf) = timestamp;
  GST_BUFFER_DURATION (outbuf) =
      gst_util_uint64_scale_int (outsamples, GST_SECOND, oar->outinfo.rate);

  GST_BUFFER_OFFSET (outbuf) = offset;
  GST_BUFFER_OFFSET_END (outbuf) = offset + outsamples;
}

static GstFlowReturn
dlb_oar_push_drain (DlbOar * oar)
{
  GstFlowReturn ret;
  GstBuffer *inbuf, *outbuf;
  GstClockTime timestamp;
  guint64 offset;

  GstAllocator *allocator;
  GstAllocationParams params;

  gsize outsize, insize, residuesize, adaptersize;
  gint blocks_num;

  GstBaseTransform *trans = GST_BASE_TRANSFORM_CAST (oar);
  gst_base_transform_get_allocator (trans, &allocator, &params);

  adaptersize = gst_adapter_available (oar->adapter);
  outsize = oar->latency + adaptersize / oar->ininfo.bpf * oar->outinfo.bpf;

  adaptersize %= oar->min_block_size;
  residuesize = adaptersize;
  residuesize += oar->latency_samples * oar->ininfo.bpf;

  /* nothing to do */
  if (!residuesize)
    return GST_FLOW_OK;

  blocks_num = ((residuesize + oar->min_block_size - 1) / oar->min_block_size);
  insize = blocks_num * oar->min_block_size - adaptersize;

  inbuf = gst_buffer_new_allocate (allocator, insize, &params);
  gst_buffer_memset (inbuf, 0, 0, insize);

  if (allocator)
    gst_object_unref (allocator);

  dlb_oar_get_input_timing (oar, &timestamp, &offset);
  dlb_oar_set_output_timing (oar, inbuf, timestamp, offset);

  ret =
      GST_BASE_TRANSFORM_CLASS (dlb_oar_parent_class)->prepare_output_buffer
      (trans, inbuf, &outbuf);

  if (ret != GST_FLOW_OK)
    goto outbuf_error;

  ret = dlb_oar_transform (trans, inbuf, outbuf);
  if (ret != GST_FLOW_OK)
    goto transform_error;


  gst_buffer_unref (inbuf);
  gst_buffer_resize (outbuf, 0, outsize);
  GST_DEBUG_OBJECT (oar, "Flushing buff o %" G_GSIZE_FORMAT " bytes", outsize);

  ret = gst_pad_push (GST_BASE_TRANSFORM_SRC_PAD (oar), outbuf);
  if (ret != GST_FLOW_OK)
    GST_WARNING_OBJECT (oar, "Pushing failed: %s", gst_flow_get_name (ret));

  return ret;

transform_error:
  gst_buffer_unref (inbuf);
  gst_buffer_unref (outbuf);

outbuf_error:
  gst_buffer_unref (inbuf);

  GST_WARNING_OBJECT (oar, "Draining failed: %s", gst_flow_get_name (ret));
  return ret;
}

/* transform */
static GstFlowReturn
dlb_oar_transform (GstBaseTransform * trans, GstBuffer * inbuf,
    GstBuffer * outbuf)
{
  DlbOar *oar = DLB_OAR (trans);
  GstMapInfo outbuf_map, inbuf_map;

  gsize insize, outsize = 0;
  gint samples = 0, num_payloads = 0;
  gint inbpf = GST_AUDIO_INFO_BPF (&oar->ininfo);
  gint outbpf = GST_AUDIO_INFO_BPF (&oar->outinfo);

  GstClockTime timestamp;
  guint64 offset;

  dlb_buffer *in, *out;
  const guint8 *outdata;

  GST_LOG_OBJECT (oar, "transform");
  GST_OBJECT_LOCK (trans);

  /* buffer incoming data in adapter */
  gst_buffer_ref (inbuf);

  num_payloads = gst_buffer_get_oamd (oar, inbuf);
  dlb_oar_push_oamd_payload (oar->oar_instance, oar->oamd_payloads, num_payloads);

  gst_adapter_push (oar->adapter, inbuf);

  insize = get_next_block_size (oar);

  if (G_UNLIKELY (insize < oar->min_block_size || insize <= oar->prefill))
    goto no_output;

  dlb_oar_get_input_timing (oar, &timestamp, &offset);

  /* process all buffered data with calculated block size */
  gst_buffer_map (outbuf, &outbuf_map, GST_MAP_READWRITE);
  outdata = outbuf_map.data;

  while (insize >= oar->min_block_size) {
    GstBuffer *inbuf = gst_adapter_take_buffer (oar->adapter, insize);
    gst_buffer_map (inbuf, &inbuf_map, GST_MAP_READ);

    in = dlb_buffer_new_wrapped (inbuf_map.data, &oar->ininfo, TRUE);
    out = dlb_buffer_new_wrapped (outdata + outsize, &oar->outinfo, TRUE);

    samples = insize / inbpf;

    transform_data_block (oar, in, out, samples);

    dlb_buffer_free (in);
    dlb_buffer_free (out);
    gst_buffer_unmap (inbuf, &inbuf_map);
    gst_buffer_unref (inbuf);

    outsize += samples * outbpf;
    insize = get_next_block_size (oar);
  }

  gst_buffer_resize (outbuf, 0, outsize);
  gst_buffer_unmap (outbuf, &outbuf_map);

  if (G_UNLIKELY (oar->prefill)) {
    outsize -= oar->latency;
    GST_DEBUG_OBJECT (oar, "Trimming latency from the output");

    gst_buffer_resize (outbuf, oar->latency, outsize);
    oar->prefill = 0;
  } else {
    timestamp -= oar->latency_time;
    offset -= oar->latency_samples;
  }

  dlb_oar_set_output_timing (oar, outbuf, timestamp, offset);

  GST_LOG_OBJECT (oar, "inbuf %" GST_PTR_FORMAT ", outbuf %" GST_PTR_FORMAT,
      inbuf, outbuf);

  GST_OBJECT_UNLOCK (trans);
  return GST_FLOW_OK;

no_output:
  GST_OBJECT_UNLOCK (trans);
  return GST_BASE_TRANSFORM_FLOW_DROPPED;
}

static gboolean
oar_open (DlbOar * oar)
{
  GST_DEBUG_OBJECT (oar,
      "open speaker_bitfield=%ld sample_rate=%d, limiter_enable=%d",
      oar->oar_config.speaker_mask, oar->oar_config.sample_rate,
      oar->oar_config.limiter_enable);

  oar->oar_instance = dlb_oar_new (&oar->oar_config);
  if (oar->oar_instance == NULL)
    goto error;

  /* Initlize OAMD payloads memory */
  oar->max_payloads = dlb_oar_query_max_payloads (oar->oar_instance);
  oar->oamd_payloads = g_new0 (dlb_oar_payload, oar->max_payloads);

  /* Initialize input buffer adapter */
  oar->adapter = gst_adapter_new ();
  return TRUE;

error:
  GST_ELEMENT_ERROR (oar, LIBRARY, INIT, (NULL), ("Failed to open OAR"));
  return FALSE;
}

static void
oar_close (DlbOar * oar)
{
  GST_DEBUG_OBJECT (oar, "close");

  if (oar->oar_instance)
    dlb_oar_free (oar->oar_instance);
  if (oar->oamd_payloads)
    g_free (oar->oamd_payloads);
  if (oar->adapter)
    g_object_unref (oar->adapter);

  oar->oar_instance = NULL;
  oar->oamd_payloads = NULL;
  oar->adapter = NULL;
}

static gboolean
oar_restart (DlbOar * oar)
{
  GST_DEBUG_OBJECT (oar, "restart");

  oar_close (oar);
  return oar_open (oar);
}

static gboolean
plugin_init (GstPlugin * plugin)
{
  #ifdef DLB_OAR_OPEN_DYNLIB
  if (dlb_oar_try_open_dynlib ())
    return FALSE;
  #endif

  if (!gst_element_register (plugin, "dlboar", GST_RANK_PRIMARY, DLB_TYPE_OAR))
    return FALSE;

  return TRUE;
}

GST_PLUGIN_DEFINE (GST_VERSION_MAJOR,
    GST_VERSION_MINOR,
    dlboar,
    "Audio Object Renderer", plugin_init, VERSION, LICENSE, PACKAGE, ORIGIN)
