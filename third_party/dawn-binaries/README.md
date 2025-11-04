# Dawn Precompiled Binaries

This directory handles fetching precompiled Dawn WebGPU binaries from [winterberryice/flint-and-timber-dawn](https://github.com/winterberryice/flint-and-timber-dawn).

## Configuration

- **Version**: Specified in `dawn-git-tag.txt` (currently `chromium/7187`)
- **Repository**: `winterberryice/flint-and-timber-dawn`
- **Platforms**: Linux x64, Windows x64, macOS Universal

## Platform-Specific Notes

### macOS Universal Binaries

The macOS binaries are **universal** (contain both arm64 and x86_64 architectures in a single file).

**Important for CI/CD**: When creating macOS artifacts with multiple architecture builds:

```bash
# ✅ DO: Copy Dawn binary from either build (they're identical)
cp build/release-arm64/libwebgpu_dawn.dylib artifact/

# ❌ DON'T: Try to lipo Dawn binaries together
# lipo -create build/release-arm64/libwebgpu_dawn.dylib \
#              build/release-x86_64/libwebgpu_dawn.dylib \
#              -output artifact/libwebgpu_dawn.dylib
# This will fail because both already contain x86_64

# ✅ DO: Lipo other libraries (like SDL3) that need it
lipo -create build/release-arm64/libSDL3.dylib \
             build/release-x86_64/libSDL3.dylib \
             -output artifact/libSDL3.dylib
```

### Detecting Universal Binaries in Scripts

You can check if a binary is universal:

```bash
# Check architectures in a dylib
lipo -info libwebgpu_dawn.dylib
# Output: "Architectures in the fat file: ... are: x86_64 arm64"

# Or use file command
file libwebgpu_dawn.dylib
```

### CMake Integration

The CMakeLists.txt in this directory:
1. Downloads the appropriate binary for your platform
2. Creates the `webgpu` interface target
3. Provides `target_copy_webgpu_binaries()` function
4. Sets `DAWN_IS_UNIVERSAL_BINARY=TRUE` on macOS (for CI detection)

## Updating Dawn Version

To use a different Dawn version:

1. Update `dawn-git-tag.txt` with the new tag (e.g., `chromium/7188`)
2. Ensure corresponding release exists in winterberryice/flint-and-timber-dawn
3. Clean build directory and reconfigure

## Directory Structure Expected in Release Archives

### Linux (`dawn-linux-x64.tar.gz`)
```
lib/libwebgpu_dawn.so
include/webgpu/webgpu.h
include/dawn/...
```

### Windows (`dawn-windows-x64.zip`)
```
lib/webgpu_dawn.dll
lib/webgpu_dawn.lib
include/webgpu/webgpu.h
include/dawn/...
```

### macOS (`dawn-macos-universal.tar.gz`)
```
lib/libwebgpu_dawn.dylib  (universal: arm64 + x86_64)
include/webgpu/webgpu.h
include/dawn/...
```
