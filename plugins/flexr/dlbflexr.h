/*******************************************************************************

 * Dolby Home Audio GStreamer Plugins
 * Copyright (C) 2021-2022, Dolby Laboratories

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

#ifndef __GST_FLEXR_H__
#define __GST_FLEXR_H__

#include <gst/gst.h>
#include <gst/audio/audio.h>
#include <gst/audio/gstaudioaggregator.h>

#include "dlb_flexr.h"

G_BEGIN_DECLS

#define DLB_TYPE_FLEXR            (dlb_flexr_get_type())
#define DLB_FLEXR(obj)            (G_TYPE_CHECK_INSTANCE_CAST((obj),DLB_TYPE_FLEXR,DlbFlexr))
#define DLB_IS_FLEXR(obj)         (G_TYPE_CHECK_INSTANCE_TYPE((obj),DLB_TYPE_FLEXR))
#define DLB_FLEXR_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST((klass) ,DLB_TYPE_FLEXR,DlbFlexrClass))
#define DLB_IS_FLEXR_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass) ,DLB_TYPE_FLEXR))
#define DLB_FLEXR_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj) ,DLB_TYPE_FLEXR,DlbFlexrClass))

typedef struct _DlbFlexr             DlbFlexr;
typedef struct _DlbFlexrClass        DlbFlexrClass;

typedef struct _DlbFlexrPad DlbFlexrPad;
typedef struct _DlbFlexrPadClass DlbFlexrPadClass;

/**
 * DlbFlexr:
 *
 * The flexible renderer object structure.
 */
struct _DlbFlexr {
  GstAudioAggregator element;

  /*< private >*/
  dlb_flexr *flexr_instance;

  gint channels;
  gint streams;
  gint latency;
  gint blksize;
  gint ext_gain_step;
  gdouble ext_gain;

  gchar *config_path;

  gboolean active_channels_enable;
  guint64 active_channels_mask;

  GList *flushing_streams;
};

struct _DlbFlexrClass {
  GstAudioAggregatorClass parent_class;
};

GType    dlb_flexr_get_type (void);

#define DLB_TYPE_FLEXR_PAD            (dlb_flexr_pad_get_type())
#define DLB_FLEXR_PAD(obj)            (G_TYPE_CHECK_INSTANCE_CAST((obj),DLB_TYPE_FLEXR_PAD,DlbFlexrPad))
#define DLB_IS_FLEXR_PAD(obj)         (G_TYPE_CHECK_INSTANCE_TYPE((obj),DLB_TYPE_FLEXR_PAD))
#define DLB_FLEXR_PAD_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST((klass) ,DLB_TYPE_FLEXR_PAD,DlbFlexrPadClass))
#define DLB_IS_FLEXR_PAD_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass) ,DLB_TYPE_FLEXR_PAD))
#define DLB_FLEXR_PAD_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj) ,DLB_TYPE_FLEXR_PAD,DlbFlexrPadClass))

struct _DlbFlexrPad {
  GstAudioAggregatorPad parent;

  gchar *config_path;
  gboolean upmix;
  gdouble internal_user_gain;
  gdouble content_normalization_gain;

  /*< private >*/
  GHashTable *props_set;

  gboolean force_order;

  dlb_flexr_input_format fmt;
  dlb_flexr_stream_handle stream;
  dlb_flexr_interp_mode interp;
};

struct _DlbFlexrPadClass {
  GstAudioAggregatorPadClass parent_class;
};

GType dlb_flexr_pad_get_type (void);
G_END_DECLS


#endif /* __GST_FLEXR_H__ */
