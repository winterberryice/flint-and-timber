/**
 * This file is part of the "Learn WebGPU for C++" book.
 *   https://github.com/eliemichel/LearnWebGPU
 *
 * MIT License
 * Copyright (c) 2022-2024 Elie Michel
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include "webgpu-utils.h"

#include <iostream>
#include <vector>
#include <cassert>
#include <iostream>
#include <cstring>
#include <chrono>
#include <thread>

WGPUAdapter requestAdapterSync(WGPUInstance instance, WGPURequestAdapterOptions const *options)
{
    // A simple structure holding the local information shared with the
    // onAdapterRequestEnded callback.
    struct UserData
    {
        WGPUAdapter adapter = nullptr;
        bool requestEnded = false;
    };
    UserData userData;

    // Callback called by wgpuInstanceRequestAdapter when the request returns
    // This is a C++ lambda function, but could be any function defined in the
    // global scope. It must be non-capturing (the brackets [] are empty) so
    // that it behaves like a regular C function pointer, which is what
    // wgpuInstanceRequestAdapter expects (WebGPU being a C API). The workaround
    // is to convey what we want to capture through the pUserData pointer,
    // provided as the last argument of wgpuInstanceRequestAdapter and received
    // by the callback as its last argument.
    auto onAdapterRequestEnded = [](WGPURequestAdapterStatus status, WGPUAdapter adapter, WGPUStringView message, void *pUserData, void *)
    {
        std::cout << "Adapter request ended with status: " << status << std::endl;
        UserData &userData = *reinterpret_cast<UserData *>(pUserData);
        if (status == WGPURequestAdapterStatus_Success)
        {
            userData.adapter = adapter;
        }
        else
        {
            std::cout << "Could not get WebGPU adapter: " << message.data << std::endl;
        }
        userData.requestEnded = true;
    };

    WGPURequestAdapterCallbackInfo callbackInfo;
    callbackInfo.nextInChain = nullptr;
    callbackInfo.callback = onAdapterRequestEnded;
    callbackInfo.userdata1 = &userData;
    callbackInfo.mode = WGPUCallbackMode_AllowProcessEvents;
    // callbackInfo.userdata2 = nullptr;

    // Call to the WebGPU request adapter procedure
    wgpuInstanceRequestAdapter(
        instance /* equivalent of navigator.gpu */,
        options,
        callbackInfo);

    // assert(userData.requestEnded);
    auto start = std::chrono::steady_clock::now();
    int timeoutMs = 5000;

    while (userData.requestEnded == false)
    {
        // Process events to allow callbacks to be called
        if (instance)
        {
            wgpuInstanceProcessEvents(instance);
        }

        // Check timeout
        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - start).count();

        if (elapsed > timeoutMs)
        {
            std::cout << "Adapter request timed out after " << timeoutMs << "ms" << std::endl;
            return nullptr;
        }

        // Small delay to avoid busy waiting
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    return userData.adapter;
}

void inspectAdapter(WGPUAdapter adapter)
{
    WGPUSupportedFeatures supportedFeatures;

    // Call the function a first time with a null return address, just to get
    // the entry count.
    wgpuAdapterGetFeatures(adapter, &supportedFeatures);

    std::cout << "Adapter features:" << std::endl;
    std::cout << std::hex; // Write integers as hexadecimal to ease comparison with webgpu.h literals
    for (size_t i = 0; i < supportedFeatures.featureCount; i++)
    {
        std::cout << " - 0x" << supportedFeatures.features[i] << std::endl;
    }
    std::cout << std::dec; // Restore decimal numbers

    WGPUAdapterInfo properties = {};
    properties.nextInChain = nullptr;
    wgpuAdapterGetInfo(adapter, &properties);

    std::cout << "Adapter properties:" << std::endl;
    std::cout << " - vendorID: " << properties.vendorID << std::endl;
    if (properties.vendor.data)
    {
        std::cout << " - vendorName: " << properties.vendor.data << std::endl;
    }
    if (properties.architecture.data)
    {
        std::cout << " - architecture: " << properties.architecture.data << std::endl;
    }
    std::cout << " - deviceID: " << properties.deviceID << std::endl;
    if (properties.description.data)
    {
        std::cout << " - description: " << properties.description.data << std::endl;
    }
    std::cout << std::hex;
    std::cout << " - adapterType: 0x" << properties.adapterType << std::endl;
    std::cout << " - backendType: 0x" << properties.backendType << std::endl;
    std::cout << std::dec; // Restore decimal numbers
}

