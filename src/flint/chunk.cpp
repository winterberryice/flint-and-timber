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
                    // Create a grassy floor
                    if (y < CHUNK_HEIGHT / 2)
                    {
                        if (y == CHUNK_HEIGHT / 2 - 1)
                        {
                            m_blocks[x][y][z] = BlockType::Grass;
                        }
                        else
                        {
                            m_blocks[x][y][z] = BlockType::Dirt;
                        }
                    }
                    else
                    {
                        m_blocks[x][y][z] = BlockType::Air;
                    }
                }
            }
        }
    }

    namespace
    {
        // Based on the rust implementation context
        glm::vec2 get_texture_coords(BlockType block_type, int face_index)
        {
            // face_index: 0=front, 1=back, 2=top, 3=bottom, 4=right, 5=left
            switch (block_type)
            {
            case BlockType::Grass:
                switch (face_index)
                {
                case 2:
                    return {0, 0}; // Top face
                case 3:
                    return {2, 0}; // Bottom face (dirt)
                default:
                    return {3, 0}; // Side faces
                }
            case BlockType::Dirt:
                return {2, 0}; // All faces
            default:
                return {0, 0}; // Default/error texture
            }
        }
    }

    void Chunk::generateMesh()
    {
        m_vertices.clear();
        m_indices.clear();

        const float ATLAS_COLS = 16.0f;
        const float ATLAS_ROWS = 16.0f; // Assuming 16x16 atlas, can be adjusted
        const float TEX_SIZE_X = 1.0f / ATLAS_COLS;
        const float TEX_SIZE_Y = 1.0f / ATLAS_ROWS;

        glm::vec3 default_color = {1.0f, 1.0f, 1.0f};
        glm::vec3 grass_top_color = {0.1f, 0.9f, 0.1f}; // Sentinel color for tinting

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

                    glm::vec3 position = {(float)x, (float)y, (float)z};
                    BlockType block_type = m_blocks[x][y][z];

                    // Check each face
                    const glm::vec3 face_normals[6] = {
                        {0, 0, 1},  // Front
                        {0, 0, -1}, // Back
                        {0, 1, 0},  // Top
                        {0, -1, 0}, // Bottom
                        {1, 0, 0},  // Right
                        {-1, 0, 0}, // Left
                    };

                    for (int i = 0; i < 6; ++i)
                    {
                        int checkX = x + face_normals[i].x;
                        int checkY = y + face_normals[i].y;
                        int checkZ = z + face_normals[i].z;

                        if (!is_solid(checkX, checkY, checkZ))
                        {
                            uint16_t baseIndex = m_vertices.size();

                            glm::vec2 tex_idx = get_texture_coords(block_type, i);
                            float u_min = tex_idx.x * TEX_SIZE_X;
                            float v_min = tex_idx.y * TEX_SIZE_Y;
                            float u_max = u_min + TEX_SIZE_X;
                            float v_max = v_min + TEX_SIZE_Y;

                            glm::vec2 uv_tl = {u_min, v_min};
                            glm::vec2 uv_tr = {u_max, v_min};
                            glm::vec2 uv_bl = {u_min, v_max};
                            glm::vec2 uv_br = {u_max, v_max};

                            glm::vec3 face_color = default_color;
                            if (block_type == BlockType::Grass && i == 2) // Top face of grass
                            {
                                face_color = grass_top_color;
                            }

                            switch (i)
                            {
                            case 0: // Front face
                                m_vertices.push_back({position + glm::vec3(0.5f, 0.5f, 0.5f), face_color, uv_tr});
                                m_vertices.push_back({position + glm::vec3(-0.5f, 0.5f, 0.5f), face_color, uv_tl});
                                m_vertices.push_back({position + glm::vec3(-0.5f, -0.5f, 0.5f), face_color, uv_bl});
                                m_vertices.push_back({position + glm::vec3(0.5f, -0.5f, 0.5f), face_color, uv_br});
                                break;
                            case 1: // Back face
                                m_vertices.push_back({position + glm::vec3(-0.5f, 0.5f, -0.5f), face_color, uv_tr});
                                m_vertices.push_back({position + glm::vec3(0.5f, 0.5f, -0.5f), face_color, uv_tl});
                                m_vertices.push_back({position + glm::vec3(0.5f, -0.5f, -0.5f), face_color, uv_bl});
                                m_vertices.push_back({position + glm::vec3(-0.5f, -0.5f, -0.5f), face_color, uv_br});
                                break;
                            case 2: // Top face
                                m_vertices.push_back({position + glm::vec3(0.5f, 0.5f, -0.5f), face_color, uv_tr});
                                m_vertices.push_back({position + glm::vec3(-0.5f, 0.5f, -0.5f), face_color, uv_tl});
                                m_vertices.push_back({position + glm::vec3(-0.5f, 0.5f, 0.5f), face_color, uv_bl});
                                m_vertices.push_back({position + glm::vec3(0.5f, 0.5f, 0.5f), face_color, uv_br});
                                break;
                            case 3: // Bottom face
                                m_vertices.push_back({position + glm::vec3(0.5f, -0.5f, 0.5f), face_color, uv_tr});
                                m_vertices.push_back({position + glm::vec3(-0.5f, -0.5f, 0.5f), face_color, uv_tl});
                                m_vertices.push_back({position + glm::vec3(-0.5f, -0.5f, -0.5f), face_color, uv_bl});
                                m_vertices.push_back({position + glm::vec3(0.5f, -0.5f, -0.5f), face_color, uv_br});
                                break;
                            case 4: // Right face
                                m_vertices.push_back({position + glm::vec3(0.5f, 0.5f, -0.5f), face_color, uv_tr});
                                m_vertices.push_back({position + glm::vec3(0.5f, 0.5f, 0.5f), face_color, uv_tl});
                                m_vertices.push_back({position + glm::vec3(0.5f, -0.5f, 0.5f), face_color, uv_bl});
                                m_vertices.push_back({position + glm::vec3(0.5f, -0.5f, -0.5f), face_color, uv_br});
                                break;
                            case 5: // Left face
                                m_vertices.push_back({position + glm::vec3(-0.5f, 0.5f, 0.5f), face_color, uv_tr});
                                m_vertices.push_back({position + glm::vec3(-0.5f, 0.5f, -0.5f), face_color, uv_tl});
                                m_vertices.push_back({position + glm::vec3(-0.5f, -0.5f, -0.5f), face_color, uv_bl});
                                m_vertices.push_back({position + glm::vec3(-0.5f, -0.5f, 0.5f), face_color, uv_br});
                                break;
                            }
                            m_indices.push_back(baseIndex);
                            m_indices.push_back(baseIndex + 1);
                            m_indices.push_back(baseIndex + 2);
                            m_indices.push_back(baseIndex);
                            m_indices.push_back(baseIndex + 2);
                            m_indices.push_back(baseIndex + 3);
                        }
                    }
                }
            }
        }
    }

} // namespace flint
