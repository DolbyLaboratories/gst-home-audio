/*******************************************************************************

 * Dolby Home Audio GStreamer Plugins
 * Copyright (C) 2021, Dolby Laboratories

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

#include "dlbflexr.h"
#include "dlbaudiometa.h"
#include "dlbutils.h"

#define GST_CAT_DEFAULT dlb_flexr_debug
GST_DEBUG_CATEGORY_STATIC (GST_CAT_DEFAULT);

#define DEFAULT_PAD_VOLUME (1.0)
#define DEFAULT_PAD_UPMIX (FALSE)

enum
{
  PROP_PAD_0,
  PROP_PAD_STREAM_CONFIG,
  PROP_PAD_VOLUME,
  PROP_PAD_UPMIX,
  PROP_PAD_INTERP_MODE,
};

G_DEFINE_TYPE (DlbFlexrPad, dlb_flexr_pad, GST_TYPE_AUDIO_AGGREGATOR_PAD);

#define DLB_TYPE_FLEXR_INTERP_MODE (dlb_flexr_interp_mode_get_type())
static GType
dlb_flexr_interp_mode_get_type (void)
{
  static GType interp_mode_type = 0;
  static const GEnumValue interp_mode_types[] = {
    {DLB_FLEXR_INTERP_RUNTIME, "Runtime", "runtime"},
    {DLB_FLEXR_INTERP_OFFLINE, "Offline", "offline"},
    {0, NULL, NULL}
  };

  if (!interp_mode_type) {
    interp_mode_type =
        g_enum_register_static ("DlbFlexrInterpMode", interp_mode_types);
  }

  return interp_mode_type;
}

static void
dlb_flexr_pad_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec)
{
  DlbFlexrPad *pad = DLB_FLEXR_PAD (object);

  switch (prop_id) {
    case PROP_PAD_STREAM_CONFIG:
      g_value_set_string (value, pad->config_path);
      break;
    case PROP_PAD_VOLUME:
      g_value_set_double (value, pad->volume);
      break;
    case PROP_PAD_UPMIX:
      g_value_set_boolean (value, pad->upmix);
      break;
    case PROP_PAD_INTERP_MODE:
      g_value_set_enum (value, pad->interp);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
dlb_flexr_pad_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec)
{
  DlbFlexrPad *pad = DLB_FLEXR_PAD (object);

  switch (prop_id) {
    case PROP_PAD_STREAM_CONFIG:
      GST_OBJECT_LOCK (pad);
      g_free (pad->config_path);
      pad->config_path = g_strdup (g_value_get_string (value));
      g_hash_table_add (pad->props_set,
          GINT_TO_POINTER (PROP_PAD_STREAM_CONFIG));
      GST_OBJECT_UNLOCK (pad);
      break;
    case PROP_PAD_VOLUME:
      GST_OBJECT_LOCK (pad);
      pad->volume = g_value_get_double (value);
      g_hash_table_add (pad->props_set, GINT_TO_POINTER (PROP_PAD_VOLUME));
      GST_OBJECT_UNLOCK (pad);
      break;
    case PROP_PAD_UPMIX:
      GST_OBJECT_LOCK (pad);
      pad->upmix = g_value_get_boolean (value);
      g_hash_table_add (pad->props_set, GINT_TO_POINTER (PROP_PAD_UPMIX));
      GST_OBJECT_UNLOCK (pad);
      break;
    case PROP_PAD_INTERP_MODE:
      GST_OBJECT_LOCK (pad);
      pad->interp = g_value_get_enum (value);
      g_hash_table_add (pad->props_set, GINT_TO_POINTER (PROP_PAD_INTERP_MODE));
      GST_OBJECT_UNLOCK (pad);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
dlb_flexr_pad_init (DlbFlexrPad * pad)
{
  pad->stream = DLB_FLEXR_STREAM_HANDLE_INVALID;
  pad->props_set = g_hash_table_new (g_direct_hash, g_direct_equal);

  pad->config_path = NULL;
  pad->upmix = 1;
  pad->volume = 1;
}

static void
dlb_flexr_pad_finalize (GObject * object)
{
  DlbFlexrPad *flexrpad = DLB_FLEXR_PAD (object);

  g_hash_table_destroy (flexrpad->props_set);
  g_free (flexrpad->config_path);
}

static void
dlb_flexr_pad_class_init (DlbFlexrPadClass * klass)
{
  GObjectClass *gobject_class = (GObjectClass *) klass;

  gobject_class->set_property = dlb_flexr_pad_set_property;
  gobject_class->get_property = dlb_flexr_pad_get_property;
  gobject_class->finalize = dlb_flexr_pad_finalize;

  g_object_class_install_property (gobject_class, PROP_PAD_STREAM_CONFIG,
      g_param_spec_string ("stream-config", "Stream configuration",
          "Serialized stream configuration file for this pad", NULL,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS |
          G_PARAM_CONSTRUCT | GST_PARAM_MUTABLE_PLAYING));

  g_object_class_install_property (gobject_class, PROP_PAD_INTERP_MODE,
      g_param_spec_enum ("interp-mode", "Interpolation mode",
          "Interpolation mode for this pad.", DLB_TYPE_FLEXR_INTERP_MODE,
          DLB_FLEXR_INTERP_OFFLINE,
          G_PARAM_READWRITE | G_PARAM_CONSTRUCT | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_PAD_UPMIX,
      g_param_spec_boolean ("upmix", "Upmix", "Upmix stream from this pad",
          DEFAULT_PAD_UPMIX,
          G_PARAM_READWRITE | G_PARAM_CONSTRUCT | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_PAD_VOLUME,
      g_param_spec_double ("volume", "Volume", "Volume of this pad",
          0.0, 10.0, DEFAULT_PAD_VOLUME,
          G_PARAM_READWRITE | GST_PARAM_CONTROLLABLE | G_PARAM_STATIC_STRINGS));
}

enum
{
  PROP_0,
  PROP_DEVICE_CONFIG,
  PROP_ACTIVE_CHANNELS_ENABLE,
  PROP_ACTIVE_CHANNELS_MASK,
};

#define SRC_CAPS                                                        \
  "audio/x-raw, "                                                       \
    "format = (string) { "GST_AUDIO_NE (F32)" } ,"                      \
    "channels = (int) [ 1, 32 ], "                                      \
    "rate = (int) 48000, "                                              \
    "layout = (string) interleaved;"                                    \

#define SINK_CAPS                                                       \
  "audio/x-raw, "                                                       \
    "format = (string) { "GST_AUDIO_NE (F32)" } ,"                      \
    "channels = (int) {1, 2, 6, 8 }, "                                  \
    "rate = (int) 48000, "                                              \
    "layout = (string) interleaved; "                                   \
  "audio/x-raw(" DLB_CAPS_FEATURE_META_OBJECT_AUDIO_META "), "          \
    "format = (string) { "GST_AUDIO_NE (F32)" } ,"                      \
    "channels = (int) [ 1, 32 ], "                                      \
    "rate = (int) 48000, "                                              \
    "layout = (string) interleaved;"                                    \

static GstStaticPadTemplate dlb_flexr_src_template =
GST_STATIC_PAD_TEMPLATE ("src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS (SRC_CAPS)
    );

static GstStaticPadTemplate dlb_flexr_sink_template =
GST_STATIC_PAD_TEMPLATE ("sink_%u",
    GST_PAD_SINK,
    GST_PAD_REQUEST,
    GST_STATIC_CAPS (SINK_CAPS)
    );

static void dlb_flexr_child_proxy_init (gpointer g_iface, gpointer iface_data);

#define dlb_flexr_parent_class parent_class
G_DEFINE_TYPE_WITH_CODE (DlbFlexr, dlb_flexr,
    GST_TYPE_AUDIO_AGGREGATOR, G_IMPLEMENT_INTERFACE (GST_TYPE_CHILD_PROXY,
        dlb_flexr_child_proxy_init));

static void dlb_flexr_finalize (GObject * object);
static void dlb_flexr_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec);
static void dlb_flexr_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec);
static gboolean dlb_flexr_open (DlbFlexr * flexr);
static void dlb_flexr_close (DlbFlexr * flexr);
static gboolean dlb_flexr_set_caps (DlbFlexr * flexr, GstAggregatorPad * aggpad,
    GstCaps * caps);
static GstCaps *dlb_flexr_get_caps (DlbFlexr * flexr, GstAggregatorPad * aggpad,
    GstCaps * filter);
static gboolean dlb_flexr_sink_event (GstAggregator * agg,
    GstAggregatorPad * aggpad, GstEvent * event);
static gboolean dlb_flexr_sink_query (GstAggregator * agg,
    GstAggregatorPad * aggpad, GstQuery * query);
static GstFlowReturn dlb_flexr_update_src_caps (GstAggregator * agg,
    GstCaps * caps, GstCaps ** ret);
static GstPad *dlb_flexr_request_new_pad (GstElement * element,
    GstPadTemplate * temp, const gchar * req_name, const GstCaps * caps);
static void dlb_flexr_release_pad (GstElement * element, GstPad * pad);
static gboolean dlb_flexr_aggregate_one_buffer (GstAudioAggregator * aagg,
    GstAudioAggregatorPad * aaggpad, GstBuffer * inbuf, guint in_offset,
    GstBuffer * outbuf, guint out_offset, guint num_samples);

static void
dlb_flexr_class_init (DlbFlexrClass * klass)
{
  GObjectClass *gobject_class = (GObjectClass *) klass;
  GstElementClass *gstelement_class = (GstElementClass *) klass;
  GstAggregatorClass *agg_class = (GstAggregatorClass *) klass;
  GstAudioAggregatorClass *aagg_class = (GstAudioAggregatorClass *) klass;

  gobject_class->set_property = dlb_flexr_set_property;
  gobject_class->get_property = dlb_flexr_get_property;
  gobject_class->finalize = dlb_flexr_finalize;

  gst_element_class_add_static_pad_template_with_gtype (gstelement_class,
      &dlb_flexr_src_template, GST_TYPE_AUDIO_AGGREGATOR_PAD);
  gst_element_class_add_static_pad_template_with_gtype (gstelement_class,
      &dlb_flexr_sink_template, DLB_TYPE_FLEXR_PAD);
  gst_element_class_set_static_metadata (gstelement_class, "Flexr",
      "Generic/Audio", "Renders and mixes multiple audio streams",
      "Dolby Support <support@dolby.com>");

  g_object_class_install_property (gobject_class, PROP_DEVICE_CONFIG,
      g_param_spec_string ("device-config", "Device configuration",
          "Serialized device configuration file", NULL,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS | G_PARAM_CONSTRUCT |
          GST_PARAM_MUTABLE_PLAYING));

  g_object_class_install_property (gobject_class, PROP_ACTIVE_CHANNELS_ENABLE,
      g_param_spec_boolean ("active-channels-enable", "Active channels enable",
          "Enable filtering of render channels", FALSE,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_ACTIVE_CHANNELS_MASK,
      g_param_spec_uint64 ("active-channels-mask", "Active channels mask",
          "A bitmask of channels that will be rendered when active-channels-enable = TRUE",
          1, G_MAXUINT64, 1,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS |
          GST_PARAM_MUTABLE_READY));


  gstelement_class->request_new_pad =
      GST_DEBUG_FUNCPTR (dlb_flexr_request_new_pad);
  gstelement_class->release_pad = GST_DEBUG_FUNCPTR (dlb_flexr_release_pad);

  agg_class->sink_query = GST_DEBUG_FUNCPTR (dlb_flexr_sink_query);
  agg_class->sink_event = GST_DEBUG_FUNCPTR (dlb_flexr_sink_event);
  agg_class->update_src_caps = dlb_flexr_update_src_caps;
  aagg_class->aggregate_one_buffer = dlb_flexr_aggregate_one_buffer;
}

static void
dlb_flexr_init (DlbFlexr * flexr)
{
  flexr->flexr_instance = NULL;
  flexr->config_path = NULL;
  flexr->channels = 0;
  flexr->streams = 0;
  flexr->latency = 0;
  flexr->blksize = 0;
}

static void
dlb_flexr_finalize (GObject * object)
{
  DlbFlexr *flexr = DLB_FLEXR (object);

  dlb_flexr_close (flexr);
  g_free (flexr->config_path);
}

static gboolean
dlb_flexr_open (DlbFlexr * flexr)
{
  GstAggregator *agg;
  GstAudioAggregator *aagg;
  dlb_flexr_init_info info = { 0 };
  guint64 duration;

  GError *error = NULL;
  gchar *contents = NULL;
  gsize length;

  if (!flexr->config_path)
    goto path_error;

  g_file_get_contents (flexr->config_path, &contents, &length, &error);
  if (error != NULL)
    goto config_error;

  info.active_channels_enable = flexr->active_channels_enable;
  info.active_channels_mask = flexr->active_channels_mask;
  info.serialized_config = (guint8 *) contents;
  info.serialized_config_size = length;

  flexr->flexr_instance = dlb_flexr_new (&info);
  if (!flexr->flexr_instance)
    goto mixer_error;

  flexr->channels = dlb_flexr_query_num_outputs (flexr->flexr_instance);
  flexr->latency = dlb_flexr_query_latency (flexr->flexr_instance);
  flexr->blksize = dlb_flexr_query_block_samples (flexr->flexr_instance);

  duration = gst_util_uint64_scale_int_ceil (flexr->blksize, GST_SECOND, 48000);

  agg = GST_AGGREGATOR (flexr);
  gst_aggregator_set_latency (agg, duration, duration);

  aagg = GST_AUDIO_AGGREGATOR (flexr);
  g_object_set (G_OBJECT (aagg), "output-buffer-duration", duration, NULL);

  g_free (contents);
  return TRUE;

mixer_error:
  GST_ELEMENT_ERROR (flexr, LIBRARY, INIT, (NULL), ("Failed to open FLEXR"));
  g_free (contents);
  return FALSE;

path_error:
  GST_ELEMENT_ERROR (flexr, LIBRARY, INIT, (NULL),
      ("device-config property cannot be empty"));
  g_free (contents);
  return FALSE;

config_error:
  GST_ELEMENT_ERROR (flexr, RESOURCE, OPEN_READ, ("%s: %d", error->message,
          error->code), ("Reading Stream config failed"));

  g_error_free (error);
  return FALSE;
}

static void
dlb_flexr_close (DlbFlexr * flexr)
{
  dlb_flexr_free (flexr->flexr_instance);

  flexr->flexr_instance = NULL;
  flexr->channels = 0;
  flexr->streams = 0;
}

static void
dlb_flexr_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec)
{
  DlbFlexr *flexr = DLB_FLEXR (object);

  switch (prop_id) {
    case PROP_DEVICE_CONFIG:
      g_value_set_string (value, flexr->config_path);
      break;
    case PROP_ACTIVE_CHANNELS_ENABLE:
      g_value_set_boolean (value, flexr->active_channels_enable);
      break;
    case PROP_ACTIVE_CHANNELS_MASK:
      g_value_set_uint64 (value, flexr->active_channels_mask);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
dlb_flexr_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec)
{
  DlbFlexr *flexr = DLB_FLEXR (object);

  switch (prop_id) {
    case PROP_DEVICE_CONFIG:
      GST_OBJECT_LOCK (flexr);
      g_free (flexr->config_path);
      flexr->config_path = g_strdup (g_value_get_string (value));
      GST_OBJECT_UNLOCK (flexr);
      break;
    case PROP_ACTIVE_CHANNELS_ENABLE:
      GST_OBJECT_LOCK (flexr);
      flexr->active_channels_enable = g_value_get_boolean (value);
      GST_OBJECT_UNLOCK (flexr);
      break;
    case PROP_ACTIVE_CHANNELS_MASK:
      GST_OBJECT_LOCK (flexr);
      flexr->active_channels_mask = g_value_get_uint64 (value);
      GST_OBJECT_UNLOCK (flexr);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static gboolean
dlb_flexr_are_all_pads_ready (DlbFlexr * flexr)
{
  GList *l = GST_ELEMENT_CAST (flexr)->sinkpads;

  for (; l; l = l->next) {
    DlbFlexrPad *flexrpad = DLB_FLEXR_PAD (l->data);
    gint samples;

    samples = dlb_flexr_query_pushed_samples (flexr->flexr_instance,
        flexrpad->stream);

    if (samples < flexr->blksize)
      return FALSE;
  }

  return TRUE;
}

static void
dlb_flexr_update_stream (DlbFlexr * flexr, DlbFlexrPad * pad)
{
  dlb_flexr *df = flexr->flexr_instance;
  dlb_flexr_stream_handle h = pad->stream;

  GHashTableIter iter;
  gpointer prop_id;

  g_hash_table_iter_init (&iter, pad->props_set);

  while (g_hash_table_iter_next (&iter, &prop_id, &prop_id)) {
    switch (GPOINTER_TO_INT (prop_id)) {
      case PROP_PAD_STREAM_CONFIG:{
        GError *error = NULL;
        gchar *contents = NULL;
        gsize length;

        GST_DEBUG_OBJECT (flexr, "Updating stream config %s", pad->config_path);
        g_file_get_contents (pad->config_path, &contents, &length, &error);

        if (error != NULL) {
          GST_WARNING_OBJECT (flexr, "Stream config update failed: %s",
              error->message);
          g_error_free (error);
          break;
        }

        dlb_flexr_set_render_config (df, h, (guint8 *) contents, length,
            pad->interp, 1);
        g_free (contents);
        break;
      }
      case PROP_PAD_VOLUME:
        GST_DEBUG_OBJECT (flexr, "Updating stream volume %f", pad->volume);
        dlb_flexr_set_volume (df, h, pad->volume);
        break;
      case PROP_PAD_UPMIX:
        break;
      case PROP_PAD_INTERP_MODE:
        break;
      default:
        g_assert_not_reached ();
        break;
    }
  }

  g_hash_table_remove_all (pad->props_set);
}

static gboolean
dlb_flexr_set_caps (DlbFlexr * flexr, GstAggregatorPad * aggpad, GstCaps * caps)
{
  DlbFlexrPad *pad = DLB_FLEXR_PAD (aggpad);
  dlb_flexr_stream_info info;
  dlb_flexr_input_format fmt;

  GstCapsFeatures *features = NULL;
  GError *error = NULL;
  gchar *contents = NULL;
  gsize length;
  gboolean ret;
  gboolean have_meta = FALSE;

  GST_DEBUG_OBJECT (flexr, "Adding stream for caps %" GST_PTR_FORMAT, caps);

  /* for the first stream open flexr mixer */
  if (!flexr->flexr_instance) {
    ret = dlb_flexr_open (flexr);
    if (!ret)
      return FALSE;
  }

  /* already initialized */
  if (pad->stream)
    return TRUE;

  if (!pad->config_path)
    goto path_error;

  g_file_get_contents (pad->config_path, &contents, &length, &error);
  if (error != NULL)
    goto config_error;

  if ((features = gst_caps_get_features (caps, 0))) {
    have_meta =
        gst_caps_features_contains (features,
        DLB_CAPS_FEATURE_META_OBJECT_AUDIO_META);
  }

  if (have_meta) {
    fmt = DLB_FLEXR_INPUT_FORMAT_OBJECT;
  } else {
    GstAudioInfo info;

    if (!gst_audio_info_from_caps (&info, caps))
      goto format_error;

    switch (info.channels) {
      case 1:
        fmt = DLB_FLEXR_INPUT_FORMAT_1_0;
        break;
      case 2:
        fmt = DLB_FLEXR_INPUT_FORMAT_2_0;
        break;
      case 6:
        fmt = DLB_FLEXR_INPUT_FORMAT_5_1;
        break;
      case 8:
        fmt = DLB_FLEXR_INPUT_FORMAT_7_1;
        break;
      default:
        goto format_error;
    }
  }

  info.serialized_config = (guint8 *) contents;
  info.serialized_config_size = length;
  info.upmix_enable = pad->upmix;
  info.interp = pad->interp;
  info.format = fmt;

  pad->stream = dlb_flexr_add_stream (flexr->flexr_instance, &info);
  if (!pad->stream)
    goto stream_error;

  dlb_flexr_set_volume (flexr->flexr_instance, pad->stream, pad->volume);

  GST_DEBUG_OBJECT (flexr, "adding new %s stream",
      have_meta ? "object" : "channel");

  flexr->streams++;
  g_hash_table_remove_all (pad->props_set);
  g_free (contents);

  return TRUE;

