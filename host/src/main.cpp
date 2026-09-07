#include <iostream>
#include <vector>
#include <mdspan>
#include <tuple>

#include "stb_image_resize2.h"
#include "canvas.hpp"
#include "pixels.hpp"


class BitStreamHost {
    DesktopCanvas canvas;
    std::vector<uint8_t> low_res; // down sampling buffer to the resized params
    std::vector<uint8_t> packed;  // this holds the payload to be sent over the network

    std::mdspan<const uint8_t, std::dextents<size_t, 2>> view; // this is only for read, no write will be performed to it (C++23 shorthand for 2 dynamic dimensions (Height, Width))
    size_t original_width, original_height;
    size_t resized_width, resized_height;
    PixelFormat format;

public:
    BitStreamHost(size_t resized_width, size_t resized_height, PixelFormat format = PIXEL_FORMAT_RGB565): resized_width(resized_width), resized_height(resized_height), format(format) {
        this->canvas.init(); // warmup 
        
        std::tie(original_width, original_height) = this->canvas.get_master_resolution();

        // Re-assign the view with updated dimensions
        this->view = std::mdspan(this->canvas.get_master_buffer_ptr(), this->original_height, this->original_width);
        this->low_res.resize(this->view.size());

        // Allocate the packed buffer
        size_t pixel_counts = this->low_res.size() / 3; // since we know each pixel takes about 3 bytes
        size_t size = get_packed_buffer_size(this->format, pixel_counts); // total resulting size after packing the pixels 

        packed.resize(size);
    }

    void tick() { // this captures and resizing populating the low_res buffer
        bool success = this->canvas.capture_frame();

        if (success) {
            // Resize the captured frame to the desired dimensions
            stbir_resize_uint8_linear(
                this->view.data_handle(),   // returns the underlying pointer to the data (similar to .data() for vectors and other STLs)
                this->original_width,       // original width of the captured frame
                this->original_height,      // original height of the captured frame
                0,                          // stride in bytes (0 means tightly packed)
                this->low_res.data(),       // pointer to the output buffer where the resized image will be stored
                this->resized_width,        // desired width of the resized frame
                this->resized_height,       // desired height of the resized frame
                0,                          // stride in bytes for the output buffer (0 means tightly packed)
                STBIR_RGBA                  // specifies the pixel layout (3 for RGB format since our buffer is in RGB format)
            );
        } 
        // else reuse old buffer (i.e. low_res) if capture fails, this is to avoid returning empty buffer to the user
    }

    void setup_payload() {
        pack_pixels(
            this->format,           // format to use
            this->low_res.data(),   // src RGB888
            this->packed.data(),    // dst [RGB888, RGB444, RGB565]
            this->packed.size()     // total pixel count
        );
    }
};

int main() {
    BitStreamHost host(800, 480); // Initialize the FrameCapture with desired resized dimensions (800x480)
    host.tick(); // Capture and resize the frame

    return 0;
}