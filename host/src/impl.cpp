// host/src/impl.cpp
// #define BITSTREAM_BUILD_HOST
#define STB_IMAGE_WRITE_IMPLEMENTATION
#define STB_IMAGE_RESIZE_IMPLEMENTATION
#define PIXEL_IMPLEMENTATION

// Include headers AFTER defining the macros
#include "stb_image_resize2.h"
#include "shared/include/pixels.hpp"
#include "shared/include/jpeg.hpp"