WGPUDevice requestDeviceSync(WGPUInstance instance, WGPUAdapter adapter, WGPUDeviceDescriptor const *descriptor)
{
    struct UserData
    {
        WGPUDevice device = nullptr;
        bool requestEnded = false;
    };
    UserData userData;

    auto onDeviceRequestEnded = [](WGPURequestDeviceStatus status, WGPUDevice device, WGPUStringView message, void *pUserData, void *)
    {
        std::cout << "Device request ended with status: " << status << std::endl;
        UserData &userData = *reinterpret_cast<UserData *>(pUserData);
        if (status == WGPURequestDeviceStatus_Success)
        {
            userData.device = device;
        }
        else
        {
            std::cout << "Could not get WebGPU device: " << message.data << std::endl;
        }
        userData.requestEnded = true;
    };

    WGPURequestDeviceCallbackInfo callbackInfo;
    callbackInfo.nextInChain = nullptr;
    callbackInfo.callback = onDeviceRequestEnded;
    callbackInfo.userdata1 = &userData;
    callbackInfo.userdata2 = nullptr;

    wgpuAdapterRequestDevice(
        adapter,
        descriptor,
        callbackInfo);

    // assert(userData.requestEnded);
    auto start = std::chrono::steady_clock::now();
    int timeoutMs = 5000;

    while (userData.requestEnded == false)
    {
        // Process events to allow callbacks to be called
        if (instance)
        {
            wgpuInstanceProcessEvents(instance);
        }

        // Check timeout
        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - start).count();

        if (elapsed > timeoutMs)
        {
            std::cout << "Adapter request timed out after " << timeoutMs << "ms" << std::endl;
            return nullptr;
        }

        // Small delay to avoid busy waiting
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    return userData.device;
}

void inspectDevice(WGPUDevice device)
{
    WGPUSupportedFeatures features;
    wgpuDeviceGetFeatures(device, &features);

    std::cout << "Device features:" << std::endl;
    std::cout << std::hex;
    for (size_t i = 0; i < features.featureCount; i++)
    {
        std::cout << " - 0x" << features.features[i] << std::endl;
    }
    std::cout << std::dec;

    WGPULimits limits = {};
    limits.nextInChain = nullptr;
    bool success = wgpuDeviceGetLimits(device, &limits);
    if (success)
    {
        std::cout << "Device limits:" << std::endl;
        std::cout << " - maxTextureDimension1D: " << limits.maxTextureDimension1D << std::endl;
        std::cout << " - maxTextureDimension2D: " << limits.maxTextureDimension2D << std::endl;
        std::cout << " - maxTextureDimension3D: " << limits.maxTextureDimension3D << std::endl;
        std::cout << " - maxTextureArrayLayers: " << limits.maxTextureArrayLayers << std::endl;
        std::cout << " - maxBindGroups: " << limits.maxBindGroups << std::endl;
        std::cout << " - maxDynamicUniformBuffersPerPipelineLayout: " << limits.maxDynamicUniformBuffersPerPipelineLayout << std::endl;
        std::cout << " - maxDynamicStorageBuffersPerPipelineLayout: " << limits.maxDynamicStorageBuffersPerPipelineLayout << std::endl;
        std::cout << " - maxSampledTexturesPerShaderStage: " << limits.maxSampledTexturesPerShaderStage << std::endl;
        std::cout << " - maxSamplersPerShaderStage: " << limits.maxSamplersPerShaderStage << std::endl;
        std::cout << " - maxStorageBuffersPerShaderStage: " << limits.maxStorageBuffersPerShaderStage << std::endl;
        std::cout << " - maxStorageTexturesPerShaderStage: " << limits.maxStorageTexturesPerShaderStage << std::endl;
        std::cout << " - maxUniformBuffersPerShaderStage: " << limits.maxUniformBuffersPerShaderStage << std::endl;
        std::cout << " - maxUniformBufferBindingSize: " << limits.maxUniformBufferBindingSize << std::endl;
        std::cout << " - maxStorageBufferBindingSize: " << limits.maxStorageBufferBindingSize << std::endl;
        std::cout << " - minUniformBufferOffsetAlignment: " << limits.minUniformBufferOffsetAlignment << std::endl;
        std::cout << " - minStorageBufferOffsetAlignment: " << limits.minStorageBufferOffsetAlignment << std::endl;
        std::cout << " - maxVertexBuffers: " << limits.maxVertexBuffers << std::endl;
        std::cout << " - maxVertexAttributes: " << limits.maxVertexAttributes << std::endl;
        std::cout << " - maxVertexBufferArrayStride: " << limits.maxVertexBufferArrayStride << std::endl;
        std::cout << " - maxInterStageShaderVariables: " << limits.maxInterStageShaderVariables << std::endl;
        std::cout << " - maxComputeWorkgroupStorageSize: " << limits.maxComputeWorkgroupStorageSize << std::endl;
        std::cout << " - maxComputeInvocationsPerWorkgroup: " << limits.maxComputeInvocationsPerWorkgroup << std::endl;
        std::cout << " - maxComputeWorkgroupSizeX: " << limits.maxComputeWorkgroupSizeX << std::endl;
        std::cout << " - maxComputeWorkgroupSizeY: " << limits.maxComputeWorkgroupSizeY << std::endl;
        std::cout << " - maxComputeWorkgroupSizeZ: " << limits.maxComputeWorkgroupSizeZ << std::endl;
        std::cout << " - maxComputeWorkgroupsPerDimension: " << limits.maxComputeWorkgroupsPerDimension << std::endl;
    }
}