path_error:
  GST_ELEMENT_ERROR (flexr, LIBRARY, INIT, (NULL),
      ("stream-config property cannot be empty"));
  return FALSE;

stream_error:
  GST_ELEMENT_ERROR (flexr, LIBRARY, INIT, (NULL), ("Failed to add stream"));
  g_free (contents);
  return FALSE;

format_error:
  GST_ERROR_OBJECT (flexr, "invalid format set as caps: %" GST_PTR_FORMAT,
      caps);

  g_free (contents);
  return FALSE;

config_error:
  GST_ELEMENT_ERROR (flexr, RESOURCE, OPEN_READ, ("%s: %d", error->message,
          error->code), ("Reading Stream config failed"));

  g_error_free (error);
  return FALSE;
}

static gboolean
dlb_flexr_sink_event (GstAggregator * agg, GstAggregatorPad * aggpad,
    GstEvent * event)
{
  DlbFlexr *flexr = DLB_FLEXR (agg);
  gboolean res = TRUE;

  GST_DEBUG_OBJECT (flexr, "Got %s event on sink pad",
      GST_EVENT_TYPE_NAME (event));

  switch (GST_EVENT_TYPE (event)) {
    case GST_EVENT_CAPS:
    {
      GstCaps *caps;

      gst_event_parse_caps (event, &caps);
      res = dlb_flexr_set_caps (flexr, aggpad, caps);
      break;
    }
    default:
      break;
  }

  if (res)
    return GST_AGGREGATOR_CLASS (parent_class)->sink_event (agg, aggpad, event);

  return res;
}

