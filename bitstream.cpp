
#include "bitstream.h"
#include "math.h"
#include "memory.h"

namespace evx {

bitstream::bitstream() 
{
    read_index = 0;
    write_index = 0;
    data_store = 0;
    data_capacity = 0;
}

bitstream::bitstream(uint32 size) 
{
    data_store = 0;

    if (size != resize_capacity(size)) 
    {
        evx_post_error(EVX_ERROR_EXECUTION_FAILURE);
    }
}

bitstream::bitstream(void *bytes, uint32 size) 
{
    data_store = 0;

    if (0 != assign(bytes, size)) 
    {
        evx_post_error(EVX_ERROR_EXECUTION_FAILURE);
    }
}

bitstream::~bitstream() 
{
    clear();
}

uint8 *bitstream::query_data() const 
{
    return data_store;
}

uint32 bitstream::query_capacity() const 
{
    return data_capacity << 3;
}

uint32 bitstream::query_occupancy() const 
{
    return write_index - read_index;
}

uint32 bitstream::query_byte_occupancy() const 
{
    return align(query_occupancy(), 8) >> 3;
}

uint32 bitstream::resize_capacity(uint32 size_in_bits) 
{
    if (EVX_PARAM_CHECK) 
    {
        if (0 == size_in_bits) 
        {
            evx_post_error(EVX_ERROR_INVALIDARG);
            return 0;
        }
    }

    clear();

    uint32 byte_size = align(size_in_bits, 8) >> 3;
    data_store = new uint8[byte_size];

    if (!data_store) 
    {
        evx_post_error(EVX_ERROR_EXECUTION_FAILURE);
        return 0;
    }

    data_capacity = byte_size;
    return size_in_bits;
}

evx_status bitstream::seek(uint32 bit_offset) 
{
    if (bit_offset >= write_index) 
    {
        bit_offset = write_index;
    }
    
    read_index = bit_offset;
    return 0;
}

evx_status bitstream::assign(void *bytes, uint32 size) 
{
    if (EVX_PARAM_CHECK) 
    {
        if (0 == size || !bytes) 
        {
            return evx_post_error(EVX_ERROR_INVALIDARG);
        }
    }

    clear();

    /* Copy the data into our own buffer and adjust our indices. */
    data_store = new uint8[size];

    if (!data_store) 
    {
        return evx_post_error(EVX_ERROR_OUTOFMEMORY);
    }

    memcpy(data_store, bytes, size);

    read_index  = 0;
    write_index = size << 3;
    data_capacity = size;

    return EVX_SUCCESS;
}

void bitstream::clear() 
{
    empty();

    delete [] data_store;
    data_store = 0;
    data_capacity = 0;
}

void bitstream::empty() 
{
    write_index = 0;
    read_index = 0;
}

bool bitstream::is_empty() const 
{
    return (write_index == read_index);
}

bool bitstream::is_full() const 
{
    return (write_index == query_capacity());
}

evx_status bitstream::write_byte(uint8 value) 
{
    if (write_index + 8 > query_capacity()) 
    {
        return EVX_ERROR_CAPACITY_LIMIT;
    }

    /* Determine the current byte to write. */
    uint32 dest_byte = write_index >> 3;
    uint8 dest_bit = write_index % 8;

    if (0 == dest_bit) 
    {
        /* This is a byte aligned write, so we perform it at byte level. */
        uint8 *data = &(data_store[dest_byte]);
        *data = value;
        write_index += 8;
    } 
    else 
    {
        /* Slower byte unaligned write. */
        for (uint8 i = 0; i < 8; ++i) 
        {
            write_bit((value >> i) & 0x1);
        }
    }

    return EVX_SUCCESS;
}

evx_status bitstream::write_bit(uint8 value) 
{
    if (write_index + 1 > query_capacity()) 
    {
        return EVX_ERROR_CAPACITY_LIMIT;
    }

    /* Determine the current byte to write. */
    uint32 dest_byte = write_index >> 3;
    uint8 dest_bit = write_index % 8;

    /* Pull the correct byte from our data store, update it, and then store it.
       Note that we do not guarantee that unused buffer memory was zero filled,
       thus we safely clear the write bit. */
    uint8 *data = &(data_store[dest_byte]);
    *data = ((*data) & ~(0x1 << dest_bit)) | (value & 0x1) << dest_bit;
    write_index++;

    return EVX_SUCCESS;
}

evx_status bitstream::write_bits(void *data, uint32 bit_count) 
{
    if (EVX_PARAM_CHECK) 
    {
        if (!data || 0 == bit_count) 
        {
            return evx_post_error(EVX_ERROR_INVALIDARG);
        }
    }

    if (write_index + bit_count > query_capacity()) 
    {
        return EVX_ERROR_CAPACITY_LIMIT;
    }

    uint32 bits_copied = 0;
    uint8 *source = reinterpret_cast<uint8 *>(data);

    if (0 == (write_index % 8) && (bit_count >= 8)) 
    {
        /* We can perform a (partial) fast copy because our source and destination 
           are byte aligned. We handle any trailing bits below. */
        bits_copied = aligned_bit_copy(data_store, write_index, source, 0, bit_count);

        if (!bits_copied) 
        {
            return evx_post_error(EVX_ERROR_EXECUTION_FAILURE);
        }
    }

    if (bits_copied < bit_count) 
    {
        /* Perform unaligned copies of our data. */
        bits_copied += unaligned_bit_copy(data_store, write_index + bits_copied, source, bits_copied, bit_count - bits_copied);
    }

    write_index += bits_copied;

    return EVX_SUCCESS;
}

evx_status bitstream::write_bytes(void *data, uint32 byte_count) 
{
    return write_bits(data, byte_count << 3);
}

evx_status bitstream::read_bit(void *data) 
{
    if (EVX_PARAM_CHECK) 
    {
        if (!data) 
        {
            return evx_post_error(EVX_ERROR_INVALIDARG);
        }
    }

    if (read_index >= write_index) 
    {
        return evx_post_error(EVX_ERROR_INVALID_RESOURCE);
    }

    /* Determine the current byte to read from. */
    uint32 source_byte = read_index >> 3;
    uint8 source_bit = read_index % 8;
    uint8 *dest = reinterpret_cast<uint8 *>(data);

    /* Pull the correct byte from our data store. Note that we 
       preserve the high bits of *dest. */
    *dest &= 0xFE;
    *dest |= ((data_store[source_byte]) >> source_bit) & 0x1;
    read_index++;

    return EVX_SUCCESS;
}

evx_status bitstream::read_byte(void *data) 
{
    if (EVX_PARAM_CHECK) 
    {
        if (!data) 
        {
            return evx_post_error(EVX_ERROR_INVALIDARG);
        }
    }

    if (read_index + 8 > write_index) 
    {
        return EVX_ERROR_INVALID_RESOURCE;
    }

    /* Determine the current byte to read from. */
    uint32 source_byte = read_index >> 3;
    uint8 source_bit = read_index % 8;
    uint8 *dest = reinterpret_cast<uint8 *>(data);

    if (0 == (source_bit % 8)) 
    {
        *dest = data_store[source_byte];
        read_index += 8;
    } 
    else
    {
        /* Slower byte unaligned read. */
        for (uint8 i = 0; i < 8; ++i) 
        {
            uint8 temp = 0;
            read_bit(&temp);
            *dest = (*dest & ~(0x1 << i)) | (temp << i);
        }
    }

    return EVX_SUCCESS;
}

evx_status bitstream::read_bits(void *data, uint32 *bit_count) 
{
    if (EVX_PARAM_CHECK) 
    {
        if (!data || !bit_count || 0 == *bit_count) 
        {
            return evx_post_error(EVX_ERROR_INVALIDARG);
        }
    }

    /* We attempt to read *puiBitCount bits and replace it with the number
       actually read. */
    if (read_index + (*bit_count) > write_index) 
    {
        (*bit_count) = write_index - read_index;
    }

    uint32 bits_copied = 0;
    uint8 *dest = reinterpret_cast<uint8 *>(data);

    if (0 == (read_index % 8) && ((*bit_count) >= 8 )) 
    {
        /* We can perform a (partial) fast copy because our source and destination 
           are byte aligned. We handle any trailing bits below. */
        bits_copied = aligned_bit_copy(dest, 0, data_store, read_index, (*bit_count));

        if (!bits_copied) 
        {
            return evx_post_error(EVX_ERROR_EXECUTION_FAILURE);
        }
    }

    /* Perform unaligned copies of our data. */
    if (bits_copied < (*bit_count)) 
    {
        bits_copied += unaligned_bit_copy(dest, bits_copied, data_store, read_index + bits_copied, (*bit_count) - bits_copied);
    }

    read_index += bits_copied;

    return EVX_SUCCESS;
}

evx_status bitstream::read_bytes(void *data, uint32 *byte_count) 
{
    if (EVX_PARAM_CHECK) 
    {
        if (!byte_count || 0 == *byte_count) 
        {
            return evx_post_error(EVX_ERROR_INVALIDARG);
        }
    }

    uint32 bit_count = (*byte_count) << 3;
    evx_status result = read_bits(data, &bit_count);
    (*byte_count) = bit_count >> 3;

    return result;
}

} // namespace EVX