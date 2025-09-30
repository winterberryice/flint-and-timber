#include "debug.h"
#include "cube_geometry.h"
#include "vertex.h"
#include <vector>
#include <iostream>

namespace flint
{
    // --- DebugBox Implementation ---

    DebugBox::DebugBox() : m_device(nullptr), m_vertexBuffer(nullptr), m_indexBuffer(nullptr), m_indexCount(0) {}

    DebugBox::~DebugBox()
    {
        cleanup();
    }

    void DebugBox::cleanup()
    {
        if (m_vertexBuffer)
        {
            wgpuBufferDestroy(m_vertexBuffer);
            wgpuBufferRelease(m_vertexBuffer);
            m_vertexBuffer = nullptr;
        }
        if (m_indexBuffer)
        {
            wgpuBufferDestroy(m_indexBuffer);
            wgpuBufferRelease(m_indexBuffer);
            m_indexBuffer = nullptr;
        }
        m_indexCount = 0;
    }

    void DebugBox::create(WGPUDevice device, const flint::player::Player &player)
    {
        m_device = device;
        cleanup(); // Ensure old resources are released.

        // Define the geometry for a single cube.
        // We will update the vertex positions each frame, but the indices remain constant.
        std::vector<uint16_t> indices;
        uint16_t currentIndex = 0;

        const std::array<CubeGeometry::Face, 6> &faces = CubeGeometry::getAllFaces();
        for (size_t i = 0; i < faces.size(); ++i)
        {
            const std::vector<uint16_t> &faceIndices = CubeGeometry::getLocalFaceIndices();
            for (const auto &index : faceIndices)
            {
                indices.push_back(currentIndex + index);
            }
            currentIndex += 4; // Each face has 4 vertices
        }
        m_indexCount = indices.size();

        // Create the index buffer (only needs to be done once).
        WGPUBufferDescriptor indexBufferDesc = {};
        indexBufferDesc.size = indices.size() * sizeof(uint16_t);
        indexBufferDesc.usage = WGPUBufferUsage_Index | WGPUBufferUsage_CopyDst;
        indexBufferDesc.mappedAtCreation = false;
        m_indexBuffer = wgpuDeviceCreateBuffer(m_device, &indexBufferDesc);
        wgpuQueueWriteBuffer(wgpuDeviceGetQueue(m_device), m_indexBuffer, 0, indices.data(), indexBufferDesc.size);

        // Create the vertex buffer. Its content will be updated each frame.
        // Size it for one cube (6 faces * 4 vertices per face).
        WGPUBufferDescriptor vertexBufferDesc = {};
        vertexBufferDesc.size = 24 * sizeof(flint::Vertex);
        vertexBufferDesc.usage = WGPUBufferUsage_Vertex | WGPUBufferUsage_CopyDst;
        vertexBufferDesc.mappedAtCreation = false;
        m_vertexBuffer = wgpuDeviceCreateBuffer(m_device, &vertexBufferDesc);

        // Perform an initial update to populate the vertex buffer.
        update(player);
    }

    void DebugBox::update(const flint::player::Player &player)
    {
        if (!m_vertexBuffer || !m_device)
            return;

        physics::AABB worldBox = player.get_world_bounding_box();
        glm::vec3 min = worldBox.min;
        glm::vec3 max = worldBox.max;

        // Vertices of the bounding box in world coordinates.
        std::vector<glm::vec3> positions = {
            // Bottom face
            {min.x, min.y, min.z},
            {max.x, min.y, min.z},
            {max.x, min.y, max.z},
            {min.x, min.y, max.z},
            // Top face
            {min.x, max.y, min.z},
            {max.x, max.y, min.z},
            {max.x, max.y, max.z},
            {min.x, max.y, max.z},
        };

        // Reconstruct the full vertex list for the cube.
        std::vector<Vertex> vertices = {
            // Front face (+Z)
            {positions[3], {1.0f, 1.0f, 0.0f}}, // Bottom-left
            {positions[2], {1.0f, 1.0f, 0.0f}}, // Bottom-right
            {positions[6], {1.0f, 1.0f, 0.0f}}, // Top-right
            {positions[7], {1.0f, 1.0f, 0.0f}}, // Top-left
            // Back face (-Z)
            {positions[1], {1.0f, 1.0f, 0.0f}},
            {positions[0], {1.0f, 1.0f, 0.0f}},
            {positions[4], {1.0f, 1.0f, 0.0f}},
            {positions[5], {1.0f, 1.0f, 0.0f}},
            // Left face (-X)
            {positions[0], {1.0f, 1.0f, 0.0f}},
            {positions[3], {1.0f, 1.0f, 0.0f}},
            {positions[7], {1.0f, 1.0f, 0.0f}},
            {positions[4], {1.0f, 1.0f, 0.0f}},
            // Right face (+X)
            {positions[2], {1.0f, 1.0f, 0.0f}},
            {positions[1], {1.0f, 1.0f, 0.0f}},
            {positions[5], {1.0f, 1.0f, 0.0f}},
            {positions[6], {1.0f, 1.0f, 0.0f}},
            // Top face (+Y)
            {positions[7], {1.0f, 1.0f, 0.0f}},
            {positions[6], {1.0f, 1.0f, 0.0f}},
            {positions[5], {1.0f, 1.0f, 0.0f}},
            {positions[4], {1.0f, 1.0f, 0.0f}},
            // Bottom face (-Y)
            {positions[0], {1.0f, 1.0f, 0.0f}},
            {positions[1], {1.0f, 1.0f, 0.0f}},
            {positions[2], {1.0f, 1.0f, 0.0f}},
            {positions[3], {1.0f, 1.0f, 0.0f}},
        };

        // Update the vertex buffer with the new positions.
        wgpuQueueWriteBuffer(wgpuDeviceGetQueue(m_device), m_vertexBuffer, 0, vertices.data(), vertices.size() * sizeof(flint::Vertex));
    }

