// shared/include/view.hpp
#pragma once

#include <cstddef>

#include <cstddef>
#include <utility>

/// @brief Non-owning 2D view over contiguous or strided image buffers.
/// Compatible with C++17/20, runs on both PC and ESP32 with zero overhead.
template <typename T>
struct ImageView2D
{
    T *data = nullptr;
    size_t width = 0;
    size_t height = 0;
    size_t stride_elements = 0; // Stride in elements (not bytes)
    using type = T;             // to save the type (might need later)

    // Default constructor
    constexpr ImageView2D() noexcept = default;

    // Construct view over a tightly packed buffer
    constexpr ImageView2D(T *ptr, size_t w, size_t h) noexcept
        : data(ptr), width(w), height(h), stride_elements(w) {}

    // Construct view over a strided buffer
    constexpr ImageView2D(T *ptr, size_t w, size_t h, size_t stride) noexcept
        : data(ptr), width(w), height(h), stride_elements(stride) {}

    // ------------------------------------------------------------------
    // Copy Operations (Views copy cheaply by copying pointers/metadata)
    // ------------------------------------------------------------------
    constexpr ImageView2D(const ImageView2D &) noexcept = default;
    constexpr ImageView2D &operator=(const ImageView2D &) noexcept = default;

    // ------------------------------------------------------------------
    // Move Operations
    // Note: Moving a non-owning view performs a copy of the pointer/sizes
    // and resets the source view to a null state.
    // ------------------------------------------------------------------
    constexpr ImageView2D(ImageView2D &&other) noexcept
        : data(std::exchange(other.data, nullptr)),
          width(std::exchange(other.width, 0)),
          height(std::exchange(other.height, 0)),
          stride_elements(std::exchange(other.stride_elements, 0)) {}

    constexpr ImageView2D &operator=(ImageView2D &&other) noexcept
    {
        if (this != &other)
        {
            data = std::exchange(other.data, nullptr);
            width = std::exchange(other.width, 0);
            height = std::exchange(other.height, 0);
            stride_elements = std::exchange(other.stride_elements, 0);
        }
        return *this;
    }

    // ------------------------------------------------------------------
    // Accessors
    // ------------------------------------------------------------------
    // Read-only 2D access for const instances or const T
    constexpr const T &operator()(size_t y, size_t x) const noexcept
    {
        return data[y * stride_elements + x];
    }

    // Mutable 2D access for non-const instances
    constexpr T &operator()(size_t y, size_t x) noexcept
    {
        return data[y * stride_elements + x];
    }

    constexpr size_t size() const noexcept { return width * height; }
    constexpr bool empty() const noexcept { return data == nullptr || size() == 0; }
};

/// @brief Slice a sub-region (block) out of an existing ImageView2D.
template <typename T>
constexpr ImageView2D<T> make_subview(
    const ImageView2D<T> &parent,
    size_t start_x, size_t start_y,
    size_t block_w, size_t block_h)
{
    T *sub_ptr = &parent(start_y, start_x);
    return ImageView2D<T>(sub_ptr, block_w, block_h, parent.stride_elements);
}