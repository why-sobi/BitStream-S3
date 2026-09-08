# BitStreamer-S3

A custom, ultra-low-latency "zero-decode" raw pixel streaming pipeline designed for ESP32-S3 hardware.

## Project Overview

BitStreamer-S3 was an experimental hardware streaming architecture built to bypass the lack of onboard video decoding silicon on low-cost microcontrollers. Instead of relying on traditional compressed video streams (like H.264 or HEVC) that require heavy CPU or hardware decoding pipelines, this project aimed to stream raw RGB565 pixel data directly from a Windows host PC over UDP straight to an ESP32-S3-DevKitC-1-N16R8 display terminal.

### Key Technical Features

* **Host-Side Capture Engine:** A C++23 capture utility (`BitStreamHost`) utilizing Windows DXGI Desktop Duplication for low-overhead screen grabbing.
* **Custom UDP Framing Protocol:** A packet structure designed for multi-port chunking to bypass single-socket buffer limits.
* **External PSRAM Utilization:** Leveraged the 8MB PSRAM on the ESP32-S3 to handle incoming network buffers without exhausting internal SRAM.
* **Downsampling & Pushing:** Real-time frame reduction to fit target display constraints (400x240 @ 30 FPS).

---

## Where and Why It Was Abandoned

Development was officially suspended at the **host-side pipeline validation and network feasibility stage**, prior to writing the final firmware display loop.

The project hit an insurmountable physical and architectural wall dictated by hardware constraints:

1. **The 2.4GHz Bandwidth Ceiling:** Empirical testing and mathematical modeling revealed a hard physical throughput ceiling of roughly 25–35 Mbps on the ESP32-S3's 2.4GHz Wi-Fi radio.
2. **The Raw Pixel Math:** Streaming raw RGB565 data at 400x240 resolution at 30 FPS demands a stable data rate that drastically exceeds what standard 2.4GHz embedded Wi-Fi can reliably sustain without catastrophic packet loss and latency spikes.
3. **The Missing Silicon Wish:** If an "ESP32-S3 kind of thing" existed with native 5GHz Wi-Fi support combined with its symmetric dual-core layout, the bandwidth constraint would have evaporated. Instead, chipmakers split the roadmap—leaving high-throughput 5GHz radios stranded on single-core setups (like the ESP32-C5's single RISC-V core) or pairing dual cores with hardware decoders while omitting clean, direct wireless paths.
4. **The Multi-C5 Mad Science Trap:** Entertaining the thought of hacking together multiple ESP32-C5 chips—using one strictly as a 5GHz network coprocessor bridged via SPI/UART to another—quickly turns into an over-engineered nightmare. You end up trading a network bottleneck for an internal inter-chip bus bottleneck, all while dealing with a single-core main processor that lacks the dual-core muscle of the S3.
5. **The Architectural Trap:** While dirty rectangle tracking could theoretically lower bandwidth requirements, implementing differential region tracking on an MCU introduced severe CPU overhead and complexity, defeating the purpose of a lightweight streaming terminal.

### Conclusion

Without a single monolithic microcontroller uniting a high-performance dual-core setup, a hardware decompression engine, and clean 5GHz Wi-Fi bandwidth under a sub-$10 price tag, pushing uncompressed pixel data over the air remains mathematically unviable on current-generation hobbyist silicon. The project repository stands as a monument to pushing the ESP32-S3 past its intended design boundaries.