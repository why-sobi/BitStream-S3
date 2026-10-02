// host/src/host.cpp
#include "host.hpp"
#include "stb_image_resize2.h"

#include <cstring>
#include <algorithm>
#include <iostream>

#include <chrono>

constexpr size_t MAX_MTU_PAYLOAD = 1200;

BitStreamHost::BitStreamHost(
    size_t resized_width, 
    size_t resized_height, 
    const std::vector<Device>& devices,
    uint8_t jpeg_quality,
    uint16_t host_listen_port
) : network(devices, host_listen_port),
    resized_width(resized_width), 
    resized_height(resized_height), 
    jpeg_quality(jpeg_quality) 
{
    this->canvas.init();
    std::tie(original_width, original_height) = this->canvas.get_master_resolution();

    this->view = ImageView2D<const uint8_t>(
        this->canvas.get_master_buffer_ptr(), 
        this->original_width, 
        this->original_height
    );
    
    // Allocate RGB888 downsample buffer
    this->low_res.resize(this->resized_width * this->resized_height * 3);
}

void BitStreamHost::tick() { // this captures and resizing populating the low_res buffer
    bool new_frame = this->canvas.capture_frame();

    if (new_frame) {
        // Resize the captured frame to the desired dimensions
        stbir_resize_uint8_linear(
            this->view.data,            // returns the underlying pointer to the data (similar to .data() for vectors and other STLs)
            this->original_width,       // original width of the captured frame
            this->original_height,      // original height of the captured frame
            0,                          // stride in bytes (0 means tightly packed)
            this->low_res.data(),       // pointer to the output buffer where the resized image will be stored
            this->resized_width,        // desired width of the resized frame
            this->resized_height,       // desired height of the resized frame
            0,                          // stride in bytes for the output buffer (0 means tightly packed)
            STBIR_RGB                   // specifies the pixel layout (3 for RGB format since our buffer is in RGB format)
        );
    } 
    // else reuse old buffer (i.e. low_res) if capture fails, this is to avoid returning empty buffer to the user
}

void BitStreamHost::setup_payload() {
    // Wrap downsampled low_res in ImageView2D for encoder
    ImageView2D<const uint8_t> frame_view(
        this->low_res.data(), 
        this->resized_width, 
        this->resized_height
    );

    // Encode frame to JPEG
    this->jpeg_frame = JPEG::encode(frame_view, this->jpeg_quality);
}

bool BitStreamHost::send_payload() {
    if (this->jpeg_frame.empty()) return false;

    this->frame_sequence_id++;
    
    const uint8_t* payload_data = this->jpeg_frame.ptr();
    size_t total_bytes = this->jpeg_frame.size();

    size_t sent = 0;
    uint8_t chunk_idx = 0;
    uint8_t total_chunks = static_cast<uint8_t>((total_bytes + MAX_MTU_PAYLOAD - 1) / MAX_MTU_PAYLOAD);

    std::array<uint8_t, sizeof(PacketHeader) + MAX_MTU_PAYLOAD> pkt_buf;

    while (sent < total_bytes) {
        size_t chunk_len = (std::min)(MAX_MTU_PAYLOAD, total_bytes - sent);

        PacketHeader hdr{
            .magic        = PROTOCOL_MAGIC,      // 0xB3
            .msg_type     = MSG_VIDEO,           // 0x01
            .sequence_id  = this->frame_sequence_id,
            .chunk_index  = chunk_idx,
            .total_chunks = total_chunks,
            .payload_len  = static_cast<uint16_t>(chunk_len)
        };

        std::memcpy(pkt_buf.data(), &hdr, sizeof(PacketHeader));
        std::memcpy(pkt_buf.data() + sizeof(PacketHeader), payload_data + sent, chunk_len);

        // Target Device 0, Port Index 0 (ESP1_VID_PORT_CORE0 = 8080)
        this->network.send(0, 0, std::span<const uint8_t>(pkt_buf.data(), sizeof(PacketHeader) + chunk_len));

        sent += chunk_len;
        chunk_idx++;
    }

    return true;
}

// void BitStreamHost::step() {
//     this->tick();
//     this->setup_payload();
//     this->send_payload();
//     this->network.poll();
// }


void BitStreamHost::step() {
    using namespace std::chrono;

    auto t0 = high_resolution_clock::now();
    this->tick(); // DXGI capture + downsample
    auto t1 = high_resolution_clock::now();

    this->setup_payload(); // JPEG encode
    auto t2 = high_resolution_clock::now();

    this->send_payload(); // Network send
    auto t3 = high_resolution_clock::now();

    this->network.poll();

    // Print breakdown every 60 frames
    static int frame_counter = 0;
    if (++frame_counter % 60 == 0) {
        double cap_ms = duration<double, std::milli>(t1 - t0).count();
        double enc_ms = duration<double, std::milli>(t2 - t1).count();
        double net_ms = duration<double, std::milli>(t3 - t2).count();

        std::cout << "[Profile] Capture+Resize: " << cap_ms << "ms"
                  << " | JPEG Encode: " << enc_ms << "ms"
                  << " | UDP Send: " << net_ms << "ms"
                  << " | Total: " << (cap_ms + enc_ms + net_ms) << "ms\n";
    }
}

size_t BitStreamHost::poll_inputs(std::span<uint8_t> out_buffer) {
    return this->network.receive(out_buffer);
}