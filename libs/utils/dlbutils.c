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

#include "dlbutils.h"

/**
 * _GSTMSK:
 * @ch: the GST_AUDIO_CHANNEL_POSITION enum value
 * returns: GStreamer Audio Channel mask for given position enum value
 */
#define GSTMSK(ch) (G_GUINT64_CONSTANT (1) << ch)

static const GstAudioChannelPosition dlb_channel_order [] = {
  GST_AUDIO_CHANNEL_POSITION_FRONT_LEFT,
  GST_AUDIO_CHANNEL_POSITION_FRONT_RIGHT,
  GST_AUDIO_CHANNEL_POSITION_FRONT_CENTER,
  GST_AUDIO_CHANNEL_POSITION_LFE1,
  GST_AUDIO_CHANNEL_POSITION_SIDE_LEFT,
  GST_AUDIO_CHANNEL_POSITION_SIDE_RIGHT,
  GST_AUDIO_CHANNEL_POSITION_REAR_LEFT,
  GST_AUDIO_CHANNEL_POSITION_REAR_RIGHT,
  GST_AUDIO_CHANNEL_POSITION_REAR_CENTER,
  GST_AUDIO_CHANNEL_POSITION_FRONT_LEFT_OF_CENTER,
  GST_AUDIO_CHANNEL_POSITION_FRONT_RIGHT_OF_CENTER,
  GST_AUDIO_CHANNEL_POSITION_WIDE_LEFT,
  GST_AUDIO_CHANNEL_POSITION_WIDE_RIGHT,
  GST_AUDIO_CHANNEL_POSITION_TOP_FRONT_LEFT,
  GST_AUDIO_CHANNEL_POSITION_TOP_FRONT_RIGHT,
  GST_AUDIO_CHANNEL_POSITION_TOP_SIDE_LEFT,
  GST_AUDIO_CHANNEL_POSITION_TOP_SIDE_RIGHT,
  GST_AUDIO_CHANNEL_POSITION_TOP_REAR_LEFT,
  GST_AUDIO_CHANNEL_POSITION_TOP_REAR_RIGHT,
  GST_AUDIO_CHANNEL_POSITION_SURROUND_LEFT,
  GST_AUDIO_CHANNEL_POSITION_SURROUND_RIGHT,
  GST_AUDIO_CHANNEL_POSITION_TOP_FRONT_CENTER,
  GST_AUDIO_CHANNEL_POSITION_TOP_CENTER,
  GST_AUDIO_CHANNEL_POSITION_TOP_REAR_CENTER,
  GST_AUDIO_CHANNEL_POSITION_LFE2,
};

static gboolean
dlb_channel_positions_from_channel_mask (gint channels,
    const guint64 channel_mask, GstAudioChannelPosition *positions)
{
  guint64 tmp_mask = channel_mask;
  guint i, tmp_channels = 0;

  /* fill present channels in gst_chmask using DOLBY_ORDER */
  for (i = 0; i < G_N_ELEMENTS (dlb_channel_order); ++i) {
    const guint64 mask = channel_mask & GSTMSK (dlb_channel_order[i]);
    positions[i] = GST_AUDIO_CHANNEL_POSITION_INVALID;

    if (mask) {
      tmp_mask &= ~mask;
      positions[tmp_channels++] = dlb_channel_order[i];
    }
  }

  /**
   * on input we expect only channel positions supported by Dolby
   * if any of not supported channel mask was added then tmp != 0
   */
  return !tmp_mask;
}

