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

#include "dlb_audio_parser.h"
#include "common_shim.h"

typedef struct dlb_audio_parser_dispatch_table_s
{
  dlb_audio_parser *(*new) (dlb_audio_parser_type type);
  void (*free) (dlb_audio_parser * parser);
  size_t (*query_max_inbuff_size) (const dlb_audio_parser * const parser);
  size_t (*query_min_frame_size) (const dlb_audio_parser * const parser);
    dlb_audio_parser_status (*parse) (dlb_audio_parser * parser,
      const uint8_t * const input, size_t insize, dlb_audio_parser_info * info,
      size_t *skipbytes);
} dlb_audio_parser_dispatch_table;

static dlb_audio_parser_dispatch_table dispatch_table;

int dlb_audio_parser_try_open_dynlib (void)
{
  void *libap = open_dynamic_lib (DLB_AUDIO_PARSER_LIBNAME);
  if (!libap)
    return 1;

  dispatch_table.new = get_proc_address (libap, "dlb_audio_parser_new");
  dispatch_table.free = get_proc_address (libap, "dlb_audio_parser_free");
  dispatch_table.query_max_inbuff_size =
      get_proc_address (libap, "dlb_audio_parser_query_max_inbuff_size");
  dispatch_table.query_min_frame_size =
      get_proc_address (libap, "dlb_audio_parser_query_min_frame_size");
  dispatch_table.parse = get_proc_address (libap, "dlb_audio_parser_parse");

  return 0;
}

dlb_audio_parser *
dlb_audio_parser_new (dlb_audio_parser_type type)
{
  return dispatch_table.new (type);
}

void
dlb_audio_parser_free (dlb_audio_parser * parser)
{
  dispatch_table.free (parser);
}

size_t
dlb_audio_parser_query_max_inbuff_size (const dlb_audio_parser * const parser)
{
  return dispatch_table.query_max_inbuff_size (parser);
}

size_t
dlb_audio_parser_query_min_frame_size (const dlb_audio_parser * const parser)
{
  return dispatch_table.query_min_frame_size (parser);
}

dlb_audio_parser_status
dlb_audio_parser_parse (dlb_audio_parser * parser,
    const uint8_t * const input, size_t insize, dlb_audio_parser_info * info,
    size_t *skipbytes)
{
  return dispatch_table.parse (parser, input, insize, info, skipbytes);
}