static GstCaps *
dlb_flexr_get_caps (DlbFlexr * flexr, GstAggregatorPad * aggpad,
    GstCaps * filter)
{
  GstPad *pad = GST_PAD_CAST (aggpad);
  GstCaps *result = gst_pad_get_pad_template_caps (pad);

  GST_INFO_OBJECT (flexr, "Getting caps with filter %" GST_PTR_FORMAT, filter);

  if (filter) {
    GstCaps *tmp = gst_caps_intersect_full (result, filter,
        GST_CAPS_INTERSECT_FIRST);

    gst_caps_unref (result);
    result = tmp;
  }

  GST_INFO_OBJECT (flexr, "returned sink caps : %" GST_PTR_FORMAT, result);

  return result;
}

static gboolean
dlb_flexr_sink_query (GstAggregator * agg, GstAggregatorPad * aggpad,
    GstQuery * query)
{
  DlbFlexr *flexr = DLB_FLEXR (agg);
  gboolean res = FALSE;

  GST_DEBUG_OBJECT (flexr, "Got %s query on sink pad",
      GST_QUERY_TYPE_NAME (query));

  switch (GST_QUERY_TYPE (query)) {
    case GST_QUERY_CAPS:
    {
      GstCaps *filter, *caps;

      gst_query_parse_caps (query, &filter);
      caps = dlb_flexr_get_caps (flexr, aggpad, filter);
      gst_query_set_caps_result (query, caps);
      gst_caps_unref (caps);
      res = TRUE;
      break;
    }
    default:
      res =
          GST_AGGREGATOR_CLASS (parent_class)->sink_query (agg, aggpad, query);
      break;
  }

  return res;
}

