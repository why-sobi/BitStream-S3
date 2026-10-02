// host/include/host.hpp
#pragma once

#include "stb_image_resize2.h"
#include "canvas.hpp"
#include "network.hpp"

#include "shared/include/protocol.hpp"
#include "shared/include/view.hpp"
#include "shared/include/jpeg.hpp"

#include <vector>
#include <cstdint>
#include <cstddef>
#include <tuple>

class BitStreamHost {
    DesktopCanvas canvas;
    NetworkEngine network;

    std::vector<uint8_t> low_res; // Downsampled RGB888 image buffer
    JPEG::JPEG jpeg_frame;        // Current encoded JPEG frame

    ImageView2D<const uint8_t> view;
    size_t original_width{0}, original_height{0};
    size_t resized_width{0},  resized_height{0};
    uint8_t jpeg_quality{75};

    uint16_t frame_sequence_id{0};

public:
    BitStreamHost(
        size_t resized_width, 
        size_t resized_height, 
        const std::vector<Device>& devices,
        uint8_t jpeg_quality = 75,
        uint16_t host_listen_port = HOST_INPUT_PORT
    );

    void tick();
    void setup_payload();
    bool send_payload();
    void step();

    size_t poll_inputs(std::span<uint8_t> out_buffer);

    size_t get_last_frame_bytes() const { return this->jpeg_frame.size(); }
    double get_current_mbps(double target_fps = 30.0) const { return (static_cast<double>(this->jpeg_frame.size() * 8) * target_fps) / 1'000'000.0; }
};