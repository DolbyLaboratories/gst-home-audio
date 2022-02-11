#ifndef _DLB_UDC_H_
#define _DLB_UDC_H_

#include "dlb_buffer.h"
#include "dlb_evo_dec.h"

#include <stdint.h>

typedef struct dlb_udc_s dlb_udc;

typedef enum
{
    DLB_UDC_OUTPUT_MODE_2_0,
    DLB_UDC_OUTPUT_MODE_2_1,
    DLB_UDC_OUTPUT_MODE_3_0,
    DLB_UDC_OUTPUT_MODE_3_1,
    DLB_UDC_OUTPUT_MODE_4_0,
    DLB_UDC_OUTPUT_MODE_4_1,
    DLB_UDC_OUTPUT_MODE_5_0,
    DLB_UDC_OUTPUT_MODE_5_1,
    DLB_UDC_OUTPUT_MODE_6_0,
    DLB_UDC_OUTPUT_MODE_6_1,
    DLB_UDC_OUTPUT_MODE_7_0,
    DLB_UDC_OUTPUT_MODE_7_1,
    DLB_UDC_OUTPUT_MODE_CORE,
    DLB_UDC_OUTPUT_MODE_RAW,
} dlb_udc_output_mode;

typedef enum
{
    DLB_UDC_CHANNEL_NONE                    = -1,
    DLB_UDC_CHANNEL_LEFT                    =  0,
    DLB_UDC_CHANNEL_RIGHT                   =  1,
    DLB_UDC_CHANNEL_CENTER                  =  2,
    DLB_UDC_CHANNEL_LFE                     =  3,
    DLB_UDC_CHANNEL_SIDE_LEFT               =  4,
    DLB_UDC_CHANNEL_SIDE_RIGHT              =  5,
    DLB_UDC_CHANNEL_CENTER_FRONT_LEFT       =  6,
    DLB_UDC_CHANNEL_CENTER_FRONT_RIGHT      =  7,
    DLB_UDC_CHANNEL_BACK_LEFT               =  8,
    DLB_UDC_CHANNEL_BACK_RIGHT              =  9,
    DLB_UDC_CHANNEL_BACK_CENTER             = 10,
    DLB_UDC_CHANNEL_TOP_SURROUND            = 11,
    DLB_UDC_CHANNEL_SIDE_DIRECT_LEFT        = 12,
    DLB_UDC_CHANNEL_SIDE_DIRECT_RIGHT       = 13,
    DLB_UDC_CHANNEL_WIDE_LEFT               = 14,
    DLB_UDC_CHANNEL_WIDE_RIGHT              = 15,
    DLB_UDC_CHANNEL_VERTICAL_HEIGHT_LEFT    = 16,
    DLB_UDC_CHANNEL_VERTICAL_HEIGHT_RIGHT   = 17,
    DLB_UDC_CHANNEL_VERTICAL_HEIGHT_CENTER  = 18,
    DLB_UDC_CHANNEL_TOP_SURROUND_LEFT       = 19,
    DLB_UDC_CHANNEL_TOP_SURROUND_RIGHT      = 20,
    DLB_UDC_CHANNEL_LFE2                    = 21,
} dlb_udc_channel;

typedef enum
{
    DLB_UDC_OK         =  0,
    DLB_UDC_EGENERIC   = -1,
    DLB_UDC_ENOTFRAMED = -2,
} dlb_udc_error_id;

#define DLB_UDC_CHANNEL_MASK(ch)        (1ULL << (DLB_UDC_CHANNEL_##ch))

#define DLB_UDC_METADATA_ID_INVALID     (0)

#define DLB_UDC_MAX_RAW_OUTPUT_CHANNELS (16)
#define DLB_UDC_MAX_PCM_OUTPUT_CHANNELS (8)
#define DLB_UDC_MAX_MD_SIZE             (0x1c00)
#define DLB_UDC_MAX_BLOCKS_PER_FRAME    (6u)      /* Maximum of 6 blocks per frame */

#define DLB_UDC_OUTBUF_MEMORY_ALIGNMENT (32)
#define DLB_UDC_SAMPLES_PER_FRAME       (1536)
#define DLB_UDC_SAMPLES_PER_BLOCK       (256)     /* For 48kHz, 256 samples per block */

typedef struct
{
    dlb_udc_output_mode outmode;
    int dmx_enable;
} dlb_udc_init_info;

typedef struct
{
    double boost; /**< UDC DRC boost setting (0.00 - 1.00).    */
    double cut;   /**< UDC DRC cut setting (0.00 - 1.00).      */
} dlb_udc_drc_settings;

typedef struct
{
    uint64_t channel_mask;
    int channels;
    int rate;
    int object_audio;
} dlb_udc_audio_info;


/**
 * The dlb_udc_process_block function will decode an audio block of data,
 * DLB_UDC_SAMPLES_PER_BLOCK samples. There are up to DLB_UDC_MAX_BLOCKS_PER_FRAME
 * audio blocks per timeslice.  framesz == 0 can be used to detect the last audio
 * block has already been decoded.
 *
 * Example decoding routine:
 *
 * dlb_udc_push_timeslice (dec_instance, indata, indatasz);
 *
 * for (block_num = 0; block_num <= DLB_UDC_MAX_BLOCKS_PER_FRAME; block_num++)
 * {
 *     Decode an audio block from the timeslice:
 *     status = dlb_udc_process_block
 *         (udc_instance
 *         ,outbuf            Note: This needs to be aligned to DLB_UDC_OUTBUF_MEMORY_ALIGNMENT
 *         ,&block_size       Note: Channels * Bytes per Sample * Audio Block Size in Bytes
 *         ,&metadata         Note: Serialized metadata associated with decoded block
 *         ,&audio_info
 *         );
 *     if (status)
 *     {
 *         fprintf(stderr, "ERROR: dlb_udc_process_block encountered error (%d)\n", status);
 *         return 1;
 *     }
 *     if (!ddp_dec_o_block_size)
 *     {
 *         The last audio block has been processed
 *         break;
 *     }
 * }
 */


dlb_udc  * dlb_udc_new                       (const dlb_udc_init_info *info);

void       dlb_udc_free                      (dlb_udc *self);

void       dlb_udc_drc_settings_init         (dlb_udc_drc_settings *drc);

int        dlb_udc_drc_settings_set          (dlb_udc *self,
                                              const dlb_udc_drc_settings *drc);

int        dlb_udc_query_latency_samples     (dlb_udc *self);

size_t     dlb_udc_query_max_outbuf_size     (dlb_udc_output_mode mode,
                                              int data_type);

int        dlb_udc_query_max_output_channels (dlb_udc_output_mode mode);

int        dlb_udc_push_timeslice            (dlb_udc *self,
                                              const char *indata,
                                              size_t indatasz);

int        dlb_udc_process_block             (dlb_udc *self,
                                              dlb_buffer *outbuf,
                                              size_t *blocksz,
                                              dlb_evo_payload *metadata,
                                              dlb_udc_audio_info *audio_info);


#endif /* _DLB_UDC_H_ */
