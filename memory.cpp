
#include "memory.h"
#include "math.h"

namespace evx {

uint32 aligned_bit_copy(uint8 *dest, uint32 dest_bit_offset, uint8 *source, uint32 source_bit_offset, uint32 copy_bit_count) 
{
    if (EVX_PARAM_CHECK) 
    {
        if (!dest || !source ||
            0 != (dest_bit_offset % 8) ||
            0 != (source_bit_offset % 8) || 
            0 == (copy_bit_count >> 3)) 
        {
            evx_post_error(EVX_ERROR_INVALIDARG);
            return 0;
        }
    }

    uint32 dest_byte_offset	= dest_bit_offset >> 3;
    uint32 source_byte_offset = source_bit_offset >> 3;
    uint32 bytes_copied	= copy_bit_count >> 3;

    memcpy(dest + dest_byte_offset, source + source_byte_offset, bytes_copied);

    return (bytes_copied << 3);
}

uint32 unaligned_bit_copy( uint8 *dest, uint32 dest_offset, uint8 *source, uint32 source_offset, uint32 copy_bit_count ) 
{
    if (EVX_PARAM_CHECK) 
    {
        if (!dest || 0 == copy_bit_count || !source) 
        {
            evx_post_error(EVX_ERROR_INVALIDARG);
            return 0;
        }
    }

    uint32 source_copy_limit = source_offset + copy_bit_count;

    /* Perform an unaligned copy of our data. */
    while (source_offset < source_copy_limit)
    {
        uint32 target_byte = dest_offset >> 3;
        uint8  target_bit = dest_offset % 8;
        uint32 source_byte = source_offset >> 3;
        uint8  source_bit = source_offset % 8;
        uint32 bits_left = source_copy_limit - source_offset;

        /* We traverse our buffer and perform copies in as large of increments as possible. */
        uint8 write_capacity = evx_min2(8 - target_bit, 8 - source_bit);
        uint8 write_count = evx_min2(write_capacity, bits_left);
        uint8 write_fill_mask = (0x1 << write_count) - 1;

        uint8 *target_data = &(dest[target_byte]);
        uint8 *source_data = &(source[source_byte]);

        *target_data = (*target_data & ~(write_fill_mask << target_bit));
        *target_data |= ((*source_data & (write_fill_mask << source_bit)) >> source_bit) << target_bit;

        source_offset += write_count;
        dest_offset += write_count;
    }

    return copy_bit_count;
}

} // namespace evx