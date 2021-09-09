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

#include "dlbaudiodecoder.h"

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

GType
dlb_audio_decoder_out_ch_config_get_type (void)
{
  static GType config_type = 0;
  static const GEnumValue config_types[] = {
    {DLB_AUDIO_DECODER_OUT_MODE_2_0, "channels: L,R", "2.0"},
    {DLB_AUDIO_DECODER_OUT_MODE_2_1, "channels: L,R,LFE", "2.1"},
    {DLB_AUDIO_DECODER_OUT_MODE_3_0, "channels: L,R,C", "3.0"},
    {DLB_AUDIO_DECODER_OUT_MODE_3_1, "channels: L,R,C,LFE", "3.1"},
    {DLB_AUDIO_DECODER_OUT_MODE_4_0, "channels: L,R,Rs,Ls", "4.0"},
    {DLB_AUDIO_DECODER_OUT_MODE_4_1, "channels: L,R,LFE,Ls,Rs", "4.1"},
    {DLB_AUDIO_DECODER_OUT_MODE_5_0, "channels: L,R,C,Ls,Rs", "5.0"},
    {DLB_AUDIO_DECODER_OUT_MODE_5_1, "channels: L,R,C,LFE,Ls,Rs", "5.1"},
    {DLB_AUDIO_DECODER_OUT_MODE_6_0, "channels: L,R,Ls,Rs,Lrs,Rrs", "6.0"},
    {DLB_AUDIO_DECODER_OUT_MODE_6_1, "channels: L,R,LFE,Ls,Rs,Lrs,Rrs", "6.1"},
    {DLB_AUDIO_DECODER_OUT_MODE_7_0, "channels: L,R,C,Ls,Rs,Lrs,Rrs", "7.0"},
    {DLB_AUDIO_DECODER_OUT_MODE_7_1, "channels: L,R,C,LFE,Ls,Rs,Lrs,Rrs", "7.1"},
    {DLB_AUDIO_DECODER_OUT_MODE_RAW, "RAW, Atmos enabled", "raw"},
    {DLB_AUDIO_DECODER_OUT_MODE_CORE, "Core: 2.x, 3.x, 4.x, 5.x, Atmos disabled", "core"},
    {0, NULL, NULL}
  };

  if (!config_type) {
    config_type =
        g_enum_register_static ("DlbAudioDecoderOutModeType", config_types);
  }

  return config_type;
}

GType
dlb_audio_decoder_drc_mode_get_type (void)
{
  static GType drc_mode_type = 0;
  static const GEnumValue drc_mode_types[] = {
    {DLB_AUDIO_DECODER_DRC_MODE_DISABLE, "Disable", "disable"},
    {DLB_AUDIO_DECODER_DRC_MODE_ENABLE, "Enable", "enable"},
    {DLB_AUDIO_DECODER_DRC_MODE_AUTO, "Auto", "auto"},
    {0, NULL, NULL}
  };

  if (!drc_mode_type) {
    drc_mode_type =
        g_enum_register_static ("DlbAudioDecoderDrcModeType", drc_mode_types);
  }

  return drc_mode_type;
}