    void DebugBox::render(WGPURenderPassEncoder renderPass)
    {
        if (!m_vertexBuffer || !m_indexBuffer || m_indexCount == 0)
        {
            return; // Nothing to render
        }

        wgpuRenderPassEncoderSetVertexBuffer(renderPass, 0, m_vertexBuffer, 0, WGPU_WHOLE_SIZE);
        wgpuRenderPassEncoderSetIndexBuffer(renderPass, m_indexBuffer, WGPUIndexFormat_Uint16, 0, WGPU_WHOLE_SIZE);
        wgpuRenderPassEncoderDrawIndexed(renderPass, m_indexCount, 1, 0, 0, 0);
    }

    // --- Original Debug Print Functions ---

    void printVideoSystemInfo()
    {
        const char *videoDriver = SDL_GetCurrentVideoDriver();
        printf("\n=== SDL3 Video System Information ===\n");
        printf("Current Video Driver: %s\n", videoDriver ? videoDriver : "NULL");
        if (videoDriver)
        {
            if (strcmp(videoDriver, "wayland") == 0)
                printf("🌊 Running on WAYLAND\n");
            else if (strcmp(videoDriver, "x11") == 0)
                printf("🖥️  Running on X11\n");
            else
                printf("🤔 Running on: %s (other)\n", videoDriver);
        }
        else
        {
            printf("❌ No video driver detected\n");
        }
        int numDrivers = SDL_GetNumVideoDrivers();
        printf("\nAvailable video drivers (%d total):\n", numDrivers);
        for (int i = 0; i < numDrivers; i++)
        {
            const char *driverName = SDL_GetVideoDriver(i);
            bool isCurrent = (videoDriver && strcmp(videoDriver, driverName) == 0);
            printf("  %s%s%s\n", isCurrent ? "[ACTIVE] " : "         ", driverName ? driverName : "NULL", isCurrent ? " ←" : "");
        }
    }

    void printDetailedVideoInfo(SDL_Window *window)
    {
        const char *videoDriver = SDL_GetCurrentVideoDriver();
        printf("\n=== Detailed Video System Info ===\n");
        printf("SDL Video Driver: %s\n", videoDriver ? videoDriver : "NULL");
        if (!videoDriver)
            return;
        if (strcmp(videoDriver, "wayland") == 0)
        {
            printf("🌊 **WAYLAND SESSION DETECTED** 🌊\n");
            if (window)
            {
                SDL_PropertiesID props = SDL_GetWindowProperties(window);
                if (SDL_HasProperty(props, SDL_PROP_WINDOW_WAYLAND_DISPLAY_POINTER))
                {
                    void *waylandDisplay = SDL_GetPointerProperty(props, SDL_PROP_WINDOW_WAYLAND_DISPLAY_POINTER, NULL);
                    void *waylandSurface = SDL_GetPointerProperty(props, SDL_PROP_WINDOW_WAYLAND_SURFACE_POINTER, NULL);
                    printf("  Wayland Display: %p\n", waylandDisplay);
                    printf("  Wayland Surface: %p\n", waylandSurface);
                    printf("  ✓ Native Wayland window detected\n");
                }
                else
                {
                    printf("  ❌ Wayland properties not available (running via XWayland?)\n");
                }
            }
        }
        else if (strcmp(videoDriver, "x11") == 0)
        {
            printf("🖥️  **X11 SESSION DETECTED** 🖥️\n");
            if (window)
            {
                SDL_PropertiesID props = SDL_GetWindowProperties(window);
                if (SDL_HasProperty(props, SDL_PROP_WINDOW_X11_DISPLAY_POINTER))
                {
                    void *x11Display = SDL_GetPointerProperty(props, SDL_PROP_WINDOW_X11_DISPLAY_POINTER, NULL);
                    Uint64 x11Window = SDL_GetNumberProperty(props, SDL_PROP_WINDOW_X11_WINDOW_NUMBER, 0);
                    printf("  X11 Display: %p\n", x11Display);
                    printf("  X11 Window: 0x%llx\n", (unsigned long long)x11Window);
                    printf("  ✓ Native X11 window detected\n");
                }
                else
                {
                    printf("  ❌ X11 properties not available\n");
                }
            }
        }
        else
        {
            printf("🤔 **OTHER DRIVER: %s** 🤔\n", videoDriver);
        }
        const char *waylandDisplay = getenv("WAYLAND_DISPLAY");
        const char *x11Display = getenv("DISPLAY");
        const char *xdgSessionType = getenv("XDG_SESSION_TYPE");
        printf("\n=== Environment Variables ===\n");
        printf("WAYLAND_DISPLAY: %s\n", waylandDisplay ? waylandDisplay : "not set");
        printf("DISPLAY: %s\n", x11Display ? x11Display : "not set");
        printf("XDG_SESSION_TYPE: %s\n", xdgSessionType ? xdgSessionType : "not set");
    }
}