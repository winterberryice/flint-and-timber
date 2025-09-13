#include "chunk.hpp"
#include <iostream>
#include <stdexcept>
#include <utility> // For std::move

namespace flint
{
    // A static "air block" to return for out-of-bounds non-mutable requests.
    static const Block S_AIR_BLOCK = Block(BlockType::Air);

    // Face data, inspired by cube_geometry.rs
    const glm::vec3 FACE_NORMALS[6] = {
        {0.0f, 0.0f, 1.0f},  // Front (+Z)
        {0.0f, 0.0f, -1.0f}, // Back (-Z)
        {0.0f, 1.0f, 0.0f},  // Top (+Y)
        {0.0f, -1.0f, 0.0f}, // Bottom (-Y)
        {1.0f, 0.0f, 0.0f},  // Right (+X)
        {-1.0f, 0.0f, 0.0f}  // Left (-X)
    };
    const glm::vec3 CUBE_FACE_VERTICES[6][4] = {
        {{-0.5f, -0.5f, 0.5f}, {0.5f, -0.5f, 0.5f}, {0.5f, 0.5f, 0.5f}, {-0.5f, 0.5f, 0.5f}},
        {{0.5f, -0.5f, -0.5f}, {-0.5f, -0.5f, -0.5f}, {-0.5f, 0.5f, -0.5f}, {0.5f, 0.5f, -0.5f}},
        {{-0.5f, 0.5f, 0.5f}, {0.5f, 0.5f, 0.5f}, {0.5f, 0.5f, -0.5f}, {-0.5f, 0.5f, -0.5f}},
        {{-0.5f, -0.5f, -0.5f}, {0.5f, -0.5f, -0.5f}, {0.5f, -0.5f, 0.5f}, {-0.5f, -0.5f, 0.5f}},
        {{0.5f, -0.5f, 0.5f}, {0.5f, -0.5f, -0.5f}, {0.5f, 0.5f, -0.5f}, {0.5f, 0.5f, 0.5f}},
        {{-0.5f, -0.5f, -0.5f}, {-0.5f, -0.5f, 0.5f}, {-0.5f, 0.5f, 0.5f}, {-0.5f, 0.5f, -0.5f}}};
    const glm::vec2 UV_COORDS[4] = {{0.0f, 1.0f}, {1.0f, 1.0f}, {1.0f, 0.0f}, {0.0f, 0.0f}};
    const uint16_t FACE_INDICES[6] = {0, 1, 2, 0, 2, 3};

    Chunk::Chunk(int chunk_x, int chunk_z) : m_chunk_x(chunk_x), m_chunk_z(chunk_z)
    {
    }

    Chunk::~Chunk()
    {
        cleanup();
    }

    Chunk::Chunk(Chunk&& other) noexcept
        : m_chunk_x(other.m_chunk_x), m_chunk_z(other.m_chunk_z),
          m_vertices(std::move(other.m_vertices)), m_indices(std::move(other.m_indices)),
          m_vertexBuffer(other.m_vertexBuffer), m_indexBuffer(other.m_indexBuffer),
          m_device(other.m_device)
    {
        // Nullify other's pointers to prevent double free
        other.m_vertexBuffer = nullptr;
        other.m_indexBuffer = nullptr;
        other.m_device = nullptr;
        // Copy blocks data
        std::copy(&other.m_blocks[0][0][0], &other.m_blocks[0][0][0] + CHUNK_WIDTH * CHUNK_HEIGHT * CHUNK_DEPTH, &m_blocks[0][0][0]);
    }

    Chunk& Chunk::operator=(Chunk&& other) noexcept
    {
        if (this != &other)
        {
            cleanup();
            m_chunk_x = other.m_chunk_x;
            m_chunk_z = other.m_chunk_z;
            m_vertices = std::move(other.m_vertices);
            m_indices = std::move(other.m_indices);
            m_vertexBuffer = other.m_vertexBuffer;
            m_indexBuffer = other.m_indexBuffer;
            m_device = other.m_device;

            other.m_vertexBuffer = nullptr;
            other.m_indexBuffer = nullptr;
            other.m_device = nullptr;
            std::copy(&other.m_blocks[0][0][0], &other.m_blocks[0][0][0] + CHUNK_WIDTH * CHUNK_HEIGHT * CHUNK_DEPTH, &m_blocks[0][0][0]);
        }
        return *this;
    }


