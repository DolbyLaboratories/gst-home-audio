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

#ifndef __DLBAUDIODECODER_H_
#define __DLBAUDIODECODER_H_

#include <gst/gst.h>

G_BEGIN_DECLS

#define DLB_TYPE_AUDIO_DECODER \
  (dlb_audio_decoder_get_type ())
#define DLB_AUDIO_DECODER(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), DLB_TYPE_AUDIO_DECODER, DlbAudioDecoder))
#define DLB_IS_AUDIO_DECODER(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), DLB_TYPE_AUDIO_DECODER))
#define DLB_AUDIO_DECODER_GET_INTERFACE(inst) \
  (G_TYPE_INSTANCE_GET_INTERFACE ((inst), DLB_TYPE_AUDIO_DECODER, DlbAudioDecoderInterface))

#define DLB_TYPE_AUDIO_DECODER_OUT_MODE                                        \
  (dlb_audio_decoder_out_ch_config_get_type())

#define DLB_TYPE_AUDIO_DECODER_DRC_MODE                                        \
  (dlb_audio_decoder_drc_mode_get_type())

typedef struct _DlbAudioDecoder DlbAudioDecoder;
typedef struct _DlbAudioDecoderInterface DlbAudioDecoderInterface;

struct _DlbAudioDecoderInterface {
  GTypeInterface iface;
};

/**
 * DlbAudioDecoderOutmode:
 * Supported output channel configurations
 */
typedef enum
{
  DLB_AUDIO_DECODER_OUT_MODE_INVALID = -1,
  DLB_AUDIO_DECODER_OUT_MODE_2_0 = 0,
  DLB_AUDIO_DECODER_OUT_MODE_2_1,
  DLB_AUDIO_DECODER_OUT_MODE_3_0,
  DLB_AUDIO_DECODER_OUT_MODE_3_1,
  DLB_AUDIO_DECODER_OUT_MODE_4_0,
  DLB_AUDIO_DECODER_OUT_MODE_4_1,
  DLB_AUDIO_DECODER_OUT_MODE_5_0,
  DLB_AUDIO_DECODER_OUT_MODE_5_1,
  DLB_AUDIO_DECODER_OUT_MODE_6_0,
  DLB_AUDIO_DECODER_OUT_MODE_6_1,
  DLB_AUDIO_DECODER_OUT_MODE_7_0,
  DLB_AUDIO_DECODER_OUT_MODE_7_1,
  DLB_AUDIO_DECODER_OUT_MODE_9_1,
  DLB_AUDIO_DECODER_OUT_MODE_5_1_2,
  DLB_AUDIO_DECODER_OUT_MODE_5_1_4,
  DLB_AUDIO_DECODER_OUT_MODE_7_1_2,
  DLB_AUDIO_DECODER_OUT_MODE_7_1_4,
  DLB_AUDIO_DECODER_OUT_MODE_7_1_6,
  DLB_AUDIO_DECODER_OUT_MODE_9_1_2,
  DLB_AUDIO_DECODER_OUT_MODE_9_1_4,
  DLB_AUDIO_DECODER_OUT_MODE_9_1_6,
  DLB_AUDIO_DECODER_OUT_MODE_RAW,
  DLB_AUDIO_DECODER_OUT_MODE_CORE,
} DlbAudioDecoderOutMode;

/**
 * DlbAudioDecoderDrcMode:
 * Enumeration dynamic range control modes
 */
typedef enum
{
  DLB_AUDIO_DECODER_DRC_MODE_DISABLE,
  DLB_AUDIO_DECODER_DRC_MODE_ENABLE,
  DLB_AUDIO_DECODER_DRC_MODE_AUTO,
  DLB_AUDIO_DECODER_DRC_MODE_DEFAULT = DLB_AUDIO_DECODER_DRC_MODE_DISABLE,
} DlbAudioDecoderDrcMode;

G_END_DECLS

GType                  dlb_audio_decoder_get_type       (void);

gint                   dlb_audio_decoder_get_channels   (DlbAudioDecoderOutMode mode);

void                   dlb_audio_decoder_set_out_mode   (DlbAudioDecoder *decoder,
                                                         DlbAudioDecoderOutMode mode);

DlbAudioDecoderOutMode dlb_audio_decoder_get_out_mode   (DlbAudioDecoder *decoder);

void                   dlb_audio_decoder_set_drc_mode   (DlbAudioDecoder *decoder,
                                                         DlbAudioDecoderDrcMode mode);

DlbAudioDecoderDrcMode dlb_audio_decoder_get_drc_mode   (DlbAudioDecoder *decoder);

void                   dlb_audio_decoder_set_drc_cut    (DlbAudioDecoder *decoder,
                                                         gdouble val);

gdouble                dlb_audio_decoder_get_drc_cut    (DlbAudioDecoder *decoder);

void                   dlb_audio_decoder_set_drc_boost  (DlbAudioDecoder *decoder,
                                                         gdouble val);

gdouble                dlb_audio_decoder_get_drc_boost  (DlbAudioDecoder *decoder);

void                   dlb_audio_decoder_set_dmx_enable (DlbAudioDecoder *decoder,
                                                         gboolean val);

gboolean               dlb_audio_decoder_get_dmx_enable (DlbAudioDecoder *decoder);

GType                  dlb_audio_decoder_out_ch_config_get_type (void);

GType                  dlb_audio_decoder_drc_mode_get_type (void);

#endif // __DLBAUDIODECODER_H_
