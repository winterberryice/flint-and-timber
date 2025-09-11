#include <iostream>
#include <vector>
#include <string>
#include <filesystem>
#include <fstream>
#include <iomanip>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

namespace fs = std::filesystem;

const int TILE_WIDTH = 16;
const int TILE_HEIGHT = 16;
const int ATLAS_COLS = 16;

bool write_header_file(const fs::path& header_path, const std::vector<unsigned char>& png_data, const std::string& var_name) {
    std::ofstream header_file(header_path);
    if (!header_file.is_open()) {
        std::cerr << "Failed to open header file for writing: " << header_path << std::endl;
        return false;
    }

    header_file << "#pragma once\n\n";
    header_file << "#include <cstddef>\n\n";
    header_file << "namespace flint {\n";
    header_file << "    namespace generated {\n";
    header_file << "        constexpr unsigned char " << var_name << "[] = {\n";

    for (size_t i = 0; i < png_data.size(); ++i) {
        if (i % 16 == 0) {
            header_file << "            ";
        }
        header_file << "0x" << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(png_data[i]);
        if (i < png_data.size() - 1) {
            header_file << ",";
        }
        if (i % 16 == 15) {
            header_file << "\n";
        }
    }

    header_file << "\n        };\n";
    header_file << "        constexpr size_t " << var_name << "_len = " << std::dec << png_data.size() << ";\n";
    header_file << "    } // namespace generated\n";
    header_file << "} // namespace flint\n";

    return true;
}


int main(int argc, char* argv[]) {
    if (argc != 4) {
        std::cerr << "Usage: " << argv[0] << " <asset_directory> <output_png_path> <output_header_path>" << std::endl;
        return 1;
    }

    fs::path asset_dir = argv[1];
    fs::path output_png_path = argv[2];
    fs::path output_header_path = argv[3];

    std::vector<std::string> texture_files = {
        "grass_block_top.png",
        "grass_block_side.png",
        "dirt.png",
        "bedrock.png",
        "oak_log.png",
        "oak_log_top.png",
        "oak_leaves.png"
    };

    int atlas_rows = (texture_files.size() + ATLAS_COLS - 1) / ATLAS_COLS;
    int atlas_width = ATLAS_COLS * TILE_WIDTH;
    int atlas_height = atlas_rows * TILE_HEIGHT;

    std::vector<unsigned char> atlas_data(atlas_width * atlas_height * 4, 0);

    for (size_t i = 0; i < texture_files.size(); ++i) {
        fs::path texture_path = asset_dir / texture_files[i];

        int img_width, img_height, img_channels;
        unsigned char* img_data = stbi_load(texture_path.c_str(), &img_width, &img_height, &img_channels, 4);

        if (!img_data) {
            std::cerr << "Failed to load texture: " << texture_path << std::endl;
            return 1;
        }

        if (img_width != TILE_WIDTH || img_height != TILE_HEIGHT) {
            std::cerr << "Texture " << texture_path << " has incorrect dimensions ("
                      << img_width << "x" << img_height << ")" << std::endl;
            stbi_image_free(img_data);
            return 1;
        }

        int row = i / ATLAS_COLS;
        int col = i % ATLAS_COLS;
        int dest_x = col * TILE_WIDTH;
        int dest_y = row * TILE_HEIGHT;

        for (int y = 0; y < TILE_HEIGHT; ++y) {
            for (int x = 0; x < TILE_WIDTH; ++x) {
                int src_idx = (y * TILE_WIDTH + x) * 4;
                int dest_idx = ((dest_y + y) * atlas_width + (dest_x + x)) * 4;
                atlas_data[dest_idx + 0] = img_data[src_idx + 0];
                atlas_data[dest_idx + 1] = img_data[src_idx + 1];
                atlas_data[dest_idx + 2] = img_data[src_idx + 2];
                atlas_data[dest_idx + 3] = img_data[src_idx + 3];
            }
        }

        stbi_image_free(img_data);
    }

    int png_data_len;
    unsigned char* png_data = stbi_write_png_to_mem(atlas_data.data(), atlas_width * 4, atlas_width, atlas_height, 4, &png_data_len);

    if (!png_data) {
        std::cerr << "Failed to encode atlas to PNG in memory" << std::endl;
        return 1;
    }

    // Write PNG to file
    std::ofstream png_file(output_png_path, std::ios::binary);
    if (!png_file.is_open()) {
        std::cerr << "Failed to open PNG file for writing: " << output_png_path << std::endl;
        free(png_data);
        return 1;
    }
    png_file.write(reinterpret_cast<const char*>(png_data), png_data_len);
    png_file.close();

    std::cout << "Successfully generated texture atlas: " << output_png_path << std::endl;

    // Write header file
    std::vector<unsigned char> png_vec(png_data, png_data + png_data_len);
    if (!write_header_file(output_header_path, png_vec, "atlas_png")) {
        free(png_data);
        return 1;
    }

    std::cout << "Successfully generated C++ header: " << output_header_path << std::endl;

    free(png_data);
    return 0;
}
