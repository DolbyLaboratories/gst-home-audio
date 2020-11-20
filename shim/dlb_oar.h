#ifndef __DLB_OAR_H_
#define __DLB_OAR_H_

#include "dlb_buffer.h"
#include <stdint.h>

typedef struct dlb_oar_s dlb_oar;

enum
{
    DLB_OAR_SPEAKER_CONFIG_L_R = 0,
    DLB_OAR_SPEAKER_CONFIG_C,
    DLB_OAR_SPEAKER_CONFIG_LFE,
    DLB_OAR_SPEAKER_CONFIG_LS_RS,
    DLB_OAR_SPEAKER_CONFIG_LRS_RRS,
    DLB_OAR_SPEAKER_CONFIG_CS,
    DLB_OAR_SPEAKER_CONFIG_LC_RC,
    DLB_OAR_SPEAKER_CONFIG_LW_RW,
    DLB_OAR_SPEAKER_CONFIG_LFH_RFH,
    DLB_OAR_SPEAKER_CONFIG_LTF_RTF,
    DLB_OAR_SPEAKER_CONFIG_LTM_RTM,
    DLB_OAR_SPEAKER_CONFIG_LTR_RTR,
    DLB_OAR_SPEAKER_CONFIG_LRH_RRH,
    DLB_OAR_SPEAKER_CONFIG_LSC_RSC,
    DLB_OAR_SPEAKER_CONFIG_LS1_RS1,
    DLB_OAR_SPEAKER_CONFIG_LS2_RS2,
    DLB_OAR_SPEAKER_CONFIG_LRS1_RRS1,
    DLB_OAR_SPEAKER_CONFIG_LRS2_RRS2,
    DLB_OAR_SPEAKER_CONFIG_LCS_RCS,
    DLB_OAR_SPEAKER_CONFIG_CFH,
    DLB_OAR_SPEAKER_CONFIG_CTM,
    DLB_OAR_SPEAKER_CONFIG_CRH,
};

#define DLB_OAR_SPEAKER_CONFIG_MASK(s) (1UL << (DLB_OAR_SPEAKER_CONFIG_ ##s))

typedef struct dlb_oar_init_info_s {
    int sample_rate;
    int limiter_enable;
    unsigned long speaker_mask;
} dlb_oar_init_info;

typedef struct
{
    int sample_offset;
    size_t size;
    uint8_t *data;
} dlb_oar_payload;

dlb_oar * dlb_oar_new                     (const dlb_oar_init_info *info);

void      dlb_oar_free                    (dlb_oar *self);

void      dlb_oar_push_oamd_payload       (dlb_oar *self,
                                           const dlb_oar_payload *payload,
                                           int payload_num);

void      dlb_oar_process                 (dlb_oar *self,
                                           const dlb_buffer *inbuf,
                                           dlb_buffer *outbuf,
                                           int samples);

void      dlb_oar_reset                   (dlb_oar *self,
                                           int sample_rate);

int       dlb_oar_query_latency           (dlb_oar *self);


int       dlb_oar_query_min_block_samples (dlb_oar *self);

int       dlb_oar_query_max_block_samples (dlb_oar *self);

int       dlb_oar_query_max_payloads      (dlb_oar *self);

#endif // __DLB_OAR_H_
