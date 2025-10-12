#pragma once

#include <webgpu/webgpu.h>

#include <map>
#include <memory>

#include "../camera.h"
#include "../world.h"
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

        void init(WGPUDevice device, WGPUQueue queue, WGPUTextureFormat surfaceFormat, WGPUTextureFormat depthTextureFormat, World &world);
        void render(WGPURenderPassEncoder renderPass, WGPUQueue queue, const Camera &camera, World &world);
        void cleanup();

        void rebuildChunkMesh(WGPUDevice device, int chunkX, int chunkZ, const Chunk &chunk);

    private:
        WGPUShaderModule m_vertexShader = nullptr;
        WGPUShaderModule m_fragmentShader = nullptr;

        std::map<std::pair<int, int>, std::unique_ptr<ChunkMesh>> m_chunkMeshes;
        Texture m_atlas;

        RenderPipeline m_renderPipeline;

        WGPUBuffer m_uniformBuffer = nullptr;
        CameraUniform m_cameraUniform;
    };

} // namespace flint::graphics