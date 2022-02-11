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

#include "dlb_dap.h"
#include "common_shim.h"

typedef struct dlb_dap_dispatch_table_s
{
  dlb_dap *(*new) (const dlb_dap_init_info * info);
  void (*free) (dlb_dap * self);
  void (*preprocess_serialized_config) (const uint8_t * serialized_config,
      dlb_dap_channel_format * intermediate_format, int *output_channels,
      int *virtualizer_enable);
  void (*propose_input_format) (const dlb_dap_channel_format * output_format,
      int virtulizer_enable, dlb_dap_channel_format * input_format);
  int (*process) (dlb_dap * self, const dlb_dap_channel_format * input_format,
      const dlb_buffer * inbuf, dlb_buffer * outbuf);
  int (*query_latency) (dlb_dap * self);
  int (*query_block_samples) (dlb_dap * self);
  void (*virtualizer_settings_init) (dlb_dap_virtualizer_settings * settings);
  void (*set_virtualizer_settings) (dlb_dap * self,
      const dlb_dap_virtualizer_settings * settings);
  void (*profile_settings_init) (dlb_dap_profile_settings * settings);
  void (*set_profile_settings) (dlb_dap * self,
      const dlb_dap_profile_settings * settings);
  void (*gain_settings_init) (dlb_dap_gain_settings * settings);
  void (*set_gain_settings) (dlb_dap * self,
      const dlb_dap_gain_settings * settings);
} dlb_dap_dispatch_table;

static dlb_dap_dispatch_table dispatch_table;

int dlb_dap_try_open_dynlib (void)
{
  void *libdap = open_dynamic_lib (DLB_DAP_LIBNAME);

  if (!libdap)
    return 1;

  dispatch_table.new = get_proc_address (libdap, "dlb_dap_new");
  dispatch_table.free = get_proc_address (libdap, "dlb_dap_free");
  dispatch_table.preprocess_serialized_config =
      get_proc_address (libdap, "dlb_dap_preprocess_serialized_config");
  dispatch_table.propose_input_format =
      get_proc_address (libdap, "dlb_dap_propose_input_format");
  dispatch_table.process = get_proc_address (libdap, "dlb_dap_process");
  dispatch_table.query_latency =
      get_proc_address (libdap, "dlb_dap_query_latency");
  dispatch_table.query_block_samples =
      get_proc_address (libdap, "dlb_dap_query_block_samples");
  dispatch_table.virtualizer_settings_init =
      get_proc_address (libdap, "dlb_dap_virtualizer_settings_init");
  dispatch_table.set_virtualizer_settings =
      get_proc_address (libdap, "dlb_dap_set_virtualizer_settings");
  dispatch_table.profile_settings_init =
      get_proc_address (libdap, "dlb_dap_profile_settings_init");
  dispatch_table.set_profile_settings =
      get_proc_address (libdap, "dlb_dap_set_profile_settings");
  dispatch_table.gain_settings_init =
      get_proc_address (libdap, "dlb_dap_gain_settings_init");
  dispatch_table.set_gain_settings =
      get_proc_address (libdap, "dlb_dap_set_gain_settings");

  return 0;
}

dlb_dap *
dlb_dap_new (const dlb_dap_init_info * info)
{
  return dispatch_table.new (info);
}

void
dlb_dap_free (dlb_dap * self)
{
  dispatch_table.free (self);
}

void
dlb_dap_preprocess_serialized_config (const uint8_t * serialized_config,
    dlb_dap_channel_format * intermediate_format, int *output_channels,
    int *virtualizer_enable)
{
  dispatch_table.preprocess_serialized_config (serialized_config,
      intermediate_format, output_channels, virtualizer_enable);
}

void
dlb_dap_propose_input_format (const dlb_dap_channel_format * output_format,
    int virtulizer_enable, dlb_dap_channel_format * input_format)
{
  dispatch_table.propose_input_format (output_format, virtulizer_enable,
      input_format);
}

int
dlb_dap_process (dlb_dap * self, const dlb_dap_channel_format * input_format,
    const dlb_buffer * inbuf, dlb_buffer * outbuf)
{
  return dispatch_table.process (self, input_format, inbuf, outbuf);
}

int
dlb_dap_query_latency (dlb_dap * self)
{
  return dispatch_table.query_latency (self);
}

int
dlb_dap_query_block_samples (dlb_dap * self)
{
  return dispatch_table.query_block_samples (self);
}

void
dlb_dap_virtualizer_settings_init (dlb_dap_virtualizer_settings * settings)
{
  dispatch_table.virtualizer_settings_init (settings);
}

void
dlb_dap_set_virtualizer_settings (dlb_dap * self,
    const dlb_dap_virtualizer_settings * settings)
{
  dispatch_table.set_virtualizer_settings (self, settings);
}

void
dlb_dap_profile_settings_init (dlb_dap_profile_settings * settings)
{
  dispatch_table.profile_settings_init (settings);
}

void
dlb_dap_set_profile_settings (dlb_dap * self,
    const dlb_dap_profile_settings * settings)
{
  dispatch_table.set_profile_settings (self, settings);
}

void
dlb_dap_gain_settings_init (dlb_dap_gain_settings * settings)
{
  dispatch_table.gain_settings_init (settings);
}

void
dlb_dap_set_gain_settings (dlb_dap * self,
    const dlb_dap_gain_settings * settings)
{
  dispatch_table.set_gain_settings (self, settings);
}
