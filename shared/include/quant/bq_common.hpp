// bq_common.hpp
// Shared by every quant scheme: color formats, block geometry config,
// bit-packing helpers.
//
// COLOR FORMAT DESIGN: colors are packed as PAIRS (ColorPairTraits),
// not individually. Every quant scheme here stores exactly two
// endpoints per block, and packing them as a unit lets RGB444 use its
// natural 2-items -> 3-bytes packing (borrowed directly from Sobi's
// pixel_formats.h pack_rgb444/unpack_rgb444 — a block's 2 endpoints
// are the same shape as 2 pixels, so the exact same bit-math applies
// unmodified). RGB565 packing is explicit big-endian (also ported from
// pixel_formats.h) rather than relying on native uint16_t layout, so
// the wire format doesn't silently depend on PC and ESP32 sharing
// endianness.
//
// EXTENSION POINT 1 lives here: new color format = new enum value +
// new ColorPairTraits specialization. Nothing else in the project
// changes.

#pragma once
#include <cstdint>
#include <cstddef>

#include "../view.hpp"

namespace bq
{

    enum class ColorFormat : uint8_t
    {
        RGB888, // 6 B/pair - passthrough/uncompressed
        RGB565, // 4 B/pair - high quality, native ESP32 SPI, explicit big-endian
        RGB444, // 3 B/pair - reuses pixel_formats.h's 2px->3B trick on the 2 endpoints
        RGB332, // 2 B/pair - most aggressive
    };

    struct RGB8
    {
        uint8_t r, g, b;
    };

    // tells the compiler that ColorPairTraits accept ColorFormat enum but not the definition yet
    template <ColorFormat>
    struct ColorPairTraits;

    // These are specific defitions for each of the enum (compile time dispatch)
    // --- RGB888: direct passthrough, 3 bytes per endpoint ---
    template <>
    struct ColorPairTraits<ColorFormat::RGB888>
    {
        static constexpr size_t pair_size_bytes = 6;
        static void pack_pair(const RGB8 &c0, const RGB8 &c1, uint8_t *dst)
        {
            dst[0] = c0.r; dst[1] = c0.g; dst[2] = c0.b;
            dst[3] = c1.r; dst[4] = c1.g; dst[5] = c1.b;
        }
        static void unpack_pair(const uint8_t *src, RGB8 &c0, RGB8 &c1)
        {
            c0.r = src[0]; c0.g = src[1]; c0.b = src[2];
            c1.r = src[3]; c1.g = src[4]; c1.b = src[5];
        }
    };

    // --- RGB565: explicit big-endian, ported from pack_rgb565 ---
    template <>
    struct ColorPairTraits<ColorFormat::RGB565>
    {
        static constexpr size_t pair_size_bytes = 4;
        static void pack_one(const RGB8 &c, uint8_t *dst)
        {
            uint16_t v = static_cast<uint16_t>(((c.r & 0xF8) << 8) | ((c.g & 0xFC) << 3) | (c.b >> 3));
            dst[0] = static_cast<uint8_t>(v >> 8); // high byte first (big-endian)
            dst[1] = static_cast<uint8_t>(v & 0xFF);
        }
        static RGB8 unpack_one(const uint8_t *src)
        {
            uint16_t v = static_cast<uint16_t>((src[0] << 8) | src[1]);
            RGB8 c;
            c.r = static_cast<uint8_t>((v >> 8) & 0xF8);
            c.g = static_cast<uint8_t>((v >> 3) & 0xFC);
            c.b = static_cast<uint8_t>((v << 3) & 0xF8);
            return c;
        }
        static void pack_pair(const RGB8 &c0, const RGB8 &c1, uint8_t *dst)
        {
            pack_one(c0, dst);
            pack_one(c1, dst + 2);
        }
        static void unpack_pair(const uint8_t *src, RGB8 &c0, RGB8 &c1)
        {
            c0 = unpack_one(src);
            c1 = unpack_one(src + 2);
        }
    };

    // --- RGB444: ported directly from pack_rgb444/unpack_rgb444. Packing
    //     exactly 2 colors -> 3 bytes is precisely what that function
    //     already did for 2 pixels; a block's 2 endpoints fit the same
    //     shape with zero adaptation. ---
    template <>
    struct ColorPairTraits<ColorFormat::RGB444>
    {
        static constexpr size_t pair_size_bytes = 3;
        static void pack_pair(const RGB8 &c0, const RGB8 &c1, uint8_t *dst)
        {
            dst[0] = static_cast<uint8_t>((c0.r & 0xF0) | (c0.g >> 4));
            dst[1] = static_cast<uint8_t>((c0.b & 0xF0) | (c1.r >> 4));
            dst[2] = static_cast<uint8_t>((c1.g & 0xF0) | (c1.b >> 4));
        }
        static void unpack_pair(const uint8_t *src, RGB8 &c0, RGB8 &c1)
        {
            uint8_t r0_4 = src[0] >> 4, g0_4 = src[0] & 0x0F;
            uint8_t b0_4 = src[1] >> 4, r1_4 = src[1] & 0x0F;
            uint8_t g1_4 = src[2] >> 4, b1_4 = src[2] & 0x0F;
            
            c0.r = static_cast<uint8_t>((r0_4 << 4) | r0_4);
            c0.g = static_cast<uint8_t>((g0_4 << 4) | g0_4);
            c0.b = static_cast<uint8_t>((b0_4 << 4) | b0_4);
            c1.r = static_cast<uint8_t>((r1_4 << 4) | r1_4);
            c1.g = static_cast<uint8_t>((g1_4 << 4) | g1_4);
            c1.b = static_cast<uint8_t>((b1_4 << 4) | b1_4);
        }
    };

