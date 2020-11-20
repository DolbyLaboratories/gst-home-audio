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

#ifndef _DLB_AC3_PARSE_H_
#define _DLB_AC3_PARSE_H_

#include <gst/base/gstbaseparse.h>

#include "dlb_audio_parser.h"

G_BEGIN_DECLS
#define DLB_TYPE_AC3_PARSE   (dlb_ac3_parse_get_type())
#define DLB_AC3_PARSE(obj)   (G_TYPE_CHECK_INSTANCE_CAST((obj),DLB_TYPE_AC3_PARSE,DlbAc3Parse))
#define DLB_AC3_PARSE_CLASS(klass)   (G_TYPE_CHECK_CLASS_CAST((klass),DLB_TYPE_AC3_PARSE,DlbAc3ParseClass))
#define DLB_IS_AC3_PARSE(obj)   (G_TYPE_CHECK_INSTANCE_TYPE((obj),DLB_TYPE_AC3_PARSE))
#define DLB_IS_AC3_PARSE_CLASS(obj)   (G_TYPE_CHECK_CLASS_TYPE((klass),DLB_TYPE_AC3_PARSE))
typedef struct _DlbAc3Parse DlbAc3Parse;
typedef struct _DlbAc3ParseClass DlbAc3ParseClass;

struct _DlbAc3Parse
{
  GstBaseParse base_ac3parse;

  /*< private > */
  dlb_audio_parser *parser;
  dlb_audio_parser_info stream_info;

  gboolean tag_published;
};

struct _DlbAc3ParseClass
{
  GstBaseParseClass base_ac3parse_class;
};

GType dlb_ac3_parse_get_type (void);

G_END_DECLS
#endif