static GstFlowReturn
dlb_flexr_update_src_caps (GstAggregator * agg, GstCaps * caps, GstCaps ** ret)
{
  DlbFlexr *flexr = DLB_FLEXR (agg);
  GstStructure *s;

  /* we need to know number of ouput channels at this point, so if mixer is
   * not opened yet, open it now */
  if (!flexr->flexr_instance) {
    if (!dlb_flexr_open (flexr)) {
      return GST_FLOW_ERROR;
    }
  }

  GST_OBJECT_LOCK (flexr);

  *ret = gst_caps_copy (caps);
  s = gst_caps_get_structure (*ret, 0);

  gst_structure_set (s, "channels", G_TYPE_INT, flexr->channels, "layout",
      G_TYPE_STRING, "interleaved", "channel-mask", GST_TYPE_BITMASK,
      gst_audio_channel_get_fallback_mask (flexr->channels), NULL);

  GST_OBJECT_UNLOCK (flexr);

  return GST_FLOW_OK;
}

static GstPad *
dlb_flexr_request_new_pad (GstElement * element, GstPadTemplate * templ,
    const gchar * req_name, const GstCaps * caps)
{
  DlbFlexr *flexr = DLB_FLEXR (element);

  DlbFlexrPad *newpad = (DlbFlexrPad *)
      GST_ELEMENT_CLASS (parent_class)->request_new_pad (element,
      templ, req_name, caps);

  if (newpad == NULL)
    goto error;

  gst_child_proxy_child_added (GST_CHILD_PROXY (element), G_OBJECT (newpad),
      GST_OBJECT_NAME (newpad));

  GST_DEBUG_OBJECT (flexr, "new pad %s:%s", GST_DEBUG_PAD_NAME (newpad));

  return GST_PAD_CAST (newpad);

error:
  {
    GST_DEBUG_OBJECT (element, "could not create/add pad");
    return NULL;
  }
}

