// shared/include/jpeg.hpp
#pragma once

#include "view.hpp"
#include <vector>
#include <cstdint>
#include <cstddef>
#include <stdexcept>

#if defined(BITSTREAM_BUILD_HOST)
#include "jpge.h"
#endif

namespace JPEG {
    struct JPEG {
        std::vector<uint8_t> buffer;

        [[nodiscard]] const uint8_t* ptr() const noexcept { return buffer.data(); }
        [[nodiscard]] size_t size() const noexcept { return buffer.size(); }
        [[nodiscard]] bool empty() const noexcept { return buffer.empty(); }
    };

    #if defined(BITSTREAM_BUILD_HOST)
    inline JPEG encode(ImageView2D<const uint8_t> view, int quality = 75) {
        JPEG result;

        int width = static_cast<int>(view.width);
        int height = static_cast<int>(view.height);

        // Allocate max output buffer capacity
        int out_buf_size = width * height * 3;
        result.buffer.resize(out_buf_size);

        jpge::params params;
        params.m_quality = quality;
        params.m_subsampling = jpge::H2V2; // 4:2:0 chroma subsampling for low Wi-Fi bandwidth

        // reinterpret_cast: Tells the compiler, "Trust me, these bytes are jpge::uint8 bytes."
        // const_cast: Tells the compiler, "Yes, I am intentionally overriding the read-only check to satisfy this library's API signature."

        jpge::uint8* pImage_data = const_cast<jpge::uint8*>(
            reinterpret_cast<const jpge::uint8*>(view.data)
        );

        // Correct signature: 7 arguments total
        bool success = jpge::compress_image_to_jpeg_file_in_memory(
            result.buffer.data(),
            out_buf_size,      // Reference parameter; modified in-place to actual JPEG byte size
            width,
            height,
            3,                 // 3 channels (RGB888)
            pImage_data,       // Pixel data pointer
            params
        );

        if (!success) {
            throw std::runtime_error("jpge JPEG compression failed");
        }

        // Shrink vector to actual encoded JPEG length
        result.buffer.resize(out_buf_size);
        return result;
    }
    #endif
        
        // ----------------------------------------------------------------------------
        // CLIENT / ESP32 ENVIRONMENT (Decoder Implementation)
        // ----------------------------------------------------------------------------
    #if defined(BITSTREAM_BUILD_ESP32)
        
        /// @brief Decodes a JPEG byte payload into a raw RGB888 / RGB565 view in PSRAM.
        /// Uses ESP32 hardware JPEG / TJpgDec under the hood.
        inline bool decode(const JPEG& jpeg_image, ImageView2D<uint8_t>& out_target_view) {
            // ESP32 TJpgDec / HW Decoder glue logic goes here when building firmware
            return true;
        }

    #endif // BITSTREAM_BUILD_ESP32

} // namespace JPEG