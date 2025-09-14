#include "chunk.hpp"

#include <iostream>

namespace flint
{
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

        // Upload vertex data
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

        // Upload index data
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

        // Bind buffers and draw
        wgpuRenderPassEncoderSetVertexBuffer(renderPass, 0, m_vertexBuffer, 0, WGPU_WHOLE_SIZE);
        wgpuRenderPassEncoderSetIndexBuffer(renderPass, m_indexBuffer, WGPUIndexFormat_Uint16, 0, WGPU_WHOLE_SIZE);
        wgpuRenderPassEncoderDrawIndexed(renderPass, static_cast<uint32_t>(m_indices.size()), 1, 0, 0, 0);
    }

    bool Chunk::is_solid(int x, int y, int z) const
    {
        if (x < 0 || x >= CHUNK_WIDTH || y < 0 || y >= CHUNK_HEIGHT || z < 0 || z >= CHUNK_DEPTH)
        {
            return false; // Outside of this chunk, considered not solid by this chunk.
        }
        return m_blocks[x][y][z] != BlockType::Air;
    }

    void Chunk::generateChunkData()
    {
        for (int x = 0; x < CHUNK_WIDTH; ++x)
        {
            for (int z = 0; z < CHUNK_DEPTH; ++z)
            {
                for (int y = 0; y < CHUNK_HEIGHT; ++y)
                {
                    // Create a floor
                    if (y < CHUNK_HEIGHT / 2)
                    {
                         m_blocks[x][y][z] = BlockType::Dirt;
                    }
                    else
                    {
                        m_blocks[x][y][z] = BlockType::Air;
                    }
                }
            }
        }

        // Add some features
        // A pillar
        m_blocks[5][CHUNK_HEIGHT / 2][5] = BlockType::Grass;
        m_blocks[5][CHUNK_HEIGHT / 2 + 1][5] = BlockType::Grass;

        // A higher platform
        m_blocks[10][CHUNK_HEIGHT / 2][10] = BlockType::Grass;
        m_blocks[11][CHUNK_HEIGHT / 2][10] = BlockType::Grass;
        m_blocks[10][CHUNK_HEIGHT / 2][11] = BlockType::Grass;
        m_blocks[11][CHUNK_HEIGHT / 2][11] = BlockType::Grass;
        m_blocks[10][CHUNK_HEIGHT / 2 + 1][10] = BlockType::Grass;
        m_blocks[11][CHUNK_HEIGHT / 2 + 1][10] = BlockType::Grass;
        m_blocks[10][CHUNK_HEIGHT / 2 + 1][11] = BlockType::Grass;
        m_blocks[11][CHUNK_HEIGHT / 2 + 1][11] = BlockType::Grass;
        m_blocks[10][CHUNK_HEIGHT / 2 + 2][11] = BlockType::Grass;
    }

    void Chunk::generateMesh()
    {
        m_vertices.clear();
        m_indices.clear();

        glm::vec3 dirtColor = {0.5f, 0.25f, 0.0f};
        glm::vec3 grassColor = {0.0f, 1.0f, 0.0f};

        for (int x = 0; x < CHUNK_WIDTH; ++x)
        {
            for (int y = 0; y < CHUNK_HEIGHT; ++y)
            {
                for (int z = 0; z < CHUNK_DEPTH; ++z)
                {
                    if (m_blocks[x][y][z] == BlockType::Air)
                    {
                        continue;
                    }

                    glm::vec3 color = (m_blocks[x][y][z] == BlockType::Grass) ? grassColor : dirtColor;
                    glm::vec3 position = {(float)x, (float)y, (float)z};

                    // Check each face
                    // Front face
                    if (z == CHUNK_DEPTH - 1 || m_blocks[x][y][z + 1] == BlockType::Air)
                    {
                        uint16_t baseIndex = m_vertices.size();
                        m_vertices.push_back({{position + glm::vec3(-0.5f, -0.5f, 0.5f)}, color});
                        m_vertices.push_back({{position + glm::vec3(0.5f, -0.5f, 0.5f)}, color});
                        m_vertices.push_back({{position + glm::vec3(0.5f, 0.5f, 0.5f)}, color});
                        m_vertices.push_back({{position + glm::vec3(-0.5f, 0.5f, 0.5f)}, color});
                        m_indices.push_back(baseIndex);
                        m_indices.push_back(baseIndex + 1);
                        m_indices.push_back(baseIndex + 2);
                        m_indices.push_back(baseIndex + 2);
                        m_indices.push_back(baseIndex + 3);
                        m_indices.push_back(baseIndex);
                    }
                    // Back face
                    if (z == 0 || m_blocks[x][y][z - 1] == BlockType::Air)
                    {
                        uint16_t baseIndex = m_vertices.size();
                        m_vertices.push_back({{position + glm::vec3(-0.5f, -0.5f, -0.5f)}, color});
                        m_vertices.push_back({{position + glm::vec3(0.5f, -0.5f, -0.5f)}, color});
                        m_vertices.push_back({{position + glm::vec3(0.5f, 0.5f, -0.5f)}, color});
                        m_vertices.push_back({{position + glm::vec3(-0.5f, 0.5f, -0.5f)}, color});
                        m_indices.push_back(baseIndex);
                        m_indices.push_back(baseIndex + 2);
                        m_indices.push_back(baseIndex + 1);
                        m_indices.push_back(baseIndex + 2);
                        m_indices.push_back(baseIndex);
                        m_indices.push_back(baseIndex + 3);
                    }
                    // Top face
                    if (y == CHUNK_HEIGHT - 1 || m_blocks[x][y + 1][z] == BlockType::Air)
                    {
                        uint16_t baseIndex = m_vertices.size();
                        m_vertices.push_back({{position + glm::vec3(-0.5f, 0.5f, 0.5f)}, color});
                        m_vertices.push_back({{position + glm::vec3(0.5f, 0.5f, 0.5f)}, color});
                        m_vertices.push_back({{position + glm::vec3(0.5f, 0.5f, -0.5f)}, color});
                        m_vertices.push_back({{position + glm::vec3(-0.5f, 0.5f, -0.5f)}, color});
                        m_indices.push_back(baseIndex);
                        m_indices.push_back(baseIndex + 1);
                        m_indices.push_back(baseIndex + 2);
                        m_indices.push_back(baseIndex + 2);
                        m_indices.push_back(baseIndex + 3);
                        m_indices.push_back(baseIndex);
                    }
                    // Bottom face
                    if (y == 0 || m_blocks[x][y - 1][z] == BlockType::Air)
                    {
                        uint16_t baseIndex = m_vertices.size();
                        m_vertices.push_back({{position + glm::vec3(-0.5f, -0.5f, 0.5f)}, color});
                        m_vertices.push_back({{position + glm::vec3(0.5f, -0.5f, 0.5f)}, color});
                        m_vertices.push_back({{position + glm::vec3(0.5f, -0.5f, -0.5f)}, color});
                        m_vertices.push_back({{position + glm::vec3(-0.5f, -0.5f, -0.5f)}, color});
                        m_indices.push_back(baseIndex);
                        m_indices.push_back(baseIndex + 2);
                        m_indices.push_back(baseIndex + 1);
                        m_indices.push_back(baseIndex + 2);
                        m_indices.push_back(baseIndex);
                        m_indices.push_back(baseIndex + 3);
                    }
                    // Right face
                    if (x == CHUNK_WIDTH - 1 || m_blocks[x + 1][y][z] == BlockType::Air)
                    {
                        uint16_t baseIndex = m_vertices.size();
                        m_vertices.push_back({{position + glm::vec3(0.5f, -0.5f, 0.5f)}, color});
                        m_vertices.push_back({{position + glm::vec3(0.5f, -0.5f, -0.5f)}, color});
                        m_vertices.push_back({{position + glm::vec3(0.5f, 0.5f, -0.5f)}, color});
                        m_vertices.push_back({{position + glm::vec3(0.5f, 0.5f, 0.5f)}, color});
                        m_indices.push_back(baseIndex);
                        m_indices.push_back(baseIndex + 1);
                        m_indices.push_back(baseIndex + 2);
                        m_indices.push_back(baseIndex + 2);
                        m_indices.push_back(baseIndex + 3);
                        m_indices.push_back(baseIndex);
                    }
                    // Left face
                    if (x == 0 || m_blocks[x - 1][y][z] == BlockType::Air)
                    {
                        uint16_t baseIndex = m_vertices.size();
                        m_vertices.push_back({{position + glm::vec3(-0.5f, -0.5f, 0.5f)}, color});
                        m_vertices.push_back({{position + glm::vec3(-0.5f, -0.5f, -0.5f)}, color});
                        m_vertices.push_back({{position + glm::vec3(-0.5f, 0.5f, -0.5f)}, color});
                        m_vertices.push_back({{position + glm::vec3(-0.5f, 0.5f, 0.5f)}, color});
                        m_indices.push_back(baseIndex);
                        m_indices.push_back(baseIndex + 2);
                        m_indices.push_back(baseIndex + 1);
                        m_indices.push_back(baseIndex + 2);
                        m_indices.push_back(baseIndex);
                        m_indices.push_back(baseIndex + 3);
                    }
                }
            }
        }
    }

} // namespace flint
