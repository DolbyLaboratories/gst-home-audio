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

#define GLIB_DISABLE_DEPRECATION_WARNINGS 1

#include "dlbaudiometa.h"

static gboolean
dlb_object_audio_meta_init (GstMeta * meta, gpointer G_GNUC_UNUSED params,
    GstBuffer G_GNUC_UNUSED * buffer)
{
  DlbObjectAudioMeta *oameta = (DlbObjectAudioMeta *) meta;

  oameta->payload = NULL;
  oameta->size = 0;
  oameta->offset = 0;
  oameta->bpf = 1;

  return TRUE;
}

static void
dlb_object_audio_meta_free (GstMeta * meta, GstBuffer G_GNUC_UNUSED * buffer)
{
  DlbObjectAudioMeta *oameta = (DlbObjectAudioMeta *) meta;

  if (oameta->payload)
    g_free (oameta->payload);
}

static gboolean
dlb_object_audio_meta_transform (GstBuffer * dest, GstMeta * meta,
    GstBuffer G_GNUC_UNUSED * buffer, GQuark type, gpointer data)
{
  DlbObjectAudioMeta *smeta, *dmeta;
  GstMetaTransformCopy *copy;
  gsize byte_offset;
  gboolean res = TRUE;

  if (GST_META_TRANSFORM_IS_COPY (type) && data) {
    copy = (GstMetaTransformCopy *) data;
    smeta = (DlbObjectAudioMeta *) meta;
    if (copy->region) {
      byte_offset = smeta->offset * smeta->bpf;
      if (copy->offset <= byte_offset
          && byte_offset <= copy->offset + copy->size) {
        const gsize offset = (byte_offset - copy->offset) / smeta->bpf;
        dmeta =
            dlb_audio_object_meta_add (dest, smeta->payload, smeta->size,
            offset, smeta->bpf);
        res = NULL != dmeta;
      }
    } else {
      dmeta =
          dlb_audio_object_meta_add (dest, smeta->payload, smeta->size,
          smeta->offset, smeta->bpf);
      res = NULL != dmeta;
    }
  }

  return res;
}

GType
dlb_object_audio_meta_api_get_type (void)
{
  static GType type;
  static const gchar *tags[] = { "oamdi", NULL };

  if (g_once_init_enter (&type)) {
    GType _type = gst_meta_api_type_register ("DlbObjectAudioMetaAPI", tags);
    g_once_init_leave (&type, _type);
  }
  return type;
}

const GstMetaInfo *
dlb_object_audio_meta_get_info (void)
{
  static const GstMetaInfo *object_audio_meta_info = NULL;

  if (g_once_init_enter ((GstMetaInfo **) & object_audio_meta_info)) {
    const GstMetaInfo *meta = gst_meta_register (DLB_OBJECT_AUDIO_META_API_TYPE,
        "DlbObjectAudioMeta", sizeof (DlbObjectAudioMeta),
        dlb_object_audio_meta_init, dlb_object_audio_meta_free,
        dlb_object_audio_meta_transform);
    g_once_init_leave ((GstMetaInfo **) & object_audio_meta_info,
        (GstMetaInfo *) meta);
  }
  return object_audio_meta_info;
}

DlbObjectAudioMeta *
dlb_audio_object_meta_add (GstBuffer * buffer, gconstpointer payload,
    const gsize size, const gsize offset, const gsize bpf)
{
  DlbObjectAudioMeta *meta;

  g_return_val_if_fail (payload != NULL, NULL);
  g_return_val_if_fail (size > 0, NULL);

  meta =
      (DlbObjectAudioMeta *) gst_buffer_add_meta (buffer,
      DLB_OBJECT_AUDIO_META_INFO, NULL);

  meta->payload = g_memdup (payload, size);
  meta->size = size;
  meta->offset = offset;
  meta->bpf = bpf;

  return meta;
}

guint
dlb_audio_object_meta_get_n (GstBuffer * buffer)
{
  return gst_buffer_get_n_meta (buffer, DLB_OBJECT_AUDIO_META_API_TYPE);
}

DlbObjectAudioMeta *
dlb_audio_object_meta_get (GstBuffer * buffer)
{
  return (DlbObjectAudioMeta *) gst_buffer_get_meta (buffer,
      DLB_OBJECT_AUDIO_META_API_TYPE);
}

DlbObjectAudioMeta *
dlb_audio_object_meta_iterate (GstBuffer * buffer, gpointer * state)
{
  return (DlbObjectAudioMeta *) gst_buffer_iterate_meta_filtered (buffer, state,
      DLB_OBJECT_AUDIO_META_API_TYPE);
}