    bool Chunk::initialize(WGPUDevice device)
    {
        m_device = device;
        generateChunkData();
        generateMesh();

        if (m_vertices.empty()) return true;

        WGPUBufferDescriptor vertexBufferDesc = {};
        vertexBufferDesc.size = m_vertices.size() * sizeof(graphics::Vertex);
        vertexBufferDesc.usage = WGPUBufferUsage_Vertex | WGPUBufferUsage_CopyDst;
        m_vertexBuffer = wgpuDeviceCreateBuffer(device, &vertexBufferDesc);
        if (!m_vertexBuffer) return false;
        wgpuQueueWriteBuffer(wgpuDeviceGetQueue(device), m_vertexBuffer, 0, m_vertices.data(), vertexBufferDesc.size);

        WGPUBufferDescriptor indexBufferDesc = {};
        indexBufferDesc.size = m_indices.size() * sizeof(uint16_t);
        indexBufferDesc.usage = WGPUBufferUsage_Index | WGPUBufferUsage_CopyDst;
        m_indexBuffer = wgpuDeviceCreateBuffer(device, &indexBufferDesc);
        if (!m_indexBuffer) {
            cleanup();
            return false;
        }
        wgpuQueueWriteBuffer(wgpuDeviceGetQueue(device), m_indexBuffer, 0, m_indices.data(), indexBufferDesc.size);

        return true;
    }

    void Chunk::cleanup()
    {
        if (m_vertexBuffer) wgpuBufferRelease(m_vertexBuffer);
        m_vertexBuffer = nullptr;
        if (m_indexBuffer) wgpuBufferRelease(m_indexBuffer);
        m_indexBuffer = nullptr;
    }

    void Chunk::render(WGPURenderPassEncoder renderPass) const
    {
        if (!m_vertexBuffer || !m_indexBuffer || m_indices.empty()) return;
        wgpuRenderPassEncoderSetVertexBuffer(renderPass, 0, m_vertexBuffer, 0, WGPU_WHOLE_SIZE);
        wgpuRenderPassEncoderSetIndexBuffer(renderPass, m_indexBuffer, WGPUIndexFormat_Uint16, 0, WGPU_WHOLE_SIZE);
        wgpuRenderPassEncoderDrawIndexed(renderPass, static_cast<uint32_t>(m_indices.size()), 1, 0, 0, 0);
    }

    const Block &Chunk::get_block(int x, int y, int z) const
    {
        if (x < 0 || x >= CHUNK_WIDTH || y < 0 || y >= CHUNK_HEIGHT || z < 0 || z >= CHUNK_DEPTH)
        {
            return S_AIR_BLOCK;
        }
        return m_blocks[x][y][z];
    }

    Block &Chunk::get_block_mut(int x, int y, int z)
    {
        if (x < 0 || x >= CHUNK_WIDTH || y < 0 || y >= CHUNK_HEIGHT || z < 0 || z >= CHUNK_DEPTH)
        {
            throw std::out_of_range("Block coordinates are out of chunk bounds");
        }
        return m_blocks[x][y][z];
    }

    void Chunk::set_block(int x, int y, int z, Block block)
    {
        if (x < 0 || x >= CHUNK_WIDTH || y < 0 || y >= CHUNK_HEIGHT || z < 0 || z >= CHUNK_DEPTH)
        {
            return;
        }
        m_blocks[x][y][z] = block;
    }

