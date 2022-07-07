#ifndef DLB_FLEXR_H_
#define DLB_FLEXR_H_

#include <stddef.h>
#include <stdint.h>

#include "dlb_buffer.h"

#define DLB_FLEXR_STREAM_HANDLE_INVALID (0)

typedef struct dlb_flexr_s dlb_flexr;
typedef uint32_t dlb_flexr_stream_handle;

/* These are the supported input channel configurations */
typedef enum dlb_flexr_input_format_e {
  DLB_FLEXR_INPUT_FORMAT_1_0 = 0,
  DLB_FLEXR_INPUT_FORMAT_2_0,
  DLB_FLEXR_INPUT_FORMAT_5_1,
  DLB_FLEXR_INPUT_FORMAT_7_1,
  DLB_FLEXR_INPUT_FORMAT_5_1_2,
  DLB_FLEXR_INPUT_FORMAT_5_1_4,
  DLB_FLEXR_INPUT_FORMAT_7_1_4,
  DLB_FLEXR_INPUT_FORMAT_OBJECT,
} dlb_flexr_input_format;

typedef enum dlb_flexr_interp_mode_e {
  DLB_FLEXR_INTERP_OFFLINE = 0,
  DLB_FLEXR_INTERP_RUNTIME,
} dlb_flexr_interp_mode;

typedef struct dlb_flexr_init_info_s
{
    int rate;
    int active_channels_enable;
    uint64_t active_channels_mask;

    const uint8_t *serialized_config;
    size_t serialized_config_size;
} dlb_flexr_init_info;

typedef struct dlb_flexr_stream_info_s
{
    dlb_flexr_input_format format;
    dlb_flexr_interp_mode interp;
    /* upmix stream internally to create a more immersive experience, it is only
     * considered for following input formats:
     * - DLB_FLEXR_INPUT_FORMAT_2_0
     * - DLB_FLEXR_INPUT_FORMAT_5_1
     * - DLB_FLEXR_INPUT_FORMAT_7_1
     * ignored otherwise
     */
    int upmix_enable;
    /* indicates that crossfading of stream render configs is enabled */
    int crossfade_enable;
    /* the maximum number of samples that need to be buffered internally between
     * calls of generate_output */
    int max_input_blk_samples ;
    /* the maximum stream render config rendering table xyz resolutions that will
     * be supported by this stream */
    int max_num_x;
    int max_num_y;
    int max_num_z;
    /* the maximum number of stream render config rendering table levels that
     * will be supported by this stream */
    int max_num_ldr_levels;

    const uint8_t *serialized_config;
    size_t serialized_config_size;
} dlb_flexr_stream_info;

typedef struct dlb_flexr_object_metadata_s
{
    unsigned offset;
    unsigned char *payload;
    size_t payload_size;
} dlb_flexr_object_metadata;

dlb_flexr *             dlb_flexr_new                    (const dlb_flexr_init_info *info);

void                    dlb_flexr_free                   (dlb_flexr *self);

dlb_flexr_stream_handle dlb_flexr_add_stream             (dlb_flexr *self,
                                                          const dlb_flexr_stream_info *info);

void                    dlb_flexr_rm_stream              (dlb_flexr *self,
                                                          dlb_flexr_stream_handle stream);

int                     dlb_flexr_push_stream            (dlb_flexr *self,
                                                          dlb_flexr_stream_handle stream,
                                                          dlb_flexr_object_metadata *md,
                                                          dlb_buffer *inbuf,
                                                          int samples);

int                     dlb_flexr_generate_output        (dlb_flexr *self,
                                                          dlb_buffer *outbuf,
                                                          int *samples);

void                    dlb_flexr_reset                  (dlb_flexr *self);

void                    dlb_flexr_set_external_user_gain (dlb_flexr *self,
                                                          float gain);

void                    dlb_flexr_set_external_user_gain_by_step
                                                         (dlb_flexr *self,
                                                          int step);

int                     dlb_flexr_query_num_outputs      (const dlb_flexr *self);

int                     dlb_flexr_query_latency          (const dlb_flexr *self);

int                     dlb_flexr_query_outblk_samples   (const dlb_flexr *self);

int                     dlb_flexr_query_ext_gain_steps   (const dlb_flexr *self);

int                     dlb_flexr_query_pushed_samples   (const dlb_flexr *self,
                                                          dlb_flexr_stream_handle stream);

int                     dlb_flexr_finished               (const dlb_flexr *self,
                                                          dlb_flexr_stream_handle stream);

void                    dlb_flexr_set_render_config      (dlb_flexr *self,
                                                          dlb_flexr_stream_handle stream,
                                                          const uint8_t *serialized_config,
                                                          size_t serialized_config_size,
                                                          dlb_flexr_interp_mode interp,
                                                          int xfade_blocks);

void                    dlb_flexr_stream_info_init       (dlb_flexr_stream_info *info,
                                                          const uint8_t *serialized_config,
                                                          size_t serialized_config_size);

void                    dlb_flexr_set_internal_user_gain (dlb_flexr *self,
                                                          dlb_flexr_stream_handle stream,
                                                          float gain);

void                    dlb_flexr_set_content_norm_gain  (dlb_flexr *self,
                                                          dlb_flexr_stream_handle stream,
                                                          float gain);

#endif // DLB_FLEXR_H_
