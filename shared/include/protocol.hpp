#pragma once
#include <cstdint>

// Protocol Identifiers
constexpr uint8_t PROTOCOL_MAGIC       = 0xB3; // 'B' + '3' (BitStream-S3)

// Host Configuration
constexpr uint16_t HOST_INPUT_PORT     = 8090; // Receives input/telemetry from clients

// ESP32 Node 1 Configuration (Video + Dual Core Execution)
constexpr uint16_t ESP1_VID_PORT_CORE0 = 8080; // Core 0: Processes Top Frame Half
constexpr uint16_t ESP1_VID_PORT_CORE1 = 8081; // Core 1: Processes Bottom Frame Half

// ESP32 Node 2 Configuration (Audio Node)
constexpr uint16_t ESP2_AUD_PORT       = 8083; // Core 0: I2S PCM Audio Buffer Pipeline
constexpr uint16_t ESP2_INPUT_PORT     = 8082; // Core 1: Sends touch/button state back to Host

#pragma pack(push, 1) // enforces no padded bytes (allow misaligned data structures)
// Fixed Header size = (1 + 1 + 2 + 1 + 1 + 2) = 8 bytes
struct PacketHeader {      
    uint8_t  magic;        // 0xBS (BitStream)
    uint8_t  msg_type;     // 0x01 = Video Chunk, 0x02 = Audio Chunk, 0x03 = Input
    uint16_t sequence_id;  // Frame counter to detect dropped frames
    uint8_t  chunk_index;  // Current slice index
    uint8_t  total_chunks; // Total slices for this frame
    uint16_t payload_len;  // Length of raw pixel data following this header
};
#pragma pack(pop) // reset the compiler to allow padding following this line

enum MsgType : uint8_t {
    MSG_VIDEO = 0x01,
    MSG_AUDIO = 0x02,
    MSG_INPUT = 0x03
};