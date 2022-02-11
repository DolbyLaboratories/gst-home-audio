#ifndef dlb_buffer_H
#define dlb_buffer_H

#include <stddef.h>

/******************************************************************************
Represents one or more channel buffers of audio or bitstream data.
******************************************************************************/
typedef struct dlb_buffer_s
{
    /** Channel count */
    unsigned    nchannel;
    
    /** Distance, in units of the chosen data type, between consecutive samples
     *  of the same channel. For DLB_BUFFER_OCTET_PACKED, this is in units of
     *  unsigned char. For interleaved audio data, set this equal to nchannel.
     *  For separate channel buffers, set this to 1.
     */
    ptrdiff_t   nstride;

    /** One of the DLB_BUFFER_* codes */
    int         data_type;

    /** Array of pointers to data, one for each channel. */
    void      **ppdata;
} dlb_buffer;

/******************************************************************************
data type codes

How to choose which data type to use:
-------------------------------------
1. What are you trying to do?
    a. If you are integrating an existing Dolby signal processing component
       into a system, go to 2.
    b. If you are gluing two Dolby signal processing components together,
       use DLB_BUFFER_LFRACT.
    c. If you are creating a new Dolby signal processing component, go to 5.

I'm integrating a Dolby component into a system:
------------------------------------------------
2. Am I dealing with bitstream data?
    a. If so, continue to 3.
    b. If not, go to 4.
3. Does my unsigned char data type have 16 bits or more (this is common on
   DSPs)?
    a. If the answer is "no" or you don't know, choose
       DLB_BUFFER_OCTET_UNPACKED.
    b. If the answer is "yes", you need to think about how octets are stored in
       each unsigned chars.
        i.  If multiple octets exist in each unsigned char, use
            DLB_BUFFER_OCTET_PACKED.
        ii. If there is only one octet in each unsigned char, use
            DLB_BUFFER_OCTET_UNPACKED.
4. Does your system already have a fixed idea of how the data is formatted?
    a. If so, you need to work out which of the formats below correspond to
       your system's format. The most common answers for PCM systems are:
        - DLB_BUFFER_SHORT_16 (16 bit data in shorts)
        - DLB_BUFFER_LONG_32 (32 bit data in longs)
        - DLB_BUFFER_FLOAT (single-precision floating point data)
    b. If you can ask your system to produce or consume data in a variety of
       formats you need to work out which one of the formats below to specify.
        i.   Prefer DLB_BUFFER_INT if your system supports it. eg, 24 or 32 bit
             data left aligned in 32 bit ints on a PC.
        ii.  If more than 16 bit precision is required and the int data type
             has only 16 bits use DLB_BUFFER_LONG_32.
        iii. If the machine has a floating point unit and the float data type
             had enough precision, use DLB_BUFFER_FLOAT.
        iv.  Otherwise, use DLB_BUFFER_SHORT_16.

I'm creating a new Dolby component:
-----------------------------------
5. Use the dlb_buffer component to manage the interface for you.

******************************************************************************/

/** An array of octets stored in unsigned chars.
 *  This is a good choice for bitstreams where ease of data access is the
 *  primary concern.
 *  When an unsigned char has more than 8 bits, the octet is in the lowest
 *  eight bits and is zero-extended to fill the char. For example, in the case
 *  of a 20 bit unsigned char (CHAR_BIT == 20), the data are stored as follows:
 *
 *  bit 16        8        0
 *       |        |        |
 *    0000 00000000 11111111
 *
 * where:
 *  0 - zero extension
 *  1 - bits of first (only) octet
 */
#define DLB_BUFFER_OCTET_UNPACKED   1

/** An array of octets packed into unsigned chars.
 *  This is a good choice for bitstreams where minimum memory use is the
 *  primary concern.
 *  Octets are packed into unsigned chars starting at the least significant
 *  bit. Successive octets are packed into successively higher bits in the
 *  unsigned char until fewer than 8 bits remain. These extra bits are zeroed.
 *  For example, in the case of a 20 bit unsigned char (CHAR_BIT == 20), the
 *  data are stored as follows:
 *
 *  bit 16        8        0
 *       |        |        |
 *    0000 22222222 11111111
 *
 * where:
 *  0 - zero extension
 *  1 - first octet 
 *  2 - second octet 
 */
#define DLB_BUFFER_OCTET_PACKED     2

/** Data stored in internal DLB_LFRACT format.
 *  Use this data type to share data efficiently between components that use
 *  the Dolby Intrinsics. You can't do anything with data in this format except
 *  plumb Dolby components together.
 */
#define DLB_BUFFER_LFRACT           3

/** Signed shorts in Qx.15 format.
 *  This format is a good choice when the data have no more than 15 bits of
 *  mantissa precision, commonly referred to as "16 bit data".
 *  If the short data type has more than 16 bits, then the data are
 *  sign-extended to the width of the short and the radix point
 *  remains immediately to the left of bit 15.
 *  short is guaranteed by the C90 standard to represent [-(2^15)+1, +(2^15)-1].
 *  On a 24 bit DSP, this will give you 16 bit data (right aligned in a 24 bit word).
 *  On a 16 bit DSP, this will give you 16 bit data.
 *  On an ARM or a PC, this will give you 16 bit data.
 */
#define DLB_BUFFER_SHORT_16         4

/** Signed ints with data left-aligned.
 *  Use this format when you want a fixed point data type that has as many bits
 *  than can be dealt with in a fast idomatic manor on the target processor.
 *  int is guaranteed by the C90 standard to represent [-(2^15)+1, +(2^15)-1].
 *  Since the guaranteed range of the int type is the same as that of the short,
 *  there is no point fixing the radix point. However, using int on many platforms
 *  will be faster and use less memory than long so this data type is provided as
 *  as way to specify data that is "as precise as can be handled conveniently" on
 *  the target processor.
 *  On a 24 bit DSP, this will give you 24 bit data.
 *  On a 16 bit DSP, this will give you 16 bit data.
 *  On an ARM or a PC, this will give you 32 bit data.
 */
#define DLB_BUFFER_INT_LEFT         5

/** Signed longs in Qx.31 format.
 *  This format is a good choice when more than 16 bit precision is required,
 *  even if that makes the implementation slow.
 *  If the long data type has more than 32 bits, then the data are
 *  sign-extended to the width of the long and the radix point remains
 *  immediately to the left of bit 31.
 *  long is guaranteed by the C90 standard to represent [-(2^31)+1, +(2^31)-1].
 *  On a traditional 24 bit DSP, this is probably 32 bits right-aligned in a
 *      48 (or 56 bit) accumulator.
 *  On a C6x, this is a 40 bit data type containing 32 bit precision.
 *  On a 64 bit PC, this is a 64 bit data type containing 32 bit precision.
 *  On most other processors, this is a 32 bit data type.
 */
#define DLB_BUFFER_LONG_32          6

/** Float (single precision floating point) format in the range [-1, +1].
 *  Use this format to interface with components that only deal in floating point
 *  data. Support of this format by a component is entirely optional and may limit
 *  portability of the component to fixed-point DSPs.
 */
#define DLB_BUFFER_FLOAT            7

/** Double (double precision floating point) format in the range [-1, +1].
 *  Use this format to interface with components that only deal in floating point
 *  data. Support of this format by a component is entirely optional and may limit
 *  portability of the component to fixed-point DSPs.
 */
#define DLB_BUFFER_DOUBLE           8

#endif
