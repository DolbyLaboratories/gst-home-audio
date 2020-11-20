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

#include "dlb_evo_dec.h"
#include "common_shim.h"

typedef struct dlb_evo_dec_dispatch_table_s
{
  dlb_evo_dec *(*new) (void);
  void (*free) (dlb_evo_dec * self);
  int (*decode_oamd_payload) (dlb_evo_dec * evodec, dlb_evo_bitstream * evo,
      dlb_evo_payload * evo_oamd);
} dlb_evo_dec_dispatch_table;

static dlb_evo_dec_dispatch_table dispatch_table;

int
dlb_evo_try_open_dynlib (void)
{
  void *libevo = open_dynamic_lib ("libdlb_evolution.so");
  if (!libevo)
    return 1;

  dispatch_table.new = get_proc_address (libevo, "dlb_evo_dec_new");
  dispatch_table.free = get_proc_address (libevo, "dlb_evo_dec_free");
  dispatch_table.decode_oamd_payload =
      get_proc_address (libevo, "dlb_evo_dec_decode_oamd_process");

  return 0;
}

dlb_evo_dec *
dlb_evodec_new (void)
{
  return dispatch_table.new ();
}

void
dlb_evodec_free (dlb_evo_dec * self)
{
  dispatch_table.free (self);
}

int
dlb_evodec_decode_oamd_payload (dlb_evo_dec * self, dlb_evo_bitstream * evo,
    dlb_evo_payload * evo_oamd)
{
  return dispatch_table.decode_oamd_payload (self, evo, evo_oamd);
}
