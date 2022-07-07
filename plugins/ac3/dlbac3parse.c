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

#include "dlbac3parse.h"

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gst/gst.h>
#include <gst/audio/audio.h>
#include <gst/base/gstbaseparse.h>
#include <gst/pbutils/pbutils.h>

GST_DEBUG_CATEGORY_STATIC (dlb_ac3_parse_debug_category);
#define GST_CAT_DEFAULT dlb_ac3_parse_debug_category


/* prototypes */
static gboolean dlb_ac3_parse_start (GstBaseParse * parse);
static gboolean dlb_ac3_parse_stop (GstBaseParse * parse);
static GstFlowReturn dlb_ac3_parse_handle_frame (GstBaseParse * parse,
    GstBaseParseFrame * frame, gint * skipsize);
static GstFlowReturn dlb_ac3_parse_pre_push_frame (GstBaseParse * parse,
    GstBaseParseFrame * frame);

#define AC3_SAMPLES_PER_BLOCK 256

/* pad templates */
static GstStaticPadTemplate dlb_ac3_parse_src_template =
    GST_STATIC_PAD_TEMPLATE ("src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("audio/x-ac3, framed = (boolean) true, "
        " channels = (int) [ 1, 6 ], rate = (int) [ 8000, 48000 ], "
        " alignment = (string) { frame }; "
        "audio/x-eac3, framed = (boolean) true, "
        " channels = (int) [ 1, 8 ], rate = (int) [ 8000, 48000 ], "
        " alignment = (string) { frame }; ")
    );

static GstStaticPadTemplate dlb_ac3_parse_sink_template =
    GST_STATIC_PAD_TEMPLATE ("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("audio/x-ac3; " "audio/x-eac3; " "audio/ac3; "
        "audio/eac3; " "audio/x-private1-ac3")
    );


/* class initialization */
G_DEFINE_TYPE_WITH_CODE (DlbAc3Parse, dlb_ac3_parse, GST_TYPE_BASE_PARSE,
    GST_DEBUG_CATEGORY_INIT (dlb_ac3_parse_debug_category, "dlbac3parse", 0,
        "debug category for ac3parse element"));

static void
dlb_ac3_parse_class_init (DlbAc3ParseClass * klass)
{
  GstBaseParseClass *base_parse_class = GST_BASE_PARSE_CLASS (klass);

  /* Setting up pads and setting metadata should be moved to
     base_class_init if you intend to subclass this class. */
  gst_element_class_add_static_pad_template (GST_ELEMENT_CLASS (klass),
      &dlb_ac3_parse_src_template);
  gst_element_class_add_static_pad_template (GST_ELEMENT_CLASS (klass),
      &dlb_ac3_parse_sink_template);

  gst_element_class_set_static_metadata (GST_ELEMENT_CLASS (klass),
      "Dolby AC-3 and E-AC-3 Parser",
      "Codec/Parser/Audio",
      "Parse AC-3 and E-AC-3 audio stream",
      "Dolby Support <support@dolby.com>");

  base_parse_class->start = GST_DEBUG_FUNCPTR (dlb_ac3_parse_start);
  base_parse_class->stop = GST_DEBUG_FUNCPTR (dlb_ac3_parse_stop);
  base_parse_class->handle_frame =
      GST_DEBUG_FUNCPTR (dlb_ac3_parse_handle_frame);
  base_parse_class->pre_push_frame =
      GST_DEBUG_FUNCPTR (dlb_ac3_parse_pre_push_frame);
}

static void
dlb_ac3_parse_init (DlbAc3Parse * ac3parse)
{
  GST_PAD_SET_ACCEPT_INTERSECT (GST_BASE_PARSE_SINK_PAD (ac3parse));
  GST_PAD_SET_ACCEPT_TEMPLATE (GST_BASE_PARSE_SINK_PAD (ac3parse));

  ac3parse->tag_published = FALSE;
}

static gboolean
dlb_ac3_parse_start (GstBaseParse * parse)
{
  DlbAc3Parse *ac3parse = DLB_AC3_PARSE (parse);
  guint frmsize = 0;

  GST_DEBUG_OBJECT (ac3parse, "start");

  ac3parse->parser = dlb_audio_parser_new (DLB_AUDIO_PARSER_TYPE_AC3);
  ac3parse->stream_info.data_type = 0;
  ac3parse->stream_info.channels = 0;
  ac3parse->stream_info.sample_rate = 0;
  ac3parse->stream_info.framesize = 0;
  ac3parse->stream_info.samples = 0;
  ac3parse->stream_info.object_audio = 0;

  frmsize = dlb_audio_parser_query_min_frame_size (ac3parse->parser);
  gst_base_parse_set_min_frame_size (parse, frmsize);

  return TRUE;
}

static gboolean
dlb_ac3_parse_stop (GstBaseParse * parse)
{
  DlbAc3Parse *ac3parse = DLB_AC3_PARSE (parse);
  GST_DEBUG_OBJECT (ac3parse, "stop");

  dlb_audio_parser_free (ac3parse->parser);

  return TRUE;
}