static void
dlb_flexr_release_pad (GstElement * element, GstPad * pad)
{
  DlbFlexr *flexr = DLB_FLEXR (element);
  DlbFlexrPad *sinkpad = DLB_FLEXR_PAD (pad);

  GST_DEBUG_OBJECT (flexr, "release pad %s:%s", GST_DEBUG_PAD_NAME (pad));

  if (sinkpad->stream) {
    dlb_flexr_rm_stream (flexr->flexr_instance, sinkpad->stream);
    sinkpad->stream = DLB_FLEXR_STREAM_HANDLE_INVALID;
  }

  gst_child_proxy_child_removed (GST_CHILD_PROXY (flexr), G_OBJECT (pad),
      GST_OBJECT_NAME (pad));

  GST_ELEMENT_CLASS (parent_class)->release_pad (element, pad);
}

static gboolean
dlb_flexr_aggregate_one_buffer (GstAudioAggregator * aagg,
    GstAudioAggregatorPad * aaggpad, GstBuffer * inbuf, guint in_offset,
    GstBuffer * outbuf, guint out_offset, guint num_samples)
{
  GstMapInfo inmap, outmap;
  gboolean ret = FALSE;

  GstAggregator *agg = GST_AGGREGATOR (aagg);
  DlbFlexr *flexr = DLB_FLEXR (aagg);

  DlbFlexrPad *flexrpad = DLB_FLEXR_PAD (aaggpad);
  GstAudioAggregatorPad *sinkpad = aaggpad;
  GstAudioAggregatorPad *srcpad = GST_AUDIO_AGGREGATOR_PAD (agg->srcpad);

  DlbObjectAudioMeta *meta;
  dlb_buffer *in, *out;
  gint samples;

  dlb_flexr_object_metadata md = { 0 };
  dlb_flexr_stream_handle stream = flexrpad->stream;

  const guint8 *indata;
  const guint8 *outdata;

  GST_OBJECT_LOCK (aagg);
  GST_OBJECT_LOCK (aaggpad);

  dlb_flexr_update_stream (flexr, flexrpad);

  gst_buffer_map (outbuf, &outmap, GST_MAP_READWRITE);
  gst_buffer_map (inbuf, &inmap, GST_MAP_READ);

  indata = inmap.data + in_offset * GST_AUDIO_INFO_BPF (&sinkpad->info);
  outdata = outmap.data;

  GST_LOG_OBJECT (flexrpad, "mixing %u samples, in_offset %u, out_offset %u",
      num_samples, in_offset, out_offset);

  if ((meta = dlb_audio_object_meta_get (inbuf))) {
    md.offset = meta->offset;
    md.payload = meta->payload;
    md.payload_size = meta->size;
  }

  in = dlb_buffer_new_wrapped (indata, &sinkpad->info, TRUE);
  dlb_flexr_push_stream (flexr->flexr_instance, stream, &md, in, num_samples);

  if (dlb_flexr_are_all_pads_ready (flexr)) {
    GST_LOG_OBJECT (flexr, "All pads ready... processing streams");

    out = dlb_buffer_new_wrapped (outdata, &srcpad->info, TRUE);
    dlb_flexr_generate_output (flexr->flexr_instance, out, &samples);

    dlb_buffer_free (out);
    ret = TRUE;
  }

  GST_LOG_OBJECT (flexr, "inbuf %" GST_PTR_FORMAT ", outbuf %" GST_PTR_FORMAT,
      inbuf, outbuf);

  dlb_buffer_free (in);
  gst_buffer_unmap (inbuf, &inmap);
  gst_buffer_unmap (outbuf, &outmap);

  GST_OBJECT_UNLOCK (aaggpad);
  GST_OBJECT_UNLOCK (aagg);

  return ret;
}

