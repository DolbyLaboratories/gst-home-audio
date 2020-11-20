#ifndef __DLB_DAP_H_
#define __DLB_DAP_H_

#include "dlb_buffer.h"
#include <stdint.h>

#define DLB_DAP_GRAPHIC_EQUALIZER_MAX_BANDS_NUM             (20u)
#define DLB_DAP_IEQ_MAX_BANDS_NUM                           (20u)

typedef struct dlb_dap_s dlb_dap;

typedef struct dlb_dap_channel_format_s {
    int channels_floor;
    int channels_top;
    int lfe;
} dlb_dap_channel_format;

typedef struct dlb_dap_init_info_s {
    int sample_rate;
    int virtualizer_enable;
    dlb_dap_channel_format output_format;

    /* Optional. Overrides output format if set */
    const uint8_t *serialized_config;
} dlb_dap_init_info;

typedef struct dlb_dap_gain_settings_s {
  int pregain;
  int postgain;
  int system_gain;
} dlb_dap_gain_settings;

typedef struct dlb_dap_virtualizer_settings_s {
    int front_speaker_angle;
    int surround_speaker_angle;
    int rear_surround_speaker_angle;
    int height_speaker_angle;
    int rear_height_speaker_angle;
    int height_filter_enable;
} dlb_dap_virtualizer_settings;

typedef struct dlb_dap_profile_settings_s {
    int bass_enhancer_enable;
    int bass_enhancer_boost;
    int bass_enhancer_cutoff_frequency;
    int bass_enhancer_width;
    int calibration_boost;
    int dialog_enhancer_enable;
    int dialog_enhancer_amount;
    int dialog_enhancer_ducking;
    int graphic_equalizer_enable;
    unsigned graphic_equalizer_bands_num;
    unsigned graphic_equalizer_bands[DLB_DAP_GRAPHIC_EQUALIZER_MAX_BANDS_NUM];
    int graphic_equalizer_gains[DLB_DAP_GRAPHIC_EQUALIZER_MAX_BANDS_NUM];
    int ieq_enable;
    int ieq_amount;
    unsigned ieq_bands_num;
    unsigned ieq_bands[DLB_DAP_IEQ_MAX_BANDS_NUM];
    int ieq_gains[DLB_DAP_IEQ_MAX_BANDS_NUM];
    int mi_dialog_enhancer_steering_enable;
    int mi_dv_leveler_steering_enable;
    int mi_ieq_steering_enable;
    int mi_surround_compressor_steering_enable;
    int surround_decoder_enable;
    int surround_decoder_center_spreading_enable;
    int surround_boost;
    int volmax_boost;
    int volume_leveler_enable;
    int volume_leveler_amount;
    int volume_leveler_in_target;
    int volume_leveler_out_target;

} dlb_dap_profile_settings;


dlb_dap * dlb_dap_new                          (const dlb_dap_init_info *info);

void      dlb_dap_free                         (dlb_dap *self);

void      dlb_dap_preprocess_serialized_config (const uint8_t * serialized_config,
                                                dlb_dap_channel_format *intermediate_format,
                                                int *output_channels,
                                                int *virtualizer_enable);

void      dlb_dap_propose_input_format         (const dlb_dap_channel_format *output_format,
                                                int virtulizer_enable,
                                                dlb_dap_channel_format *input_format);

int       dlb_dap_process                      (dlb_dap *self,
                                                const dlb_dap_channel_format *input_format,
                                                const dlb_buffer *inbuf,
                                                dlb_buffer *outbuf);

int       dlb_dap_query_latency                (dlb_dap *self);

int       dlb_dap_query_block_samples          (dlb_dap *self);

void      dlb_dap_virtualizer_settings_init    (dlb_dap_virtualizer_settings *settings);

void      dlb_dap_set_virtualizer_settings     (dlb_dap *self,
                                                const dlb_dap_virtualizer_settings *settings);

void      dlb_dap_profile_settings_init        (dlb_dap_profile_settings *settings);

void      dlb_dap_set_profile_settings         (dlb_dap *self,
                                                const dlb_dap_profile_settings *settings);

void      dlb_dap_gain_settings_init           (dlb_dap_gain_settings *settings);

void      dlb_dap_set_gain_settings            (dlb_dap *self,
                                                const dlb_dap_gain_settings *settings);
#endif // __DLB_DAP_H_
