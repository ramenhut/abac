
#include "cabac.h"
#include "math.h"

using namespace evx;

uint8 test_kernel(uint8 value)
{
    return value % 4;
}

void test_basic_cabac_rt()
{
    entropy_coder coder;
    bitstream a((uint32) 512);
    bitstream b((uint32) 512);
    bitstream c((uint32) 512);

    for (uint8 i = 0; i < 32; ++i)
    {
        a.write_byte(test_kernel(i));
    }

    uint16 raw_size = a.query_occupancy();
    evx_msg("raw size: %i bits", raw_size);

    if (raw_size != (32 << 3))
    {
        evx_err("Failed to write data to the source bitstream.");
        return;
    }

    coder.encode(&a, &b);
    evx_msg("encoded size: %i bits", b.query_occupancy());
    coder.decode(raw_size, &b, &c);

    for (uint8 i = 0; i < c.query_byte_occupancy(); ++i)
    {
        if (test_kernel(i) != c.query_data()[i])
        {
            evx_err("Data integrity check failure.");
            return;
        }
    }

    evx_msg("test completed successfully.");
}

int main() 
{
    test_basic_cabac_rt();
	return 0;
}
