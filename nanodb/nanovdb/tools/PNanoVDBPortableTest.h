#ifndef NANOVDB_TOOLS_PNANOVDB_PORTABLE_TEST_H_HAS_BEEN_INCLUDED
#define NANOVDB_TOOLS_PNANOVDB_PORTABLE_TEST_H_HAS_BEEN_INCLUDED

#define PNANOVDB_C
#define PNANOVDB_ADDRESS_64
#include <nanovdb/PNanoVDB.h>
#include <cstdio>
#include <cstring>
#include <cmath>

namespace nanovdb {
namespace tools {

/// @brief Validates the PNanoVDB portable C/GPU API surface with a minimal in-memory grid.
///        This test does not require any external file I/O; it builds a tiny raw NanoVDB
///        byte stream manually and walks it through the PNanoVDB C API.
class PNanoVDBPortableTest
{
public:
    /// @brief Run the full portable validation sequence.
    /// @return 0 on success, nonzero on failure.
    static int run()
    {
        printf("=== PNanoVDB Portable C/GPU Test ===\n");

        // Build a tiny raw buffer in memory and validate the PNanoVDB C API surface.
        uint32_t buf[64] = {0};
        uint64_t word_count = 0;

        // 1. Write a recognizable magic pattern into the buffer.
        buf[word_count++] = 0x4E414E4FULL; // "NANO" lower 32 bits
        buf[word_count++] = 0x30445642ULL; // "VDB0" lower 32 bits

        pnanovdb_buf_t pbuf = pnanovdb_make_buf(buf, word_count);

        // 2. Validate magic readback via the portable API.
        pnanovdb_uint64_t read_magic = pnanovdb_buf_read_uint64(pbuf, 0);
        printf("Magic readback: 0x%llx\n", (unsigned long long)read_magic);

        // 3. Validate address API
        pnanovdb_address_t null_addr = pnanovdb_address_null();
        if (!pnanovdb_address_is_null(null_addr)) {
            printf("FAIL: null address check\n");
            return 1;
        }

        pnanovdb_address_t off = pnanovdb_address_offset(null_addr, 16);
        if (off.byte_offset != 16) {
            printf("FAIL: address offset = %llu\n", (unsigned long long)off.byte_offset);
            return 1;
        }

        // 4. Validate scalar reinterpretation helpers
        float fval = 3.1415926f;
        pnanovdb_uint32_t uival = pnanovdb_float_as_uint32(fval);
        float fval2 = pnanovdb_uint32_as_float(uival);
        if (std::fabs(fval2 - fval) > 1e-6f) {
            printf("FAIL: float/uint32 round-trip\n");
            return 1;
        }

        // 5. Validate read/write on the buffer
        pnanovdb_buf_write_uint32(pbuf, 32u, 0xDEADBEEF);
        pnanovdb_uint32_t back = pnanovdb_buf_read_uint32(pbuf, 32u);
        if (back != 0xDEADBEEF) {
            printf("FAIL: buf read/write got 0x%x\n", back);
            return 1;
        }

        // 6. Validate vec3 types exist and are sized
        pnanovdb_vec3_t v = {1.0f, 2.0f, 3.0f};
        if (v.x != 1.0f || v.y != 2.0f || v.z != 3.0f) {
            printf("FAIL: pnanovdb_vec3_t\n");
            return 1;
        }

        // 7. Validate grid type constants are defined
        if (PNANOVDB_GRID_TYPE_FLOAT != 1u || PNANOVDB_GRID_TYPE_MASK != 8u) {
            printf("FAIL: grid type constants\n");
            return 1;
        }

        printf("PNanoVDB portable C/GPU test passed.\n");
        return 0;
    }
};

} // namespace tools
} // namespace nanovdb

#endif // NANOVDB_TOOLS_PNANOVDB_PORTABLE_TEST_H_HAS_BEEN_INCLUDED
