// bq_quant_flat.hpp
// Quant scheme: flat min/max, Q4_0-style. Endpoints are packed via
// Cfg::pair_traits_t (see bq_common.hpp) instead of two separate
// fields, so the byte layout matches whatever ColorPairTraits
// specialization Cfg::fmt selects — including RGB444's 3-byte pair
// packing and RGB565's explicit big-endian layout.
//
// encode()/decode() now operate on ImageView2D subviews instead of raw
// pointer+stride+offset params. The caller is responsible for slicing
// out the block-sized subview (via make_subview) before calling in —
// that's where offset math lives now, once, instead of being
// recomputed inside every quant scheme that gets added later.

#pragma once
#include "bq_common.hpp"
#include "../view.hpp"
#include <cstdint>

namespace bq {

/// @brief Flat min/max block quantization (Q4_0-style).
///
/// Reduces a block to two endpoint colors (the block's per-channel
/// min and max) plus a per-pixel index that linearly interpolates
/// between them. One implicit "scale" per block — the whole block
/// shares the same endpoint range, so a block with a sharp internal
/// edge (endpoint distance is large) will show more banding than a
/// smooth block at the same INDEX_BITS. This is the scheme to A/B
/// against HierarchicalQuant when testing mixed-detail content (UI
/// over background).
///
/// @tparam Cfg        A BlockConfig<...> instance supplying block
///                     dimensions and color format.
/// @tparam INDEX_BITS Bits spent per pixel on its interpolation index
///                     between the two endpoints. 2 bits = 4 levels,
///                     3 bits = 8 levels, etc. — higher values reduce
///                     banding within a block at the cost of more
///                     bits/pixel; this is the main quality/size knob
///                     independent of block size or color format.
template <typename Cfg, int INDEX_BITS>
struct FlatMinMaxQuant {
    // INDEX_BITS determines how many bits are spent per pixel to choose its color from the gradient created between the block's two endpoint colors (c0 and c1).
    // INDEX_BITS is your primary quality vs. bandwidth knob.
    static constexpr int index_bits    = INDEX_BITS; 
    static constexpr int index_levels  = 1 << INDEX_BITS; 
    static constexpr size_t pair_bytes = Cfg::pair_traits_t::pair_size_bytes; // total bytes taken by a fair in this particular ColorFormat
    using ConfigType = Cfg;

    #pragma pack(push, 1)
    struct Wire { // is sent over the network as is makes it easy 
        uint16_t block_x, block_y;
        uint8_t endpoint_pair[pair_bytes];   // both endpoints, packed as a unit

        // Total packed storage space for pixel indices in bytes.
        // - Cfg::pixel_count * INDEX_BITS : total raw bits needed for all block pixels
        // - (+ 7) / 8                     : ceiling division (ceil(N / 8)), so a
        //                                   partial tail byte still gets allocated
        uint8_t indices[(Cfg::pixel_count * INDEX_BITS + 7) / 8];

        static constexpr size_t wire_size = sizeof(block_x) + sizeof(block_y) + pair_bytes + ((Cfg::pixel_count * INDEX_BITS + 7) / 8);
    };

    #pragma pack(pop)

