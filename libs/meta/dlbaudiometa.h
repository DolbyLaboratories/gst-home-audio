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

#ifndef _GST_DLB_META_H_
#define _GST_DLB_META_H_

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gst/gst.h>

G_BEGIN_DECLS
#define DLB_CAPS_FEATURE_META_OBJECT_AUDIO_META "meta:DlbObjectAudioMeta"
#define DLB_OBJECT_AUDIO_META_API_TYPE (dlb_object_audio_meta_api_get_type())
#define DLB_OBJECT_AUDIO_META_INFO  (dlb_object_audio_meta_get_info())
typedef struct _DlbObjectAudioMeta DlbObjectAudioMeta;

/**
 * DlbObjectAudioMeta:
 * @meta: the parent structure
 * @payload: OAMDI serialized payload 
 * @size: payload size
 * @offset: metadata offset in samples
 * @bpf: number of bytes used for 1-sample on each channel. When
 *      metadata copy operation is performed new offset needs to be
 *      calculated using it
 * 
 * Metadada with OAMDI bitstream payload.
 */
struct _DlbObjectAudioMeta
{
  GstMeta meta;

  gpointer payload;
  gsize size;
  gsize offset;
  gsize bpf;
};

/**
 * dlb_object_audio_meta_api_get_type:
 * Returns: Audio Object metadata type.
 */
GType dlb_object_audio_meta_api_get_type (void);

/**
 * dlb_object_audio_meta_get_info:
 * Returns: Audio Object metadata info structure
 */
const GstMetaInfo *dlb_object_audio_meta_get_info (void);

/**
 * dlb_audio_object_meta_add:
 * @buffer: a #GstBuffer where metadata will be added
 * @payload (transfer none): packed metadata bitstream payload, memory copied
 * @size: payload size
 * @offset: byte position, related to buffer 0-byte, where metadata should be applied
 * @bpf: number of bytes for 1-sample on all channels, needed to recalcualte offsets
 *          when metadata is copied
 * 
 * Adds #DlbObjectAudioMeta to given buffer. Copies provided metadata duplicates.
 * The caller is still responsible for memory management of given data.
 * 
 * Returns: Created and added #DlbObjectAudioMeta or NULL
 */
DlbObjectAudioMeta *dlb_audio_object_meta_add (GstBuffer *
    buffer, gconstpointer payload, const gsize size, const gsize offset,
    const gsize bpf);

/**
 * dlb_audio_object_meta_get_n:
 * @buffer: a #GstBuffer to scann
 * Returns: number of #DlbObjectAudioMeta in the given #GstBuffer
 */
guint dlb_audio_object_meta_get_n (GstBuffer * buffer);

/**
 * dlb_audio_object_meta_get:
 * @buffer: a #GstBuffer
 * Returns: first instance of #DlbObjectAudioMeta from the given #GstBuffer
 *      or NULL when no metadata found.
 */
DlbObjectAudioMeta *dlb_audio_object_meta_get (GstBuffer *
    buffer);
/**
 * dlb_audio_object_meta_iterate:
 * @buffer: a #GstBuffer
 * @state: (out caller-allocates): an opaque state pointer
 * 
 * Retrieve the next #DlbObjectAudioMeta after @current. If @state points
 * to %NULL, the first metadata is returned.
 *
 * @state will be updated with an opaque state pointer
 *
 * Returns: (transfer none) (nullable): The next #DlbObjectAudioMeta or %NULL
 * when there are no more items.
 */
DlbObjectAudioMeta *dlb_audio_object_meta_iterate (GstBuffer *
    buffer, gpointer * state);

G_END_DECLS
#endif /* _GST_DLB_META_H_ */
