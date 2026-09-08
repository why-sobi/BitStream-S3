#include "network.hpp"

NetworkEngine::NetworkEngine(
    const std::vector<Device>& devices, 
    uint16_t host_listen_port
): devices(devices),
   udp_socket(io, asio::ip::udp::endpoint(asio::ip::udp::v4(), host_listen_port)) 
{
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

size_t NetworkEngine::receive(std::span<uint8_t> out_buffer) {
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