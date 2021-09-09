#ifndef DLB_AUDIO_PARSER_H_
#define DLB_AUDIO_PARSER_H_

#include <stdlib.h>
#include <stdint.h>
#include <stddef.h>

/**
 * @file
 * @brief Declares audio parser base class.
 */

/**
 * @brief Fetch audio parser base class.
 */
#define AUDIO_PARSER(obj) (&(obj)->parent)

#define MEMBER_TO_STRUCT_PTR(struct_name, member_name, member_ptr) \
            ( (struct_name *)((char *)(member_ptr) - offsetof(struct_name, member_name)) )

/**
 * @brief Audio parser type.
 */
typedef enum {
    DLB_AUDIO_PARSER_TYPE_AC3,
    DLB_AUDIO_PARSER_TYPE_MAT,
    DLB_AUDIO_PARSER_TYPE_DUMMY
}dlb_audio_parser_type;

/**
 * @brief Data type.
 */
typedef enum {
    DATA_TYPE_ERROR  = 0,//!< DATA_TYPE_ERROR
    DATA_TYPE_AC3    = 1,//!< DATA_TYPE_AC3
    DATA_TYPE_EAC3   = 2,//!< DATA_TYPE_EAC3
    DATA_TYPE_AC4    = 3,//!< DATA_TYPE_AC4
    DATA_TYPE_MAT    = 4,//!< DATA_TYPE_MAT
    DATA_TYPE_TRUEHD = 5,//!< DATA_TYPE_TRUEHD
} dlb_audio_parser_data_type;

/**
 * @brief Sample rate.
 */
typedef enum {
    SAMPLE_RATE_ERROR      = 0,     //!< SAMPLE_RATE_ERROR
    SAMPLE_RATE_32kHz      = 32000, //!< SAMPLE_RATE_32kHz
    SAMPLE_RATE_44_1kHz    = 44100, //!< SAMPLE_RATE_44_1kHz
    SAMPLE_RATE_48kHz      = 48000, //!< SAMPLE_RATE_48kHz
    SAMPLE_RATE_88_2kHz    = 88200, //!< SAMPLE_RATE_88_2kHz
    SAMPLE_RATE_96kHz      = 96000, //!< SAMPLE_RATE_96kHz
    SAMPLE_RATE_176_4kHz   = 176400,//!< SAMPLE_RATE_176_4kHz
    SAMPLE_RATE_192kHz     = 192000 //!< SAMPLE_RATE_192kHz
} dlb_audio_parser_sample_rate;

/**
 * @brief Status and error codes.
 */
typedef enum {
    DLB_AUDIO_PARSER_STATUS_OK             =  0, //!< DLB_AUDIO_PARSER_STATUS_OK
    DLB_AUDIO_PARSER_STATUS_NEED_MORE_DATA =  1, //!< DLB_AUDIO_PARSER_STATUS_NEED_MORE_DATA
    DLB_AUDIO_PARSER_STATUS_OUT_OF_SYNC    =  2, //!< DLB_AUDIO_PARSER_STATUS_OUT_OF_SYNC
    DLB_AUDIO_PARSER_STATUS_NO_SYNCWORD    =  3, //!< DLB_AUDIO_PARSER_STATUS_NO_SYNCWORD
    DLB_AUDIO_PARSER_STATUS_ERROR          = -1, //!< DLB_AUDIO_PARSER_STATUS_ERROR
    DLB_AUDIO_PARSER_NUM_STATUS_CODES,           //!< DLB_AUDIO_PARSER_NUM_STATUS_CODES
} dlb_audio_parser_status;

typedef struct dlb_audio_parser_s dlb_audio_parser;

/**
 * @brief Audio parser info.
 *
 * Contains information about parsed stream.
 */
typedef struct dlb_audio_parser_info_s {
    /** Data type. */
    dlb_audio_parser_data_type  data_type;
    /** Number of channels. */
    unsigned int channels;
    /** Sample rate. */
    dlb_audio_parser_sample_rate sample_rate;
    /** Frame size in bytes. */
    size_t framesize;
    /** Frame duration in samples. */
    unsigned int samples;
    /** Object audio. */
    int object_audio;
} dlb_audio_parser_info;

/**
 * @brief Audio parser operations.
 *
 * Subclass need to override those methods.
 *
 * @query_max_inbuff_size Returns maximum size of possible audio frame.
 * @query_min_frame_size Returns minimum size of frame needed for parsing next
 *                       frame.
 * @find_syncword Search input buffer for syncword and return number of bytes
 *                to be skipped.
 * @parse Parse input buffer, check consistency of frame and return information
 *        about it.
 *
 */
