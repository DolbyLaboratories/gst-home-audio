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

#ifndef _DLB_AC3DEC_H_
#define _DLB_AC3DEC_H_

#include "dlbutils.h"
#include "dlbaudiodecoder.h"

#include "dlb_udc.h"

G_BEGIN_DECLS
#define DLB_TYPE_AC3DEC   (dlb_ac3dec_get_type())
#define DLB_AC3DEC(obj)   (G_TYPE_CHECK_INSTANCE_CAST((obj),DLB_TYPE_AC3DEC,DlbAc3Dec))
#define DLB_AC3DEC_CLASS(klass)   (G_TYPE_CHECK_CLASS_CAST((klass),DLB_TYPE_AC3DEC,GstAc3DecClass))
#define DLB_IS_AC3DEC(obj)   (G_TYPE_CHECK_INSTANCE_TYPE((obj),DLB_TYPE_AC3DEC))
#define DLB_IS_AC3DEC_CLASS(obj)   (G_TYPE_CHECK_CLASS_TYPE((klass),DLB_TYPE_AC3DEC))
typedef struct _DlbAc3Dec DlbAc3Dec;
typedef struct _DlbAc3DecClass DlbAc3DecClass;

struct _DlbAc3Dec
{
  GstAudioDecoder base_ac3dec;

  dlb_udc *udc;
  dlb_udc_audio_info info;
  dlb_udc_output_mode mode;

  dlb_buffer *outbuf;

  GstAllocator *alloc_dec;
  GstAllocationParams *alloc_params;
  guint8 *metadata_buffer;
  GstTagList *tags;

  /* reorder positions from dolby to gstreamer */
  GstAudioChannelPosition gstpos[16];
  GstAudioChannelPosition dlbpos[16];

  /* maximum output block size in bytes */
  gsize max_output_blocksz;

  /* max output channels */
  gint max_channels;

  /* bytes per sample */
  gsize bps;

  /* target layout (depends on downstream source pad peer Caps) */
  GstAudioFormat output_format;

  /* static params */
  DlbAudioDecoderOutMode outmode;

  /* dynamic params */
  gint drc_mode;
  dlb_udc_drc_settings drc;

  gboolean dmx_enable;
};

struct _DlbAc3DecClass
{
  GstAudioDecoderClass base_ac3dec_class;
};

GType dlb_ac3dec_get_type (void);

G_END_DECLS
#endif
