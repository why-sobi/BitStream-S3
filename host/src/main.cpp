#include "host.hpp"
#include <thread>
#include <chrono>
#include <iostream>

int main() {
    std::vector<Device> targets = {
        {
            .ip = "127.0.0.1",
            .ports = { ESP1_VID_PORT_CORE0, ESP1_VID_PORT_CORE1 }
        }
    };

    BitStreamHost host(320, 240, targets, PIXEL_FORMAT_RGB565);

    while (true) {
        host.step(); // Capture -> Downsample -> Pack -> UDP Send -> Asio Poll
        std::this_thread::sleep_for(std::chrono::milliseconds(16)); // ~60 FPS
    }

    return 0;
}