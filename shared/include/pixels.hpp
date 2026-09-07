#ifndef PIXEL_FORMATS_H
#define PIXEL_FORMATS_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    PIXEL_FORMAT_RGB888 = 0, // 3.0 B/px - Passthrough / Uncompressed
    PIXEL_FORMAT_RGB565 = 1, // 2.0 B/px - High quality, native ESP32 SPI
    PIXEL_FORMAT_RGB444 = 2  // 1.5 B/px - High bandwidth efficiency (2 px -> 3 B)
} PixelFormat;

// Helper to calculate required output buffer size in bytes
size_t get_packed_buffer_size(PixelFormat fmt, size_t pixel_count);

// Router Functions
void pack_pixels(PixelFormat fmt, const uint8_t* src_rgb888, uint8_t* dst, size_t pixel_count);
void unpack_pixels(PixelFormat fmt, const uint8_t* src, uint8_t* dst_rgb888, size_t pixel_count);

// Low-level Encoders & Decoders
void pack_rgb565(const uint8_t* src, uint8_t* dst, size_t pixel_count);
void pack_rgb444(const uint8_t* src, uint8_t* dst, size_t pixel_count);
void unpack_rgb444(const uint8_t* src, uint8_t* dst, size_t pixel_count);

#ifdef __cplusplus
}
#endif

#endif // PIXEL_FORMATS_H

// ============================================================================
// IMPLEMENTATION SECTION
// ============================================================================
#ifdef PIXEL_IMPLEMENTATION
#undef PIXEL_IMPLEMENTATION

#ifdef __cplusplus
extern "C" {
#endif

size_t get_packed_buffer_size(PixelFormat fmt, size_t pixel_count) {
    switch (fmt) {
        case PIXEL_FORMAT_RGB888: return pixel_count * 3;
        case PIXEL_FORMAT_RGB565: return pixel_count * 2;
        case PIXEL_FORMAT_RGB444: return (pixel_count * 3 + 1) / 2; // Handles odd pixel bounds
        default: return 0;
    }
}

// Host-side Router
void pack_pixels(PixelFormat fmt, const uint8_t* src, uint8_t* dst, size_t pixel_count) {
    switch (fmt) {
        case PIXEL_FORMAT_RGB565:
            pack_rgb565(src, dst, pixel_count);
            break;

        case PIXEL_FORMAT_RGB444:
            pack_rgb444(src, dst, pixel_count);
            break;

        case PIXEL_FORMAT_RGB888:
        default:
            // Direct memory copy if format is uncompressed RGB888
            for (size_t i = 0; i < pixel_count * 3; ++i) {
                dst[i] = src[i];
            }
            break;
    }
}

// Client-side (ESP32) Router
void unpack_pixels(PixelFormat fmt, const uint8_t* src, uint8_t* dst, size_t pixel_count) {
    switch (fmt) {
        case PIXEL_FORMAT_RGB444:
            unpack_rgb444(src, dst, pixel_count);
            break;

        case PIXEL_FORMAT_RGB565:
            // RGB565 is usually streamed directly to SPI DMA on ESP32,
            // but this passthrough/copy exists if unpacking to RGB888 is required.
            for (size_t i = 0; i < pixel_count * 2; ++i) {
                dst[i] = src[i];
            }
            break;

        case PIXEL_FORMAT_RGB888:
        default:
            for (size_t i = 0; i < pixel_count * 3; ++i) {
                dst[i] = src[i];
            }
            break;
    }
}

// --- Implementation Logic ---

void pack_rgb565(const uint8_t* src, uint8_t* dst, size_t pixel_count) {
    for (size_t i = 0; i < pixel_count; ++i) {
        uint8_t r = src[i * 3 + 0];
        uint8_t g = src[i * 3 + 1];
        uint8_t b = src[i * 3 + 2];

        uint16_t rgb565 = ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);

        dst[i * 2 + 0] = (uint8_t)(rgb565 >> 8);   // High byte (Big Endian)
        dst[i * 2 + 1] = (uint8_t)(rgb565 & 0xFF); // Low byte
    }
}

void pack_rgb444(const uint8_t* src, uint8_t* dst, size_t pixel_count) {
    size_t pairs = pixel_count / 2;
    for (size_t i = 0; i < pairs; ++i) {
        const uint8_t* p0 = src + (i * 2) * 3;
        const uint8_t* p1 = src + (i * 2 + 1) * 3;
        uint8_t* d = dst + (i * 3);

        d[0] = (p0[0] & 0xF0) | (p0[1] >> 4);
        d[1] = (p0[2] & 0xF0) | (p1[0] >> 4);
        d[2] = (p1[1] & 0xF0) | (p1[2] >> 4);
    }

    if (pixel_count % 2 != 0) {
        const uint8_t* p0 = src + (pairs * 2) * 3;
        uint8_t* d = dst + (pairs * 3);
        d[0] = (p0[0] & 0xF0) | (p0[1] >> 4);
        d[1] = (p0[2] & 0xF0);
        d[2] = 0x00;
    }
}

void unpack_rgb444(const uint8_t* src, uint8_t* dst, size_t pixel_count) {
    size_t pairs = pixel_count / 2;
    for (size_t i = 0; i < pairs; ++i) {
        const uint8_t* s = src + (i * 3);
        uint8_t* p0 = dst + (i * 2) * 3;
        uint8_t* p1 = dst + (i * 2 + 1) * 3;

        uint8_t r0_4 = s[0] >> 4,  g0_4 = s[0] & 0x0F;
        uint8_t b0_4 = s[1] >> 4,  r1_4 = s[1] & 0x0F;
        uint8_t g1_4 = s[2] >> 4,  b1_4 = s[2] & 0x0F;

        p0[0] = (r0_4 << 4) | r0_4; 
        p0[1] = (g0_4 << 4) | g0_4; 
        p0[2] = (b0_4 << 4) | b0_4;

        p1[0] = (r1_4 << 4) | r1_4; 
        p1[1] = (g1_4 << 4) | g1_4; 
        p1[2] = (b1_4 << 4) | b1_4;
    }
}

#ifdef __cplusplus
}
#endif

#endif // PIXEL_IMPLEMENTATION