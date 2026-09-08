# This file tests whether the network from the host side is working correctly by listening to the ports of the HOST device itself set to the ESP's port
import socket
import threading

def esp32_core_listener(port, core_name):
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.bind(("127.0.0.1", port))
    print(f"[{core_name}] Listening on UDP port {port}...")
    
    packet_count = 0
    total_bytes = 0

    while True:
        data, addr = sock.recvfrom(2048)
        packet_count += 1
        total_bytes += len(data)
        
        # Check for Magic Byte 0xB3
        magic = hex(data[0]) if len(data) > 0 else "0x0"
        
        if packet_count % 60 == 0: # Print status roughly every second at 60fps
            print(f"[{core_name}] Recv {packet_count} packets | Total Bytes: {total_bytes} | Last Magic: {magic}")

# Start Core 0 (Port 8080) and Core 1 (Port 8081) on background threads
threading.Thread(target=esp32_core_listener, args=(8080, "ESP32 Core 0"), daemon=True).start()
threading.Thread(target=esp32_core_listener, args=(8081, "ESP32 Core 1"), daemon=True).start()

input("Press Enter to stop mock server...\n")