static void
dlb_audio_decoder_class_init (DlbAudioDecoderInterface * iface)
{
  g_object_interface_install_property (iface,
      g_param_spec_enum ("out-mode", "Output mode",
          "Set decoder output mode",
          DLB_TYPE_AUDIO_DECODER_OUT_MODE, DLB_AUDIO_DECODER_OUT_MODE_RAW,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_interface_install_property (iface,
      g_param_spec_enum ("drc-mode", "DRC mode",
          "Set decoder dynamic range control mode",
          DLB_TYPE_AUDIO_DECODER_DRC_MODE, DLB_AUDIO_DECODER_DRC_MODE_DISABLE,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_interface_install_property (iface,
      g_param_spec_double ("drc-cut", "DRC cut",
          "Set the dynamic range control cut scale factor, 1.0 = max",
          0.0, 1.0, 1.0, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_interface_install_property (iface,
      g_param_spec_double ("drc-boost", "DRC boost",
          "Set the dynamic range control boost scale factor, 1.0 = max",
          0.0, 1.0, 1.0, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_interface_install_property (iface,
      g_param_spec_boolean ("dmx-enable", "dwnmixer enable",
          "Enable standard downmixer, the channel based output of decoder is "
          "limited to standard channel layouts: 2.0, 5.1 and 7.1",
          TRUE, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
}

GType
dlb_audio_decoder_get_type (void)
{
  static gsize type = 0;
  if (g_once_init_enter (&type)) {
    GType tmp;
    static const GTypeInfo info = {
      sizeof (DlbAudioDecoderInterface),
      NULL,                     /* base_init */
      NULL,                     /* base_finalize */
      (GClassInitFunc) dlb_audio_decoder_class_init,    /* class_init */
      NULL,                     /* class_finalize */
      NULL,                     /* class_data */
      0,
      0,                        /* n_preallocs */
      NULL                      /* instance_init */
    };
    tmp = g_type_register_static (G_TYPE_INTERFACE,
        "DlbAudioDecoder", &info, 0);
    g_type_interface_add_prerequisite (tmp, G_TYPE_OBJECT);

    g_once_init_leave (&type, tmp);
  }
  return type;
}

gint
dlb_audio_decoder_get_channels  (DlbAudioDecoderOutMode mode)
{
  gint num_channels = 0;

  if (DLB_AUDIO_DECODER_OUT_MODE_2_0 == mode) {
    num_channels = 2;
  } else if (DLB_AUDIO_DECODER_OUT_MODE_2_1 == mode) {
    num_channels = 3;
  } else if (DLB_AUDIO_DECODER_OUT_MODE_3_0 == mode) {
    num_channels = 3;
  } else if (DLB_AUDIO_DECODER_OUT_MODE_3_1 == mode) {
    num_channels = 4;
  } else if (DLB_AUDIO_DECODER_OUT_MODE_4_0 == mode) {
    num_channels = 4;
  } else if (DLB_AUDIO_DECODER_OUT_MODE_4_1 == mode) {
    num_channels = 5;
  } else if (DLB_AUDIO_DECODER_OUT_MODE_5_0 == mode) {
    num_channels = 5;
  } else if (DLB_AUDIO_DECODER_OUT_MODE_5_1 == mode) {
    num_channels = 6;
  } else if (DLB_AUDIO_DECODER_OUT_MODE_6_0 == mode) {
    num_channels = 6;
  } else if (DLB_AUDIO_DECODER_OUT_MODE_6_1 == mode) {
    num_channels = 7;
  } else if (DLB_AUDIO_DECODER_OUT_MODE_7_0 == mode) {
    num_channels = 7;
  } else if (DLB_AUDIO_DECODER_OUT_MODE_7_1 == mode) {
    num_channels = 8;
  } else if (DLB_AUDIO_DECODER_OUT_MODE_9_1 == mode) {
    num_channels = 10;
  } else if (DLB_AUDIO_DECODER_OUT_MODE_5_1_2 == mode) {
    num_channels = 8;
  } else if (DLB_AUDIO_DECODER_OUT_MODE_5_1_4 == mode) {
    num_channels = 10;
  } else if (DLB_AUDIO_DECODER_OUT_MODE_7_1_2 == mode) {
    num_channels = 10;
  } else if (DLB_AUDIO_DECODER_OUT_MODE_7_1_4 == mode) {
    num_channels = 12;
  } else if (DLB_AUDIO_DECODER_OUT_MODE_7_1_6 == mode) {
    num_channels = 14;
  } else if (DLB_AUDIO_DECODER_OUT_MODE_9_1_2 == mode) {
    num_channels = 12;
  } else if (DLB_AUDIO_DECODER_OUT_MODE_9_1_4 == mode) {
    num_channels = 14;
  } else if (DLB_AUDIO_DECODER_OUT_MODE_9_1_6 == mode) {
    num_channels = 16;
  } else if (DLB_AUDIO_DECODER_OUT_MODE_RAW == mode) {
    num_channels = 16;
  } else if (DLB_AUDIO_DECODER_OUT_MODE_CORE == mode) {
    num_channels = 6;
  } else {
    g_assert_not_reached ();
  }

  return num_channels;
}

void
dlb_audio_decoder_set_out_mode (DlbAudioDecoder * decoder,
    DlbAudioDecoderOutMode mode)
{
  g_return_if_fail (DLB_IS_AUDIO_DECODER (decoder));
  g_object_set (decoder, "out-mode", mode, NULL);
}

DlbAudioDecoderOutMode
dlb_audio_decoder_get_out_mode (DlbAudioDecoder * decoder)
{
  DlbAudioDecoderOutMode val;

  g_return_val_if_fail (DLB_IS_AUDIO_DECODER (decoder),
      DLB_AUDIO_DECODER_OUT_MODE_RAW);

  g_object_get (decoder, "out-mode", &val, NULL);
  return val;
}

void
dlb_audio_decoder_set_drc_mode (DlbAudioDecoder * decoder,
    DlbAudioDecoderDrcMode mode)
{
  g_return_if_fail (DLB_IS_AUDIO_DECODER (decoder));
  g_object_set (decoder, "drc-mode", mode, NULL);
}

DlbAudioDecoderDrcMode
dlb_audio_decoder_get_drc_mode (DlbAudioDecoder * decoder)
{
  DlbAudioDecoderDrcMode val;

  g_return_val_if_fail (DLB_IS_AUDIO_DECODER (decoder),
      DLB_AUDIO_DECODER_DRC_MODE_AUTO);

  g_object_get (decoder, "drc-mode", &val, NULL);
  return val;
}

void
dlb_audio_decoder_set_drc_cut (DlbAudioDecoder * decoder, gdouble val)
{
  g_return_if_fail (DLB_IS_AUDIO_DECODER (decoder));
  g_object_set (decoder, "drc-cut", val, NULL);
}

gdouble
dlb_audio_decoder_get_drc_cut (DlbAudioDecoder * decoder)
{
  gdouble val;

  g_return_val_if_fail (DLB_IS_AUDIO_DECODER (decoder), 0.0);
  g_object_get (decoder, "drc-cut", &val, NULL);
  return val;
}

void
dlb_audio_decoder_set_drc_boost (DlbAudioDecoder * decoder, gdouble val)
{
  g_return_if_fail (DLB_IS_AUDIO_DECODER (decoder));
  g_object_set (decoder, "drc-boost", val, NULL);
}

gdouble
dlb_audio_decoder_get_drc_boost (DlbAudioDecoder * decoder)
{
  gdouble val;

  g_return_val_if_fail (DLB_IS_AUDIO_DECODER (decoder), 0.0);
  g_object_get (decoder, "drc-boost", &val, NULL);
  return val;
}


void
dlb_audio_decoder_set_dmx_enable (DlbAudioDecoder *decoder, gboolean val)
{
  g_return_if_fail (DLB_IS_AUDIO_DECODER (decoder));
  g_object_set (decoder, "dmx-enable", val, NULL);
}

gboolean
dlb_audio_decoder_get_dmx_enable (DlbAudioDecoder *decoder)
{
  gboolean val;

  g_return_val_if_fail (DLB_IS_AUDIO_DECODER (decoder), 0.0);
  g_object_get (decoder, "dmx-enable", &val, NULL);
  return val;
}
