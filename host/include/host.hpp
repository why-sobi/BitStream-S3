#pragma once

#include "stb_image_resize2.h"
#include "canvas.hpp"
#include "pixels.hpp"
#include "network.hpp"
#include "protocol.hpp"

#include <vector>
#include <cstdint>
#include <cstddef>
#include <tuple>
#include <mdspan>

class BitStreamHost {
    DesktopCanvas canvas;
    NetworkEngine network;

    std::vector<uint8_t> low_res; // down sampling buffer to the resized params
    std::vector<uint8_t> packed;  // this holds the payload to be sent over the network

    std::mdspan<const uint8_t, std::dextents<size_t, 2>> view; // this is only for read, no write will be performed to it (C++23 shorthand for 2 dynamic dimensions (Height, Width))
    size_t original_width, original_height;
    size_t resized_width, resized_height;
    PixelFormat format;

    uint16_t frame_sequence_id{0};

public:
    BitStreamHost(
        size_t resized_width, 
        size_t resized_height, 
        const std::vector<Device>& devices,
        PixelFormat format = PIXEL_FORMAT_RGB565,
        uint16_t host_listen_port = HOST_INPUT_PORT
    );

    void tick(); // this captures and resizing populating the low_res buffer
    void setup_payload();
    bool send_payload();

    // Clean unified execution step driving capture -> downsample -> pack -> chunk -> transmit
    void step();

    // Check for inbound telemetry/inputs from clients
    size_t poll_inputs(std::span<uint8_t> out_buffer);
};