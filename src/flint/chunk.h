#pragma once

#include "block.h"
#include <cstddef> // For size_t

namespace flint
{

    // Global constants for chunk dimensions, equivalent to `pub const`.
    // Using `constexpr` makes them available at compile-time.
    constexpr size_t CHUNK_WIDTH = 16;
    constexpr size_t CHUNK_HEIGHT = 32;
    constexpr size_t CHUNK_DEPTH = 16;

    class Chunk
    {
    public:
        // Constructor, equivalent to `new()` and the `Default` trait implementation.
        Chunk();

        // Member function to generate the chunk's terrain.
        void generateTerrain();

        // Gets a read-only pointer to a block.
        // Returning a pointer (`const Block*`) is the C++ equivalent of Rust's `Option<&Block>`.
        // It will be `nullptr` if the coordinates are out of bounds.
        Block *getBlock(size_t x, size_t y, size_t z);

        const Block *getBlock(size_t x, size_t y, size_t z) const;

        void setBlockSkyLight(size_t x, size_t y, size_t z, uint8_t sky_light);

        // Sets a block at the given coordinates.
        // Returning a `bool` is a common C++ way to handle Rust's `Result<(), &str>`.
        // It returns `true` on success and `false` on failure (out of bounds).
        bool setBlock(size_t x, size_t y, size_t z, BlockType type);

        // Checks if a block at the given world coordinates is solid.
        // This is a new method for physics checks.
        bool is_solid(int x, int y, int z) const;

    private:
        // A 3D C-style array is much more efficient than a Vec<Vec<Vec<...>>>
        // for a fixed-size grid. It allocates all blocks in a single contiguous memory block.
        Block m_blocks[CHUNK_WIDTH][CHUNK_HEIGHT][CHUNK_DEPTH];
    };

} // namespace flint