    void Chunk::regenerate_mesh()
    {
        cleanup(); // Release old buffers
        generateMesh();

        if (m_vertices.empty()) return;

        WGPUQueue queue = wgpuDeviceGetQueue(m_device);

        WGPUBufferDescriptor vertexBufferDesc = {};
        vertexBufferDesc.size = m_vertices.size() * sizeof(graphics::Vertex);
        vertexBufferDesc.usage = WGPUBufferUsage_Vertex | WGPUBufferUsage_CopyDst;
        m_vertexBuffer = wgpuDeviceCreateBuffer(m_device, &vertexBufferDesc);
        if (!m_vertexBuffer) return;
        wgpuQueueWriteBuffer(queue, m_vertexBuffer, 0, m_vertices.data(), vertexBufferDesc.size);

        WGPUBufferDescriptor indexBufferDesc = {};
        indexBufferDesc.size = m_indices.size() * sizeof(uint16_t);
        indexBufferDesc.usage = WGPUBufferUsage_Index | WGPUBufferUsage_CopyDst;
        m_indexBuffer = wgpuDeviceCreateBuffer(m_device, &indexBufferDesc);
        if (!m_indexBuffer) {
            cleanup();
            return;
        }
        wgpuQueueWriteBuffer(queue, m_indexBuffer, 0, m_indices.data(), indexBufferDesc.size);
    }

    void Chunk::generateChunkData()
    {
        for (int x = 0; x < CHUNK_WIDTH; ++x)
        {
            for (int z = 0; z < CHUNK_DEPTH; ++z)
            {
                for (int y = 0; y < CHUNK_HEIGHT; ++y)
                {
                    if (y < CHUNK_HEIGHT / 2 - 1)
                    {
                        m_blocks[x][y][z] = Block(BlockType::Dirt);
                    }
                    else if (y == CHUNK_HEIGHT / 2 - 1)
                    {
                        m_blocks[x][y][z] = Block(BlockType::Grass);
                    }
                    else
                    {
                        m_blocks[x][y][z] = Block(BlockType::Air);
                    }
                }
            }
        }
    }

    void Chunk::generateMesh()
    {
        m_vertices.clear();
        m_indices.clear();

        const float ATLAS_SIZE = 16.0f;

        for (int x = 0; x < CHUNK_WIDTH; ++x)
        {
            for (int y = 0; y < CHUNK_HEIGHT; ++y)
            {
                for (int z = 0; z < CHUNK_DEPTH; ++z)
                {
                    const Block &current_block = get_block(x, y, z);
                    if (!current_block.is_solid()) continue;

                    glm::vec3 position = {
                        (float)(m_chunk_x * CHUNK_WIDTH + x),
                        (float)y,
                        (float)(m_chunk_z * CHUNK_DEPTH + z)};

                    auto texture_indices = current_block.get_texture_atlas_indices();

                    const glm::ivec3 neighbors[6] = {
                        {x, y, z + 1}, {x, y, z - 1}, {x, y + 1, z},
                        {x, y - 1, z}, {x + 1, y, z}, {x - 1, y, z}};

                    for (int i = 0; i < 6; ++i)
                    {
                        const Block &neighbor_block = get_block(neighbors[i].x, neighbors[i].y, neighbors[i].z);

                        if (neighbor_block.is_transparent())
                        {
                            uint16_t baseIndex = static_cast<uint16_t>(m_vertices.size());
                            glm::vec3 normal = FACE_NORMALS[i];
                            auto tex_idx = texture_indices[i];

                            for (int j = 0; j < 4; ++j)
                            {
                                graphics::Vertex v;
                                v.position = position + CUBE_FACE_VERTICES[i][j];
                                v.normal = normal;

                                float u = (tex_idx[0] + UV_COORDS[j].x) / ATLAS_SIZE;
                                float v_coord = (tex_idx[1] + UV_COORDS[j].y) / ATLAS_SIZE;
                                v.tex_coords = {u, v_coord};
                                v.light = 15.0f;

                                m_vertices.push_back(v);
                            }

                            for (int j = 0; j < 6; ++j)
                            {
                                m_indices.push_back(baseIndex + FACE_INDICES[j]);
                            }
                        }
                    }
                }
            }
        }
    }
}
