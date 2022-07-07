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

#ifndef DLB_AUDIO_DEC_BIN_H
#define DLB_AUDIO_DEC_BIN_H

#include <gst/gst.h>
#include <gst/audio/audio.h>

G_BEGIN_DECLS
#define DLB_TYPE_AUDIO_DEC_BIN            (dlb_audio_dec_bin_get_type())
#define DLB_AUDIO_DEC_BIN(obj)            (G_TYPE_CHECK_INSTANCE_CAST((obj),DLB_TYPE_AUDIO_DEC_BIN,DlbAudioDecBin))
#define DLB_AUDIO_DEC_BIN_CAST(obj)       ((DlbAudioDecBin*)obj)
#define DLB_AUDIO_DEC_BIN_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST((klass),DLB_TYPE_AUDIO_DEC_BIN,DlbAudioDecBinClass))
#define GST_IS_VIEWFINDER_BIN(obj)        (G_TYPE_CHECK_INSTANCE_TYPE((obj),DLB_TYPE_AUDIO_DEC_BIN))
#define GST_IS_VIEWFINDER_BIN_CLASS(obj)  (G_TYPE_CHECK_CLASS_TYPE((klass),DLB_TYPE_AUDIO_DEC_BIN))
typedef struct _DlbAudioDecBin DlbAudioDecBin;
typedef struct _DlbAudioDecBinClass DlbAudioDecBinClass;

struct _DlbAudioDecBin
{
  GstBin bin;

  GstPad *sink;
  GstPad *src;

  gchar *stream;
  gint outmode;
  gint drcmode;
  gdouble drccut;
  gdouble drcboost;
  gboolean dmxenable;

  /* Processing elements */
  GstElement *typefind;
  GstElement *parser;
  GstElement *dec;
  GstElement *oar;
  GstElement *conv;
  GstElement *capsfilter;

  guint32 caps_seqnum;

  gboolean have_type;
  gulong have_type_id;
  gulong dec_probe_id;
};

struct _DlbAudioDecBinClass
{
  GstBinClass bin_class;
};

GType dlb_audio_dec_bin_get_type (void);
gboolean dlb_audio_dec_bin_plugin_init (GstPlugin * plugin);

G_END_DECLS
#endif /* DLB_AUDIO_DEC_BIN_H */
