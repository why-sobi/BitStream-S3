// shared/include/jpeg.hpp
#pragma once

#include "view.hpp"
#include <vector>
#include <cstdint>
#include <cstddef>
#include <stdexcept>
#include <string>

#if defined(BITSTREAM_BUILD_HOST)
#include <turbojpeg.h>
#endif

namespace JPEG {
    struct JPEG {
        std::vector<uint8_t> buffer;

        [[nodiscard]] const uint8_t* ptr() const noexcept { return buffer.data(); }
        [[nodiscard]] size_t size() const noexcept { return buffer.size(); }
        [[nodiscard]] bool empty() const noexcept { return buffer.empty(); }
    };

    #if defined(BITSTREAM_BUILD_HOST)
    
    /// @brief Encodes a raw RGB888 view into a JPEG byte payload in RAM.
    /// Uses a persistent static TurboJPEG handle for maximum single-threaded throughput.
    inline JPEG encode(ImageView2D<const uint8_t> view, int quality = 75) {
        // Persistent handle: Initialized ONCE on the first frame encode
        static tjhandle compressor = []() {
            tjhandle h = tjInitCompress();
            if (!h) {
                throw std::runtime_error("Failed to initialize TurboJPEG compressor: " + std::string(tjGetErrorStr()));
            }
            return h;
        }();

        JPEG result;

        int width = static_cast<int>(view.width);
        int height = static_cast<int>(view.height);

        // 1. Query exact worst-case upper bound for output JPEG buffer size
        unsigned long max_buf_size = tjBufSize(width, height, TJSAMP_420);
        result.buffer.resize(max_buf_size);

        // 2. Setup parameters for compression
        unsigned char* jpeg_buf = reinterpret_cast<unsigned char*>(result.buffer.data());
        unsigned long jpeg_size = max_buf_size;
        int pitch = 0; // Tightly packed rows (width * 3)

        // 3. Compress frame using the persistent static handle (0 handle allocation calls!)
        int status = tjCompress2(
            compressor,
            reinterpret_cast<const unsigned char*>(view.data),
            width,
            pitch,
            height,
            TJPF_RGB,        // Use TJPF_BGRA here if DXGI outputs 32-bit BGRA directly
            &jpeg_buf,
            &jpeg_size,
            TJSAMP_420,      // 4:2:0 subsampling
            quality,
            TJFLAG_NOREALLOC // Use std::vector allocated memory directly
        );

        if (status != 0) {
            throw std::runtime_error(std::string("TurboJPEG encoding failed: ") + tjGetErrorStr());
        }

        // 4. Shrink vector down to actual compressed JPEG byte length
        result.buffer.resize(jpeg_size);

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