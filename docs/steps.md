That is a crystal-clear, step-by-step roadmap to build the system from the ground up without getting bogged down.

Here is the breakdown of your project execution plan, starting with the removal of `std::mdspan`:

---

### **Step 1: Strip Out `std::mdspan**`

`std::mdspan` requires C++23, compiler flag hassle, and non-standard library overhead.

* **Where it lives:** `host.hpp` and `host.cpp` currently construct `this->view = std::mdspan(...)` over the screen capture buffer.


* **The Replacement:** Replace `std::mdspan` entirely with your zero-overhead `ImageView2D<const uint8_t>` from `view.hpp`.


* **Why this is clean:** `ImageView2D` runs identically on both host C++ and ESP32 GCC without any C++23 dependencies.



---

### **Step 2: Simple Block Quantization ($4 \times 4$ Grid)**

Once `mdspan` is gone, we divide the downsampled frame buffer into $4 \times 4$ blocks using `make_subview()`:

1. **Calculate Block Count:** For a $320 \times 240$ frame, there are $80 \times 60 = 4,800$ total $4 \times 4$ blocks.
2. **Min-Max Color Endpoints:** For each $4 \times 4$ block (16 pixels), find the lowest color $C_0$ and highest color $C_1$.
3. **2-Bit Index Packing:** Interpolate 4 color steps between $C_0$ and $C_1$. Each pixel picks index `0, 1, 2, or 3` (2 bits per pixel = 4 bytes total for 16 pixels).
4. **Wire Struct Layout:** Store $(X, Y)$ coords + $C_0, C_1$ + 4 packed index bytes.

---

### **Step 3: Dirty Block Delta Encoding**

Instead of sending all 4,800 blocks every frame (which chokes Wi-Fi), filter static blocks:

1. **Maintain a Reference Frame:** Keep a `prev_low_res` buffer on the host.
2. **Block Variance/Diff Test:** Compare the current block's RGB values against `prev_low_res`.
3. **Dirty Thresholding:** If the sum of squared pixel differences is below a threshold, **do not write the block to the UDP payload**.


4. **Bandwidth Savings:** For typical games or desktop usage, this drops sent blocks by $60\% \text{--} 80\%$ on static/semi-static scenes.



---

### **Step 4: Global Motion Vector Shift (Camera Panning)**

When playing a game or watching video, panning the camera changes every pixel on screen, which tricks simple dirty-block detection into re-transmitting the whole screen.

1. **Phase Correlation / Anchor Vectors:** Sample 4--8 small anchor patches from the previous frame and search the new frame to find the average $(dX, dY)$ camera displacement vector.


2. **Shift the Reference Frame:** Translate `prev_low_res` by $(dX, dY)$.
3. **Send Vector + Novel Blocks Only:** Transmit a lightweight 2-byte Motion Header $(dX, dY)$ to the ESP32 first. The ESP32 shifts its display buffer in PSRAM, and then the host *only* sends the newly exposed dirty blocks along the edges.



---

### **Step 5: ESP32 Firmware & Decoding**

The ESP32 firmware remains "thin and dumb":

1. **Dual Core Pipeline:**
* **Core 0:** Handles non-blocking UDP socket reception and packet reassembly.


* **Core 1:** Runs fast bitwise unpacking routines to decode $C_0, C_1$ and 2-bit color steps into an RGB565 framebuffer in PSRAM.




2. **Direct Memory Access (DMA):** Asynchronously flushes the updated framebuffer out to the LCD screen via SPI or i80 parallel bus with minimal CPU intervention.



---

### Ready for Step 1?

When you are ready, we can start by removing `std::mdspan` from `host.hpp` and `host.cpp` and swapping in `ImageView2D`.