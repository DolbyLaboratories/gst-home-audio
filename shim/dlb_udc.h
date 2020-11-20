#ifndef _DLB_UDC_H_
#define _DLB_UDC_H_

#include <stdint.h>
#include "dlb_buffer.h"
#include "dlb_evo_dec.h"

typedef struct dlb_udc_s dlb_udc;

/**
 * UDC output mode.
 */
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

typedef enum {
  DLB_UDC_DATA_TYPE_OCTET_UNPACKED,
  DLB_UDC_DATA_TYPE_SHORT,
  DLB_UDC_DATA_TYPE_INT,
  DLB_UDC_DATA_TYPE_FLOAT,
  DLB_UDC_DATA_TYPE_DOUBLE
}dlb_udc_data_type;

typedef enum {
  DLB_UDC_CHANNEL_NONE                    = -2,
  DLB_UDC_CHANNEL_MONO                    = -1,
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

#define DLB_UDC_CHANNEL_MASK(ch)        (1 << (ch))
#define DLB_UDC_MAX_RAW_OUTPUT_CHANNELS (16)
#define DLB_UDC_MAX_PCM_OUTPUT_CHANNELS (8)
#define DLB_UDC_MAX_MD_SIZE             (0x1c00)
#define DLB_UDC_MEMORY_ALIGNMENT        (32)
#define DLB_UDC_SAMPLES_PER_FRAME       (1536)
#define DLB_UDC_SAMPLES_PER_BLOCK       (256)    /**< For 48kHz, 256 samples per block                      */
#define DLB_UDC_MAX_BLOCKS_PER_FRAME    (6u)      /**< Maximum of 6 blocks per frame                         */

/**
 * UDC DRC settings.
 */
typedef struct
{
  double boost; /**< UDC DRC boost setting (0.00 - 1.00).    */
  double cut;   /**< UDC DRC cut setting (0.00 - 1.00).      */
} dlb_udc_drc_settings;

/**
 * @brief UDC audio info.
 *
 * Describes audio output from UDC.
 */
typedef struct {
  int channels;       /**< Number of channels.           */
  int channel_mask;   /**< Channel mask                  */
  int rate;           /**< Sample rate                   */
  int object_audio;   /**< TRUE if object audio present. */
} dlb_udc_audio_info;

/**
 * Creates new instance of UDC decoder.
 *
 * @param outmode Output mode.
 */
dlb_udc  *
dlb_udc_new (dlb_udc_output_mode outmode, dlb_udc_data_type data_type);

/**
 * Destroys UDC decoder instance.
 *
 * @self Instance to destroy.
 */
void
dlb_udc_free (dlb_udc *self);

/**
 * Initializes drc settings.
 *
 * @param drc DRC settings.
 */
void
dlb_udc_drc_settings_init (dlb_udc_drc_settings *drc);

/**
 * Sets DRC settings.
 *
 * @param self Decoder instance.
 * @param drc DRC settings.
 *
 * @return 0 if success, err code otherwise.
 */
int
dlb_udc_drc_settings_set (dlb_udc *self, const dlb_udc_drc_settings *drc);

/**
 * Pushes one frame to decoder.
 *
 * @param self Decoder instance.
 * @param indata Input data pointer.
 * @param indatasz Input data size in bytes.
 *
 * @return 0 if success, error code otherwise.
 */
int
dlb_udc_push_frame (dlb_udc *self, const char *indata, size_t indatasz);

/**
 * Decodes one frame.
 *
 * @param self Decoder instance.
 * @param indata Input data pointer.
 * @param indatasz Input data size in bytes.
 * @param outbuf Output buffer.
 * @param framesz Returned output buffer size.
 * @param metadata Serialized metada associated with the decoded frame.
 * @param audio_info Audio info describing decoded frame.
 *
 * @return 0 if success, err code otherwise.
 */
int
dlb_udc_process_frame (dlb_udc *self, uint8_t *outbuf, size_t *framesz,
        dlb_evo_payload *metadata, dlb_udc_audio_info *audio_info,
        size_t *audio_info_offset);

/**
 * Returns decoder latency in samples.
 *
 * @param self Decoder instance.
 *
 * @return Decoder's latency in samples.
 */
int
dlb_udc_query_latency (dlb_udc *self);


#endif /* _DLB_UDC_H_ */
