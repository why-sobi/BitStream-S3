**1. Capture (PC, already built)**
DXGI capture → flat 1D buffer → ~60Hz native rate.

**2. Downsample**
To target panel resolution + 30fps (adjustable via harness).

**3. Global motion compensation**
- 4-8 fixed anchor points, ±8px local search each vs previous frame
- Median (dx, dy) across anchors → single global motion vector
- Shift previous frame by that vector → motion-compensated reference

**4. Dirty-block delta**
Compare current frame against the motion-compensated reference (not raw previous frame) → only blocks with genuinely new content get marked dirty → static/panned-but-unchanged content skipped.

**5. Per-block classification (for dirty blocks only)**
- Compute variance (max-min per channel) per block
- Low-variance (smooth/background) → apply small Gaussian (3×3 or 5×5) before quantizing
- High-variance (edges/HUD/text) → skip Gaussian, quantize raw

**6. Block quantization**
- Block size: testable (4×4, 8×8, 16×16)
- Scheme: testable (flat Q4_0-style single scale, or hierarchical Q4_K-style superblock+sub-block)
- Endpoint color depth: testable (RGB332, RGB444, RGB565)
- Output per block: 2 endpoint colors + N-bit index per pixel

**7. Transmit**
UDP over MicroLink (Tailscale-compatible tunnel).

**8. Decode (ESP32-S3, dual chip)**
- Chip #1: apply global shift to reference, overlay decoded dirty blocks, core-split left/right half render+push to panel
- Chip #2: one core input (BLE gamepad via the custom controller), one core audio output

**9. Test harness (build first, before any ESP32-side work)**
CLI args: `--block-size --quant-scheme --endpoint-depth --gaussian --motion-comp --resolution --fps`
PC-to-same-PC loopback: encode → local socket → decode → output image + bandwidth number, per config, for comparison.

**10. Reinvestment decision (after harness gives real numbers)**
Whatever bandwidth motion-comp + delta actually saves, spend it on color depth first (RGB565 endpoints — biggest visible quality win per Mbps), then resolution, then fps last, unless the numbers say otherwise.