#include <iostream>
#include <vector>
#include <mdspan>
#include <tuple>

#include "stb_image_resize2.h"
#include "canvas.hpp"


class FrameCapture {
    DesktopCanvas canvas;
    std::vector<uint8_t> down_sampled;
    std::mdspan<const uint8_t, std::dextents<size_t, 2>> view; // this is only for read, no write will be performed to it (C++23 shorthand for 2 dynamic dimensions (Height, Width))
    size_t original_width, original_height;
    size_t resized_width, resized_height;

public:
    FrameCapture(size_t resized_width, size_t resized_height): resized_width(resized_width), resized_height(resized_height) {
        this->canvas.init();
        
        std::tie(original_width, original_height) = this->canvas.get_master_resolution();

        // Re-assign the view with updated dimensions
        this->view = std::mdspan(this->canvas.get_master_buffer_ptr(), this->original_height, this->original_width);
        this->down_sampled.resize(this->view.size());
    }

    void capture() { // this captures and resizing populating the down_sampled buffer
        bool success = this->canvas.capture_frame();

        if (success) {
            // Resize the captured frame to the desired dimensions
            stbir_resize_uint8_linear(
                this->view.data_handle(),   // returns the underlying pointer to the data (similr to .data() for vectors and other STLs)
                this->original_width,       // original width of the captured frame
                this->original_height,      // original height of the captured frame
                0,                          // stride in bytes (0 means tightly packed)
                this->down_sampled.data(),  // pointer to the output buffer where the resized image will be stored
                this->resized_width,        // desired width of the resized frame
                this->resized_height,       // desired height of the resized frame
                0,                          // stride in bytes for the output buffer (0 means tightly packed)
                STBIR_RGBA                  // specifies the pixel layout (3 for RGB format since our buffer is in RGB format)
            );
        } 
        // else reuse old buffer (i.e. down_sampled) if capture fails, this is to avoid returning empty buffer to the user
    }
};

int main() {
    FrameCapture capture(800, 480); // Initialize the FrameCapture with desired resized dimensions (800x480)
    

    return 0;
}