/* GstChildProxy implementation */
static GObject *
dlb_flexr_child_proxy_get_child_by_index (GstChildProxy * child_proxy,
    guint index)
{
  DlbFlexr *flexr = DLB_FLEXR (child_proxy);
  GObject *obj = NULL;

  GST_OBJECT_LOCK (flexr);
  obj = g_list_nth_data (GST_ELEMENT_CAST (flexr)->sinkpads, index);
  if (obj)
    gst_object_ref (obj);
  GST_OBJECT_UNLOCK (flexr);

  return obj;
}

static guint
dlb_flexr_child_proxy_get_children_count (GstChildProxy * child_proxy)
{
  guint count = 0;
  DlbFlexr *flexr = DLB_FLEXR (child_proxy);

  GST_OBJECT_LOCK (flexr);
  count = GST_ELEMENT_CAST (flexr)->numsinkpads;
  GST_OBJECT_UNLOCK (flexr);
  GST_INFO_OBJECT (flexr, "Children Count: %d", count);

  return count;
}

static void
dlb_flexr_child_proxy_init (gpointer g_iface, gpointer iface_data)
{
  GstChildProxyInterface *iface = g_iface;

  GST_INFO ("initializing child proxy interface");
  iface->get_child_by_index = dlb_flexr_child_proxy_get_child_by_index;
  iface->get_children_count = dlb_flexr_child_proxy_get_children_count;
}

static gboolean
plugin_init (GstPlugin * plugin)
{

#ifdef DLB_FLEXR_OPEN_DYNLIB
  if (dlb_flexr_try_open_dynlib ())
    return FALSE;
#endif

  GST_DEBUG_CATEGORY_INIT (GST_CAT_DEFAULT, "dlbflexr", 0,
      "audio rendering element");

  if (!gst_element_register (plugin, "dlbflexr", GST_RANK_NONE, DLB_TYPE_FLEXR))
    return FALSE;

  return TRUE;
}

GST_PLUGIN_DEFINE (GST_VERSION_MAJOR,
    GST_VERSION_MINOR,
    dlbflexr,
    "Flexible Renderer", plugin_init, VERSION, LICENSE, PACKAGE, ORIGIN)
