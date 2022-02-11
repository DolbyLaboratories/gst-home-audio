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

#include "dlb_udc.h"
#include "common_shim.h"

typedef struct dlb_udc_dispatch_table_s
{
  dlb_udc *(*new) (const dlb_udc_init_info * info);
  void (*free) (dlb_udc * self);
  void (*drc_settings_init) (dlb_udc_drc_settings * drc);
  int (*drc_settings_set) (dlb_udc * self, const dlb_udc_drc_settings * drc);
  int (*query_latency) (dlb_udc * self);
  size_t (*query_max_outbuf_size) (dlb_udc_output_mode mode, int data_type);
  int (*query_max_output_channels) (dlb_udc_output_mode mode);
  int (*push_timeslice) (dlb_udc * self, const char *indata, size_t indatasz);
  int (*process_block) (dlb_udc * self, dlb_buffer * outbuf, size_t *framesz,
      dlb_evo_payload * metadata, dlb_udc_audio_info * audio_info);
} dlb_udc_dispatch_table;

static dlb_udc_dispatch_table dispatch_table;

int
dlb_udc_try_open_dynlib (void)
{
  void *libudc = open_dynamic_lib (DLB_UDC_LIBNAME);
  if (!libudc)
    return 1;

  dispatch_table.new = get_proc_address (libudc, "dlb_udc_new");
  dispatch_table.free = get_proc_address (libudc, "dlb_udc_free");
  dispatch_table.drc_settings_init =
      get_proc_address (libudc, "dlb_udc_drc_settings_init");
  dispatch_table.drc_settings_set =
      get_proc_address (libudc, "dlb_udc_drc_settings_set");
  dispatch_table.query_max_outbuf_size =
      get_proc_address (libudc, "dlb_udc_query_max_outbuf_size");
  dispatch_table.query_max_output_channels =
      get_proc_address (libudc, "dlb_udc_query_max_output_channels");
  dispatch_table.push_timeslice =
      get_proc_address (libudc, "dlb_udc_push_timeslice");
  dispatch_table.process_block =
      get_proc_address (libudc, "dlb_udc_process_block");
  dispatch_table.query_latency =
      get_proc_address (libudc, "dlb_udc_query_latency");

  return 0;
}

dlb_udc *
dlb_udc_new (const dlb_udc_init_info * info)
{
  return dispatch_table.new (info);
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
dlb_udc_query_latency_samples (dlb_udc * self)
{
  return dispatch_table.query_latency (self);
}


size_t
dlb_udc_query_max_outbuf_size (dlb_udc_output_mode mode, int data_type)
{
  return dispatch_table.query_max_outbuf_size (mode, data_type);
}

int
dlb_udc_query_max_output_channels (dlb_udc_output_mode mode)
{
  return dispatch_table.query_max_output_channels (mode);
}

int
dlb_udc_push_timeslice (dlb_udc * self, const char *indata, size_t indatasz)
{
  return dispatch_table.push_timeslice (self, indata, indatasz);
}

int
dlb_udc_process_block (dlb_udc * self, dlb_buffer * outbuf, size_t *framesz,
    dlb_evo_payload * metadata, dlb_udc_audio_info * audio_info)
{
  return dispatch_table.process_block (self, outbuf, framesz, metadata,
      audio_info);
}
