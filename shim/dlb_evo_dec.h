#ifndef _DLB_EVODEC_H_
#define _DLB_EVODEC_H_

#include <stddef.h>

/** Maximum payload size, in byte, of the Evolution OAMD container. */
#define DLB_EVODEC_MAX_MD_SIZE      0x1c00
#define DLB_EVODEC_METADATA_ID_OAMD 0xB

typedef struct _dlb_evo_dec dlb_evo_dec;

typedef struct
{
    size_t size;          /* Size of the following payload-array in BYTES */
    unsigned char* data;  /* binary data payload */
} dlb_evo_bitstream;

typedef struct
{
    unsigned char*  data;  /* binary data payload */
    size_t id;             /* payload id */
    size_t size;           /* size of p_data field in bytes */
    size_t offset;         /* Evo payload offset in samples. */
} dlb_evo_payload;

/**
 * @brief allocate new instance of dlb_evo_dec
 *
 * @return (transfer full) dlb_evo_dec instance, free with dlb_evodec_free
 */
dlb_evo_dec* dlb_evodec_new(void);

/**
 * @brief Free DlbEvoDec instance allocated with dlb_evodec_new
 * and sets instance pointer to NULL
 *
 * @param instance to be freed
 */
void dlb_evodec_free(dlb_evo_dec *instance);

/**
 * Decode evolution bitstream and return OAMD payloads.
 *
 * @param evodec [in] evolution decoder instance
 * @param evo [in] evolution bitstream
 * @param evo_oamd [out] exctracted OAMD payloads
 *
 * @return TRUE if success, FALSE otherwise
 */
int dlb_evodec_decode_oamd_payload (dlb_evo_dec * evodec, dlb_evo_bitstream *evo,
    dlb_evo_payload * evo_oamd);

#endif
