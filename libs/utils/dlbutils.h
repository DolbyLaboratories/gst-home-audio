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

#ifndef _GST_DLB_UTILS_H_
#define _GST_DLB_UTILS_H_

#include <gst/gst.h>
#include <gst/audio/audio.h>
#include <dlb_buffer.h>

G_BEGIN_DECLS

#define DLB_CHANNEL_MASK_MONO                                                  \
   GST_AUDIO_CHANNEL_POSITION_MASK(FRONT_CENTER)
#define DLB_CHANNEL_MASK_2_0                                                   \
  (GST_AUDIO_CHANNEL_POSITION_MASK(FRONT_LEFT) |                               \
   GST_AUDIO_CHANNEL_POSITION_MASK(FRONT_RIGHT))
#define DLB_CHANNEL_MASK_5_1                                                   \
  (DLB_CHANNEL_MASK_2_0 | GST_AUDIO_CHANNEL_POSITION_MASK(FRONT_CENTER) |      \
   GST_AUDIO_CHANNEL_POSITION_MASK(LFE1) |                                     \
   GST_AUDIO_CHANNEL_POSITION_MASK(REAR_LEFT) |                                \
   GST_AUDIO_CHANNEL_POSITION_MASK(REAR_RIGHT))
#define DLB_CHANNEL_MASK_5_1_2                                                 \
  (DLB_CHANNEL_MASK_5_1 | GST_AUDIO_CHANNEL_POSITION_MASK(TOP_SIDE_LEFT) |     \
   GST_AUDIO_CHANNEL_POSITION_MASK(TOP_SIDE_RIGHT))
#define DLB_CHANNEL_MASK_5_1_4                                                 \
  (DLB_CHANNEL_MASK_5_1 | GST_AUDIO_CHANNEL_POSITION_MASK(TOP_FRONT_LEFT) |    \
   GST_AUDIO_CHANNEL_POSITION_MASK(TOP_FRONT_RIGHT) |                          \
   GST_AUDIO_CHANNEL_POSITION_MASK(TOP_REAR_LEFT) |                            \
   GST_AUDIO_CHANNEL_POSITION_MASK(TOP_REAR_RIGHT))
#define DLB_CHANNEL_MASK_7_1                                                   \
  (DLB_CHANNEL_MASK_5_1 | GST_AUDIO_CHANNEL_POSITION_MASK(SIDE_LEFT) |         \
   GST_AUDIO_CHANNEL_POSITION_MASK(SIDE_RIGHT))
#define DLB_CHANNEL_MASK_7_1_2                                                 \
  (DLB_CHANNEL_MASK_7_1 | GST_AUDIO_CHANNEL_POSITION_MASK(TOP_SIDE_LEFT) |     \
   GST_AUDIO_CHANNEL_POSITION_MASK(TOP_SIDE_RIGHT))
#define DLB_CHANNEL_MASK_7_1_4                                                 \
  (DLB_CHANNEL_MASK_7_1 | GST_AUDIO_CHANNEL_POSITION_MASK(TOP_FRONT_LEFT) |    \
   GST_AUDIO_CHANNEL_POSITION_MASK(TOP_FRONT_RIGHT) |                          \
   GST_AUDIO_CHANNEL_POSITION_MASK(TOP_REAR_LEFT) |                            \
   GST_AUDIO_CHANNEL_POSITION_MASK(TOP_REAR_RIGHT))

/**
 * dlb_buffer_new:
 * @info: the #GstAudioInfo structure describing how the #dlb_buffer
 *          should be configured
 * 
 * Creates #dlb_buffer and configures its format, channels according to the
 *      provided #GstAudioInfo. Buffer does not have assigned memory for
 *      channel samples. These needs to be assigned manually.
 * 
 * returns : (transfer full): the #dlb_buffer that needs to be released using
 *              #dlb_buffer_free function
 */
dlb_buffer *
dlb_buffer_new (const GstAudioInfo * info);

/**
 * dlb_buffer_new_wrapped:
 * @data: data pointer to the channel samples to be assigned to the #dlb_buffer
 * @info: the #GstAudioInfo structure describing how the #dlb_buffer
 *          should be configured
 * @order_force: forces order required by GStreamer
 * 
 * Creates the #dlb_buffer and configures its format, channels according to the
 *     provided #GstAudioInfo. It interprets provided data pointer as channel
 *     samples and configures #dlb_buffer for read/write.
 * 
 * returns : (transfer full): the #dlb_buffer that needs to be released using
 *              #dlb_buffer_free function
 */
dlb_buffer *
dlb_buffer_new_wrapped (const guint8 * data, GstAudioInfo * info,
    gboolean force_order);

/**
 * dlb_buffer_free:
 * @buf: the #dlb_buffer pointer
 * 
 * Releases memory associated with the #dlb_buffer structure and
 * pointer table required hold pointers to channels data.
 *
 * Note: It does not free wrapped memory for channel samples. It needs
 *      to be deallocated manually
 */
void
dlb_buffer_free (dlb_buffer * buf);

/**
 * dlb_buffer_map_channels:
 * @buf: the #dlb_buffer pointer
 * @data: the #data for mapping
 *
 * Maps memory to internal ppdata channel pointers
 */
void
dlb_buffer_map_memory (dlb_buffer * buf, const guint8 * data);

/**
 * dlb_buffer_reorder_channelsL
 * @buf: the #dlb_buffer pointer.
 * @samples: Number of samples in the #dlb_buffer.
 * @order: Channel order map.
 * @bps: Bytes per samples.
 * @cache: Cache buffer. Size of buffer must be able to hold whole #buf.
 *
 * Reorders channels in #dlb_buffer according to provided channel order map.
 */
void
dlb_buffer_reorder_channels (dlb_buffer *buf, gint channels, gsize samples,
    const guint *order, guint bps, guint8 *cache);

/**
 * dlb_get_reorder_map:
 * @orig: Original position map.
 * @reordered: Reorder position map.
 * @channels: Number of channels.
 * @map: Output channels order map.
 *
 * Creates channel order map.
 */
void
dlb_get_reorder_map (const GstAudioChannelPosition * orig,
    const GstAudioChannelPosition * reordered, guint channels, guint * map);

G_END_DECLS

#endif /* _GST_DLB_UTILS_H_ */
