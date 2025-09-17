#pragma once

#include <vector>
#include "glm/glm.hpp"
#include "block.hpp"
#include "constants.hpp"
#include "graphics/mesh.hpp"

namespace flint {

    class World; // Forward declaration

    class Chunk {
    public:
        explicit Chunk(glm::ivec2 coord);
        ~Chunk();

        void generate_terrain();
        void calculate_sky_light();

        Block get_block(int x, int y, int z) const;
        void set_block(int x, int y, int z, Block block);

        graphics::ChunkMeshData build_mesh(const World* world) const;

        glm::ivec2 get_coord() const { return coord; }

    private:
        glm::ivec2 coord;
        std::vector<Block> blocks;

        static inline int get_block_index(int x, int y, int z);
    };

}
