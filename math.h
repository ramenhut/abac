
/*
//
// Copyright (c) 2002-2015 Joe Bertolami. All Right Reserved.
//
// math.h
//
//   Redistribution and use in source and binary forms, with or without
//   modification, are permitted provided that the following conditions are met:
//
//   * Redistributions of source code must retain the above copyright notice, this
//     list of conditions and the following disclaimer.
//
//   * Redistributions in binary form must reproduce the above copyright notice,
//     this list of conditions and the following disclaimer in the documentation
//     and/or other materials provided with the distribution.
//
//   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
//   AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
//   IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
//   DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
//   FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
//   DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
//   SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
//   CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
//   OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
//   OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
// Additional Information:
//
//   For more information, visit http://www.bertolami.com.
//
*/

#ifndef __EV_MATH_H__
#define __EV_MATH_H__

#include "base.h"

#define EVX_KB                  ((uint32) 1024)
#define EVX_MB                  (EVX_KB * EVX_KB)
#define EVX_GB                  (EVX_MB * EVX_KB)

#define EVX_MAX_INT64           (0x7FFFFFFFFFFFFFFF)
#define EVX_MAX_INT32           (0x7FFFFFFF)
#define EVX_MAX_INT16           (0x7FFF)
#define EVX_MAX_INT8            (0x7F)

#define EVX_MAX_UINT64          (0xFFFFFFFFFFFFFFFF)
#define EVX_MAX_UINT32          (0xFFFFFFFF)
#define EVX_MAX_UINT16          (0xFFFF)
#define EVX_MAX_UINT8           (0xFF)

#define EVX_MIN_INT64           (-EVX_MAX_INT64 - 1)
#define EVX_MIN_INT32           (-EVX_MAX_INT32 - 1)
#define EVX_MIN_INT16           (-EVX_MAX_INT16 - 1)
#define EVX_MIN_INT8            (-EVX_MAX_INT8 - 1)

#define evx_min2( a, b )        ((a) < (b) ? (a) : (b))
#define evx_max2( a, b )        ((a) > (b) ? (a) : (b))
#define evx_min3( a, b, c )     ((c) < (a) ? ((c) < (b) ? (c) : (b)) : (a) < (b) ? (a) : (b))
#define evx_max3( a, b, c )     ((c) > (a) ? ((c) > (b) ? (c) : (b)) : (a) > (b) ? (a) : (b))

namespace evx {

const uint8 log2_byte_lut[] = {
    0, 0, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 3, 3,
    4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
    5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
    5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
    6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
    6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
    6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
    6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
    7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
    7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
    7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
    7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
    7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
    7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
    7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
    7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
};

inline uint8 log2(uint8 value) 
{
    return log2_byte_lut[value];
}

inline uint8 log2(uint16 value) 
{
    if (value <= 0xFF) 
    {
        return log2((uint8) value);
    }

    return 8 + log2((uint8) (value >> 8));
}

inline uint8 log2(uint32 value) 
{
    if (value <= 0xFFFF) 
    {
        return log2((uint16) value);
    }

    return 16 + log2((uint16) (value >> 16));
}

inline int8 abs(int8 value) 
{
    if (0x80 == value) 
    {
        return 0x7F;
    }

    return (value < 0 ? -value : value);
}

inline int16 abs(int16 value) 
{
    if (0x1000 == value) 
    {
        return 0x7FFF;
    }

    return (value < 0 ? -value : value);
}

inline int32 abs(int32 value) 
{
    if (0x10000000 == value) 
    {
        return 0x7FFFFFFF;
    }

    return (value < 0 ? -value : value);
}

inline int16 clip_range(int16 value, int16 min, int16 max) 
{
    return (value < min ? min : (value > max ? max : value));
}

inline uint32 greater_multiple(uint32 value, uint32 multiple) 
{
    uint32 mod = value % multiple;

    if (0 != mod) 
    {
        value += multiple - mod;
    }

    return value;
}

inline uint32 align(uint32 value, uint32 alignment) 
{
    return greater_multiple(value, alignment);
}   

} // namespace evx

#endif // __EV_MATH_H__