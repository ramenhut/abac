
#include "cabac.h"
#include "math.h"

#define EVX_ENTROPY_PRECISION					(16)
#define EVX_ENTROPY_PRECISION_MAX				((uint32(0x1) << EVX_ENTROPY_PRECISION) - 1)
#define EVX_ENTROPY_PRECISION_MASK				((uint32(0x1) << EVX_ENTROPY_PRECISION) - 1)
#define EVX_ENTROPY_HALF_RANGE					((EVX_ENTROPY_PRECISION_MAX >> 1))
#define EVX_ENTROPY_QTR_RANGE					(EVX_ENTROPY_HALF_RANGE >> 1)
#define EVX_ENTROPY_3QTR_RANGE					(3 * EVX_ENTROPY_QTR_RANGE)
#define EVX_ENTROPY_MSB_MASK					(uint64(0x1) << (EVX_ENTROPY_PRECISION - 1))
#define EVX_ENTROPY_SMSB_MASK					(EVX_ENTROPY_MSB_MASK >> 1)

#if (EVX_ENTROPY_PRECISION > 32)
  #error "EVX_ENTROPY_PRECISION must be <= 32"
#endif

namespace evx {

/* 
// ABAC Ranging
//
// + Our range for 0 is [0, mid]			(inclusive)
// + Our range for 1 is [mid+1, high]		(inclusive)
//
// Thus, when encoding a zero, low should remain the same, high becomes mid.
// When encoding a one, low should be set to mid + 1, high remains the same. 
*/

entropy_coder::entropy_coder() 
{
    history[0] = 1;
    history[1] = 1;

    e3_count = 0;
    adaptive = 1;
    model	= EVX_ENTROPY_HALF_RANGE;
    value = 0;

    low = 0;
    high = EVX_ENTROPY_PRECISION_MAX;
    mid = EVX_ENTROPY_HALF_RANGE;
}

entropy_coder::entropy_coder(uint32 input_model) 
{
    history[0] = 0;
    history[1] = 0;

    model = input_model;
    e3_count = 0;
    adaptive = 0;
    value = 0;

    low	= 0;
    high = EVX_ENTROPY_PRECISION_MAX;
    mid = model;
}

void entropy_coder::clear() 
{
    low	= 0;
    value = 0;
    e3_count = 0;
  
    if (adaptive) 
    {
        history[0] = 1;
        history[1] = 1;
        high = EVX_ENTROPY_PRECISION_MAX;
        mid	= EVX_ENTROPY_HALF_RANGE;
    } 
    else 
    {
        high = EVX_ENTROPY_PRECISION_MAX;
        mid = model;
    }
}

void entropy_coder::resolve_model() 
{
    uint64 mid_range = 0; 
    uint64 range = high - low;
    
    if (adaptive) 
    {
        mid_range = range * history[0] / (history[0] + history[1]);
    } 
    else 
    {
        mid_range = range * model / EVX_ENTROPY_PRECISION_MAX;
    }

    mid = low + mid_range;
}

evx_status entropy_coder::encode_symbol(uint8 value) 
{
    /* We only encode the first 2 GB instances of each symbol. */
    if (history[value] >= (2 * EVX_GB)) 
    {
        return evx_post_error(EVX_ERROR_INVALID_RESOURCE);
    }

    /* Adapt our model with knowledge of our recently processed value. */
    resolve_model();

    /* Encode our bit. */
    value = value & 0x1;

    if (value) 
    {
        low = mid + 1;		
    } 
    else 
    {
        high = mid;			  
    }

    history[value]++;	

    return EVX_SUCCESS;
}

evx_status entropy_coder::decode_symbol(uint32 value, bitstream *dest) 
{
    if (EVX_PARAM_CHECK) 
    {
        if (!dest) 
        {
            return evx_post_error(EVX_ERROR_INVALIDARG);
        }
    }

    /* Adapt our model with knowledge of our recently processed value. */
    resolve_model();

    /* Decode our bit. */
    if (value >= low && value <= mid) 
    {
        high = mid;
        history[0]++;
        dest->write_bit(0);
    } 
    else if (value > mid && value <= high) 
    {
        low = mid + 1;
        history[1]++;
        dest->write_bit(1);
    }

    return EVX_SUCCESS;
}

evx_status entropy_coder::flush_inverse_bits(uint8 value, bitstream *dest) 
{
    if (EVX_PARAM_CHECK) 
    {
        if (!dest) 
        {
            return evx_post_error(EVX_ERROR_INVALIDARG);
        }
    }

    value = !value;

    for (uint32 i = 0; i < e3_count; ++i) 
    {
        if (EVX_SUCCESS != dest->write_bit(value)) 
        {
            return evx_post_error(EVX_ERROR_EXECUTION_FAILURE);
        }
    }

    e3_count = 0;

    return EVX_SUCCESS;
}

evx_status entropy_coder::resolve_encode_scaling(bitstream *dest) 
{
    if (EVX_PARAM_CHECK) 
    {
        if (!dest) 
        {
            return evx_post_error(EVX_ERROR_INVALIDARG);
        }
    }

    while (true) 
    {
        if ((high & EVX_ENTROPY_MSB_MASK) == (low & EVX_ENTROPY_MSB_MASK)) 
        {
            /* E1/E2 scaling violation. */
            uint8 msb = (high & EVX_ENTROPY_MSB_MASK) >> (EVX_ENTROPY_PRECISION - 1);
            low -= EVX_ENTROPY_HALF_RANGE * msb + msb;	
            high -= EVX_ENTROPY_HALF_RANGE * msb + msb;	

            if (EVX_SUCCESS != dest->write_bit(msb)) 
            {
                return evx_post_error(EVX_ERROR_INVALID_RESOURCE);
            }

            if (EVX_SUCCESS != flush_inverse_bits(msb, dest)) 
            {
                return evx_post_error(EVX_ERROR_INVALID_RESOURCE);
            }
        } 
        else if (high <= EVX_ENTROPY_3QTR_RANGE && low > EVX_ENTROPY_QTR_RANGE) 
        {
            /* E3 scaling violation. */
            high -= EVX_ENTROPY_QTR_RANGE + 1;
            low	-= EVX_ENTROPY_QTR_RANGE + 1;
            e3_count += 1;
        } 
        else 
        {
            break;
        }

        high = ((high << 0x1) & EVX_ENTROPY_PRECISION_MAX) | 0x1;
        low  = ((low  << 0x1) & EVX_ENTROPY_PRECISION_MAX) | 0x0;
    }

    return EVX_SUCCESS;
}

evx_status entropy_coder::resolve_decode_scaling(uint32 *value, bitstream *source, bitstream *dest) 
{
    if (EVX_PARAM_CHECK) 
    {
        if (!value || !source || !dest) 
        {
            return evx_post_error(EVX_ERROR_INVALIDARG);
        }
    }

    uint8 bit = 0;

    while (true) 
    {
        if (high <= EVX_ENTROPY_HALF_RANGE) 
        {
            /* If our high value is less than half we do nothing (but
               prevent the loop from exiting). */
        } 
        else if (low > EVX_ENTROPY_HALF_RANGE) 
        {
            high -= (EVX_ENTROPY_HALF_RANGE + 1);
            low	-= (EVX_ENTROPY_HALF_RANGE + 1);
            *value -= (EVX_ENTROPY_HALF_RANGE + 1);
        }	
        else if (high <= EVX_ENTROPY_3QTR_RANGE && low > EVX_ENTROPY_QTR_RANGE) 
        {
            /* E3 scaling violation. */
            high -= EVX_ENTROPY_QTR_RANGE + 1;
            low	-= EVX_ENTROPY_QTR_RANGE + 1;
            *value -= EVX_ENTROPY_QTR_RANGE + 1;
        } 
        else
        {
            break;
        }   

        if (!source->is_empty()) 
        {
            if (EVX_SUCCESS != source->read_bit(&bit)) 
            {
                return evx_post_error(EVX_ERROR_EXECUTION_FAILURE);
            }
        }

        high = ((high << 0x1) & EVX_ENTROPY_PRECISION_MAX) | 0x1;
        low	= ((low  << 0x1) & EVX_ENTROPY_PRECISION_MAX) | 0x0;
        *value = ((*value << 0x1) & EVX_ENTROPY_PRECISION_MAX) | bit;
    }

    return EVX_SUCCESS;
}

evx_status entropy_coder::flush_encoder(bitstream *dest) 
{
    if (EVX_PARAM_CHECK) 
    {
        if (!dest) 
        {
            return evx_post_error(EVX_ERROR_INVALIDARG);
        }
    }

    e3_count++;

    if (low < EVX_ENTROPY_QTR_RANGE) 
    {
        if (EVX_SUCCESS != dest->write_bit(0) || 
            EVX_SUCCESS != flush_inverse_bits(0, dest)) 
        {
            return evx_post_error(EVX_ERROR_EXECUTION_FAILURE);
        }
    } 
    else 
    {
        if (EVX_SUCCESS != dest->write_bit(1) ||
            EVX_SUCCESS != flush_inverse_bits(1, dest)) 
        {
            return evx_post_error(EVX_ERROR_EXECUTION_FAILURE);
        }
    }

    clear();

    return EVX_SUCCESS;
}

evx_status entropy_coder::encode(bitstream *source, bitstream *dest, bool auto_finish) 
{
    if (EVX_PARAM_CHECK) 
    {
        if (!source || !dest) 
        {
            return evx_post_error(EVX_ERROR_INVALIDARG);
        }
    }

    uint8 value = 0;

    while (!source->is_empty()) 
    {
        if (EVX_SUCCESS != source->read_bit(&value) ||
            EVX_SUCCESS != encode_symbol(value) ||
            EVX_SUCCESS != resolve_encode_scaling(dest)) 
        {
            return evx_post_error(EVX_ERROR_INVALID_RESOURCE);
        }
    }

    if (auto_finish) 
    {
        /* We close out the tab here in order to remain consistent with the decode
           behavior. If stream support is required, this will require an update. */
        if (EVX_SUCCESS != flush_encoder(dest)) 
        {
            return evx_post_error(EVX_ERROR_EXECUTION_FAILURE);
        }

        clear();
    }

    return EVX_SUCCESS;
}

evx_status entropy_coder::decode(uint32 symbol_count, bitstream *source, bitstream *dest, bool auto_start) 
{
    if (EVX_PARAM_CHECK) 
    {
        if (0 == symbol_count || !source || !dest ) 
        {
            return evx_post_error(EVX_ERROR_INVALIDARG);
        }
    }

    if (auto_start) 
    {
        uint8 bit = 0;
        value = 0;

        clear();

        /* We read in our initial bits with padded tailing zeroes. */
        for (uint32 i = 0; i < EVX_ENTROPY_PRECISION; ++i) 
        {
            if (!source->is_empty()) 
            {
                if (EVX_SUCCESS != source->read_bit(&bit)) 
                {
                    return evx_post_error(EVX_ERROR_INVALID_RESOURCE);
                }
            }

            value <<= 0x1;
            value |= bit;
        }
    }

    /* Begin decoding the sequence. */
    for (uint32 i = 0; i < symbol_count; ++i) 
    {
        if (EVX_SUCCESS != decode_symbol(value, dest) ||
            EVX_SUCCESS != resolve_decode_scaling(&value, source, dest)) 
        {
            return evx_post_error(EVX_ERROR_EXECUTION_FAILURE);
        }
    }

    return EVX_SUCCESS;
}

evx_status entropy_coder::start_decode(bitstream *source) 
{
    uint8 bit = 0;
    value = 0;

    clear();

    /* We read in our initial bits with padded tailing zeroes. */
    for (uint32 i = 0; i < EVX_ENTROPY_PRECISION; ++i) 
    {
        if (!source->is_empty()) 
        {
            if (EVX_SUCCESS != source->read_bit(&bit)) 
            {
                return evx_post_error(EVX_ERROR_INVALID_RESOURCE);
            }
        }

        value <<= 0x1;
        value |= bit;
    }

    return EVX_SUCCESS;
}

evx_status entropy_coder::finish_encode(bitstream *dest) 
{
    if (EVX_SUCCESS != flush_encoder(dest)) 
    {
        return evx_post_error(EVX_ERROR_EXECUTION_FAILURE);
    }

    clear();

    return EVX_SUCCESS;
}

} // namespace evx