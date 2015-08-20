
/*
//
// Copyright (c) 2002-2015 Joe Bertolami. All Right Reserved.
//
// cabac.h
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

#ifndef __EV_CABAC_H__
#define __EV_CABAC_H__

#include "bitstream.h"
 
/*
// Entropy Stream Interface
//
// There are two ways to use this interface:
//
//  o: Stream coding
//
//     To code an entire stream at once, call Encode()/Decode(). This will instruct
//     the coder to initialize itself, perform the coding operation, perform a flush
//     if necessary, and clear its internal state.
//
//  o: Incremental coding
//
//     To incrementally code one or more symbols at a time, you must pass FALSE as the 
//     optional parameter to Encode and Decode. Additionally, you must call FinishEncode()
//     after all encode operations are complete, and StartDecode prior to calling the first
//     Decode(). This process allows the coder to properly initialize, flush, and reset itself.
*/

namespace evx {

class entropy_coder 
{
    bool adaptive;
    uint32 e3_count;
    uint32 history[2];
    uint32 value;

    uint32 model;
    uint32 low;
    uint32 high;
    uint32 mid;

private:

    void resolve_model();

    evx_status flush_encoder(bitstream *dest);
    evx_status flush_inverse_bits(uint8 value, bitstream *dest);

    evx_status encode_symbol(uint8 value);
    evx_status decode_symbol(uint32 value, bitstream *dest);

    evx_status resolve_encode_scaling(bitstream *dest);
    evx_status resolve_decode_scaling(uint32 *value, bitstream *source, bitstream *dest);

public:

    entropy_coder();
    explicit entropy_coder(uint32 input_model);
    void clear();

    evx_status encode(bitstream *source, bitstream *dest, bool auto_finish=true);
    evx_status decode(uint32 symbol_count, bitstream *source, bitstream *dest, bool auto_start=true);

    evx_status start_decode(bitstream *source);
    evx_status finish_encode(bitstream *dest);

private:

    EVX_DISABLE_COPY_AND_ASSIGN(entropy_coder);
};

} // namespace EVX

#endif // __EVX_CABAC_H__