#include "camera.h"
#include "glm/gtc/matrix_transform.hpp"

namespace flint {

    CameraUniform::CameraUniform() {
        // Initialize with an identity matrix.
        view_proj = glm::mat4(1.0f);
    }

} // namespace flint