typedef struct dlb_audio_parser_ops_s
{
    size_t (*query_max_inbuff_size)(const dlb_audio_parser * const parser);
    size_t (*query_min_frame_size)(const dlb_audio_parser * const parser);
    dlb_audio_parser_status (*parse)(dlb_audio_parser * parser, const uint8_t * const input, size_t insize, dlb_audio_parser_info * info, size_t *skipbytes);
    void (*release)(dlb_audio_parser *parser);
}dlb_audio_parser_ops;

/**
 * @brief Audio parser base class.
 */
struct dlb_audio_parser_s {
    /**
     * @brief operations.
     */
    dlb_audio_parser_ops ops;

    /**
     * @brief Indicate draining stage.
     */
    int draining;
};

/**
 * @defgroup parsing Audio parsing procedure.
 *
 * Example parsing procedure pseudo code.
 *
 * -# Allocate input buffer.
 * @verbatim
 *  inbuf_size = 0;
 *  inbuf = (uint8_t*)malloc(dlb_audio_parser_query_max_inbuff_size(parser));
 *  read_size = dlb_audio_parser_query_max_inbuff_size(parser);
 * @endverbatim
 *
 * -# Read data to input buffer. Assumed here reading from file.
 * @verbatim
 * read_bytes = fread(inbuf + inbuf_size, 1, read_size, f);
 * inbuf_size += read_size;
 * @endverbatim
 *
 * -# Parse frame.
 * @verbatim
 * parser_status = dlb_audio_parser_parse(parser, inbuf, inbuf_size, &info,
 *      &skip_bytes);
 * @endverbatim
 *
 * -# If parsing was successful copy/move frame from input buffer.
 * @verbatim
 * inbuf_size -= info.framesize;
 * memmove(inbuf, inbuf + info.framesize, inbuf_size);
 * @endverbatim
 *      -# Determine minimal frame size needed for parsing.
 * @verbatim
 * read_size = dlb_audio_parser_query_min_frame_size(parser);
 * if (inbuf_size < read_size)
 *     read_size -= inbuf_size;
 * else
 *     read_size = 0;
 * @endverbatim
 *      -# Read more data and get back to parsing stage.
 * @verbatim
 * read_bytes = fread(inbuf + inbuf_size, 1, read_size, f);
 * inbuf_size += read_bytes;
 * @endverbatim
 *
 * -# If parsing returned skip_bytes value not equal to 0. Move input
 *  buffer by skip_bytes value. Get back to point 2.
 */

/**
 * @brief Get the maximum input buffer size.
 *
 * @param parser [in] Parser object pointer.
 *
 * @return maximum input buffer size in bytes.
 */
size_t
dlb_audio_parser_query_max_inbuff_size
    (const dlb_audio_parser * const parser
    );

/**
 * @brief Get the minimum frame size.
 *
 * This should be called always after @dlb_audio_parser_parse to know bytes
 * needed for next frame.
 *
 * @param parser [in] Parser object pointer.
 *
 * @return Minimum frame size in bytes.
 */
size_t
dlb_audio_parser_query_min_frame_size
    (const dlb_audio_parser * const parser
    );

/**
 * @brief Parse frame.
 *
 * @param parser [in] Parser object pointer.
 * @param input [in] Input buffer.
 * @param insize [in] input buffer size.
 * @param info [out] Info from header. Valid only if success.
 * @param skipbytes [out] Indicates how many bytes from input stream should be
 *                        dropped.

 * @return DLB_AUDIO_PARSER_STATUS_OK upon successful parsing procedure.
 * Otherwise returns any error status depending on the issue.
 */
dlb_audio_parser_status
dlb_audio_parser_parse
    (dlb_audio_parser *parser
    ,const uint8_t * const input
    ,size_t insize
    ,dlb_audio_parser_info *info
    ,size_t *skipbytes
    );

/**
 * @brief Sets draining mode.
 *
 * @param parser Parser instance.
 * @param draining Draining flag.
 */
static inline void
dlb_audio_parser_draining_set
    (dlb_audio_parser *parser
    ,int draining
    )
{
    parser->draining = draining;
}

/**
 * @brief Gets draining state.
 *
 * @param parser Parser instance.
 * @return Draining flag.
 */
static inline int
dlb_audio_parser_draining_get
    (const dlb_audio_parser * const parser
    )
{
    return parser->draining;
}


/**
 * @brief Create @ref dlb_audio_parser depending on factory's
 * type.
 *
 * @return Audio parser instance or NULL.
 */
dlb_audio_parser* dlb_audio_parser_new
    (dlb_audio_parser_type type
    );

/**
 * @brief Destroy audio parser.
 *
 * @param parser Audio parser to destroy.
 */
void dlb_audio_parser_free
    (dlb_audio_parser *parser
    );

#endif /* DLB_AUDIO_PARSER_H_ */