static GstFlowReturn
dlb_ac3_parse_handle_frame (GstBaseParse * parse, GstBaseParseFrame * frame,
    gint * skipsize)
{
  DlbAc3Parse *ac3parse = DLB_AC3_PARSE (parse);
  GstMapInfo map;
  dlb_audio_parser_info info;
  dlb_audio_parser_status status;

  gboolean eac;
  gsize offset, frmsize;
  GstFlowReturn ret = GST_FLOW_OK;

  GST_LOG_OBJECT (ac3parse, "handle_frame");

  gst_buffer_map (frame->buffer, &map, GST_MAP_READ);

  if (G_UNLIKELY (GST_BASE_PARSE_DRAINING (parse))) {
    GST_DEBUG_OBJECT (parse, "draining");
    dlb_audio_parser_draining_set (ac3parse->parser, 1);
  }
  status = dlb_audio_parser_parse (ac3parse->parser, map.data, map.size,
      &info, &offset);

  if (status == DLB_AUDIO_PARSER_STATUS_OUT_OF_SYNC && offset != 0) {
    GST_DEBUG_OBJECT (parse, "out-of-sync: skipping %zd bytes to frame start",
        offset);

    *skipsize = (gint) offset;
    goto cleanup;
  } else if (status == DLB_AUDIO_PARSER_STATUS_NEED_MORE_DATA) {

    frmsize = dlb_audio_parser_query_min_frame_size (ac3parse->parser);
    GST_DEBUG_OBJECT (parse, "need-more-data: %zd", frmsize);

    gst_base_parse_set_min_frame_size (parse, frmsize);

    *skipsize = (gint) offset;
    goto cleanup;
  } else if (status != DLB_AUDIO_PARSER_STATUS_OK) {
    GST_WARNING_OBJECT (parse, "header-error: %d, skipping %zd bytes", status,
        offset);

    *skipsize = offset;
    goto cleanup;
  }

  eac = info.data_type == DATA_TYPE_EAC3;
  frmsize = info.framesize;

  GST_LOG_OBJECT (ac3parse, "found %sAC-3 frame of size %zd",
      eac ? "E-" : "", frmsize);

  /* detect current stream type and send caps if needed */
  if (G_UNLIKELY (ac3parse->stream_info.sample_rate != info.sample_rate
          || ac3parse->stream_info.channels != info.channels
          || ac3parse->stream_info.data_type != info.data_type)) {

    GstCaps *caps = gst_caps_new_simple (eac ? "audio/x-eac3" : "audio/x-ac3",
        "framed", G_TYPE_BOOLEAN, TRUE, "rate", G_TYPE_INT, info.sample_rate,
        "channels", G_TYPE_INT, info.channels, "alignment", G_TYPE_STRING,
        "frame", NULL);

    gst_pad_set_caps (GST_BASE_PARSE_SRC_PAD (parse), caps);
    gst_caps_unref (caps);

    ac3parse->stream_info = info;
    gst_base_parse_set_frame_rate (parse, info.sample_rate, info.samples, 0, 0);
  }

  /* Update frame rate if number of samples per frame has changed */
  if (G_UNLIKELY (ac3parse->stream_info.samples != info.samples))
    gst_base_parse_set_frame_rate (parse, info.sample_rate, info.samples, 0, 0);

cleanup:
  gst_buffer_unmap (frame->buffer, &map);

  if (status == DLB_AUDIO_PARSER_STATUS_OK) {
    ret = gst_base_parse_finish_frame (parse, frame, frmsize);
  }

  return ret;
}

static GstFlowReturn
dlb_ac3_parse_pre_push_frame (GstBaseParse * parse, GstBaseParseFrame * frame)
{
  DlbAc3Parse *ac3parse = DLB_AC3_PARSE (parse);
  GstTagList *taglist;
  GstCaps *caps;

  if (ac3parse->tag_published)
    return GST_FLOW_OK;

  caps = gst_pad_get_current_caps (GST_BASE_PARSE_SRC_PAD (parse));
  if (G_UNLIKELY (caps == NULL)) {
    if (GST_PAD_IS_FLUSHING (GST_BASE_PARSE_SRC_PAD (parse))) {
      GST_INFO_OBJECT (parse, "Src pad is flushing");
      return GST_FLOW_FLUSHING;
    } else {
      GST_INFO_OBJECT (parse, "Src pad is not negotiated!");
      return GST_FLOW_NOT_NEGOTIATED;
    }
  }

  taglist = gst_tag_list_new_empty ();
  gst_pb_utils_add_codec_description_to_tag_list (taglist,
      GST_TAG_AUDIO_CODEC, caps);
  gst_caps_unref (caps);

  gst_base_parse_merge_tags (parse, taglist, GST_TAG_MERGE_REPLACE);
  gst_tag_list_unref (taglist);

  /* also signals the end of first-frame processing */
  ac3parse->tag_published = TRUE;

  return GST_FLOW_OK;
}

static gboolean
plugin_init (GstPlugin * plugin)
{
  #ifdef DLB_AUDIO_PARSER_OPEN_DYNLIB
  if (dlb_audio_parser_try_open_dynlib())
    return FALSE;
  #endif

  return gst_element_register (plugin, "dlbac3parse", GST_RANK_PRIMARY + 2,
      DLB_TYPE_AC3_PARSE);
}

GST_PLUGIN_DEFINE (GST_VERSION_MAJOR,
    GST_VERSION_MINOR,
    dlbac3parse,
    "Dolby AC-3 and E-AC-3 Parser",
    plugin_init, VERSION, LICENSE, PACKAGE, ORIGIN)
