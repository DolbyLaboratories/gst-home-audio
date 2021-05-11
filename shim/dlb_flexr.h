#ifndef __DLB_FLEXR_H_
#define __DLB_FLEXR_H_

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
    int upmix_enable;

    const uint8_t *serialized_config;
    size_t serialized_config_size;
} dlb_flexr_stream_info;

typedef struct dlb_flexr_object_metadata_s
{
    unsigned offset;
    unsigned char *payload;
    size_t payload_size;
} dlb_flexr_object_metadata;

dlb_flexr *             dlb_flexr_new                  (const dlb_flexr_init_info *info);

void                    dlb_flexr_free                 (dlb_flexr *self);

dlb_flexr_stream_handle dlb_flexr_add_stream           (dlb_flexr *self,
                                                        const dlb_flexr_stream_info *info);

void                    dlb_flexr_rm_stream            (dlb_flexr *self,
                                                        dlb_flexr_stream_handle stream);

int                     dlb_flexr_push_stream          (dlb_flexr *self,
                                                        dlb_flexr_stream_handle stream,
                                                        dlb_flexr_object_metadata *md,
                                                        dlb_buffer *inbuf,
                                                        int samples);

int                     dlb_flexr_generate_output      (dlb_flexr *self,
                                                        dlb_buffer *outbuf,
                                                        int *samples);

int                     dlb_flexr_query_num_outputs    (const dlb_flexr *self);

int                     dlb_flexr_query_block_samples  (const dlb_flexr *self);

int                     dlb_flexr_query_max_samples    (const dlb_flexr *self);

int                     dlb_flexr_query_latency        (const dlb_flexr *self);


int                     dlb_flexr_query_pushed_samples (const dlb_flexr *self,
                                                        dlb_flexr_stream_handle stream);

void                    dlb_flexr_set_render_config    (dlb_flexr *self,
                                                        dlb_flexr_stream_handle stream,
                                                        const uint8_t *serialized_config,
                                                        size_t serialized_config_size,
                                                        dlb_flexr_interp_mode interp,
                                                        int xfade_blocks);

void                    dlb_flexr_set_volume           (dlb_flexr *self,
                                                        dlb_flexr_stream_handle stream,
                                                        float volume);

void                    dlb_flexr_stream_info_init     (dlb_flexr_stream_info *info);

#endif // __DLB_FLEXR_H_
