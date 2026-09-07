#include <iostream>
#include <vector>
#include <mdspan>
#include <tuple>


#include "host.hpp"

int main() {
    BitStreamHost host(800, 480); // Initialize the FrameCapture with desired resized dimensions (800x480)
    host.tick(); // Capture and resize the frame

    asio::io_context io;
    std::cout << "ASIO INIT SUCCESS!\n";

    return 0;
}