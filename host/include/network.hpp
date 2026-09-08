#pragma once

/*
1. Standalone Asio networking core for cross-platform transmission.
2. Dual-mode support: Outbound fast UDP sends + non-blocking inbound reads.
3. Decoupled from payload logic and binary protocol definitions.
*/

#include <asio.hpp>
#include <vector>
#include <string>
#include <cstdint>
#include <span>
#include <array>

struct Device {
    std::string ip;
    std::vector<uint16_t> ports;
};

/// @brief Sets up connection along with handling sending and receiving payloads via UDP ONLY
class NetworkEngine {
    std::vector<Device> devices;

    asio::io_context io;
    asio::ip::udp::socket udp_socket;

    // Direct pre-resolved endpoint targets: [device_idx][port_idx]
    std::vector<std::vector<asio::ip::udp::endpoint>> udp_endpoints;

    // Inbound telemetry/input buffer
    std::array<uint8_t, 512> recv_buf;
    asio::ip::udp::endpoint remote_sender_ep;

public:
    NetworkEngine(
        const std::vector<Device>& devices, 
        uint16_t host_listen_port = 8090
    );

    // Ultra-lean, zero-allocation UDP send
    inline void send(size_t device_idx, size_t port_idx, std::span<const uint8_t> payload) {
        udp_socket.async_send_to(
            asio::buffer(payload.data(), payload.size()),
            udp_endpoints[device_idx][port_idx],
            [](std::error_code, size_t) {}
        );
    }

    // Non-blocking poll receive: Returns read byte count (0 if no packet waiting)
    size_t receive(std::span<uint8_t> out_buffer);

    // Drive non-blocking network socket execution
    inline void poll() {
        io.poll();
    }

    const std::vector<Device>& get_devices() const { return devices; }
};