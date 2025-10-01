#include "chunk.hpp"
#include <iostream>
#include <vector>

namespace flint
{

    enum class Face
    {
        Front,
        Back,
        Top,
        Bottom,
        Right,
        Left
    };

    // Helper function to get texture atlas indices for a block type and face
    glm::vec2 get_texture_atlas_indices(BlockType blockType, Face face)
    {
        switch (blockType)
        {
        case BlockType::Dirt:
            return {2.0f, 0.0f};
        case BlockType::Grass:
            switch (face)
            {
            case Face::Top:
                return {0.0f, 0.0f};
            case Face::Bottom:
                return {2.0f, 0.0f};
            default:
                return {1.0f, 0.0f};
            }
        case BlockType::Bedrock:
            return {3.0f, 0.0f};
        case BlockType::OakLog:
            switch (face)
            {
            case Face::Top:
            case Face::Bottom:
                return {5.0f, 0.0f};
            default:
                return {4.0f, 0.0f};
            }
        case BlockType::OakLeaves:
            return {6.0f, 0.0f};
        default:
            return {15.0f, 15.0f}; // Error texture
        }
    }

    Chunk::Chunk()
    {
    }

    Chunk::~Chunk()
    {
        cleanup();
    }

    bool Chunk::initialize(WGPUDevice device)
    {
        m_device = device;

        generateChunkData();
        generateMesh();

        if (m_vertices.empty() || m_indices.empty())
        {
            return true; // No mesh to create
        }

        // Create vertex buffer
        WGPUBufferDescriptor vertexBufferDesc = {};
        vertexBufferDesc.size = m_vertices.size() * sizeof(graphics::Vertex);
        vertexBufferDesc.usage = WGPUBufferUsage_Vertex | WGPUBufferUsage_CopyDst;
        vertexBufferDesc.mappedAtCreation = false;

        m_vertexBuffer = wgpuDeviceCreateBuffer(device, &vertexBufferDesc);
        if (!m_vertexBuffer)
        {
            std::cerr << "Failed to create vertex buffer" << std::endl;
            return false;
        }

        wgpuQueueWriteBuffer(wgpuDeviceGetQueue(device), m_vertexBuffer, 0,
                             m_vertices.data(), vertexBufferDesc.size);

        // Create index buffer
        WGPUBufferDescriptor indexBufferDesc = {};
        indexBufferDesc.size = m_indices.size() * sizeof(uint16_t);
        indexBufferDesc.usage = WGPUBufferUsage_Index | WGPUBufferUsage_CopyDst;
        indexBufferDesc.mappedAtCreation = false;

        m_indexBuffer = wgpuDeviceCreateBuffer(device, &indexBufferDesc);
        if (!m_indexBuffer)
        {
            std::cerr << "Failed to create index buffer" << std::endl;
            return false;
        }

        wgpuQueueWriteBuffer(wgpuDeviceGetQueue(device), m_indexBuffer, 0,
                             m_indices.data(), indexBufferDesc.size);

        return true;
    }

    void Chunk::cleanup()
    {
        if (m_vertexBuffer)
        {
            wgpuBufferRelease(m_vertexBuffer);
            m_vertexBuffer = nullptr;
        }
        if (m_indexBuffer)
        {
            wgpuBufferRelease(m_indexBuffer);
            m_indexBuffer = nullptr;
        }
    }

    void Chunk::render(WGPURenderPassEncoder renderPass) const
    {
        if (!m_vertexBuffer || !m_indexBuffer)
            return;

        wgpuRenderPassEncoderSetVertexBuffer(renderPass, 0, m_vertexBuffer, 0, WGPU_WHOLE_SIZE);
        wgpuRenderPassEncoderSetIndexBuffer(renderPass, m_indexBuffer, WGPUIndexFormat_Uint16, 0, WGPU_WHOLE_SIZE);
        wgpuRenderPassEncoderDrawIndexed(renderPass, static_cast<uint32_t>(m_indices.size()), 1, 0, 0, 0);
    }

    bool Chunk::is_solid(int x, int y, int z) const
    {
        if (x < 0 || x >= CHUNK_WIDTH || y < 0 || y >= CHUNK_HEIGHT || z < 0 || z >= CHUNK_DEPTH)
        {
            return false;
        }
        BlockType type = m_blocks[x][y][z];
        return type != BlockType::Air && type != BlockType::OakLeaves;
    }

