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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "dlb_udc.h"
#include "common_shim.h"

typedef struct dlb_udc_dispatch_table_s
{
  dlb_udc *(*new) (dlb_udc_output_mode outmode, dlb_udc_data_type data_type);
  void (*free) (dlb_udc * self);
  void (*drc_settings_init) (dlb_udc_drc_settings * drc);
  int (*drc_settings_set) (dlb_udc * self, const dlb_udc_drc_settings * drc);
  int (*push_frame) (dlb_udc * self, const char *indata, size_t indatasz);
  int (*process_frame) (dlb_udc * self, uint8_t * outbuf, size_t *framesz,
      dlb_evo_payload * metadata, dlb_udc_audio_info * audio_info,
      size_t *audio_info_offset);
  int (*query_latency) (dlb_udc * self);
} dlb_udc_dispatch_table;

static dlb_udc_dispatch_table dispatch_table;

int dlb_udc_try_open_dynlib (void)
{
  void *libudc = open_dynamic_lib ("libdlb_udc.so");
  if (!libudc)
    return 1;

  dispatch_table.new = get_proc_address (libudc, "dlb_udc_new");
  dispatch_table.free = get_proc_address (libudc, "dlb_udc_free");
  dispatch_table.drc_settings_init =
      get_proc_address (libudc, "dlb_udc_drc_settings_init");
  dispatch_table.drc_settings_set =
      get_proc_address (libudc, "dlb_udc_drc_settings_set");
  dispatch_table.push_frame = get_proc_address (libudc, "dlb_udc_push_frame");
  dispatch_table.process_frame =
      get_proc_address (libudc, "dlb_udc_process_frame");
  dispatch_table.query_latency =
      get_proc_address (libudc, "dlb_udc_query_latency");

  return 0;
}

dlb_udc *
dlb_udc_new (dlb_udc_output_mode outmode, dlb_udc_data_type data_type)
{
  return dispatch_table.new (outmode, data_type);
}

void
dlb_udc_free (dlb_udc * self)
{
  dispatch_table.free (self);
}

void
dlb_udc_drc_settings_init (dlb_udc_drc_settings * drc)
{
  dispatch_table.drc_settings_init (drc);
}

int
dlb_udc_drc_settings_set (dlb_udc * self, const dlb_udc_drc_settings * drc)
{
  return dispatch_table.drc_settings_set (self, drc);
}

int
dlb_udc_push_frame (dlb_udc * self, const char *indata, size_t indatasz)
{
  return dispatch_table.push_frame (self, indata, indatasz);
}

int
dlb_udc_process_frame (dlb_udc * self, uint8_t * outbuf, size_t *framesz,
    dlb_evo_payload * metadata, dlb_udc_audio_info * audio_info,
    size_t *audio_info_offset)
{
  return dispatch_table.process_frame (self, outbuf, framesz, metadata,
      audio_info, audio_info_offset);
}

int
dlb_udc_query_latency (dlb_udc * self)
{
  return dispatch_table.query_latency (self);
}
