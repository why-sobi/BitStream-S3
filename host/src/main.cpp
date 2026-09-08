#include <iostream>
#include <vector>
#include <array>
#include <thread>
#include <chrono>

#include "network.hpp"
#include "protocol.hpp"

int main() {
    try {
        std::cout << "Starting NetworkEngine Pure UDP Test..." << std::endl;

        // 1. Target devices pointing to localhost loopback
        std::vector<Device> devices = {
            {
                .ip = "127.0.0.1",
                .ports = { ESP1_VID_PORT_CORE0, ESP1_VID_PORT_CORE1 } // 8080, 8081
            }
        };

        // 2. Instantiate NetworkEngine bound to host input port 8090
        NetworkEngine net(devices, HOST_INPUT_PORT);

        // 3. Create dummy frame buffer (e.g. 1200 bytes per half packet)
        constexpr size_t PAYLOAD_SIZE = 1200;
        
        std::array<uint8_t, PAYLOAD_SIZE> dummy_top_half;
        std::array<uint8_t, PAYLOAD_SIZE> dummy_bottom_half;

        // Fill with recognizable pattern & Magic Byte 0xB3
        dummy_top_half.fill(0xAA);
        dummy_top_half[0] = PROTOCOL_MAGIC; // 0xB3

        dummy_bottom_half.fill(0xBB);
        dummy_bottom_half[0] = PROTOCOL_MAGIC; // 0xB3

        std::cout << "Network engine online. Blasting dummy packets to 127.0.0.1:8080/8081..." << std::endl;

        uint64_t packet_burst_count = 0;

        while (true) {
            // Send dummy top half -> Device 0, Port Index 0 (8080)
            net.send(0, 0, dummy_top_half);

            // Send dummy bottom half -> Device 0, Port Index 1 (8081)
            net.send(0, 1, dummy_bottom_half);

            // Drain non-blocking network socket polling loop
            net.poll();

            packet_burst_count++;
            if (packet_burst_count % 60 == 0) {
                std::cout << "Sent " << packet_burst_count * 2 << " packets total across both ports." << std::endl;
            }

            // Simulate ~60 FPS dispatch rate
            std::this_thread::sleep_for(std::chrono::milliseconds(16));
        }

    } catch (const std::exception& e) {
        std::cerr << "\n[CRASH DETECTED]: " << e.what() << std::endl;
        return -1;
    }

    return 0;
}