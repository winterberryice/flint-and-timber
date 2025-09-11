#pragma once

#include <array>
#include <cstddef>

// This file is a placeholder for the texture atlas.
// The original `atlas.png` was not found in the file system.
// This generated data creates a 16x16 purple and black checkerboard pattern
// to allow the rendering pipeline development to proceed.

namespace flint
{
    namespace graphics
    {
        constexpr unsigned int ATLAS_PNG_WIDTH = 16;
        constexpr unsigned int ATLAS_PNG_HEIGHT = 16;
        constexpr unsigned int ATLAS_PNG_CHANNELS = 4;
        constexpr size_t ATLAS_PNG_SIZE = ATLAS_PNG_WIDTH * ATLAS_PNG_HEIGHT * ATLAS_PNG_CHANNELS;

        constexpr std::array<unsigned char, ATLAS_PNG_SIZE> generate_placeholder_data() {
            std::array<unsigned char, ATLAS_PNG_SIZE> data{};
            for (unsigned int y = 0; y < ATLAS_PNG_HEIGHT; ++y) {
                for (unsigned int x = 0; x < ATLAS_PNG_WIDTH; ++x) {
                    size_t index = (y * ATLAS_PNG_WIDTH + x) * ATLAS_PNG_CHANNELS;
                    // Create a checkerboard pattern (purple and black)
                    if ((x / 8 + y / 8) % 2 == 0) {
                        data[index + 0] = 0x5d; // R
                        data[index + 1] = 0x00; // G
                        data[index + 2] = 0x96; // B
                        data[index + 3] = 0xff; // A
                    } else {
                        data[index + 0] = 0x00; // R
                        data[index + 1] = 0x00; // G
                        data[index + 2] = 0x00; // B
                        data[index + 3] = 0xff; // A
                    }
                }
            }
            return data;
        }

        // Use similar variable names as xxd would have generated
        constexpr auto atlas_png = generate_placeholder_data();
        constexpr unsigned int atlas_png_len = ATLAS_PNG_SIZE;

    } // namespace graphics
} // namespace flint
