#pragma once

#include <webgpu/webgpu.h>

#include "../camera.h"
#include "../chunk.h"
#include "chunk_mesh.hpp"
#include "render_pipeline.h"
#include "texture.hpp"

namespace flint::graphics
{

    class WorldRenderer
    {
    public:
        WorldRenderer();
        ~WorldRenderer();

        void init(WGPUDevice device, WGPUQueue queue, WGPUTextureFormat surfaceFormat, WGPUTextureFormat depthTextureFormat);
        void render(WGPURenderPassEncoder renderPass, WGPUQueue queue, const Camera &camera);
        void cleanup();

        void generateChunk(WGPUDevice device);
        void rebuild_chunk_mesh(WGPUDevice device);

        Chunk &getChunk();
        const Chunk &getChunk() const;

    private:
        WGPUShaderModule m_vertexShader = nullptr;
        WGPUShaderModule m_fragmentShader = nullptr;

        Chunk m_chunk;
        ChunkMesh m_chunkMesh;
        Texture m_atlas;

        RenderPipeline m_renderPipeline;

        WGPUBuffer m_uniformBuffer = nullptr;
        CameraUniform m_cameraUniform;
    };

} // namespace flint::graphics