    void Chunk::generateChunkData()
    {
        for (int x = 0; x < CHUNK_WIDTH; ++x)
        {
            for (int z = 0; z < CHUNK_DEPTH; ++z)
            {
                m_blocks[x][0][z] = BlockType::Bedrock;
                for (int y = 1; y < CHUNK_HEIGHT / 2; ++y)
                {
                    m_blocks[x][y][z] = BlockType::Dirt;
                }
                m_blocks[x][CHUNK_HEIGHT / 2][z] = BlockType::Grass;
                for (int y = CHUNK_HEIGHT / 2 + 1; y < CHUNK_HEIGHT; ++y)
                {
                    m_blocks[x][y][z] = BlockType::Air;
                }
            }
        }

        // Add a tree
        int treeX = CHUNK_WIDTH / 2;
        int treeZ = CHUNK_DEPTH / 2;
        int trunkHeight = 5;
        for (int i = 1; i <= trunkHeight; ++i)
        {
            m_blocks[treeX][CHUNK_HEIGHT / 2 + i][treeZ] = BlockType::OakLog;
        }

        int leavesHeight = 3;
        int leavesRadius = 2;
        for (int y = CHUNK_HEIGHT / 2 + trunkHeight; y < CHUNK_HEIGHT / 2 + trunkHeight + leavesHeight; ++y)
        {
            for (int x = treeX - leavesRadius; x <= treeX + leavesRadius; ++x)
            {
                for (int z = treeZ - leavesRadius; z <= treeZ + leavesRadius; ++z)
                {
                    if (x >= 0 && x < CHUNK_WIDTH && z >= 0 && z < CHUNK_DEPTH)
                    {
                         if (m_blocks[x][y][z] == BlockType::Air) {
                            m_blocks[x][y][z] = BlockType::OakLeaves;
                         }
                    }
                }
            }
        }
    }

    void Chunk::generateMesh()
    {
        m_vertices.clear();
        m_indices.clear();

        const float ATLAS_COLS = 16.0f;
        const float ATLAS_ROWS = 1.0f;
        const float TEX_SIZE_X = 1.0f / ATLAS_COLS;
        const float TEX_SIZE_Y = 1.0f / ATLAS_ROWS;

        for (int x = 0; x < CHUNK_WIDTH; ++x)
        {
            for (int y = 0; y < CHUNK_HEIGHT; ++y)
            {
                for (int z = 0; z < CHUNK_DEPTH; ++z)
                {
                    BlockType blockType = m_blocks[x][y][z];
                    if (blockType == BlockType::Air)
                    {
                        continue;
                    }

                    glm::vec3 position = {(float)x, (float)y, (float)z};

                    auto addFace = [&](Face face, const std::vector<glm::vec3>& face_vertices) {
                        uint16_t baseIndex = m_vertices.size();

                        glm::vec2 atlas_indices = get_texture_atlas_indices(blockType, face);
                        float u_min = atlas_indices.x * TEX_SIZE_X;
                        float v_min = atlas_indices.y * TEX_SIZE_Y;
                        float u_max = u_min + TEX_SIZE_X;
                        float v_max = v_min + TEX_SIZE_Y;

                        std::vector<glm::vec2> uvs = {
                            {u_min, v_max}, {u_max, v_max}, {u_max, v_min}, {u_min, v_min}
                        };

                        for(size_t i = 0; i < 4; ++i) {
                            m_vertices.push_back({position + face_vertices[i], uvs[i]});
                        }

                        m_indices.push_back(baseIndex);
                        m_indices.push_back(baseIndex + 2);
                        m_indices.push_back(baseIndex + 1);
                        m_indices.push_back(baseIndex);
                        m_indices.push_back(baseIndex + 3);
                        m_indices.push_back(baseIndex + 2);
                    };

                    // Front face (+z)
                    if (z == CHUNK_DEPTH - 1 || !is_solid(x, y, z + 1))
                    {
                        addFace(Face::Front, {
                            {-0.5f, -0.5f, 0.5f}, {0.5f, -0.5f, 0.5f}, {0.5f, 0.5f, 0.5f}, {-0.5f, 0.5f, 0.5f}
                        });
                    }
                    // Back face (-z)
                    if (z == 0 || !is_solid(x, y, z - 1))
                    {
                        addFace(Face::Back, {
                            {-0.5f, -0.5f, -0.5f}, {-0.5f, 0.5f, -0.5f}, {0.5f, 0.5f, -0.5f}, {0.5f, -0.5f, -0.5f}
                        });
                    }
                    // Top face (+y)
                    if (y == CHUNK_HEIGHT - 1 || !is_solid(x, y + 1, z))
                    {
                        addFace(Face::Top, {
                           {-0.5f, 0.5f, -0.5f}, {-0.5f, 0.5f, 0.5f}, {0.5f, 0.5f, 0.5f}, {0.5f, 0.5f, -0.5f}
                        });
                    }
                    // Bottom face (-y)
                    if (y == 0 || !is_solid(x, y - 1, z))
                    {
                        addFace(Face::Bottom, {
                            {-0.5f, -0.5f, 0.5f}, {-0.5f, -0.5f, -0.5f}, {0.5f, -0.5f, -0.5f}, {0.5f, -0.5f, 0.5f}
                        });
                    }
                    // Right face (+x)
                    if (x == CHUNK_WIDTH - 1 || !is_solid(x + 1, y, z))
                    {
                        addFace(Face::Right, {
                            {0.5f, -0.5f, -0.5f}, {0.5f, 0.5f, -0.5f}, {0.5f, 0.5f, 0.5f}, {0.5f, -0.5f, 0.5f}
                        });
                    }
                    // Left face (-x)
                    if (x == 0 || !is_solid(x - 1, y, z))
                    {
                        addFace(Face::Left, {
                            {-0.5f, -0.5f, 0.5f}, {-0.5f, 0.5f, 0.5f}, {-0.5f, 0.5f, -0.5f}, {-0.5f, -0.5f, -0.5f}
                        });
                    }
                }
            }
        }
    }

} // namespace flint
