
#ifndef DAWNWIRE_SERVER_WGPUTRAITS_AUTOGEN_H_
#define DAWNWIRE_SERVER_WGPUTRAITS_AUTOGEN_H_

#include "dawn/dawn_proc_table.h"

namespace dawn::wire::server {

template <typename T>
struct WGPUTraits {
    static constexpr auto Release = nullptr;
};

template <>
struct WGPUTraits<WGPUAdapter> {
    static constexpr auto Release = &DawnProcTable::adapterRelease;
};
template <>
struct WGPUTraits<WGPUBindGroup> {
    static constexpr auto Release = &DawnProcTable::bindGroupRelease;
};
template <>
struct WGPUTraits<WGPUBindGroupLayout> {
    static constexpr auto Release = &DawnProcTable::bindGroupLayoutRelease;
};
template <>
struct WGPUTraits<WGPUBuffer> {
    static constexpr auto Release = &DawnProcTable::bufferRelease;
};
template <>
struct WGPUTraits<WGPUCommandBuffer> {
    static constexpr auto Release = &DawnProcTable::commandBufferRelease;
};
template <>
struct WGPUTraits<WGPUCommandEncoder> {
    static constexpr auto Release = &DawnProcTable::commandEncoderRelease;
};
template <>
struct WGPUTraits<WGPUComputePipeline> {
    static constexpr auto Release = &DawnProcTable::computePipelineRelease;
};
template <>
struct WGPUTraits<WGPUDevice> {
    static constexpr auto Release = &DawnProcTable::deviceRelease;
};
template <>
struct WGPUTraits<WGPUExternalTexture> {
    static constexpr auto Release = &DawnProcTable::externalTextureRelease;
};
template <>
struct WGPUTraits<WGPUInstance> {
    static constexpr auto Release = &DawnProcTable::instanceRelease;
};
template <>
struct WGPUTraits<WGPUPipelineLayout> {
    static constexpr auto Release = &DawnProcTable::pipelineLayoutRelease;
};
template <>
struct WGPUTraits<WGPUQuerySet> {
    static constexpr auto Release = &DawnProcTable::querySetRelease;
};
template <>
struct WGPUTraits<WGPUQueue> {
    static constexpr auto Release = &DawnProcTable::queueRelease;
};
template <>
struct WGPUTraits<WGPURenderBundle> {
    static constexpr auto Release = &DawnProcTable::renderBundleRelease;
};
template <>
struct WGPUTraits<WGPURenderPipeline> {
    static constexpr auto Release = &DawnProcTable::renderPipelineRelease;
};
template <>
struct WGPUTraits<WGPUResourceTable> {
    static constexpr auto Release = &DawnProcTable::resourceTableRelease;
};
template <>
struct WGPUTraits<WGPUSampler> {
    static constexpr auto Release = &DawnProcTable::samplerRelease;
};
template <>
struct WGPUTraits<WGPUShaderModule> {
    static constexpr auto Release = &DawnProcTable::shaderModuleRelease;
};
template <>
struct WGPUTraits<WGPUSurface> {
    static constexpr auto Release = &DawnProcTable::surfaceRelease;
};
template <>
struct WGPUTraits<WGPUTexelBufferView> {
    static constexpr auto Release = &DawnProcTable::texelBufferViewRelease;
};
template <>
struct WGPUTraits<WGPUTexture> {
    static constexpr auto Release = &DawnProcTable::textureRelease;
};
template <>
struct WGPUTraits<WGPUTextureView> {
    static constexpr auto Release = &DawnProcTable::textureViewRelease;
};
template <>
struct WGPUTraits<WGPUComputePassEncoder> {
    static constexpr auto Release = &DawnProcTable::computePassEncoderRelease;
};
template <>
struct WGPUTraits<WGPURenderBundleEncoder> {
    static constexpr auto Release = &DawnProcTable::renderBundleEncoderRelease;
};
template <>
struct WGPUTraits<WGPURenderPassEncoder> {
    static constexpr auto Release = &DawnProcTable::renderPassEncoderRelease;
};

template <>
struct WGPUTraits<WGPUSupportedFeatures> {
    static constexpr auto FreeMembers = &DawnProcTable::supportedFeaturesFreeMembers;
};
template <>
struct WGPUTraits<WGPUSupportedInstanceFeatures> {
    static constexpr auto FreeMembers = &DawnProcTable::supportedInstanceFeaturesFreeMembers;
};
template <>
struct WGPUTraits<WGPUSupportedWGSLLanguageFeatures> {
    static constexpr auto FreeMembers = &DawnProcTable::supportedWGSLLanguageFeaturesFreeMembers;
};
template <>
struct WGPUTraits<WGPUSurfaceCapabilities> {
    static constexpr auto FreeMembers = &DawnProcTable::surfaceCapabilitiesFreeMembers;
};
template <>
struct WGPUTraits<WGPUAdapterPropertiesMemoryHeaps> {
    static constexpr auto FreeMembers = &DawnProcTable::adapterPropertiesMemoryHeapsFreeMembers;
};
template <>
struct WGPUTraits<WGPUAdapterPropertiesSubgroupMatrixConfigs> {
    static constexpr auto FreeMembers = &DawnProcTable::adapterPropertiesSubgroupMatrixConfigsFreeMembers;
};
template <>
struct WGPUTraits<WGPUDawnDrmFormatCapabilities> {
    static constexpr auto FreeMembers = &DawnProcTable::dawnDrmFormatCapabilitiesFreeMembers;
};
template <>
struct WGPUTraits<WGPUAdapterInfo> {
    static constexpr auto FreeMembers = &DawnProcTable::adapterInfoFreeMembers;
};

}  // namespace dawn::wire::server

#endif  // DAWNWIRE_SERVER_WGPUTRAITS_AUTOGEN_H_
