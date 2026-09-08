#include "host.hpp"
#include "stb_image_resize2.h"
#include <cstring>
#include <algorithm>
#include <iostream>

constexpr size_t MAX_MTU_PAYLOAD = 1200;

BitStreamHost::BitStreamHost(
    size_t resized_width, 
    size_t resized_height, 
    const std::vector<Device>& devices,
    PixelFormat format,
    uint16_t host_listen_port
) : network(devices, host_listen_port),
    resized_width(resized_width), 
    resized_height(resized_height), 
    format(format) 
{
    this->canvas.init(); // warmup 
    
    std::tie(original_width, original_height) = this->canvas.get_master_resolution();

    // Re-assign the view with updated dimensions
    this->view = std::mdspan(this->canvas.get_master_buffer_ptr(), this->original_height, this->original_width);
    
    // Low-res output buffer sized to downsampled RGB888 pixels
    this->low_res.resize(this->resized_width * this->resized_height * 3);

    // Allocate the packed buffer
    size_t pixel_counts = this->resized_width * this->resized_height; 
    size_t size = get_packed_buffer_size(this->format, pixel_counts); // total resulting size after packing the pixels 

    packed.resize(size);
}

void BitStreamHost::tick() { // this captures and resizing populating the low_res buffer
    bool success = this->canvas.capture_frame();

    if (success) {
        // Resize the captured frame to the desired dimensions
        stbir_resize_uint8_linear(
            this->view.data_handle(),   // returns the underlying pointer to the data (similar to .data() for vectors and other STLs)
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

void BitStreamHost::setup_payload() { // packs pixel based on set format
    size_t pixel_counts = this->resized_width * this->resized_height;
    pack_pixels(
        this->format,           // format to use
        this->low_res.data(),   // src RGB888
        this->packed.data(),    // dst [RGB888, RGB444, RGB565]
        pixel_counts            // total pixel count
    );
}

bool BitStreamHost::send_payload() { // sends payload
    if (this->packed.empty()) return false;

    frame_sequence_id++;
    size_t total_bytes = this->packed.size();
    size_t half_bytes  = total_bytes / 2;

    // Split frame payload into top and bottom halves for dual ESP32 cores
    std::span<const uint8_t> top_region(this->packed.data(), half_bytes);
    std::span<const uint8_t> bottom_region(this->packed.data() + half_bytes, total_bytes - half_bytes);

    auto slice_and_send = [&](std::span<const uint8_t> region, size_t port_idx) {
        size_t sent = 0;
        uint8_t chunk_idx = 0;
        uint8_t total_chunks = static_cast<uint8_t>((region.size() + MAX_MTU_PAYLOAD - 1) / MAX_MTU_PAYLOAD);

        std::array<uint8_t, sizeof(PacketHeader) + MAX_MTU_PAYLOAD> pkt_buf;

        while (sent < region.size()) {
            size_t chunk_len = (std::min)(MAX_MTU_PAYLOAD, region.size() - sent);

            PacketHeader hdr{
                .magic        = PROTOCOL_MAGIC,
                .msg_type     = MSG_VIDEO,
                .sequence_id  = frame_sequence_id,
                .chunk_index  = chunk_idx,
                .total_chunks = total_chunks,
                .payload_len  = static_cast<uint16_t>(chunk_len)
            };

            std::memcpy(pkt_buf.data(), &hdr, sizeof(PacketHeader));
            std::memcpy(pkt_buf.data() + sizeof(PacketHeader), region.data() + sent, chunk_len);

            // Device 0: Target Node
            // Port Index 0 (8080) -> Core 0 Task
            // Port Index 1 (8081) -> Core 1 Task
            this->network.send(0, port_idx, std::span<const uint8_t>(pkt_buf.data(), sizeof(PacketHeader) + chunk_len));

            sent += chunk_len;
            chunk_idx++;
        }
    };

    slice_and_send(top_region, 0);    // Core 0 (Port Index 0)
    slice_and_send(bottom_region, 1); // Core 1 (Port Index 1)

    return true;
}

void BitStreamHost::step() {
    this->tick();
    this->setup_payload();
    this->send_payload();
    this->network.poll();
}

size_t BitStreamHost::poll_inputs(std::span<uint8_t> out_buffer) {
    return this->network.receive(out_buffer);
}