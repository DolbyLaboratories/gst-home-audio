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

#ifndef _GST_DAP_H_
#define _GST_DAP_H_

#include <gst/base/gstbasetransform.h>
#include <gst/base/gstadapter.h>
#include <gst/audio/audio.h>

#include "dlbdapjson.h"
#include "dlb_dap.h"

G_BEGIN_DECLS

#define GST_TYPE_DAP   (dlb_dap_get_type())
#define DLB_DAP(obj)   (G_TYPE_CHECK_INSTANCE_CAST((obj),GST_TYPE_DAP,DlbDap))
#define GST_DAP_CLASS(klass)   (G_TYPE_CHECK_CLASS_CAST((klass),GST_TYPE_DAP,DlbDapClass))
#define GST_IS_DAP(obj)   (G_TYPE_CHECK_INSTANCE_TYPE((obj),GST_TYPE_DAP))
#define GST_IS_DAP_CLASS(obj)   (G_TYPE_CHECK_CLASS_TYPE((klass),GST_TYPE_DAP))

typedef struct _DlbDap DlbDap;
typedef struct _DlbDapClass DlbDapClass;

struct _DlbDap
{
  GstBaseTransform base_dap;

  /* private */
  GMutex lock;
  GstAudioInfo ininfo;
  GstAudioInfo outinfo;
  GstAdapter *adapter;

  gint transform_blocks;
  gsize inbufsz;
  gsize outbufsz;

  gsize prefill;
  gsize latency;
  gint latency_samples;
  GstClockTime latency_time;

  gboolean discard_latency;
  gboolean force_order;

  dlb_dap *dap_instance;
  dlb_dap_channel_format infmt;
  dlb_dap_channel_format outfmt;

  gboolean virtualizer_enable;

  dlb_dap_global_settings global_conf;
  dlb_dap_virtualizer_settings virt_conf;
  dlb_dap_gain_settings gains;
  dlb_dap_profile_settings profile;

  guchar *serialized_config;

  /* json config path */
  gchar *json_config_path;
};

struct _DlbDapClass
{
  GstBaseTransformClass base_dap_class;
};

GType dlb_dap_get_type (void);

G_END_DECLS

#endif
