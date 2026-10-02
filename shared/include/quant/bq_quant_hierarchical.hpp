// bq_quant_hierarchical.hpp
// Quant scheme: hierarchical, Q4_K-style. A superblock is split into
// sub-blocks, each getting its own local scale quantized relative to
// the superblock's range — meant to handle mixed detail (sharp UI over
// smooth background) better than FlatMinMaxQuant since edge sub-blocks
// don't drag the whole block's precision down.
//
// STUB: Wire layout and call sites are real and match the pattern in
// bq_quant_flat.hpp. encode()/decode() bodies are placeholders — fill
// in when you're ready to A/B this against the flat scheme.

#pragma once
#include "bq_common.hpp"
#include <cstdint>

namespace bq {

template <typename Cfg, int SUB_BLOCKS, int SUB_INDEX_BITS>
struct HierarchicalQuant {
    static constexpr size_t pair_bytes = Cfg::pair_traits_t::pair_size_bytes;

    #pragma pack(push, 1)
    struct Wire {
        uint16_t block_x, block_y;
        uint8_t super_endpoint_pair[pair_bytes]; // superblock-level range, packed as a unit
        uint8_t sub_scales[SUB_BLOCKS];          // each sub-block's scale, quantized relative to superblock range
        uint8_t indices[(Cfg::pixel_count * SUB_INDEX_BITS + 7) / 8];
    };
    #pragma pack(pop)

    static void encode(const uint8_t* /*src*/, int /*stride*/,
                        int /*block_px_x*/, int /*block_px_y*/, Wire& /*out*/) {
        // TODO:
        //  1. Compute superblock min/max across the whole block ->
        //     super_endpoint0/1 (same scan as FlatMinMaxQuant, just
        //     over the full superblock).
        //  2. For each of SUB_BLOCKS sub-regions, compute local
        //     min/max, then quantize that local range relative to the
        //     superblock range into sub_scales[i] (few bits, e.g. a
        //     0-255 position within [super_min, super_max]).
        //  3. Quantize each pixel's index same as FlatMinMaxQuant does,
        //     but interpolate within its OWN sub-block's local range
        //     (reconstructed from sub_scales[i] + superblock range)
        //     rather than the whole block's range.
    }

    static void decode(const Wire& /*in*/, uint16_t* /*dst_rgb565*/,
                        int /*dst_stride_px*/, int /*block_px_x*/, int /*block_px_y*/) {
        // TODO: reverse of the above — for each pixel, find its
        // sub-block, reconstruct that sub-block's local range from
        // sub_scales[i] + super_endpoint0/1, interpolate by index.
    }
};

} // namespace bq
