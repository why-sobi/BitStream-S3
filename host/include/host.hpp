#pragma once

#include "canvas.hpp"
#include "pixels.hpp"
#include <vector>
#include <cstdint>
#include <cstddef>
#include <tuple>
#include <mdspan>

class BitStreamHost {
    DesktopCanvas canvas;
    std::vector<uint8_t> low_res; // down sampling buffer to the resized params
    std::vector<uint8_t> packed;  // this holds the payload to be sent over the network

    std::mdspan<const uint8_t, std::dextents<size_t, 2>> view; // this is only for read, no write will be performed to it (C++23 shorthand for 2 dynamic dimensions (Height, Width))
    size_t original_width, original_height;
    size_t resized_width, resized_height;
    PixelFormat format;

public:
    BitStreamHost(size_t resized_width, size_t resized_height, PixelFormat format = PIXEL_FORMAT_RGB565);

    void tick(); // this captures and resizing populating the low_res buffer

    void setup_payload();

    bool send_payload();
};