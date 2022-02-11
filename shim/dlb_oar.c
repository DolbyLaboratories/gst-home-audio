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

#include "dlb_oar.h"
#include "common_shim.h"

typedef struct dlb_oar_dispatch_table_s
{
  dlb_oar *(*new) (const dlb_oar_init_info * info);
  void (*free) (dlb_oar * self);

  void (*push_oamd_payload) (dlb_oar *self, const dlb_oar_payload *payload,
      int payload_num);
  void (*process) (dlb_oar * self, const dlb_buffer * inbuf,
      dlb_buffer * outbuf, int samples);
  void (*reset) (dlb_oar * self, int sample_rate);
  int (*query_latency) (dlb_oar * self);
  int (*query_min_block_samples) (dlb_oar * self);
  int (*query_max_block_samples) (dlb_oar * self);
  int (*query_max_payloads) (dlb_oar * self);
} dlb_oar_dispatch_table;

static dlb_oar_dispatch_table dispatch_table;

int
dlb_oar_try_open_dynlib (void)
{
  void *liboar = open_dynamic_lib (DLB_OAR_LIBNAME);
  if (!liboar)
    return 1;

  dispatch_table.new = get_proc_address (liboar, "dlb_oar_new");
  dispatch_table.free = get_proc_address (liboar, "dlb_oar_free");
  dispatch_table.push_oamd_payload =
      get_proc_address (liboar, "dlb_oar_push_oamd_payload");
  dispatch_table.process = get_proc_address (liboar, "dlb_oar_process");
  dispatch_table.reset = get_proc_address (liboar, "dlb_oar_reset");
  dispatch_table.query_latency =
      get_proc_address (liboar, "dlb_oar_query_latency");
  dispatch_table.query_min_block_samples =
      get_proc_address (liboar, "dlb_oar_query_min_block_samples");
  dispatch_table.query_max_block_samples =
      get_proc_address (liboar, "dlb_oar_query_max_block_samples");
  dispatch_table.query_max_payloads =
      get_proc_address (liboar, "dlb_oar_query_max_payloads");

  return 0;
}

dlb_oar *
dlb_oar_new (const dlb_oar_init_info * info)
{
  return dispatch_table.new (info);
}

void
dlb_oar_free (dlb_oar * self)
{
  dispatch_table.free (self);
}

void
dlb_oar_push_oamd_payload (dlb_oar *self, const dlb_oar_payload *payload,
    int payload_num)
{
  dispatch_table.push_oamd_payload (self, payload, payload_num);
}

void
dlb_oar_process (dlb_oar * self, const dlb_buffer * inbuf, dlb_buffer * outbuf,
    int samples)
{
  dispatch_table.process (self, inbuf, outbuf, samples);
}

void
dlb_oar_reset (dlb_oar * self, int sample_rate)
{
  dispatch_table.reset (self, sample_rate);
}

int
dlb_oar_query_latency (dlb_oar * self)
{
  return dispatch_table.query_latency (self);
}

int
dlb_oar_query_min_block_samples (dlb_oar * self)
{
  return dispatch_table.query_min_block_samples (self);
}

int
dlb_oar_query_max_block_samples (dlb_oar * self)
{
  return dispatch_table.query_max_block_samples (self);
}

int
dlb_oar_query_max_payloads (dlb_oar * self)
{
  return dispatch_table.query_max_payloads (self);
}
