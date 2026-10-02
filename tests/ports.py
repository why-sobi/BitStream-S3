import socket
import struct
import io
import time
import pygame
from PIL import Image

UDP_IP = "127.0.0.1"
UDP_PORT = 8080
HEADER_FORMAT = "<BBHBBH"
HEADER_SIZE = struct.calcsize(HEADER_FORMAT)
PROTOCOL_MAGIC = 0xB3

def main():
    pygame.init()
    screen = pygame.display.set_mode((800, 480))
    pygame.display.set_caption("BitStream MJPEG Receiver")
    
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.bind((UDP_IP, UDP_PORT))
    sock.setblocking(False)

    current_seq = -1
    chunks = {}
    
    # Stats tracking
    frames_rendered = 0
    total_bytes_received = 0
    dropped_frames = 0
    last_seq = -1
    stat_start_time = time.time()

    running = True
    while running:
        for event in pygame.event.get():
            if event.type == pygame.QUIT:
                running = False

        while True:
            try:
                data, _ = sock.recvfrom(2048)
            except BlockingIOError:
                break

            if len(data) < HEADER_SIZE:
                continue

            total_bytes_received += len(data)
            magic, msg_type, seq_id, chunk_idx, total_chunks, payload_len = struct.unpack(
                HEADER_FORMAT, data[:HEADER_SIZE]
            )

            if magic != PROTOCOL_MAGIC:
                continue

            payload = data[HEADER_SIZE : HEADER_SIZE + payload_len]

            # Sequence tracking for dropped frames
            if seq_id != current_seq:
                if last_seq != -1 and seq_id > (last_seq + 1):
                    dropped_frames += (seq_id - last_seq - 1)
                last_seq = seq_id
                current_seq = seq_id
                chunks.clear()

            chunks[chunk_idx] = payload

            # Frame Complete!
            if len(chunks) == total_chunks:
                try:
                    jpeg_bytes = b"".join(chunks[i] for i in range(total_chunks))
                    img = Image.open(io.BytesIO(jpeg_bytes))
                    pygame_surface = pygame.image.fromstring(
                        img.tobytes(), img.size, img.mode
                    )

                    screen.blit(pygame_surface, (0, 0))
                    pygame.display.flip()
                    frames_rendered += 1
                except Exception:
                    pass

                chunks.clear()

        # Update HUD stats every second
        now = time.time()
        elapsed = now - stat_start_time
        if elapsed >= 1.0:
            rx_fps = frames_rendered / elapsed
            mbps = (total_bytes_received * 8) / (elapsed * 1_000_000)
            
            title = f"BitStream Receiver | FPS: {rx_fps:.1f} | Throughput: {mbps:.2f} Mbps | Dropped Frames: {dropped_frames}"
            pygame.display.set_caption(title)
            print(f"[Rx Stats] Rendered FPS: {rx_fps:.1f} | Network Bitrate: {mbps:.2f} Mbps | Dropped: {dropped_frames}")

            # Reset
            frames_rendered = 0
            total_bytes_received = 0
            dropped_frames = 0
            stat_start_time = time.time()

        time.sleep(0.001)  # Micro-sleep to prevent 100% CPU core pinning

    pygame.quit()
    sock.close()

if __name__ == "__main__":
    main()