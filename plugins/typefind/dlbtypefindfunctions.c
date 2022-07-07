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
#include "dlb_audio_parser.h"

GST_DEBUG_CATEGORY_STATIC (dlb_type_find_debug);
#define GST_CAT_DEFAULT dlb_type_find_debug

static GstStaticCaps ac3_caps = GST_STATIC_CAPS ("audio/x-ac3");
#define AC3_CAPS (gst_static_caps_get(&ac3_caps))

static GstStaticCaps eac3_caps = GST_STATIC_CAPS ("audio/x-eac3");
#define EAC3_CAPS (gst_static_caps_get(&eac3_caps))

static void
ac3_type_find (GstTypeFind * tf, gpointer unused)
{
  dlb_audio_parser *parser;
  dlb_audio_parser_info info;
  dlb_audio_parser_status status;

  gsize frmsize = 0, offset = 0, skip = 0;
  gboolean eac;
  const guint8 *data;

  parser = dlb_audio_parser_new (DLB_AUDIO_PARSER_TYPE_AC3);
  frmsize = dlb_audio_parser_query_min_frame_size (parser);
  data = gst_type_find_peek (tf, 0, frmsize);

  while ((data = gst_type_find_peek (tf, offset, frmsize)) && offset < 8192) {
    status = dlb_audio_parser_parse (parser, data, frmsize, &info, &skip);

    offset += skip;

    if (status == DLB_AUDIO_PARSER_STATUS_OUT_OF_SYNC && offset != 0) {
      continue;
    } else if (status == DLB_AUDIO_PARSER_STATUS_NEED_MORE_DATA) {
      frmsize = dlb_audio_parser_query_min_frame_size (parser);
      continue;
    } else if (status != DLB_AUDIO_PARSER_STATUS_OK) {
      continue;
    }

    eac = (info.data_type == DATA_TYPE_EAC3);
    GST_LOG ("found %sAC-3 frame of size %zd", eac ? "E-" : "", frmsize);

    gst_type_find_suggest (tf, GST_TYPE_FIND_MAXIMUM,
        eac ? EAC3_CAPS : AC3_CAPS);

    break;
  }

  dlb_audio_parser_free (parser);
}

static gboolean
plugin_init (GstPlugin * plugin)
{
  #ifdef DLB_AUDIO_PARSER_OPEN_DYNLIB
  if (dlb_audio_parser_try_open_dynlib())
    return FALSE;
  #endif

  GST_DEBUG_CATEGORY_INIT (dlb_type_find_debug, "dlbtypefindfunctions", 0,
      "Dolby specific type find functions");

  return gst_type_find_register (plugin, "audio/x-dlb", GST_RANK_PRIMARY + 1,
      ac3_type_find, "ac3,ec3,eac3,eb3", AC3_CAPS, NULL, NULL);
}

GST_PLUGIN_DEFINE (GST_VERSION_MAJOR, GST_VERSION_MINOR,
    dlbtypefind,
    "Dolby typefind collection",
    plugin_init, VERSION, LICENSE, PACKAGE, ORIGIN)