    // --- RGB332: most aggressive, 1 byte per endpoint ---
    template <>
    struct ColorPairTraits<ColorFormat::RGB332>
    {
        static constexpr size_t pair_size_bytes = 2;
        static uint8_t pack_one(const RGB8 &c)
        {
            return static_cast<uint8_t>((c.r & 0xE0) | ((c.g & 0xE0) >> 3) | (c.b >> 6));
        }
        static RGB8 unpack_one(uint8_t v)
        {
            RGB8 c;
            c.r = v & 0xE0;
            c.g = static_cast<uint8_t>((v << 3) & 0xE0);
            c.b = static_cast<uint8_t>((v << 6) & 0xC0);
            return c;
        }
        static void pack_pair(const RGB8 &c0, const RGB8 &c1, uint8_t *dst)
        {
            dst[0] = pack_one(c0);
            dst[1] = pack_one(c1);
        }
        static void unpack_pair(const uint8_t *src, RGB8 &c0, RGB8 &c1)
        {
            c0 = unpack_one(src[0]);
            c1 = unpack_one(src[1]);
        }
    };



    // ==== CONFIG Structure that'll handle all the calling and what not. ====

    // Block geometry — independent of color format AND quant scheme.
    /// @brief Compile-time block geometry + color-format configuration.
    ///
    /// Purely descriptive — carries no quant-scheme logic itself. Any quant
    /// scheme (FlatMinMaxQuant, HierarchicalQuant, etc.) is parameterized
    /// by one of these to pick up its block dimensions and color format
    /// without needing to know about either directly.
    ///
    /// @tparam BLOCK_W Block width in pixels.
    /// @tparam BLOCK_H Block height in pixels.
    /// @tparam FMT     Color format used for this block's endpoints (see
    ///                  ColorFormat). Determines pair_traits_t, and through
    ///                  it, how many bytes the endpoint pair costs on the wire.
    template <int BLOCK_W, int BLOCK_H, ColorFormat FMT>
    struct BlockConfig
    {
        static constexpr int block_w = BLOCK_W;
        static constexpr int block_h = BLOCK_H;
        static constexpr int pixel_count = BLOCK_W * BLOCK_H;
        static constexpr ColorFormat fmt = FMT;
        using pair_traits_t = ColorPairTraits<FMT>;
    };

    using Cfg4x4_RGB332 = BlockConfig<4, 4, ColorFormat::RGB332>;
    using Cfg4x4_RGB444 = BlockConfig<4, 4, ColorFormat::RGB444>;
    using Cfg4x4_RGB565 = BlockConfig<4, 4, ColorFormat::RGB565>;

   // Bit-packing helpers — used by any scheme with a packed index array.

    /**
     * @brief Packs a multi-bit value into a continuous byte array at a specific pixel index.
     * 
     * @param buf       Pointer to the destination byte buffer.
     * @param pixel_idx Zero-based index of the pixel/element being written.
     * @param bits      Number of bits allocated per pixel (e.g., 1, 2, 4 bits).
     * @param value     The value to store (least significant `bits` will be written).
     */
    inline void set_index_bits(uint8_t *buf, int pixel_idx, int bits, uint8_t value)
    {
        int bit_pos = pixel_idx * bits; // Calculate the absolute starting bit position in the buffer for this pixel
        // Process each bit of the value individually (from LSB to MSB)
        for (int i = 0; i < bits; ++i) {
            int byte_i = (bit_pos + i) / 8; // Identify which byte in the buffer holds the current target bit
            int bit_i  = (bit_pos + i) % 8; // Determine the bit's position (0-7) within that specific byte

            uint8_t bit_val = (value >> i) & 1; // Extract the i-th bit from 'value' (0 or 1)

            // Update the buffer byte:
            // 1. `buf[byte_i] & ~(1 << bit_i)` : Clears (zeros out) the bit at bit_i
            // 2. `(bit_val << bit_i)`          : Shifts the new bit value to position bit_i
            // 3. `|`                           : Sets the cleared position to the new bit value
            buf[byte_i] = static_cast<uint8_t>((buf[byte_i] & ~(1 << bit_i)) | (bit_val << bit_i));
        }
    }

    /**
     * @brief Unpacks/reads a multi-bit value from a continuous byte array at a specific pixel index.
     * 
     * @param buf       Pointer to the source packed byte buffer.
     * @param pixel_idx Zero-based index of the pixel/element being read.
     * @param bits      Number of bits allocated per pixel (e.g., 1, 2, 4 bits).
     * @return uint8_t  The unpacked pixel value.
     */
    inline uint8_t get_index_bits(const uint8_t *buf, int pixel_idx, int bits)
    {
        int bit_pos = pixel_idx * bits; // Calculate the absolute starting bit position in the buffer for this pixel
        uint8_t value = 0;              // Accumulator variable to hold the extracted bit field

        // Read each bit from the buffer individually (from LSB to MSB)
        for (int i = 0; i < bits; ++i)
        {
            int byte_i = (bit_pos + i) / 8; // Identify which byte in the buffer holds the current target bit
            int bit_i  = (bit_pos + i) % 8; // Determine the bit's position (0-7) within that specific byte
            
            uint8_t bit_val = (buf[byte_i] >> bit_i) & 1;         // Extract the target bit from the buffer byte (shifts target bit to position 0 and masks)
            value = static_cast<uint8_t>(value | (bit_val << i)); // Insert the extracted bit into position 'i' of the return 'value'
            
        }
        return value;
    }

} // namespace bq