#include <iostream>
#include <vector>
#include <string>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include "nlohmann/json.hpp"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

namespace fs = std::filesystem;
using json = nlohmann::json;

const int TILE_WIDTH = 16;
const int TILE_HEIGHT = 16;
const int ATLAS_COLS = 16;

struct PngWriteContext
{
    std::vector<unsigned char> buffer;
};

void png_write_callback(void *context, void *data, int size)
{
    PngWriteContext *ctx = static_cast<PngWriteContext *>(context);
    const unsigned char *bytes = static_cast<const unsigned char *>(data);
    ctx->buffer.insert(ctx->buffer.end(), bytes, bytes + size);
}

bool write_header_file(const fs::path &header_path, const std::vector<unsigned char> &png_data, const std::string &var_name)
{
    std::ofstream header_file(header_path);
    if (!header_file.is_open())
    {
        std::cerr << "Failed to open header file for writing: " << header_path << std::endl;
        return false;
    }

    header_file << "#pragma once\n\n";
    header_file << "#include <cstddef>\n\n";
    header_file << "namespace flint {\n";
    header_file << "    namespace generated {\n";
    header_file << "        constexpr unsigned char " << var_name << "[] = {\n";

    for (size_t i = 0; i < png_data.size(); ++i)
    {
        if (i % 16 == 0)
        {
            header_file << "            ";
        }
        header_file << "0x" << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(png_data[i]);
        if (i < png_data.size() - 1)
        {
            header_file << ",";
        }
        if (i % 16 == 15)
        {
            header_file << "\n";
        }
    }

    header_file << "\n        };\n";
    header_file << "        constexpr size_t " << var_name << "_len = " << std::dec << png_data.size() << ";\n";
    header_file << "    } // namespace generated\n";
    header_file << "} // namespace flint\n";

    return true;
}

int main(int argc, char *argv[])
{
    if (argc != 5)
    {
        std::cerr << "Usage: " << argv[0] << " <asset_directory> <atlas_json_path> <output_png_path> <output_header_path>" << std::endl;
        return 1;
    }

    fs::path asset_dir = argv[1];
    fs::path atlas_json_path = argv[2];
    fs::path output_png_path = argv[3];
    fs::path output_header_path = argv[4];

    std::ifstream f(atlas_json_path);
    if (!f.is_open())
    {
        std::cerr << "Failed to open atlas JSON file: " << atlas_json_path << std::endl;
        return 1;
    }
    json atlas_layout = json::parse(f);

    int atlas_rows = atlas_layout.size();
    int atlas_width = ATLAS_COLS * TILE_WIDTH;
    int atlas_height = atlas_rows * TILE_HEIGHT;

    std::vector<unsigned char> atlas_data(atlas_width * atlas_height * 4, 0);

    for (int r = 0; r < atlas_layout.size(); ++r)
    {
        const auto &row_layout = atlas_layout[r];
        for (int c = 0; c < row_layout.size(); ++c)
        {
            std::string texture_file = row_layout[c];
            fs::path texture_path = asset_dir / texture_file;

            int img_width, img_height, img_channels;
            unsigned char *img_data = stbi_load(texture_path.c_str(), &img_width, &img_height, &img_channels, 4);

            if (!img_data)
            {
                std::cerr << "Failed to load texture: " << texture_path << std::endl;
                return 1;
            }

            if (img_width != TILE_WIDTH || img_height != TILE_HEIGHT)
            {
                std::cerr << "Texture " << texture_path << " has incorrect dimensions ("
                          << img_width << "x" << img_height << ")" << std::endl;
                stbi_image_free(img_data);
                return 1;
            }

            int dest_x = c * TILE_WIDTH;
            int dest_y = r * TILE_HEIGHT;

    for (int y = 0; y < TILE_HEIGHT; ++y)
            {
                for (int x = 0; x < TILE_WIDTH; ++x)
                {
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
    }

    PngWriteContext png_ctx;
    stbi_write_png_to_func(png_write_callback, &png_ctx, atlas_width, atlas_height, 4, atlas_data.data(), atlas_width * 4);

    if (png_ctx.buffer.empty())
    {
        std::cerr << "Failed to encode atlas to PNG in memory" << std::endl;
        return 1;
    }

    // Write PNG to file
    std::ofstream png_file(output_png_path, std::ios::binary);

    if (!png_file.is_open())
    {
        std::cerr << "Failed to open PNG file for writing: " << output_png_path << std::endl;
        return 1;
    }
    png_file.write(reinterpret_cast<const char *>(png_ctx.buffer.data()), png_ctx.buffer.size());
    png_file.close();

    std::cout << "Successfully generated texture atlas: " << output_png_path << std::endl;

    // Write header file
    if (!write_header_file(output_header_path, png_ctx.buffer, "atlas_png"))
    {
        return 1;
    }

    std::cout << "Successfully generated C++ header: " << output_header_path << std::endl;

    return 0;
}
