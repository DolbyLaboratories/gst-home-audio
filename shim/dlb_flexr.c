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

#include "dlb_flexr.h"
#include "common_shim.h"

typedef struct dlb_flexr_dispatch_table_s
{
  dlb_flexr *(*new) (const dlb_flexr_init_info * info);
  void (*free) (dlb_flexr * self);

  dlb_flexr_stream_handle (*add_stream) (dlb_flexr * self,
      const dlb_flexr_stream_info * info);

  void (*rm_stream) (dlb_flexr * self, dlb_flexr_stream_handle stream);

  int (*push_stream) (dlb_flexr * self,
      dlb_flexr_stream_handle stream,
      dlb_flexr_object_metadata * md, dlb_buffer * inbuf, int samples);

  int (*generate_output) (dlb_flexr * self, dlb_buffer * outbuf, int *samples);


  int (*query_num_outputs) (const dlb_flexr * self);

  int (*query_block_samples) (const dlb_flexr * self);

  int (*query_max_samples) (const dlb_flexr * self);

  int (*query_latency) (const dlb_flexr * self);

  void (*stream_info_init) (dlb_flexr_stream_info * info);

  int (*query_pushed_samples) (const dlb_flexr * self,
      dlb_flexr_stream_handle stream);

  void (*set_render_config) (dlb_flexr * self,
      dlb_flexr_stream_handle stream,
      const uint8_t * serialized_config,
      size_t serialized_config_size,
      dlb_flexr_interp_mode interp, int xfade_blocks);

  void (*set_volume) (dlb_flexr * self,
      dlb_flexr_stream_handle stream, float volume);

} dlb_flexr_dispatch_table;

static dlb_flexr_dispatch_table dispatch_table;

int
dlb_flexr_try_open_dynlib (void)
{
  void *libflexr = open_dynamic_lib ("libdlb_flexr.so");
  if (!libflexr)
    return 1;

  dispatch_table.new = get_proc_address (libflexr, "dlb_flexr_new");
  dispatch_table.free = get_proc_address (libflexr, "dlb_flexr_free");
  dispatch_table.add_stream =
      get_proc_address (libflexr, "dlb_flexr_add_stream");
  dispatch_table.rm_stream = get_proc_address (libflexr, "dlb_flexr_rm_stream");
  dispatch_table.push_stream =
      get_proc_address (libflexr, "dlb_flexr_push_stream");
  dispatch_table.generate_output =
      get_proc_address (libflexr, "dlb_flexr_generate_output");

  dispatch_table.query_num_outputs =
      get_proc_address (libflexr, "dlb_flexr_query_num_outputs");
  dispatch_table.query_block_samples =
      get_proc_address (libflexr, "dlb_flexr_query_block_samples");
  dispatch_table.query_max_samples =
      get_proc_address (libflexr, "dlb_flexr_query_max_samples");
  dispatch_table.query_latency =
      get_proc_address (libflexr, "dlb_flexr_query_latency");
  dispatch_table.stream_info_init =
      get_proc_address (libflexr, "dlb_flexr_stream_info_init");
  dispatch_table.query_pushed_samples =
      get_proc_address (libflexr, "dlb_flexr_query_pushed_samples");
  dispatch_table.set_render_config =
      get_proc_address (libflexr, "dlb_flexr_set_render_config");
  dispatch_table.set_volume =
      get_proc_address (libflexr, "dlb_flexr_set_volume");

  return 0;
}

dlb_flexr *
dlb_flexr_new (const dlb_flexr_init_info * info)
{
  return dispatch_table.new (info);
}

void
dlb_flexr_free (dlb_flexr * self)
{
  dispatch_table.free (self);
}

dlb_flexr_stream_handle
dlb_flexr_add_stream (dlb_flexr * self, const dlb_flexr_stream_info * info)
{
  return dispatch_table.add_stream (self, info);
}

void
dlb_flexr_rm_stream (dlb_flexr * self, dlb_flexr_stream_handle stream)
{
  dispatch_table.rm_stream (self, stream);
}

int
dlb_flexr_push_stream (dlb_flexr * self,
    dlb_flexr_stream_handle stream,
    dlb_flexr_object_metadata * md, dlb_buffer * inbuf, int samples)
{
  return dispatch_table.push_stream (self, stream, md, inbuf, samples);
}

int
dlb_flexr_generate_output (dlb_flexr * self, dlb_buffer * outbuf, int *samples)
{
  return dispatch_table.generate_output (self, outbuf, samples);
}

int
dlb_flexr_query_num_outputs (const dlb_flexr * self)
{
  return dispatch_table.query_num_outputs (self);
}

int
dlb_flexr_query_block_samples (const dlb_flexr * self)
{
  return dispatch_table.query_block_samples (self);
}

int
dlb_flexr_query_max_samples (const dlb_flexr * self)
{
  return dispatch_table.query_block_samples (self);
}

int
dlb_flexr_query_latency (const dlb_flexr * self)
{
  return dispatch_table.query_latency (self);
}

void
dlb_flexr_stream_info_init (dlb_flexr_stream_info * info)
{
  dispatch_table.stream_info_init (info);
}

int
dlb_flexr_query_pushed_samples (const dlb_flexr * self,
    dlb_flexr_stream_handle stream)
{
  return dispatch_table.query_pushed_samples (self, stream);
}

void
dlb_flexr_set_render_config (dlb_flexr * self,
    dlb_flexr_stream_handle stream,
    const uint8_t * serialized_config,
    size_t serialized_config_size, dlb_flexr_interp_mode interp,
    int xfade_blocks)
{
  dispatch_table.set_render_config (self, stream, serialized_config,
      serialized_config_size, interp, xfade_blocks);
}

void
dlb_flexr_set_volume (dlb_flexr * self,
    dlb_flexr_stream_handle stream, float volume)
{
  dispatch_table.set_volume (self, stream, volume);
}
