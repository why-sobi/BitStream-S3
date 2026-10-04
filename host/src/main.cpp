#include "host.hpp"
#include <thread>
#include <chrono>
#include <iostream>
#include <string>
#include <vector>

#include <windows.h>
#include <timeapi.h>
#include <immintrin.h> // For _mm_pause()

#pragma comment(lib, "winmm.lib")

void print_usage(const char* prog_name) {
    std::cout << "Usage: " << prog_name << " [options]\n"
              << "Options:\n"
              << "  -i, --ip <string>      Target IP address (default: 127.0.0.1)\n"
              << "  -w, --width <int>      Target width (default: 800)\n"
              << "  -h, --height <int>     Target height (default: 480)\n"
              << "  -q, --quality <int>    JPEG quality 1-100 (default: 70)\n"
              << "  -f, --fps <int>        Target FPS cap (default: 60)\n"
              << "  --help                 Show this help message\n";
}

int main(int argc, char* argv[]) {
    // 1. Default CLI Configuration Parameters
    std::string ip = "127.0.0.1";
    int width = 800;
    int height = 480;
    int quality = 70; // Recommended Q70 for stable ~13 Mbps Wi-Fi stream
    int target_fps = 60;

    // 2. Simple Command-Line Argument Parser
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if ((arg == "-i" || arg == "--ip") && i + 1 < argc) {
            ip = argv[++i];
        } else if ((arg == "-w" || arg == "--width") && i + 1 < argc) {
            width = std::stoi(argv[++i]);
        } else if ((arg == "-h" || arg == "--height") && i + 1 < argc) {
            height = std::stoi(argv[++i]);
        } else if ((arg == "-q" || arg == "--quality") && i + 1 < argc) {
            quality = std::stoi(argv[++i]);
        } else if ((arg == "-f" || arg == "--fps") && i + 1 < argc) {
            target_fps = std::stoi(argv[++i]);
        } else if (arg == "--help") {
            print_usage(argv[0]);
            return 0;
        }
    }

    std::cout << "========================================\n"
              << " BitStreamHost Configuration\n"
              << " - Target IP Address : " << ip << "\n"
              << " - Resolution        : " << width << "x" << height << "\n"
              << " - Target FPS        : " << target_fps << "\n"
              << " - JPEG Quality      : " << quality << "\n"
              << "========================================\n";

    // Request 1ms scheduler granularity from Windows OS
    timeBeginPeriod(1);

    const auto frame_duration = std::chrono::microseconds(1000000 / target_fps);

    std::vector<Device> targets = {
        {
            .ip = ip,
            .ports = { ESP1_VID_PORT_CORE0 }
        }
    };

    BitStreamHost host(
        width, height,
        targets,
        quality,
        HOST_INPUT_PORT
    );

    std::cout << "[BitStreamHost] Starting streaming loop...\n";

    auto start_time = std::chrono::high_resolution_clock::now();
    int frame_count = 0;

    while (true) {
        auto frame_start = std::chrono::high_resolution_clock::now();

        host.step();
        frame_count++;

        auto now = std::chrono::high_resolution_clock::now();
        double elapsed_sec = std::chrono::duration<double>(now - start_time).count();

        // Print streaming stats once per second
        if (elapsed_sec >= 1.0) {
            double actual_fps = frame_count / elapsed_sec;
            double mbps = (static_cast<double>(host.get_last_frame_bytes() * 8) * actual_fps) / 1'000'000.0;

            std::cout << "[Host Stats] Actual FPS: " << actual_fps 
                      << " | Frame Size: " << host.get_last_frame_bytes() << " bytes"
                      << " | Bitrate: " << mbps << " Mbps\n";

            frame_count = 0;
            start_time = std::chrono::high_resolution_clock::now();
        }

        // 3. High-Precision Frame Pacing Engine (Sleep + Spin-Wait)
        auto frame_end = std::chrono::high_resolution_clock::now();
        auto target_time = frame_start + frame_duration;

        if (target_time > frame_end) {
            auto remaining = std::chrono::duration_cast<std::chrono::microseconds>(target_time - frame_end);

            // Coarse Sleep: Yield CPU if remaining time is greater than 1.5ms
            if (remaining > std::chrono::microseconds(1500)) {
                std::this_thread::sleep_for(remaining - std::chrono::microseconds(1500));
            }

            // Fine Spin-Wait: Lock to exact microsecond frame boundary
            while (std::chrono::high_resolution_clock::now() < target_time) {
                #if defined(_M_X64) || defined(_M_IX86)
                _mm_pause(); // Yield pipeline execution unit inside spinlock
                #endif
            }
        }
    }

    timeEndPeriod(1);
    return 0;
}