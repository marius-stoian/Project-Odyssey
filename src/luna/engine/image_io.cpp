#include "luna/engine/image_io.h"

#include <fstream>
#include <iterator>
#include <vector>

// stb is a single-file library: its code is compiled here, once (ADR-018). Its own warnings
// are not ours, so they are switched off for these two headers only.
#pragma warning(push, 0)
#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_PNG
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image.h>
#include <stb_image_write.h>
#pragma warning(pop)

namespace luna::engine {

std::optional<Image> loadPng(const std::filesystem::path& file, std::string& error) {
    // Read the bytes ourselves, so paths with any characters work (stb takes narrow names).
    std::ifstream in(file, std::ios::binary);
    if (!in) {
        error = "cannot be opened (missing?)";
        return std::nullopt;
    }
    const std::vector<unsigned char> bytes((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    int width = 0;
    int height = 0;
    int channels = 0;
    unsigned char* pixels = stbi_load_from_memory(bytes.data(), static_cast<int>(bytes.size()), &width, &height, &channels, 4);
    if (pixels == nullptr) {
        error = std::string("is not a readable PNG (") + stbi_failure_reason() + ")";
        return std::nullopt;
    }
    Image image(width, height);
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            const unsigned char* p = pixels + (static_cast<std::size_t>(y) * static_cast<std::size_t>(width) + static_cast<std::size_t>(x)) * 4;
            image.set(x, y, Color{p[0], p[1], p[2], p[3]});
        }
    }
    stbi_image_free(pixels);
    return image;
}

bool savePng(const Image& image, const std::filesystem::path& file) {
    int length = 0;
    unsigned char* png = stbi_write_png_to_mem(image.data(), image.width() * 4, image.width(), image.height(), 4, &length);
    if (png == nullptr) {
        return false;
    }
    std::ofstream out(file, std::ios::binary | std::ios::trunc);
    out.write(reinterpret_cast<const char*>(png), length);
    STBIW_FREE(png);
    return static_cast<bool>(out);
}

} // namespace luna::engine