    /// @brief Encode one block: scan for min/max endpoints, then quantize
    ///        each pixel to its nearest interpolated index. Runs on the
    ///        PC side — safe to call from within an OpenMP parallel loop
    ///        over blocks, since it touches no shared mutable state.
    ///
    /// @param block  A read-only view over exactly this block's pixels —
    ///                construct via make_subview(frame_view, bx*block_w,
    ///                by*block_h, block_w, block_h) at the call site.
    ///                Using ImageView2D<const RGB8> (not RGB8) means the
    ///                compiler enforces read-only access here — no way
    ///                to accidentally mutate the source frame.
    /// @param out    Wire struct to fill in — endpoints and packed
    ///                indices are written here, ready to send.
    static void encode(const ImageView2D<const RGB8>& block, Wire& out)
    {
        uint8_t min_r = 255, min_g = 255, min_b = 255;
        uint8_t max_r = 0,   max_g = 0,   max_b = 0;

        // finding mix/max for RGB values
        for (size_t y = 0; y < Cfg::block_h; ++y) {
            for (size_t x = 0; x < Cfg::block_w; ++x) {
                const RGB8& px = block(y, x);
                if (px.r < min_r) min_r = px.r; if (px.r > max_r) max_r = px.r;
                if (px.g < min_g) min_g = px.g; if (px.g > max_g) max_g = px.g;
                if (px.b < min_b) min_b = px.b; if (px.b > max_b) max_b = px.b;
            }
        }

        // making min and max RGB class and packing them to create endpoints (these are going to be used to interpolate the rest)
        RGB8 c0{min_r, min_g, min_b}, c1{max_r, max_g, max_b};
        Cfg::pair_traits_t::pack_pair(c0, c1, out.endpoint_pair); // packed the endpoints together

        // interpolating
        for (size_t y = 0; y < Cfg::block_h; ++y) { 
            for (size_t x = 0; x < Cfg::block_w; ++x) { 
                
                const RGB8& px = block(y, x);               // Access the target pixel's RGB reference at coordinate (x, y)
                int best_level = 0, best_dist = INT32_MAX;  // Track the best palette level index and initialize minimum distance to max possible integer
                
                for (int lvl = 0; lvl < index_levels; ++lvl) {              // Iterate through all available color interpolation steps between min and max colors
                    float t = static_cast<float>(lvl) / (index_levels - 1); // Calculate normalized interpolation factor t in range [0.0, 1.0]
                    
                    int ir = static_cast<int>(min_r + t * (max_r - min_r)); // Interpolate red component between min_r and max_r
                    int ig = static_cast<int>(min_g + t * (max_g - min_g)); // Interpolate green component between min_g and max_g
                    int ib = static_cast<int>(min_b + t * (max_b - min_b)); // Interpolate blue component between min_b and max_b
                    
                    int dist = (ir-px.r)*(ir-px.r) + (ig-px.g)*(ig-px.g) + (ib-px.b)*(ib-px.b); // Calculate squared Euclidean color distance between pixel and candidate color
                    if (dist < best_dist) { best_dist = dist; best_level = lvl; }               // Update nearest color match if this candidate yields a lower distance
                } 
                
                set_index_bits(out.indices, static_cast<int>(y * Cfg::block_w + x), index_bits, // Compute flat 1D pixel offset and pack the best palette index into the bit array
                            static_cast<uint8_t>(best_level)); // Cast the winning palette level index to byte for bit-packing
            } 
        } 
    }

    /// @brief Decode one block: unpack the endpoint pair, then write each
    ///        pixel's interpolated color into the destination view.
    ///        Runs on the ESP32 side, per received block, per frame — kept
    ///        allocation-free and branch-light.
    ///
    /// @param in           The received Wire struct for this block.
    /// @param dst_block    A mutable view over exactly this block's region
    ///                      of the destination framebuffer — construct via
    ///                      make_subview(framebuffer_view, bx*block_w,
    ///                      by*block_h, block_w, block_h) at the call
    ///                      site. Values written are native uint16_t
    ///                      RGB565 — this is the local render target, not
    ///                      a wire format; your display library handles
    ///                      actual SPI byte order when it pushes the
    ///                      framebuffer out.
    static void decode(const Wire& in, ImageView2D<uint16_t>& dst_block) 
    // notice we're using uint16_t (since RGB565 uses 2bytes per pixel)
    // This is essential because in embedded system RGB565 is treated as native
    {
        RGB8 c0, c1; // c0 = min ; c1 = max
        Cfg::pair_traits_t::unpack_pair(in.endpoint_pair, c0, c1); // we had previously stored them as single unit 

        for (size_t y = 0; y < Cfg::block_h; ++y) { 
            for (size_t x = 0; x < Cfg::block_w; ++x) { 
                // Extract packed palette level index for current pixel at flat offset (y * width + x)
                uint8_t level = get_index_bits(in.indices, static_cast<int>(y * Cfg::block_w + x), index_bits);
                 
                float t = static_cast<float>(level) / (index_levels - 1); // Normalize index level to range [0.0, 1.0] for linear interpolation
                
                uint8_t r = static_cast<uint8_t>(c0.r + t * (c1.r - c0.r)); // Interpolate red channel value between endpoint colors c0 and c1
                uint8_t g = static_cast<uint8_t>(c0.g + t * (c1.g - c0.g)); // Interpolate green channel value between endpoint colors c0 and c1
                uint8_t b = static_cast<uint8_t>(c0.b + t * (c1.b - c0.b)); // Interpolate blue channel value between endpoint colors c0 and c1
                
                dst_block(y, x) = static_cast<uint16_t>(((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3)); // Pack 8-bit RGB components into 16-bit RGB565 pixel format (R:5 bits, G:6 bits, B:5 bits) and write to output block
            } 
        } 
    }
};

} // namespace bq