dlb_buffer *
dlb_buffer_new (const GstAudioInfo * info)
{
  gint channels;
  GstAudioFormat fmt;
  GstAudioLayout layout;

  dlb_buffer *buf = g_slice_new (dlb_buffer);

  channels = GST_AUDIO_INFO_CHANNELS (info);
  layout = GST_AUDIO_INFO_LAYOUT (info);
  fmt = GST_AUDIO_FORMAT_INFO_FORMAT (info->finfo);

  buf->nchannel = channels;
  buf->nstride = (layout == GST_AUDIO_LAYOUT_INTERLEAVED) ? channels : 1;
  buf->ppdata = g_slice_alloc (channels * sizeof (void *));

  switch (fmt) {
    case GST_AUDIO_FORMAT_F32:
      buf->data_type = DLB_BUFFER_FLOAT;
      break;
    case GST_AUDIO_FORMAT_F64:
      buf->data_type = DLB_BUFFER_DOUBLE;
      break;
    case GST_AUDIO_FORMAT_S32:
      buf->data_type = DLB_BUFFER_INT_LEFT;
      break;
    case GST_AUDIO_FORMAT_S16:
      buf->data_type = DLB_BUFFER_SHORT_16;
      break;
    case GST_AUDIO_FORMAT_S8:
      buf->data_type = DLB_BUFFER_OCTET_UNPACKED;
      break;
    default:
      dlb_buffer_free (buf);
      buf = NULL;
      GST_ERROR ("dlb_buffer_new: Unsupported data type %d", fmt);
  }

  return buf;
}

static inline void
gst_reorder_map_init (gint * reorder_map, gint channels)
{
  for (gint i = 0; i < channels; ++i)
    reorder_map[i] = i;
}

dlb_buffer *
dlb_buffer_new_wrapped (const guint8 * data, GstAudioInfo * info,
    gboolean force_order)
{
  gint channels, bps, i;
  gint map[64];
  guint64 chmask = 0;

  GstAudioChannelPosition dlb_pos[64];
  GstAudioChannelPosition *gst_pos = info->position;
  dlb_buffer *buf;

  channels = GST_AUDIO_INFO_CHANNELS (info);
  bps = GST_AUDIO_INFO_BPS (info);

  gst_reorder_map_init (map, channels);

  if (force_order) {
    gst_audio_channel_positions_to_mask (gst_pos, channels, TRUE, &chmask);

    if (chmask) {
      if (!dlb_channel_positions_from_channel_mask (channels, chmask, dlb_pos))
        return NULL;
      if (!gst_audio_get_channel_reorder_map (channels, gst_pos, dlb_pos, map))
        return NULL;
    }
  }

  buf = dlb_buffer_new (info);

  for (i = 0; i < channels; ++i) {
    buf->ppdata[i] = (void *) (data + map[i] * bps);
  }

  return buf;
}

void
dlb_buffer_free (dlb_buffer * buf)
{
  if (buf) {
    g_slice_free1 (buf->nchannel * sizeof (void *), buf->ppdata);
    g_slice_free (dlb_buffer, buf);
  }
}

void
dlb_buffer_reorder_channels (dlb_buffer * buf, gint channels, gsize samples,
    const guint * order, guint bps, guint8 * cache)
{
  guint i;
  guint8 *dataptr = (guint8 *) buf->ppdata[0];
  guint8 *tmp = cache;
  const guint stride = buf->nstride;
  const gsize blk_samples = channels * samples;

  memcpy (tmp, dataptr, stride * samples * bps);

  for (i = 0; i < blk_samples;) {
    memcpy (dataptr + (i % channels) * bps, tmp + order[i % channels] * bps,
        bps);
    if (++i % channels == 0) {
      dataptr += channels * bps;
      tmp += stride * bps;
    }
  }
}

void
dlb_get_reorder_map (const GstAudioChannelPosition * orig,
    const GstAudioChannelPosition * reordered, guint channels, guint * map)
{
  guint i, j;

  for (i = 0; i < channels; ++i) {
    for (j = 0; j < channels; ++j) {
      if (orig[i] == GST_AUDIO_CHANNEL_POSITION_NONE
          || orig[i] == GST_AUDIO_CHANNEL_POSITION_INVALID)
        continue;

      if (orig[i] == reordered[j]) {
        map[j] = i;
        break;
      }
    }
  }
}
