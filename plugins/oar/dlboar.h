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

#ifndef _DLB_OAR_H
#define _DLB_OAR_H

#include <gst/base/gstbasetransform.h>

#include "dlb_oar.h"

G_BEGIN_DECLS
#define DLB_TYPE_OAR   (dlb_oar_get_type())
#define DLB_OAR(obj)   (G_TYPE_CHECK_INSTANCE_CAST((obj),DLB_TYPE_OAR,DlbOar))
#define DLB_OAR_CLASS(klass)   (G_TYPE_CHECK_CLASS_CAST((klass),DLB_TYPE_OAR,DlbOarClass))
#define DLB_IS_OAR(obj)   (G_TYPE_CHECK_INSTANCE_TYPE((obj),DLB_TYPE_OAR))
#define DLB_IS_OAR_CLASS(obj)   (G_TYPE_CHECK_CLASS_TYPE((klass),DLB_TYPE_OAR))
typedef struct _DlbOar DlbOar;
typedef struct _DlbOarClass DlbOarClass;

struct _DlbOar
{
  GstBaseTransform base_oar;

  /* Pointer to oar state */
  dlb_oar *oar_instance;
  dlb_oar_init_info oar_config;

  /* latency info */
  gsize prefill;
  gsize latency;
  gint latency_samples;
  GstClockTime latency_time;
  gboolean discard_latency;

  /* OAMD payloads */
  dlb_oar_payload *oamd_payloads;
  gint max_payloads;

  /* Input buffer adapter */
  GstAdapter *adapter;
  gsize max_block_size;
  gsize min_block_size;

  /* Input/Output audio info */
  GstAudioInfo ininfo;
  GstAudioInfo outinfo;
};

struct _DlbOarClass
{
  GstBaseTransformClass base_oar_class;
};

GType dlb_oar_get_type (void);

G_END_DECLS
#endif
