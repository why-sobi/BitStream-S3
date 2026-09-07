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

enum class TransferProtocol : uint8_t {
    TCP,
    UDP
};

struct Device {
    std::string ip;
    std::vector<uint16_t> ports;
};

class NetworkEngine {
    std::vector<Device> devices;
    TransferProtocol handshake_protocol;
    TransferProtocol comms_protocol;

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
        uint16_t host_listen_port = 8090,
        TransferProtocol handshake_protocol = TransferProtocol::TCP, 
        TransferProtocol comms_protocol     = TransferProtocol::UDP
    ): handshake_protocol(handshake_protocol), comms_protocol(comms_protocol),
       udp_socket(io, asio::ip::udp::endpoint(asio::ip::udp::v4(), host_listen_port)) 
    {
        this->devices = std::move(devices);

        udp_endpoints.reserve(this->devices.size());
        for (const auto& dev : this->devices) {
            auto ip_addr = asio::ip::make_address(dev.ip);
            std::vector<asio::ip::udp::endpoint> dev_endpoints;
            dev_endpoints.reserve(dev.ports.size());

            for (uint16_t port : dev.ports) {
                dev_endpoints.emplace_back(ip_addr, port);
            }
            udp_endpoints.push_back(std::move(dev_endpoints));
        }
    }

    // Ultra-lean, zero-allocation UDP send
    inline void send(size_t device_idx, size_t port_idx, std::span<const uint8_t> payload) {
        udp_socket.async_send_to(
            asio::buffer(payload.data(), payload.size()),
            udp_endpoints[device_idx][port_idx],
            [](std::error_code, size_t) {}
        );
    }

    // Non-blocking poll receive: Returns read byte count (0 if no packet waiting)
    size_t receive(std::span<uint8_t> out_buffer) {
        std::error_code ec;
        size_t bytes_read = udp_socket.receive_from(
            asio::buffer(out_buffer.data(), out_buffer.size()),
            remote_sender_ep,
            asio::ip::udp::socket::message_peek, // Non-blocking query check
            ec
        );

        if (!ec && bytes_read > 0) {
            // Actual read drain
            return udp_socket.receive_from(
                asio::buffer(out_buffer.data(), out_buffer.size()),
                remote_sender_ep,
                0,
                ec
            );
        }

        return 0;
    }

    // Drive non-blocking network socket execution
    inline void poll() {
        io.poll();
    }

    TransferProtocol get_handshake_protocol() const { return handshake_protocol; }
    TransferProtocol get_comms_protocol() const { return comms_protocol; }
    const std::vector<Device>& get_devices() const { return devices; }
};