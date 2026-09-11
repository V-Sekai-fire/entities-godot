#include "dawn/dawn_thread_dispatch_proc.h"

#include <algorithm>
#include <thread>

#include "dawn/dawn_version.h"

static DawnProcTable nullProcs;
static DawnProcTable defaultProc;
static thread_local DawnProcTable perThreadProcs;

void dawnProcSetDefaultThreadProcs(const DawnProcTable* procs) {
    if (procs) {
        defaultProc = *procs;
    } else {
        defaultProc = nullProcs;
    }
}

void dawnProcSetPerThreadProcs(const DawnProcTable* procs) {
    if (procs) {
        perThreadProcs = *procs;
    } else {
        perThreadProcs = nullProcs;
    }
}

static WGPUInstance ThreadDispatchCreateInstance(WGPUInstanceDescriptor const * descriptor) {
    auto* proc = perThreadProcs.createInstance;
    if (!proc) {
        proc = defaultProc.createInstance;
    }
return     proc(descriptor);
}
static void ThreadDispatchGetInstanceFeatures(WGPUSupportedInstanceFeatures * features) {
    auto* proc = perThreadProcs.getInstanceFeatures;
    if (!proc) {
        proc = defaultProc.getInstanceFeatures;
    }
    proc(features);
}
static WGPUStatus ThreadDispatchGetInstanceLimits(WGPUInstanceLimits * limits) {
    auto* proc = perThreadProcs.getInstanceLimits;
    if (!proc) {
        proc = defaultProc.getInstanceLimits;
    }
return     proc(limits);
}
static WGPUBool ThreadDispatchHasInstanceFeature(WGPUInstanceFeatureName feature) {
    auto* proc = perThreadProcs.hasInstanceFeature;
    if (!proc) {
        proc = defaultProc.hasInstanceFeature;
    }
return     proc(feature);
}
static WGPUProc ThreadDispatchGetProcAddress(WGPUStringView procName) {
    auto* proc = perThreadProcs.getProcAddress;
    if (!proc) {
        proc = defaultProc.getProcAddress;
    }
return     proc(procName);
}

static WGPUDevice ThreadDispatchAdapterCreateDevice(WGPUAdapter adapter, WGPUDeviceDescriptor const * descriptor) {
    auto* proc = perThreadProcs.adapterCreateDevice;
    if (!proc) {
        proc = defaultProc.adapterCreateDevice;
    }
return     proc(adapter, descriptor);
}
static void ThreadDispatchAdapterGetFeatures(WGPUAdapter adapter, WGPUSupportedFeatures * features) {
    auto* proc = perThreadProcs.adapterGetFeatures;
    if (!proc) {
        proc = defaultProc.adapterGetFeatures;
    }
    proc(adapter, features);
}
static WGPUStatus ThreadDispatchAdapterGetFormatCapabilities(WGPUAdapter adapter, WGPUTextureFormat format, WGPUDawnFormatCapabilities * capabilities) {
    auto* proc = perThreadProcs.adapterGetFormatCapabilities;
    if (!proc) {
        proc = defaultProc.adapterGetFormatCapabilities;
    }
return     proc(adapter, format, capabilities);
}
static WGPUStatus ThreadDispatchAdapterGetInfo(WGPUAdapter adapter, WGPUAdapterInfo * info) {
    auto* proc = perThreadProcs.adapterGetInfo;
    if (!proc) {
        proc = defaultProc.adapterGetInfo;
    }
return     proc(adapter, info);
}
static WGPUInstance ThreadDispatchAdapterGetInstance(WGPUAdapter adapter) {
    auto* proc = perThreadProcs.adapterGetInstance;
    if (!proc) {
        proc = defaultProc.adapterGetInstance;
    }
return     proc(adapter);
}
static WGPUStatus ThreadDispatchAdapterGetLimits(WGPUAdapter adapter, WGPULimits * limits) {
    auto* proc = perThreadProcs.adapterGetLimits;
    if (!proc) {
        proc = defaultProc.adapterGetLimits;
    }
return     proc(adapter, limits);
}
static WGPUBool ThreadDispatchAdapterHasFeature(WGPUAdapter adapter, WGPUFeatureName feature) {
    auto* proc = perThreadProcs.adapterHasFeature;
    if (!proc) {
        proc = defaultProc.adapterHasFeature;
    }
return     proc(adapter, feature);
}
static WGPUFuture ThreadDispatchAdapterRequestDevice(WGPUAdapter adapter, WGPUDeviceDescriptor const * descriptor, WGPURequestDeviceCallbackInfo callbackInfo) {
    auto* proc = perThreadProcs.adapterRequestDevice;
    if (!proc) {
        proc = defaultProc.adapterRequestDevice;
    }
return     proc(adapter, descriptor, callbackInfo);
}
static void ThreadDispatchAdapterAddRef(WGPUAdapter adapter) {
    auto* proc = perThreadProcs.adapterAddRef;
    if (!proc) {
        proc = defaultProc.adapterAddRef;
    }
    proc(adapter);
}
static void ThreadDispatchAdapterRelease(WGPUAdapter adapter) {
    auto* proc = perThreadProcs.adapterRelease;
    if (!proc) {
        proc = defaultProc.adapterRelease;
    }
    proc(adapter);
}
static void ThreadDispatchAdapterInfoFreeMembers(WGPUAdapterInfo adapterInfo) {
    auto* proc = perThreadProcs.adapterInfoFreeMembers;
    if (!proc) {
        proc = defaultProc.adapterInfoFreeMembers;
    }
    proc(adapterInfo);
}
static void ThreadDispatchAdapterPropertiesMemoryHeapsFreeMembers(WGPUAdapterPropertiesMemoryHeaps adapterPropertiesMemoryHeaps) {
    auto* proc = perThreadProcs.adapterPropertiesMemoryHeapsFreeMembers;
    if (!proc) {
        proc = defaultProc.adapterPropertiesMemoryHeapsFreeMembers;
    }
    proc(adapterPropertiesMemoryHeaps);
}
static void ThreadDispatchAdapterPropertiesSubgroupMatrixConfigsFreeMembers(WGPUAdapterPropertiesSubgroupMatrixConfigs adapterPropertiesSubgroupMatrixConfigs) {
    auto* proc = perThreadProcs.adapterPropertiesSubgroupMatrixConfigsFreeMembers;
    if (!proc) {
        proc = defaultProc.adapterPropertiesSubgroupMatrixConfigsFreeMembers;
    }
    proc(adapterPropertiesSubgroupMatrixConfigs);
}
static void ThreadDispatchBindGroupSetLabel(WGPUBindGroup bindGroup, WGPUStringView label) {
    auto* proc = perThreadProcs.bindGroupSetLabel;
    if (!proc) {
        proc = defaultProc.bindGroupSetLabel;
    }
    proc(bindGroup, label);
}
static void ThreadDispatchBindGroupAddRef(WGPUBindGroup bindGroup) {
    auto* proc = perThreadProcs.bindGroupAddRef;
    if (!proc) {
        proc = defaultProc.bindGroupAddRef;
    }
    proc(bindGroup);
}
static void ThreadDispatchBindGroupRelease(WGPUBindGroup bindGroup) {
    auto* proc = perThreadProcs.bindGroupRelease;
    if (!proc) {
        proc = defaultProc.bindGroupRelease;
    }
    proc(bindGroup);
}
static void ThreadDispatchBindGroupLayoutSetLabel(WGPUBindGroupLayout bindGroupLayout, WGPUStringView label) {
    auto* proc = perThreadProcs.bindGroupLayoutSetLabel;
    if (!proc) {
        proc = defaultProc.bindGroupLayoutSetLabel;
    }
    proc(bindGroupLayout, label);
}
static void ThreadDispatchBindGroupLayoutAddRef(WGPUBindGroupLayout bindGroupLayout) {
    auto* proc = perThreadProcs.bindGroupLayoutAddRef;
    if (!proc) {
        proc = defaultProc.bindGroupLayoutAddRef;
    }
    proc(bindGroupLayout);
}
static void ThreadDispatchBindGroupLayoutRelease(WGPUBindGroupLayout bindGroupLayout) {
    auto* proc = perThreadProcs.bindGroupLayoutRelease;
    if (!proc) {
        proc = defaultProc.bindGroupLayoutRelease;
    }
    proc(bindGroupLayout);
}
static WGPUTexelBufferView ThreadDispatchBufferCreateTexelView(WGPUBuffer buffer, WGPUTexelBufferViewDescriptor const * descriptor) {
    auto* proc = perThreadProcs.bufferCreateTexelView;
    if (!proc) {
        proc = defaultProc.bufferCreateTexelView;
    }
return     proc(buffer, descriptor);
}
static void ThreadDispatchBufferDestroy(WGPUBuffer buffer) {
    auto* proc = perThreadProcs.bufferDestroy;
    if (!proc) {
        proc = defaultProc.bufferDestroy;
    }
    proc(buffer);
}
static void const * ThreadDispatchBufferGetConstMappedRange(WGPUBuffer buffer, size_t offset, size_t size) {
    auto* proc = perThreadProcs.bufferGetConstMappedRange;
    if (!proc) {
        proc = defaultProc.bufferGetConstMappedRange;
    }
return     proc(buffer, offset, size);
}
static void * ThreadDispatchBufferGetMappedRange(WGPUBuffer buffer, size_t offset, size_t size) {
    auto* proc = perThreadProcs.bufferGetMappedRange;
    if (!proc) {
        proc = defaultProc.bufferGetMappedRange;
    }
return     proc(buffer, offset, size);
}
static WGPUBufferMapState ThreadDispatchBufferGetMapState(WGPUBuffer buffer) {
    auto* proc = perThreadProcs.bufferGetMapState;
    if (!proc) {
        proc = defaultProc.bufferGetMapState;
    }
return     proc(buffer);
}
static uint64_t ThreadDispatchBufferGetSize(WGPUBuffer buffer) {
    auto* proc = perThreadProcs.bufferGetSize;
    if (!proc) {
        proc = defaultProc.bufferGetSize;
    }
return     proc(buffer);
}
static WGPUBufferUsage ThreadDispatchBufferGetUsage(WGPUBuffer buffer) {
    auto* proc = perThreadProcs.bufferGetUsage;
    if (!proc) {
        proc = defaultProc.bufferGetUsage;
    }
return     proc(buffer);
}
static WGPUFuture ThreadDispatchBufferMapAsync(WGPUBuffer buffer, WGPUMapMode mode, size_t offset, size_t size, WGPUBufferMapCallbackInfo callbackInfo) {
    auto* proc = perThreadProcs.bufferMapAsync;
    if (!proc) {
        proc = defaultProc.bufferMapAsync;
    }
return     proc(buffer, mode, offset, size, callbackInfo);
}
static WGPUStatus ThreadDispatchBufferReadMappedRange(WGPUBuffer buffer, size_t offset, void * data, size_t size) {
    auto* proc = perThreadProcs.bufferReadMappedRange;
    if (!proc) {
        proc = defaultProc.bufferReadMappedRange;
    }
return     proc(buffer, offset, data, size);
}
static void ThreadDispatchBufferSetLabel(WGPUBuffer buffer, WGPUStringView label) {
    auto* proc = perThreadProcs.bufferSetLabel;
    if (!proc) {
        proc = defaultProc.bufferSetLabel;
    }
    proc(buffer, label);
}
static void ThreadDispatchBufferUnmap(WGPUBuffer buffer) {
    auto* proc = perThreadProcs.bufferUnmap;
    if (!proc) {
        proc = defaultProc.bufferUnmap;
    }
    proc(buffer);
}
static WGPUStatus ThreadDispatchBufferWriteMappedRange(WGPUBuffer buffer, size_t offset, void const * data, size_t size) {
    auto* proc = perThreadProcs.bufferWriteMappedRange;
    if (!proc) {
        proc = defaultProc.bufferWriteMappedRange;
    }
return     proc(buffer, offset, data, size);
}
static void ThreadDispatchBufferAddRef(WGPUBuffer buffer) {
    auto* proc = perThreadProcs.bufferAddRef;
    if (!proc) {
        proc = defaultProc.bufferAddRef;
    }
    proc(buffer);
}
static void ThreadDispatchBufferRelease(WGPUBuffer buffer) {
    auto* proc = perThreadProcs.bufferRelease;
    if (!proc) {
        proc = defaultProc.bufferRelease;
    }
    proc(buffer);
}
static void ThreadDispatchCommandBufferSetLabel(WGPUCommandBuffer commandBuffer, WGPUStringView label) {
    auto* proc = perThreadProcs.commandBufferSetLabel;
    if (!proc) {
        proc = defaultProc.commandBufferSetLabel;
    }
    proc(commandBuffer, label);
}
static void ThreadDispatchCommandBufferAddRef(WGPUCommandBuffer commandBuffer) {
    auto* proc = perThreadProcs.commandBufferAddRef;
    if (!proc) {
        proc = defaultProc.commandBufferAddRef;
    }
    proc(commandBuffer);
}
static void ThreadDispatchCommandBufferRelease(WGPUCommandBuffer commandBuffer) {
    auto* proc = perThreadProcs.commandBufferRelease;
    if (!proc) {
        proc = defaultProc.commandBufferRelease;
    }
    proc(commandBuffer);
}
static WGPUComputePassEncoder ThreadDispatchCommandEncoderBeginComputePass(WGPUCommandEncoder commandEncoder, WGPUComputePassDescriptor const * descriptor) {
    auto* proc = perThreadProcs.commandEncoderBeginComputePass;
    if (!proc) {
        proc = defaultProc.commandEncoderBeginComputePass;
    }
return     proc(commandEncoder, descriptor);
}
static WGPURenderPassEncoder ThreadDispatchCommandEncoderBeginRenderPass(WGPUCommandEncoder commandEncoder, WGPURenderPassDescriptor const * descriptor) {
    auto* proc = perThreadProcs.commandEncoderBeginRenderPass;
    if (!proc) {
        proc = defaultProc.commandEncoderBeginRenderPass;
    }
return     proc(commandEncoder, descriptor);
}
static void ThreadDispatchCommandEncoderClearBuffer(WGPUCommandEncoder commandEncoder, WGPUBuffer buffer, uint64_t offset, uint64_t size) {
    auto* proc = perThreadProcs.commandEncoderClearBuffer;
    if (!proc) {
        proc = defaultProc.commandEncoderClearBuffer;
    }
    proc(commandEncoder, buffer, offset, size);
}
static void ThreadDispatchCommandEncoderCopyBufferToBuffer(WGPUCommandEncoder commandEncoder, WGPUBuffer source, uint64_t sourceOffset, WGPUBuffer destination, uint64_t destinationOffset, uint64_t size) {
    auto* proc = perThreadProcs.commandEncoderCopyBufferToBuffer;
    if (!proc) {
        proc = defaultProc.commandEncoderCopyBufferToBuffer;
    }
    proc(commandEncoder, source, sourceOffset, destination, destinationOffset, size);
}
static void ThreadDispatchCommandEncoderCopyBufferToTexture(WGPUCommandEncoder commandEncoder, WGPUTexelCopyBufferInfo const * source, WGPUTexelCopyTextureInfo const * destination, WGPUExtent3D const * copySize) {
    auto* proc = perThreadProcs.commandEncoderCopyBufferToTexture;
    if (!proc) {
        proc = defaultProc.commandEncoderCopyBufferToTexture;
    }
    proc(commandEncoder, source, destination, copySize);
}
static void ThreadDispatchCommandEncoderCopyTextureToBuffer(WGPUCommandEncoder commandEncoder, WGPUTexelCopyTextureInfo const * source, WGPUTexelCopyBufferInfo const * destination, WGPUExtent3D const * copySize) {
    auto* proc = perThreadProcs.commandEncoderCopyTextureToBuffer;
    if (!proc) {
        proc = defaultProc.commandEncoderCopyTextureToBuffer;
    }
    proc(commandEncoder, source, destination, copySize);
}
static void ThreadDispatchCommandEncoderCopyTextureToTexture(WGPUCommandEncoder commandEncoder, WGPUTexelCopyTextureInfo const * source, WGPUTexelCopyTextureInfo const * destination, WGPUExtent3D const * copySize) {
    auto* proc = perThreadProcs.commandEncoderCopyTextureToTexture;
    if (!proc) {
        proc = defaultProc.commandEncoderCopyTextureToTexture;
    }
    proc(commandEncoder, source, destination, copySize);
}
static WGPUCommandBuffer ThreadDispatchCommandEncoderFinish(WGPUCommandEncoder commandEncoder, WGPUCommandBufferDescriptor const * descriptor) {
    auto* proc = perThreadProcs.commandEncoderFinish;
    if (!proc) {
        proc = defaultProc.commandEncoderFinish;
    }
return     proc(commandEncoder, descriptor);
}
static void ThreadDispatchCommandEncoderInjectValidationError(WGPUCommandEncoder commandEncoder, WGPUStringView message) {
    auto* proc = perThreadProcs.commandEncoderInjectValidationError;
    if (!proc) {
        proc = defaultProc.commandEncoderInjectValidationError;
    }
    proc(commandEncoder, message);
}
static void ThreadDispatchCommandEncoderInsertDebugMarker(WGPUCommandEncoder commandEncoder, WGPUStringView markerLabel) {
    auto* proc = perThreadProcs.commandEncoderInsertDebugMarker;
    if (!proc) {
        proc = defaultProc.commandEncoderInsertDebugMarker;
    }
    proc(commandEncoder, markerLabel);
}
static void ThreadDispatchCommandEncoderPopDebugGroup(WGPUCommandEncoder commandEncoder) {
    auto* proc = perThreadProcs.commandEncoderPopDebugGroup;
    if (!proc) {
        proc = defaultProc.commandEncoderPopDebugGroup;
    }
    proc(commandEncoder);
}
static void ThreadDispatchCommandEncoderPushDebugGroup(WGPUCommandEncoder commandEncoder, WGPUStringView groupLabel) {
    auto* proc = perThreadProcs.commandEncoderPushDebugGroup;
    if (!proc) {
        proc = defaultProc.commandEncoderPushDebugGroup;
    }
    proc(commandEncoder, groupLabel);
}
static void ThreadDispatchCommandEncoderResolveQuerySet(WGPUCommandEncoder commandEncoder, WGPUQuerySet querySet, uint32_t firstQuery, uint32_t queryCount, WGPUBuffer destination, uint64_t destinationOffset) {
    auto* proc = perThreadProcs.commandEncoderResolveQuerySet;
    if (!proc) {
        proc = defaultProc.commandEncoderResolveQuerySet;
    }
    proc(commandEncoder, querySet, firstQuery, queryCount, destination, destinationOffset);
}
static void ThreadDispatchCommandEncoderSetLabel(WGPUCommandEncoder commandEncoder, WGPUStringView label) {
    auto* proc = perThreadProcs.commandEncoderSetLabel;
    if (!proc) {
        proc = defaultProc.commandEncoderSetLabel;
    }
    proc(commandEncoder, label);
}
static void ThreadDispatchCommandEncoderWriteBuffer(WGPUCommandEncoder commandEncoder, WGPUBuffer buffer, uint64_t bufferOffset, uint8_t const * data, uint64_t size) {
    auto* proc = perThreadProcs.commandEncoderWriteBuffer;
    if (!proc) {
        proc = defaultProc.commandEncoderWriteBuffer;
    }
    proc(commandEncoder, buffer, bufferOffset, data, size);
}
static void ThreadDispatchCommandEncoderWriteTimestamp(WGPUCommandEncoder commandEncoder, WGPUQuerySet querySet, uint32_t queryIndex) {
    auto* proc = perThreadProcs.commandEncoderWriteTimestamp;
    if (!proc) {
        proc = defaultProc.commandEncoderWriteTimestamp;
    }
    proc(commandEncoder, querySet, queryIndex);
}
static void ThreadDispatchCommandEncoderAddRef(WGPUCommandEncoder commandEncoder) {
    auto* proc = perThreadProcs.commandEncoderAddRef;
    if (!proc) {
        proc = defaultProc.commandEncoderAddRef;
    }
    proc(commandEncoder);
}
static void ThreadDispatchCommandEncoderRelease(WGPUCommandEncoder commandEncoder) {
    auto* proc = perThreadProcs.commandEncoderRelease;
    if (!proc) {
        proc = defaultProc.commandEncoderRelease;
    }
    proc(commandEncoder);
}
static void ThreadDispatchComputePassEncoderDispatchWorkgroups(WGPUComputePassEncoder computePassEncoder, uint32_t workgroupCountX, uint32_t workgroupCountY, uint32_t workgroupCountZ) {
    auto* proc = perThreadProcs.computePassEncoderDispatchWorkgroups;
    if (!proc) {
        proc = defaultProc.computePassEncoderDispatchWorkgroups;
    }
    proc(computePassEncoder, workgroupCountX, workgroupCountY, workgroupCountZ);
}
static void ThreadDispatchComputePassEncoderDispatchWorkgroupsIndirect(WGPUComputePassEncoder computePassEncoder, WGPUBuffer indirectBuffer, uint64_t indirectOffset) {
    auto* proc = perThreadProcs.computePassEncoderDispatchWorkgroupsIndirect;
    if (!proc) {
        proc = defaultProc.computePassEncoderDispatchWorkgroupsIndirect;
    }
    proc(computePassEncoder, indirectBuffer, indirectOffset);
}
static void ThreadDispatchComputePassEncoderEnd(WGPUComputePassEncoder computePassEncoder) {
    auto* proc = perThreadProcs.computePassEncoderEnd;
    if (!proc) {
        proc = defaultProc.computePassEncoderEnd;
    }
    proc(computePassEncoder);
}
static void ThreadDispatchComputePassEncoderInsertDebugMarker(WGPUComputePassEncoder computePassEncoder, WGPUStringView markerLabel) {
    auto* proc = perThreadProcs.computePassEncoderInsertDebugMarker;
    if (!proc) {
        proc = defaultProc.computePassEncoderInsertDebugMarker;
    }
    proc(computePassEncoder, markerLabel);
}
static void ThreadDispatchComputePassEncoderPopDebugGroup(WGPUComputePassEncoder computePassEncoder) {
    auto* proc = perThreadProcs.computePassEncoderPopDebugGroup;
    if (!proc) {
        proc = defaultProc.computePassEncoderPopDebugGroup;
    }
    proc(computePassEncoder);
}
static void ThreadDispatchComputePassEncoderPushDebugGroup(WGPUComputePassEncoder computePassEncoder, WGPUStringView groupLabel) {
    auto* proc = perThreadProcs.computePassEncoderPushDebugGroup;
    if (!proc) {
        proc = defaultProc.computePassEncoderPushDebugGroup;
    }
    proc(computePassEncoder, groupLabel);
}
static void ThreadDispatchComputePassEncoderSetBindGroup(WGPUComputePassEncoder computePassEncoder, uint32_t groupIndex, WGPUBindGroup group, size_t dynamicOffsetCount, uint32_t const * dynamicOffsets) {
    auto* proc = perThreadProcs.computePassEncoderSetBindGroup;
    if (!proc) {
        proc = defaultProc.computePassEncoderSetBindGroup;
    }
    proc(computePassEncoder, groupIndex, group, dynamicOffsetCount, dynamicOffsets);
}
static void ThreadDispatchComputePassEncoderSetImmediates(WGPUComputePassEncoder computePassEncoder, uint32_t offset, void const * data, size_t size) {
    auto* proc = perThreadProcs.computePassEncoderSetImmediates;
    if (!proc) {
        proc = defaultProc.computePassEncoderSetImmediates;
    }
    proc(computePassEncoder, offset, data, size);
}
static void ThreadDispatchComputePassEncoderSetLabel(WGPUComputePassEncoder computePassEncoder, WGPUStringView label) {
    auto* proc = perThreadProcs.computePassEncoderSetLabel;
    if (!proc) {
        proc = defaultProc.computePassEncoderSetLabel;
    }
    proc(computePassEncoder, label);
}
static void ThreadDispatchComputePassEncoderSetPipeline(WGPUComputePassEncoder computePassEncoder, WGPUComputePipeline pipeline) {
    auto* proc = perThreadProcs.computePassEncoderSetPipeline;
    if (!proc) {
        proc = defaultProc.computePassEncoderSetPipeline;
    }
    proc(computePassEncoder, pipeline);
}
static void ThreadDispatchComputePassEncoderSetResourceTable(WGPUComputePassEncoder computePassEncoder, WGPUResourceTable table) {
    auto* proc = perThreadProcs.computePassEncoderSetResourceTable;
    if (!proc) {
        proc = defaultProc.computePassEncoderSetResourceTable;
    }
    proc(computePassEncoder, table);
}
static void ThreadDispatchComputePassEncoderWriteTimestamp(WGPUComputePassEncoder computePassEncoder, WGPUQuerySet querySet, uint32_t queryIndex) {
    auto* proc = perThreadProcs.computePassEncoderWriteTimestamp;
    if (!proc) {
        proc = defaultProc.computePassEncoderWriteTimestamp;
    }
    proc(computePassEncoder, querySet, queryIndex);
}
static void ThreadDispatchComputePassEncoderAddRef(WGPUComputePassEncoder computePassEncoder) {
    auto* proc = perThreadProcs.computePassEncoderAddRef;
    if (!proc) {
        proc = defaultProc.computePassEncoderAddRef;
    }
    proc(computePassEncoder);
}
static void ThreadDispatchComputePassEncoderRelease(WGPUComputePassEncoder computePassEncoder) {
    auto* proc = perThreadProcs.computePassEncoderRelease;
    if (!proc) {
        proc = defaultProc.computePassEncoderRelease;
    }
    proc(computePassEncoder);
}
static WGPUBindGroupLayout ThreadDispatchComputePipelineGetBindGroupLayout(WGPUComputePipeline computePipeline, uint32_t groupIndex) {
    auto* proc = perThreadProcs.computePipelineGetBindGroupLayout;
    if (!proc) {
        proc = defaultProc.computePipelineGetBindGroupLayout;
    }
return     proc(computePipeline, groupIndex);
}
static void ThreadDispatchComputePipelineSetLabel(WGPUComputePipeline computePipeline, WGPUStringView label) {
    auto* proc = perThreadProcs.computePipelineSetLabel;
    if (!proc) {
        proc = defaultProc.computePipelineSetLabel;
    }
    proc(computePipeline, label);
}
static void ThreadDispatchComputePipelineAddRef(WGPUComputePipeline computePipeline) {
    auto* proc = perThreadProcs.computePipelineAddRef;
    if (!proc) {
        proc = defaultProc.computePipelineAddRef;
    }
    proc(computePipeline);
}
static void ThreadDispatchComputePipelineRelease(WGPUComputePipeline computePipeline) {
    auto* proc = perThreadProcs.computePipelineRelease;
    if (!proc) {
        proc = defaultProc.computePipelineRelease;
    }
    proc(computePipeline);
}
static void ThreadDispatchDawnDrmFormatCapabilitiesFreeMembers(WGPUDawnDrmFormatCapabilities dawnDrmFormatCapabilities) {
    auto* proc = perThreadProcs.dawnDrmFormatCapabilitiesFreeMembers;
    if (!proc) {
        proc = defaultProc.dawnDrmFormatCapabilitiesFreeMembers;
    }
    proc(dawnDrmFormatCapabilities);
}
static WGPUBindGroup ThreadDispatchDeviceCreateBindGroup(WGPUDevice device, WGPUBindGroupDescriptor const * descriptor) {
    auto* proc = perThreadProcs.deviceCreateBindGroup;
    if (!proc) {
        proc = defaultProc.deviceCreateBindGroup;
    }
return     proc(device, descriptor);
}
static WGPUBindGroupLayout ThreadDispatchDeviceCreateBindGroupLayout(WGPUDevice device, WGPUBindGroupLayoutDescriptor const * descriptor) {
    auto* proc = perThreadProcs.deviceCreateBindGroupLayout;
    if (!proc) {
        proc = defaultProc.deviceCreateBindGroupLayout;
    }
return     proc(device, descriptor);
}
static WGPUBuffer ThreadDispatchDeviceCreateBuffer(WGPUDevice device, WGPUBufferDescriptor const * descriptor) {
    auto* proc = perThreadProcs.deviceCreateBuffer;
    if (!proc) {
        proc = defaultProc.deviceCreateBuffer;
    }
return     proc(device, descriptor);
}
static WGPUCommandEncoder ThreadDispatchDeviceCreateCommandEncoder(WGPUDevice device, WGPUCommandEncoderDescriptor const * descriptor) {
    auto* proc = perThreadProcs.deviceCreateCommandEncoder;
    if (!proc) {
        proc = defaultProc.deviceCreateCommandEncoder;
    }
return     proc(device, descriptor);
}
static WGPUComputePipeline ThreadDispatchDeviceCreateComputePipeline(WGPUDevice device, WGPUComputePipelineDescriptor const * descriptor) {
    auto* proc = perThreadProcs.deviceCreateComputePipeline;
    if (!proc) {
        proc = defaultProc.deviceCreateComputePipeline;
    }
return     proc(device, descriptor);
}
static WGPUFuture ThreadDispatchDeviceCreateComputePipelineAsync(WGPUDevice device, WGPUComputePipelineDescriptor const * descriptor, WGPUCreateComputePipelineAsyncCallbackInfo callbackInfo) {
    auto* proc = perThreadProcs.deviceCreateComputePipelineAsync;
    if (!proc) {
        proc = defaultProc.deviceCreateComputePipelineAsync;
    }
return     proc(device, descriptor, callbackInfo);
}
static WGPUBuffer ThreadDispatchDeviceCreateErrorBuffer(WGPUDevice device, WGPUBufferDescriptor const * descriptor) {
    auto* proc = perThreadProcs.deviceCreateErrorBuffer;
    if (!proc) {
        proc = defaultProc.deviceCreateErrorBuffer;
    }
return     proc(device, descriptor);
}
static WGPUComputePipeline ThreadDispatchDeviceCreateErrorComputePipeline(WGPUDevice device, WGPUStringView label) {
    auto* proc = perThreadProcs.deviceCreateErrorComputePipeline;
    if (!proc) {
        proc = defaultProc.deviceCreateErrorComputePipeline;
    }
return     proc(device, label);
}
static WGPUExternalTexture ThreadDispatchDeviceCreateErrorExternalTexture(WGPUDevice device) {
    auto* proc = perThreadProcs.deviceCreateErrorExternalTexture;
    if (!proc) {
        proc = defaultProc.deviceCreateErrorExternalTexture;
    }
return     proc(device);
}
static WGPURenderPipeline ThreadDispatchDeviceCreateErrorRenderPipeline(WGPUDevice device, WGPUStringView label) {
    auto* proc = perThreadProcs.deviceCreateErrorRenderPipeline;
    if (!proc) {
        proc = defaultProc.deviceCreateErrorRenderPipeline;
    }
return     proc(device, label);
}
static WGPUShaderModule ThreadDispatchDeviceCreateErrorShaderModule(WGPUDevice device, WGPUShaderModuleDescriptor const * descriptor, WGPUStringView errorMessage) {
    auto* proc = perThreadProcs.deviceCreateErrorShaderModule;
    if (!proc) {
        proc = defaultProc.deviceCreateErrorShaderModule;
    }
return     proc(device, descriptor, errorMessage);
}
static WGPUTexture ThreadDispatchDeviceCreateErrorTexture(WGPUDevice device, WGPUTextureDescriptor const * descriptor) {
    auto* proc = perThreadProcs.deviceCreateErrorTexture;
    if (!proc) {
        proc = defaultProc.deviceCreateErrorTexture;
    }
return     proc(device, descriptor);
}
static WGPUExternalTexture ThreadDispatchDeviceCreateExternalTexture(WGPUDevice device, WGPUExternalTextureDescriptor const * externalTextureDescriptor) {
    auto* proc = perThreadProcs.deviceCreateExternalTexture;
    if (!proc) {
        proc = defaultProc.deviceCreateExternalTexture;
    }
return     proc(device, externalTextureDescriptor);
}
static WGPUPipelineLayout ThreadDispatchDeviceCreatePipelineLayout(WGPUDevice device, WGPUPipelineLayoutDescriptor const * descriptor) {
    auto* proc = perThreadProcs.deviceCreatePipelineLayout;
    if (!proc) {
        proc = defaultProc.deviceCreatePipelineLayout;
    }
return     proc(device, descriptor);
}
static WGPUQuerySet ThreadDispatchDeviceCreateQuerySet(WGPUDevice device, WGPUQuerySetDescriptor const * descriptor) {
    auto* proc = perThreadProcs.deviceCreateQuerySet;
    if (!proc) {
        proc = defaultProc.deviceCreateQuerySet;
    }
return     proc(device, descriptor);
}
static WGPURenderBundleEncoder ThreadDispatchDeviceCreateRenderBundleEncoder(WGPUDevice device, WGPURenderBundleEncoderDescriptor const * descriptor) {
    auto* proc = perThreadProcs.deviceCreateRenderBundleEncoder;
    if (!proc) {
        proc = defaultProc.deviceCreateRenderBundleEncoder;
    }
return     proc(device, descriptor);
}
static WGPURenderPipeline ThreadDispatchDeviceCreateRenderPipeline(WGPUDevice device, WGPURenderPipelineDescriptor const * descriptor) {
    auto* proc = perThreadProcs.deviceCreateRenderPipeline;
    if (!proc) {
        proc = defaultProc.deviceCreateRenderPipeline;
    }
return     proc(device, descriptor);
}
static WGPUFuture ThreadDispatchDeviceCreateRenderPipelineAsync(WGPUDevice device, WGPURenderPipelineDescriptor const * descriptor, WGPUCreateRenderPipelineAsyncCallbackInfo callbackInfo) {
    auto* proc = perThreadProcs.deviceCreateRenderPipelineAsync;
    if (!proc) {
        proc = defaultProc.deviceCreateRenderPipelineAsync;
    }
return     proc(device, descriptor, callbackInfo);
}
static WGPUResourceTable ThreadDispatchDeviceCreateResourceTable(WGPUDevice device, WGPUResourceTableDescriptor const * descriptor) {
    auto* proc = perThreadProcs.deviceCreateResourceTable;
    if (!proc) {
        proc = defaultProc.deviceCreateResourceTable;
    }
return     proc(device, descriptor);
}
static WGPUSampler ThreadDispatchDeviceCreateSampler(WGPUDevice device, WGPUSamplerDescriptor const * descriptor) {
    auto* proc = perThreadProcs.deviceCreateSampler;
    if (!proc) {
        proc = defaultProc.deviceCreateSampler;
    }
return     proc(device, descriptor);
}
static WGPUShaderModule ThreadDispatchDeviceCreateShaderModule(WGPUDevice device, WGPUShaderModuleDescriptor const * descriptor) {
    auto* proc = perThreadProcs.deviceCreateShaderModule;
    if (!proc) {
        proc = defaultProc.deviceCreateShaderModule;
    }
return     proc(device, descriptor);
}
static WGPUTexture ThreadDispatchDeviceCreateTexture(WGPUDevice device, WGPUTextureDescriptor const * descriptor) {
    auto* proc = perThreadProcs.deviceCreateTexture;
    if (!proc) {
        proc = defaultProc.deviceCreateTexture;
    }
return     proc(device, descriptor);
}
static void ThreadDispatchDeviceDestroy(WGPUDevice device) {
    auto* proc = perThreadProcs.deviceDestroy;
    if (!proc) {
        proc = defaultProc.deviceDestroy;
    }
    proc(device);
}
static void ThreadDispatchDeviceForceLoss(WGPUDevice device, WGPUDeviceLostReason type, WGPUStringView message) {
    auto* proc = perThreadProcs.deviceForceLoss;
    if (!proc) {
        proc = defaultProc.deviceForceLoss;
    }
    proc(device, type, message);
}
static WGPUAdapter ThreadDispatchDeviceGetAdapter(WGPUDevice device) {
    auto* proc = perThreadProcs.deviceGetAdapter;
    if (!proc) {
        proc = defaultProc.deviceGetAdapter;
    }
return     proc(device);
}
static WGPUStatus ThreadDispatchDeviceGetAdapterInfo(WGPUDevice device, WGPUAdapterInfo * adapterInfo) {
    auto* proc = perThreadProcs.deviceGetAdapterInfo;
    if (!proc) {
        proc = defaultProc.deviceGetAdapterInfo;
    }
return     proc(device, adapterInfo);
}
static WGPUStatus ThreadDispatchDeviceGetAHardwareBufferProperties(WGPUDevice device, void * handle, WGPUAHardwareBufferProperties * properties) {
    auto* proc = perThreadProcs.deviceGetAHardwareBufferProperties;
    if (!proc) {
        proc = defaultProc.deviceGetAHardwareBufferProperties;
    }
return     proc(device, handle, properties);
}
static void ThreadDispatchDeviceGetFeatures(WGPUDevice device, WGPUSupportedFeatures * features) {
    auto* proc = perThreadProcs.deviceGetFeatures;
    if (!proc) {
        proc = defaultProc.deviceGetFeatures;
    }
    proc(device, features);
}
static WGPUStatus ThreadDispatchDeviceGetLimits(WGPUDevice device, WGPULimits * limits) {
    auto* proc = perThreadProcs.deviceGetLimits;
    if (!proc) {
        proc = defaultProc.deviceGetLimits;
    }
return     proc(device, limits);
}
static WGPUFuture ThreadDispatchDeviceGetLostFuture(WGPUDevice device) {
    auto* proc = perThreadProcs.deviceGetLostFuture;
    if (!proc) {
        proc = defaultProc.deviceGetLostFuture;
    }
return     proc(device);
}
static WGPUQueue ThreadDispatchDeviceGetQueue(WGPUDevice device) {
    auto* proc = perThreadProcs.deviceGetQueue;
    if (!proc) {
        proc = defaultProc.deviceGetQueue;
    }
return     proc(device);
}
static WGPUBool ThreadDispatchDeviceHasFeature(WGPUDevice device, WGPUFeatureName feature) {
    auto* proc = perThreadProcs.deviceHasFeature;
    if (!proc) {
        proc = defaultProc.deviceHasFeature;
    }
return     proc(device, feature);
}
static WGPUSharedBufferMemory ThreadDispatchDeviceImportSharedBufferMemory(WGPUDevice device, WGPUSharedBufferMemoryDescriptor const * descriptor) {
    auto* proc = perThreadProcs.deviceImportSharedBufferMemory;
    if (!proc) {
        proc = defaultProc.deviceImportSharedBufferMemory;
    }
return     proc(device, descriptor);
}
static WGPUSharedFence ThreadDispatchDeviceImportSharedFence(WGPUDevice device, WGPUSharedFenceDescriptor const * descriptor) {
    auto* proc = perThreadProcs.deviceImportSharedFence;
    if (!proc) {
        proc = defaultProc.deviceImportSharedFence;
    }
return     proc(device, descriptor);
}
static WGPUSharedTextureMemory ThreadDispatchDeviceImportSharedTextureMemory(WGPUDevice device, WGPUSharedTextureMemoryDescriptor const * descriptor) {
    auto* proc = perThreadProcs.deviceImportSharedTextureMemory;
    if (!proc) {
        proc = defaultProc.deviceImportSharedTextureMemory;
    }
return     proc(device, descriptor);
}
static void ThreadDispatchDeviceInjectError(WGPUDevice device, WGPUErrorType type, WGPUStringView message) {
    auto* proc = perThreadProcs.deviceInjectError;
    if (!proc) {
        proc = defaultProc.deviceInjectError;
    }
    proc(device, type, message);
}
static WGPUFuture ThreadDispatchDevicePopErrorScope(WGPUDevice device, WGPUPopErrorScopeCallbackInfo callbackInfo) {
    auto* proc = perThreadProcs.devicePopErrorScope;
    if (!proc) {
        proc = defaultProc.devicePopErrorScope;
    }
return     proc(device, callbackInfo);
}
static void ThreadDispatchDevicePushErrorScope(WGPUDevice device, WGPUErrorFilter filter) {
    auto* proc = perThreadProcs.devicePushErrorScope;
    if (!proc) {
        proc = defaultProc.devicePushErrorScope;
    }
    proc(device, filter);
}
static void ThreadDispatchDeviceSetLabel(WGPUDevice device, WGPUStringView label) {
    auto* proc = perThreadProcs.deviceSetLabel;
    if (!proc) {
        proc = defaultProc.deviceSetLabel;
    }
    proc(device, label);
}
static void ThreadDispatchDeviceSetLoggingCallback(WGPUDevice device, WGPULoggingCallbackInfo callbackInfo) {
    auto* proc = perThreadProcs.deviceSetLoggingCallback;
    if (!proc) {
        proc = defaultProc.deviceSetLoggingCallback;
    }
    proc(device, callbackInfo);
}
static void ThreadDispatchDeviceTick(WGPUDevice device) {
    auto* proc = perThreadProcs.deviceTick;
    if (!proc) {
        proc = defaultProc.deviceTick;
    }
    proc(device);
}
static void ThreadDispatchDeviceValidateTextureDescriptor(WGPUDevice device, WGPUTextureDescriptor const * descriptor) {
    auto* proc = perThreadProcs.deviceValidateTextureDescriptor;
    if (!proc) {
        proc = defaultProc.deviceValidateTextureDescriptor;
    }
    proc(device, descriptor);
}
static void ThreadDispatchDeviceAddRef(WGPUDevice device) {
    auto* proc = perThreadProcs.deviceAddRef;
    if (!proc) {
        proc = defaultProc.deviceAddRef;
    }
    proc(device);
}
static void ThreadDispatchDeviceRelease(WGPUDevice device) {
    auto* proc = perThreadProcs.deviceRelease;
    if (!proc) {
        proc = defaultProc.deviceRelease;
    }
    proc(device);
}
static void ThreadDispatchExternalTextureDestroy(WGPUExternalTexture externalTexture) {
    auto* proc = perThreadProcs.externalTextureDestroy;
    if (!proc) {
        proc = defaultProc.externalTextureDestroy;
    }
    proc(externalTexture);
}
static void ThreadDispatchExternalTextureExpire(WGPUExternalTexture externalTexture) {
    auto* proc = perThreadProcs.externalTextureExpire;
    if (!proc) {
        proc = defaultProc.externalTextureExpire;
    }
    proc(externalTexture);
}
static void ThreadDispatchExternalTextureRefresh(WGPUExternalTexture externalTexture) {
    auto* proc = perThreadProcs.externalTextureRefresh;
    if (!proc) {
        proc = defaultProc.externalTextureRefresh;
    }
    proc(externalTexture);
}
static void ThreadDispatchExternalTextureSetLabel(WGPUExternalTexture externalTexture, WGPUStringView label) {
    auto* proc = perThreadProcs.externalTextureSetLabel;
    if (!proc) {
        proc = defaultProc.externalTextureSetLabel;
    }
    proc(externalTexture, label);
}
static void ThreadDispatchExternalTextureAddRef(WGPUExternalTexture externalTexture) {
    auto* proc = perThreadProcs.externalTextureAddRef;
    if (!proc) {
        proc = defaultProc.externalTextureAddRef;
    }
    proc(externalTexture);
}
static void ThreadDispatchExternalTextureRelease(WGPUExternalTexture externalTexture) {
    auto* proc = perThreadProcs.externalTextureRelease;
    if (!proc) {
        proc = defaultProc.externalTextureRelease;
    }
    proc(externalTexture);
}
static WGPUSurface ThreadDispatchInstanceCreateSurface(WGPUInstance instance, WGPUSurfaceDescriptor const * descriptor) {
    auto* proc = perThreadProcs.instanceCreateSurface;
    if (!proc) {
        proc = defaultProc.instanceCreateSurface;
    }
return     proc(instance, descriptor);
}
static void ThreadDispatchInstanceGetWGSLLanguageFeatures(WGPUInstance instance, WGPUSupportedWGSLLanguageFeatures * features) {
    auto* proc = perThreadProcs.instanceGetWGSLLanguageFeatures;
    if (!proc) {
        proc = defaultProc.instanceGetWGSLLanguageFeatures;
    }
    proc(instance, features);
}
static WGPUBool ThreadDispatchInstanceHasWGSLLanguageFeature(WGPUInstance instance, WGPUWGSLLanguageFeatureName feature) {
    auto* proc = perThreadProcs.instanceHasWGSLLanguageFeature;
    if (!proc) {
        proc = defaultProc.instanceHasWGSLLanguageFeature;
    }
return     proc(instance, feature);
}
static void ThreadDispatchInstanceProcessEvents(WGPUInstance instance) {
    auto* proc = perThreadProcs.instanceProcessEvents;
    if (!proc) {
        proc = defaultProc.instanceProcessEvents;
    }
    proc(instance);
}
static WGPUFuture ThreadDispatchInstanceRequestAdapter(WGPUInstance instance, WGPURequestAdapterOptions const * options, WGPURequestAdapterCallbackInfo callbackInfo) {
    auto* proc = perThreadProcs.instanceRequestAdapter;
    if (!proc) {
        proc = defaultProc.instanceRequestAdapter;
    }
return     proc(instance, options, callbackInfo);
}
static WGPUWaitStatus ThreadDispatchInstanceWaitAny(WGPUInstance instance, size_t futureCount, WGPUFutureWaitInfo * futures, uint64_t timeoutNS) {
    auto* proc = perThreadProcs.instanceWaitAny;
    if (!proc) {
        proc = defaultProc.instanceWaitAny;
    }
return     proc(instance, futureCount, futures, timeoutNS);
}
static void ThreadDispatchInstanceAddRef(WGPUInstance instance) {
    auto* proc = perThreadProcs.instanceAddRef;
    if (!proc) {
        proc = defaultProc.instanceAddRef;
    }
    proc(instance);
}
static void ThreadDispatchInstanceRelease(WGPUInstance instance) {
    auto* proc = perThreadProcs.instanceRelease;
    if (!proc) {
        proc = defaultProc.instanceRelease;
    }
    proc(instance);
}
static void ThreadDispatchPipelineLayoutSetLabel(WGPUPipelineLayout pipelineLayout, WGPUStringView label) {
    auto* proc = perThreadProcs.pipelineLayoutSetLabel;
    if (!proc) {
        proc = defaultProc.pipelineLayoutSetLabel;
    }
    proc(pipelineLayout, label);
}
static void ThreadDispatchPipelineLayoutAddRef(WGPUPipelineLayout pipelineLayout) {
    auto* proc = perThreadProcs.pipelineLayoutAddRef;
    if (!proc) {
        proc = defaultProc.pipelineLayoutAddRef;
    }
    proc(pipelineLayout);
}
static void ThreadDispatchPipelineLayoutRelease(WGPUPipelineLayout pipelineLayout) {
    auto* proc = perThreadProcs.pipelineLayoutRelease;
    if (!proc) {
        proc = defaultProc.pipelineLayoutRelease;
    }
    proc(pipelineLayout);
}
static void ThreadDispatchQuerySetDestroy(WGPUQuerySet querySet) {
    auto* proc = perThreadProcs.querySetDestroy;
    if (!proc) {
        proc = defaultProc.querySetDestroy;
    }
    proc(querySet);
}
static uint32_t ThreadDispatchQuerySetGetCount(WGPUQuerySet querySet) {
    auto* proc = perThreadProcs.querySetGetCount;
    if (!proc) {
        proc = defaultProc.querySetGetCount;
    }
return     proc(querySet);
}
static WGPUQueryType ThreadDispatchQuerySetGetType(WGPUQuerySet querySet) {
    auto* proc = perThreadProcs.querySetGetType;
    if (!proc) {
        proc = defaultProc.querySetGetType;
    }
return     proc(querySet);
}
static void ThreadDispatchQuerySetSetLabel(WGPUQuerySet querySet, WGPUStringView label) {
    auto* proc = perThreadProcs.querySetSetLabel;
    if (!proc) {
        proc = defaultProc.querySetSetLabel;
    }
    proc(querySet, label);
}
static void ThreadDispatchQuerySetAddRef(WGPUQuerySet querySet) {
    auto* proc = perThreadProcs.querySetAddRef;
    if (!proc) {
        proc = defaultProc.querySetAddRef;
    }
    proc(querySet);
}
static void ThreadDispatchQuerySetRelease(WGPUQuerySet querySet) {
    auto* proc = perThreadProcs.querySetRelease;
    if (!proc) {
        proc = defaultProc.querySetRelease;
    }
    proc(querySet);
}
static void ThreadDispatchQueueCopyExternalTextureForBrowser(WGPUQueue queue, WGPUImageCopyExternalTexture const * source, WGPUTexelCopyTextureInfo const * destination, WGPUExtent3D const * copySize, WGPUCopyTextureForBrowserOptions const * options) {
    auto* proc = perThreadProcs.queueCopyExternalTextureForBrowser;
    if (!proc) {
        proc = defaultProc.queueCopyExternalTextureForBrowser;
    }
    proc(queue, source, destination, copySize, options);
}
static void ThreadDispatchQueueCopyTextureForBrowser(WGPUQueue queue, WGPUTexelCopyTextureInfo const * source, WGPUTexelCopyTextureInfo const * destination, WGPUExtent3D const * copySize, WGPUCopyTextureForBrowserOptions const * options) {
    auto* proc = perThreadProcs.queueCopyTextureForBrowser;
    if (!proc) {
        proc = defaultProc.queueCopyTextureForBrowser;
    }
    proc(queue, source, destination, copySize, options);
}
static WGPUFuture ThreadDispatchQueueOnSubmittedWorkDone(WGPUQueue queue, WGPUQueueWorkDoneCallbackInfo callbackInfo) {
    auto* proc = perThreadProcs.queueOnSubmittedWorkDone;
    if (!proc) {
        proc = defaultProc.queueOnSubmittedWorkDone;
    }
return     proc(queue, callbackInfo);
}
static void ThreadDispatchQueueSetLabel(WGPUQueue queue, WGPUStringView label) {
    auto* proc = perThreadProcs.queueSetLabel;
    if (!proc) {
        proc = defaultProc.queueSetLabel;
    }
    proc(queue, label);
}
static void ThreadDispatchQueueSubmit(WGPUQueue queue, size_t commandCount, WGPUCommandBuffer const * commands) {
    auto* proc = perThreadProcs.queueSubmit;
    if (!proc) {
        proc = defaultProc.queueSubmit;
    }
    proc(queue, commandCount, commands);
}
static void ThreadDispatchQueueWriteBuffer(WGPUQueue queue, WGPUBuffer buffer, uint64_t bufferOffset, void const * data, size_t size) {
    auto* proc = perThreadProcs.queueWriteBuffer;
    if (!proc) {
        proc = defaultProc.queueWriteBuffer;
    }
    proc(queue, buffer, bufferOffset, data, size);
}
static void ThreadDispatchQueueWriteTexture(WGPUQueue queue, WGPUTexelCopyTextureInfo const * destination, void const * data, size_t dataSize, WGPUTexelCopyBufferLayout const * dataLayout, WGPUExtent3D const * writeSize) {
    auto* proc = perThreadProcs.queueWriteTexture;
    if (!proc) {
        proc = defaultProc.queueWriteTexture;
    }
    proc(queue, destination, data, dataSize, dataLayout, writeSize);
}
static void ThreadDispatchQueueAddRef(WGPUQueue queue) {
    auto* proc = perThreadProcs.queueAddRef;
    if (!proc) {
        proc = defaultProc.queueAddRef;
    }
    proc(queue);
}
static void ThreadDispatchQueueRelease(WGPUQueue queue) {
    auto* proc = perThreadProcs.queueRelease;
    if (!proc) {
        proc = defaultProc.queueRelease;
    }
    proc(queue);
}
static void ThreadDispatchRenderBundleSetLabel(WGPURenderBundle renderBundle, WGPUStringView label) {
    auto* proc = perThreadProcs.renderBundleSetLabel;
    if (!proc) {
        proc = defaultProc.renderBundleSetLabel;
    }
    proc(renderBundle, label);
}
static void ThreadDispatchRenderBundleAddRef(WGPURenderBundle renderBundle) {
    auto* proc = perThreadProcs.renderBundleAddRef;
    if (!proc) {
        proc = defaultProc.renderBundleAddRef;
    }
    proc(renderBundle);
}
static void ThreadDispatchRenderBundleRelease(WGPURenderBundle renderBundle) {
    auto* proc = perThreadProcs.renderBundleRelease;
    if (!proc) {
        proc = defaultProc.renderBundleRelease;
    }
    proc(renderBundle);
}
static void ThreadDispatchRenderBundleEncoderDraw(WGPURenderBundleEncoder renderBundleEncoder, uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance) {
    auto* proc = perThreadProcs.renderBundleEncoderDraw;
    if (!proc) {
        proc = defaultProc.renderBundleEncoderDraw;
    }
    proc(renderBundleEncoder, vertexCount, instanceCount, firstVertex, firstInstance);
}
static void ThreadDispatchRenderBundleEncoderDrawIndexed(WGPURenderBundleEncoder renderBundleEncoder, uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, int32_t baseVertex, uint32_t firstInstance) {
    auto* proc = perThreadProcs.renderBundleEncoderDrawIndexed;
    if (!proc) {
        proc = defaultProc.renderBundleEncoderDrawIndexed;
    }
    proc(renderBundleEncoder, indexCount, instanceCount, firstIndex, baseVertex, firstInstance);
}
static void ThreadDispatchRenderBundleEncoderDrawIndexedIndirect(WGPURenderBundleEncoder renderBundleEncoder, WGPUBuffer indirectBuffer, uint64_t indirectOffset) {
    auto* proc = perThreadProcs.renderBundleEncoderDrawIndexedIndirect;
    if (!proc) {
        proc = defaultProc.renderBundleEncoderDrawIndexedIndirect;
    }
    proc(renderBundleEncoder, indirectBuffer, indirectOffset);
}
static void ThreadDispatchRenderBundleEncoderDrawIndirect(WGPURenderBundleEncoder renderBundleEncoder, WGPUBuffer indirectBuffer, uint64_t indirectOffset) {
    auto* proc = perThreadProcs.renderBundleEncoderDrawIndirect;
    if (!proc) {
        proc = defaultProc.renderBundleEncoderDrawIndirect;
    }
    proc(renderBundleEncoder, indirectBuffer, indirectOffset);
}
static WGPURenderBundle ThreadDispatchRenderBundleEncoderFinish(WGPURenderBundleEncoder renderBundleEncoder, WGPURenderBundleDescriptor const * descriptor) {
    auto* proc = perThreadProcs.renderBundleEncoderFinish;
    if (!proc) {
        proc = defaultProc.renderBundleEncoderFinish;
    }
return     proc(renderBundleEncoder, descriptor);
}
static void ThreadDispatchRenderBundleEncoderInsertDebugMarker(WGPURenderBundleEncoder renderBundleEncoder, WGPUStringView markerLabel) {
    auto* proc = perThreadProcs.renderBundleEncoderInsertDebugMarker;
    if (!proc) {
        proc = defaultProc.renderBundleEncoderInsertDebugMarker;
    }
    proc(renderBundleEncoder, markerLabel);
}
static void ThreadDispatchRenderBundleEncoderPopDebugGroup(WGPURenderBundleEncoder renderBundleEncoder) {
    auto* proc = perThreadProcs.renderBundleEncoderPopDebugGroup;
    if (!proc) {
        proc = defaultProc.renderBundleEncoderPopDebugGroup;
    }
    proc(renderBundleEncoder);
}
static void ThreadDispatchRenderBundleEncoderPushDebugGroup(WGPURenderBundleEncoder renderBundleEncoder, WGPUStringView groupLabel) {
    auto* proc = perThreadProcs.renderBundleEncoderPushDebugGroup;
    if (!proc) {
        proc = defaultProc.renderBundleEncoderPushDebugGroup;
    }
    proc(renderBundleEncoder, groupLabel);
}
static void ThreadDispatchRenderBundleEncoderSetBindGroup(WGPURenderBundleEncoder renderBundleEncoder, uint32_t groupIndex, WGPUBindGroup group, size_t dynamicOffsetCount, uint32_t const * dynamicOffsets) {
    auto* proc = perThreadProcs.renderBundleEncoderSetBindGroup;
    if (!proc) {
        proc = defaultProc.renderBundleEncoderSetBindGroup;
    }
    proc(renderBundleEncoder, groupIndex, group, dynamicOffsetCount, dynamicOffsets);
}
static void ThreadDispatchRenderBundleEncoderSetImmediates(WGPURenderBundleEncoder renderBundleEncoder, uint32_t offset, void const * data, size_t size) {
    auto* proc = perThreadProcs.renderBundleEncoderSetImmediates;
    if (!proc) {
        proc = defaultProc.renderBundleEncoderSetImmediates;
    }
    proc(renderBundleEncoder, offset, data, size);
}
static void ThreadDispatchRenderBundleEncoderSetIndexBuffer(WGPURenderBundleEncoder renderBundleEncoder, WGPUBuffer buffer, WGPUIndexFormat format, uint64_t offset, uint64_t size) {
    auto* proc = perThreadProcs.renderBundleEncoderSetIndexBuffer;
    if (!proc) {
        proc = defaultProc.renderBundleEncoderSetIndexBuffer;
    }
    proc(renderBundleEncoder, buffer, format, offset, size);
}
static void ThreadDispatchRenderBundleEncoderSetLabel(WGPURenderBundleEncoder renderBundleEncoder, WGPUStringView label) {
    auto* proc = perThreadProcs.renderBundleEncoderSetLabel;
    if (!proc) {
        proc = defaultProc.renderBundleEncoderSetLabel;
    }
    proc(renderBundleEncoder, label);
}
static void ThreadDispatchRenderBundleEncoderSetPipeline(WGPURenderBundleEncoder renderBundleEncoder, WGPURenderPipeline pipeline) {
    auto* proc = perThreadProcs.renderBundleEncoderSetPipeline;
    if (!proc) {
        proc = defaultProc.renderBundleEncoderSetPipeline;
    }
    proc(renderBundleEncoder, pipeline);
}
static void ThreadDispatchRenderBundleEncoderSetResourceTable(WGPURenderBundleEncoder renderBundleEncoder, WGPUResourceTable table) {
    auto* proc = perThreadProcs.renderBundleEncoderSetResourceTable;
    if (!proc) {
        proc = defaultProc.renderBundleEncoderSetResourceTable;
    }
    proc(renderBundleEncoder, table);
}
static void ThreadDispatchRenderBundleEncoderSetVertexBuffer(WGPURenderBundleEncoder renderBundleEncoder, uint32_t slot, WGPUBuffer buffer, uint64_t offset, uint64_t size) {
    auto* proc = perThreadProcs.renderBundleEncoderSetVertexBuffer;
    if (!proc) {
        proc = defaultProc.renderBundleEncoderSetVertexBuffer;
    }
    proc(renderBundleEncoder, slot, buffer, offset, size);
}
static void ThreadDispatchRenderBundleEncoderAddRef(WGPURenderBundleEncoder renderBundleEncoder) {
    auto* proc = perThreadProcs.renderBundleEncoderAddRef;
    if (!proc) {
        proc = defaultProc.renderBundleEncoderAddRef;
    }
    proc(renderBundleEncoder);
}
static void ThreadDispatchRenderBundleEncoderRelease(WGPURenderBundleEncoder renderBundleEncoder) {
    auto* proc = perThreadProcs.renderBundleEncoderRelease;
    if (!proc) {
        proc = defaultProc.renderBundleEncoderRelease;
    }
    proc(renderBundleEncoder);
}
static void ThreadDispatchRenderPassEncoderBeginOcclusionQuery(WGPURenderPassEncoder renderPassEncoder, uint32_t queryIndex) {
    auto* proc = perThreadProcs.renderPassEncoderBeginOcclusionQuery;
    if (!proc) {
        proc = defaultProc.renderPassEncoderBeginOcclusionQuery;
    }
    proc(renderPassEncoder, queryIndex);
}
static void ThreadDispatchRenderPassEncoderDraw(WGPURenderPassEncoder renderPassEncoder, uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance) {
    auto* proc = perThreadProcs.renderPassEncoderDraw;
    if (!proc) {
        proc = defaultProc.renderPassEncoderDraw;
    }
    proc(renderPassEncoder, vertexCount, instanceCount, firstVertex, firstInstance);
}
static void ThreadDispatchRenderPassEncoderDrawIndexed(WGPURenderPassEncoder renderPassEncoder, uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, int32_t baseVertex, uint32_t firstInstance) {
    auto* proc = perThreadProcs.renderPassEncoderDrawIndexed;
    if (!proc) {
        proc = defaultProc.renderPassEncoderDrawIndexed;
    }
    proc(renderPassEncoder, indexCount, instanceCount, firstIndex, baseVertex, firstInstance);
}
static void ThreadDispatchRenderPassEncoderDrawIndexedIndirect(WGPURenderPassEncoder renderPassEncoder, WGPUBuffer indirectBuffer, uint64_t indirectOffset) {
    auto* proc = perThreadProcs.renderPassEncoderDrawIndexedIndirect;
    if (!proc) {
        proc = defaultProc.renderPassEncoderDrawIndexedIndirect;
    }
    proc(renderPassEncoder, indirectBuffer, indirectOffset);
}
static void ThreadDispatchRenderPassEncoderDrawIndirect(WGPURenderPassEncoder renderPassEncoder, WGPUBuffer indirectBuffer, uint64_t indirectOffset) {
    auto* proc = perThreadProcs.renderPassEncoderDrawIndirect;
    if (!proc) {
        proc = defaultProc.renderPassEncoderDrawIndirect;
    }
    proc(renderPassEncoder, indirectBuffer, indirectOffset);
}
static void ThreadDispatchRenderPassEncoderEnd(WGPURenderPassEncoder renderPassEncoder) {
    auto* proc = perThreadProcs.renderPassEncoderEnd;
    if (!proc) {
        proc = defaultProc.renderPassEncoderEnd;
    }
    proc(renderPassEncoder);
}
static void ThreadDispatchRenderPassEncoderEndOcclusionQuery(WGPURenderPassEncoder renderPassEncoder) {
    auto* proc = perThreadProcs.renderPassEncoderEndOcclusionQuery;
    if (!proc) {
        proc = defaultProc.renderPassEncoderEndOcclusionQuery;
    }
    proc(renderPassEncoder);
}
static void ThreadDispatchRenderPassEncoderExecuteBundles(WGPURenderPassEncoder renderPassEncoder, size_t bundleCount, WGPURenderBundle const * bundles) {
    auto* proc = perThreadProcs.renderPassEncoderExecuteBundles;
    if (!proc) {
        proc = defaultProc.renderPassEncoderExecuteBundles;
    }
    proc(renderPassEncoder, bundleCount, bundles);
}
static void ThreadDispatchRenderPassEncoderInsertDebugMarker(WGPURenderPassEncoder renderPassEncoder, WGPUStringView markerLabel) {
    auto* proc = perThreadProcs.renderPassEncoderInsertDebugMarker;
    if (!proc) {
        proc = defaultProc.renderPassEncoderInsertDebugMarker;
    }
    proc(renderPassEncoder, markerLabel);
}
static void ThreadDispatchRenderPassEncoderMultiDrawIndexedIndirect(WGPURenderPassEncoder renderPassEncoder, WGPUBuffer indirectBuffer, uint64_t indirectOffset, uint32_t maxDrawCount, WGPUBuffer drawCountBuffer, uint64_t drawCountBufferOffset) {
    auto* proc = perThreadProcs.renderPassEncoderMultiDrawIndexedIndirect;
    if (!proc) {
        proc = defaultProc.renderPassEncoderMultiDrawIndexedIndirect;
    }
    proc(renderPassEncoder, indirectBuffer, indirectOffset, maxDrawCount, drawCountBuffer, drawCountBufferOffset);
}
static void ThreadDispatchRenderPassEncoderMultiDrawIndirect(WGPURenderPassEncoder renderPassEncoder, WGPUBuffer indirectBuffer, uint64_t indirectOffset, uint32_t maxDrawCount, WGPUBuffer drawCountBuffer, uint64_t drawCountBufferOffset) {
    auto* proc = perThreadProcs.renderPassEncoderMultiDrawIndirect;
    if (!proc) {
        proc = defaultProc.renderPassEncoderMultiDrawIndirect;
    }
    proc(renderPassEncoder, indirectBuffer, indirectOffset, maxDrawCount, drawCountBuffer, drawCountBufferOffset);
}
static void ThreadDispatchRenderPassEncoderPixelLocalStorageBarrier(WGPURenderPassEncoder renderPassEncoder) {
    auto* proc = perThreadProcs.renderPassEncoderPixelLocalStorageBarrier;
    if (!proc) {
        proc = defaultProc.renderPassEncoderPixelLocalStorageBarrier;
    }
    proc(renderPassEncoder);
}
static void ThreadDispatchRenderPassEncoderPopDebugGroup(WGPURenderPassEncoder renderPassEncoder) {
    auto* proc = perThreadProcs.renderPassEncoderPopDebugGroup;
    if (!proc) {
        proc = defaultProc.renderPassEncoderPopDebugGroup;
    }
    proc(renderPassEncoder);
}
static void ThreadDispatchRenderPassEncoderPushDebugGroup(WGPURenderPassEncoder renderPassEncoder, WGPUStringView groupLabel) {
    auto* proc = perThreadProcs.renderPassEncoderPushDebugGroup;
    if (!proc) {
        proc = defaultProc.renderPassEncoderPushDebugGroup;
    }
    proc(renderPassEncoder, groupLabel);
}
static void ThreadDispatchRenderPassEncoderSetBindGroup(WGPURenderPassEncoder renderPassEncoder, uint32_t groupIndex, WGPUBindGroup group, size_t dynamicOffsetCount, uint32_t const * dynamicOffsets) {
    auto* proc = perThreadProcs.renderPassEncoderSetBindGroup;
    if (!proc) {
        proc = defaultProc.renderPassEncoderSetBindGroup;
    }
    proc(renderPassEncoder, groupIndex, group, dynamicOffsetCount, dynamicOffsets);
}
static void ThreadDispatchRenderPassEncoderSetBlendConstant(WGPURenderPassEncoder renderPassEncoder, WGPUColor const * color) {
    auto* proc = perThreadProcs.renderPassEncoderSetBlendConstant;
    if (!proc) {
        proc = defaultProc.renderPassEncoderSetBlendConstant;
    }
    proc(renderPassEncoder, color);
}
static void ThreadDispatchRenderPassEncoderSetImmediates(WGPURenderPassEncoder renderPassEncoder, uint32_t offset, void const * data, size_t size) {
    auto* proc = perThreadProcs.renderPassEncoderSetImmediates;
    if (!proc) {
        proc = defaultProc.renderPassEncoderSetImmediates;
    }
    proc(renderPassEncoder, offset, data, size);
}
static void ThreadDispatchRenderPassEncoderSetIndexBuffer(WGPURenderPassEncoder renderPassEncoder, WGPUBuffer buffer, WGPUIndexFormat format, uint64_t offset, uint64_t size) {
    auto* proc = perThreadProcs.renderPassEncoderSetIndexBuffer;
    if (!proc) {
        proc = defaultProc.renderPassEncoderSetIndexBuffer;
    }
    proc(renderPassEncoder, buffer, format, offset, size);
}
static void ThreadDispatchRenderPassEncoderSetLabel(WGPURenderPassEncoder renderPassEncoder, WGPUStringView label) {
    auto* proc = perThreadProcs.renderPassEncoderSetLabel;
    if (!proc) {
        proc = defaultProc.renderPassEncoderSetLabel;
    }
    proc(renderPassEncoder, label);
}
static void ThreadDispatchRenderPassEncoderSetPipeline(WGPURenderPassEncoder renderPassEncoder, WGPURenderPipeline pipeline) {
    auto* proc = perThreadProcs.renderPassEncoderSetPipeline;
    if (!proc) {
        proc = defaultProc.renderPassEncoderSetPipeline;
    }
    proc(renderPassEncoder, pipeline);
}
static void ThreadDispatchRenderPassEncoderSetResourceTable(WGPURenderPassEncoder renderPassEncoder, WGPUResourceTable table) {
    auto* proc = perThreadProcs.renderPassEncoderSetResourceTable;
    if (!proc) {
        proc = defaultProc.renderPassEncoderSetResourceTable;
    }
    proc(renderPassEncoder, table);
}
static void ThreadDispatchRenderPassEncoderSetScissorRect(WGPURenderPassEncoder renderPassEncoder, uint32_t x, uint32_t y, uint32_t width, uint32_t height) {
    auto* proc = perThreadProcs.renderPassEncoderSetScissorRect;
    if (!proc) {
        proc = defaultProc.renderPassEncoderSetScissorRect;
    }
    proc(renderPassEncoder, x, y, width, height);
}
static void ThreadDispatchRenderPassEncoderSetStencilReference(WGPURenderPassEncoder renderPassEncoder, uint32_t reference) {
    auto* proc = perThreadProcs.renderPassEncoderSetStencilReference;
    if (!proc) {
        proc = defaultProc.renderPassEncoderSetStencilReference;
    }
    proc(renderPassEncoder, reference);
}
static void ThreadDispatchRenderPassEncoderSetVertexBuffer(WGPURenderPassEncoder renderPassEncoder, uint32_t slot, WGPUBuffer buffer, uint64_t offset, uint64_t size) {
    auto* proc = perThreadProcs.renderPassEncoderSetVertexBuffer;
    if (!proc) {
        proc = defaultProc.renderPassEncoderSetVertexBuffer;
    }
    proc(renderPassEncoder, slot, buffer, offset, size);
}
static void ThreadDispatchRenderPassEncoderSetViewport(WGPURenderPassEncoder renderPassEncoder, float x, float y, float width, float height, float minDepth, float maxDepth) {
    auto* proc = perThreadProcs.renderPassEncoderSetViewport;
    if (!proc) {
        proc = defaultProc.renderPassEncoderSetViewport;
    }
    proc(renderPassEncoder, x, y, width, height, minDepth, maxDepth);
}
static void ThreadDispatchRenderPassEncoderWriteTimestamp(WGPURenderPassEncoder renderPassEncoder, WGPUQuerySet querySet, uint32_t queryIndex) {
    auto* proc = perThreadProcs.renderPassEncoderWriteTimestamp;
    if (!proc) {
        proc = defaultProc.renderPassEncoderWriteTimestamp;
    }
    proc(renderPassEncoder, querySet, queryIndex);
}
static void ThreadDispatchRenderPassEncoderAddRef(WGPURenderPassEncoder renderPassEncoder) {
    auto* proc = perThreadProcs.renderPassEncoderAddRef;
    if (!proc) {
        proc = defaultProc.renderPassEncoderAddRef;
    }
    proc(renderPassEncoder);
}
static void ThreadDispatchRenderPassEncoderRelease(WGPURenderPassEncoder renderPassEncoder) {
    auto* proc = perThreadProcs.renderPassEncoderRelease;
    if (!proc) {
        proc = defaultProc.renderPassEncoderRelease;
    }
    proc(renderPassEncoder);
}
static WGPUBindGroupLayout ThreadDispatchRenderPipelineGetBindGroupLayout(WGPURenderPipeline renderPipeline, uint32_t groupIndex) {
    auto* proc = perThreadProcs.renderPipelineGetBindGroupLayout;
    if (!proc) {
        proc = defaultProc.renderPipelineGetBindGroupLayout;
    }
return     proc(renderPipeline, groupIndex);
}
static void ThreadDispatchRenderPipelineSetLabel(WGPURenderPipeline renderPipeline, WGPUStringView label) {
    auto* proc = perThreadProcs.renderPipelineSetLabel;
    if (!proc) {
        proc = defaultProc.renderPipelineSetLabel;
    }
    proc(renderPipeline, label);
}
static void ThreadDispatchRenderPipelineAddRef(WGPURenderPipeline renderPipeline) {
    auto* proc = perThreadProcs.renderPipelineAddRef;
    if (!proc) {
        proc = defaultProc.renderPipelineAddRef;
    }
    proc(renderPipeline);
}
static void ThreadDispatchRenderPipelineRelease(WGPURenderPipeline renderPipeline) {
    auto* proc = perThreadProcs.renderPipelineRelease;
    if (!proc) {
        proc = defaultProc.renderPipelineRelease;
    }
    proc(renderPipeline);
}
static void ThreadDispatchResourceTableDestroy(WGPUResourceTable resourceTable) {
    auto* proc = perThreadProcs.resourceTableDestroy;
    if (!proc) {
        proc = defaultProc.resourceTableDestroy;
    }
    proc(resourceTable);
}
static uint32_t ThreadDispatchResourceTableGetSize(WGPUResourceTable resourceTable) {
    auto* proc = perThreadProcs.resourceTableGetSize;
    if (!proc) {
        proc = defaultProc.resourceTableGetSize;
    }
return     proc(resourceTable);
}
static uint32_t ThreadDispatchResourceTableInsert(WGPUResourceTable resourceTable, WGPUBindingResource const * resource) {
    auto* proc = perThreadProcs.resourceTableInsert;
    if (!proc) {
        proc = defaultProc.resourceTableInsert;
    }
return     proc(resourceTable, resource);
}
static WGPUStatus ThreadDispatchResourceTableRemove(WGPUResourceTable resourceTable, uint32_t slot) {
    auto* proc = perThreadProcs.resourceTableRemove;
    if (!proc) {
        proc = defaultProc.resourceTableRemove;
    }
return     proc(resourceTable, slot);
}
static void ThreadDispatchResourceTableSetLabel(WGPUResourceTable resourceTable, WGPUStringView label) {
    auto* proc = perThreadProcs.resourceTableSetLabel;
    if (!proc) {
        proc = defaultProc.resourceTableSetLabel;
    }
    proc(resourceTable, label);
}
static WGPUStatus ThreadDispatchResourceTableUpdate(WGPUResourceTable resourceTable, uint32_t slot, WGPUBindingResource const * resource) {
    auto* proc = perThreadProcs.resourceTableUpdate;
    if (!proc) {
        proc = defaultProc.resourceTableUpdate;
    }
return     proc(resourceTable, slot, resource);
}
static void ThreadDispatchResourceTableAddRef(WGPUResourceTable resourceTable) {
    auto* proc = perThreadProcs.resourceTableAddRef;
    if (!proc) {
        proc = defaultProc.resourceTableAddRef;
    }
    proc(resourceTable);
}
static void ThreadDispatchResourceTableRelease(WGPUResourceTable resourceTable) {
    auto* proc = perThreadProcs.resourceTableRelease;
    if (!proc) {
        proc = defaultProc.resourceTableRelease;
    }
    proc(resourceTable);
}
static void ThreadDispatchSamplerSetLabel(WGPUSampler sampler, WGPUStringView label) {
    auto* proc = perThreadProcs.samplerSetLabel;
    if (!proc) {
        proc = defaultProc.samplerSetLabel;
    }
    proc(sampler, label);
}
static void ThreadDispatchSamplerAddRef(WGPUSampler sampler) {
    auto* proc = perThreadProcs.samplerAddRef;
    if (!proc) {
        proc = defaultProc.samplerAddRef;
    }
    proc(sampler);
}
static void ThreadDispatchSamplerRelease(WGPUSampler sampler) {
    auto* proc = perThreadProcs.samplerRelease;
    if (!proc) {
        proc = defaultProc.samplerRelease;
    }
    proc(sampler);
}
static WGPUFuture ThreadDispatchShaderModuleGetCompilationInfo(WGPUShaderModule shaderModule, WGPUCompilationInfoCallbackInfo callbackInfo) {
    auto* proc = perThreadProcs.shaderModuleGetCompilationInfo;
    if (!proc) {
        proc = defaultProc.shaderModuleGetCompilationInfo;
    }
return     proc(shaderModule, callbackInfo);
}
static void ThreadDispatchShaderModuleSetLabel(WGPUShaderModule shaderModule, WGPUStringView label) {
    auto* proc = perThreadProcs.shaderModuleSetLabel;
    if (!proc) {
        proc = defaultProc.shaderModuleSetLabel;
    }
    proc(shaderModule, label);
}
static void ThreadDispatchShaderModuleAddRef(WGPUShaderModule shaderModule) {
    auto* proc = perThreadProcs.shaderModuleAddRef;
    if (!proc) {
        proc = defaultProc.shaderModuleAddRef;
    }
    proc(shaderModule);
}
static void ThreadDispatchShaderModuleRelease(WGPUShaderModule shaderModule) {
    auto* proc = perThreadProcs.shaderModuleRelease;
    if (!proc) {
        proc = defaultProc.shaderModuleRelease;
    }
    proc(shaderModule);
}
static WGPUStatus ThreadDispatchSharedBufferMemoryBeginAccess(WGPUSharedBufferMemory sharedBufferMemory, WGPUBuffer buffer, WGPUSharedBufferMemoryBeginAccessDescriptor const * descriptor) {
    auto* proc = perThreadProcs.sharedBufferMemoryBeginAccess;
    if (!proc) {
        proc = defaultProc.sharedBufferMemoryBeginAccess;
    }
return     proc(sharedBufferMemory, buffer, descriptor);
}
static WGPUBuffer ThreadDispatchSharedBufferMemoryCreateBuffer(WGPUSharedBufferMemory sharedBufferMemory, WGPUBufferDescriptor const * descriptor) {
    auto* proc = perThreadProcs.sharedBufferMemoryCreateBuffer;
    if (!proc) {
        proc = defaultProc.sharedBufferMemoryCreateBuffer;
    }
return     proc(sharedBufferMemory, descriptor);
}
static WGPUStatus ThreadDispatchSharedBufferMemoryEndAccess(WGPUSharedBufferMemory sharedBufferMemory, WGPUBuffer buffer, WGPUSharedBufferMemoryEndAccessState * descriptor) {
    auto* proc = perThreadProcs.sharedBufferMemoryEndAccess;
    if (!proc) {
        proc = defaultProc.sharedBufferMemoryEndAccess;
    }
return     proc(sharedBufferMemory, buffer, descriptor);
}
static WGPUStatus ThreadDispatchSharedBufferMemoryGetProperties(WGPUSharedBufferMemory sharedBufferMemory, WGPUSharedBufferMemoryProperties * properties) {
    auto* proc = perThreadProcs.sharedBufferMemoryGetProperties;
    if (!proc) {
        proc = defaultProc.sharedBufferMemoryGetProperties;
    }
return     proc(sharedBufferMemory, properties);
}
static WGPUBool ThreadDispatchSharedBufferMemoryIsDeviceLost(WGPUSharedBufferMemory sharedBufferMemory) {
    auto* proc = perThreadProcs.sharedBufferMemoryIsDeviceLost;
    if (!proc) {
        proc = defaultProc.sharedBufferMemoryIsDeviceLost;
    }
return     proc(sharedBufferMemory);
}
static void ThreadDispatchSharedBufferMemorySetLabel(WGPUSharedBufferMemory sharedBufferMemory, WGPUStringView label) {
    auto* proc = perThreadProcs.sharedBufferMemorySetLabel;
    if (!proc) {
        proc = defaultProc.sharedBufferMemorySetLabel;
    }
    proc(sharedBufferMemory, label);
}
static void ThreadDispatchSharedBufferMemoryAddRef(WGPUSharedBufferMemory sharedBufferMemory) {
    auto* proc = perThreadProcs.sharedBufferMemoryAddRef;
    if (!proc) {
        proc = defaultProc.sharedBufferMemoryAddRef;
    }
    proc(sharedBufferMemory);
}
static void ThreadDispatchSharedBufferMemoryRelease(WGPUSharedBufferMemory sharedBufferMemory) {
    auto* proc = perThreadProcs.sharedBufferMemoryRelease;
    if (!proc) {
        proc = defaultProc.sharedBufferMemoryRelease;
    }
    proc(sharedBufferMemory);
}
static void ThreadDispatchSharedBufferMemoryEndAccessStateFreeMembers(WGPUSharedBufferMemoryEndAccessState sharedBufferMemoryEndAccessState) {
    auto* proc = perThreadProcs.sharedBufferMemoryEndAccessStateFreeMembers;
    if (!proc) {
        proc = defaultProc.sharedBufferMemoryEndAccessStateFreeMembers;
    }
    proc(sharedBufferMemoryEndAccessState);
}
static void ThreadDispatchSharedFenceExportInfo(WGPUSharedFence sharedFence, WGPUSharedFenceExportInfo * info) {
    auto* proc = perThreadProcs.sharedFenceExportInfo;
    if (!proc) {
        proc = defaultProc.sharedFenceExportInfo;
    }
    proc(sharedFence, info);
}
static void ThreadDispatchSharedFenceSetLabel(WGPUSharedFence sharedFence, WGPUStringView label) {
    auto* proc = perThreadProcs.sharedFenceSetLabel;
    if (!proc) {
        proc = defaultProc.sharedFenceSetLabel;
    }
    proc(sharedFence, label);
}
static void ThreadDispatchSharedFenceAddRef(WGPUSharedFence sharedFence) {
    auto* proc = perThreadProcs.sharedFenceAddRef;
    if (!proc) {
        proc = defaultProc.sharedFenceAddRef;
    }
    proc(sharedFence);
}
static void ThreadDispatchSharedFenceRelease(WGPUSharedFence sharedFence) {
    auto* proc = perThreadProcs.sharedFenceRelease;
    if (!proc) {
        proc = defaultProc.sharedFenceRelease;
    }
    proc(sharedFence);
}
static WGPUStatus ThreadDispatchSharedTextureMemoryBeginAccess(WGPUSharedTextureMemory sharedTextureMemory, WGPUTexture texture, WGPUSharedTextureMemoryBeginAccessDescriptor const * descriptor) {
    auto* proc = perThreadProcs.sharedTextureMemoryBeginAccess;
    if (!proc) {
        proc = defaultProc.sharedTextureMemoryBeginAccess;
    }
return     proc(sharedTextureMemory, texture, descriptor);
}
static WGPUTexture ThreadDispatchSharedTextureMemoryCreateTexture(WGPUSharedTextureMemory sharedTextureMemory, WGPUTextureDescriptor const * descriptor) {
    auto* proc = perThreadProcs.sharedTextureMemoryCreateTexture;
    if (!proc) {
        proc = defaultProc.sharedTextureMemoryCreateTexture;
    }
return     proc(sharedTextureMemory, descriptor);
}
static WGPUStatus ThreadDispatchSharedTextureMemoryEndAccess(WGPUSharedTextureMemory sharedTextureMemory, WGPUTexture texture, WGPUSharedTextureMemoryEndAccessState * descriptor) {
    auto* proc = perThreadProcs.sharedTextureMemoryEndAccess;
    if (!proc) {
        proc = defaultProc.sharedTextureMemoryEndAccess;
    }
return     proc(sharedTextureMemory, texture, descriptor);
}
static WGPUStatus ThreadDispatchSharedTextureMemoryGetProperties(WGPUSharedTextureMemory sharedTextureMemory, WGPUSharedTextureMemoryProperties * properties) {
    auto* proc = perThreadProcs.sharedTextureMemoryGetProperties;
    if (!proc) {
        proc = defaultProc.sharedTextureMemoryGetProperties;
    }
return     proc(sharedTextureMemory, properties);
}
static WGPUBool ThreadDispatchSharedTextureMemoryIsDeviceLost(WGPUSharedTextureMemory sharedTextureMemory) {
    auto* proc = perThreadProcs.sharedTextureMemoryIsDeviceLost;
    if (!proc) {
        proc = defaultProc.sharedTextureMemoryIsDeviceLost;
    }
return     proc(sharedTextureMemory);
}
static void ThreadDispatchSharedTextureMemorySetLabel(WGPUSharedTextureMemory sharedTextureMemory, WGPUStringView label) {
    auto* proc = perThreadProcs.sharedTextureMemorySetLabel;
    if (!proc) {
        proc = defaultProc.sharedTextureMemorySetLabel;
    }
    proc(sharedTextureMemory, label);
}
static void ThreadDispatchSharedTextureMemoryAddRef(WGPUSharedTextureMemory sharedTextureMemory) {
    auto* proc = perThreadProcs.sharedTextureMemoryAddRef;
    if (!proc) {
        proc = defaultProc.sharedTextureMemoryAddRef;
    }
    proc(sharedTextureMemory);
}
static void ThreadDispatchSharedTextureMemoryRelease(WGPUSharedTextureMemory sharedTextureMemory) {
    auto* proc = perThreadProcs.sharedTextureMemoryRelease;
    if (!proc) {
        proc = defaultProc.sharedTextureMemoryRelease;
    }
    proc(sharedTextureMemory);
}
static void ThreadDispatchSharedTextureMemoryEndAccessStateFreeMembers(WGPUSharedTextureMemoryEndAccessState sharedTextureMemoryEndAccessState) {
    auto* proc = perThreadProcs.sharedTextureMemoryEndAccessStateFreeMembers;
    if (!proc) {
        proc = defaultProc.sharedTextureMemoryEndAccessStateFreeMembers;
    }
    proc(sharedTextureMemoryEndAccessState);
}
static void ThreadDispatchSupportedFeaturesFreeMembers(WGPUSupportedFeatures supportedFeatures) {
    auto* proc = perThreadProcs.supportedFeaturesFreeMembers;
    if (!proc) {
        proc = defaultProc.supportedFeaturesFreeMembers;
    }
    proc(supportedFeatures);
}
static void ThreadDispatchSupportedInstanceFeaturesFreeMembers(WGPUSupportedInstanceFeatures supportedInstanceFeatures) {
    auto* proc = perThreadProcs.supportedInstanceFeaturesFreeMembers;
    if (!proc) {
        proc = defaultProc.supportedInstanceFeaturesFreeMembers;
    }
    proc(supportedInstanceFeatures);
}
static void ThreadDispatchSupportedWGSLLanguageFeaturesFreeMembers(WGPUSupportedWGSLLanguageFeatures supportedWGSLLanguageFeatures) {
    auto* proc = perThreadProcs.supportedWGSLLanguageFeaturesFreeMembers;
    if (!proc) {
        proc = defaultProc.supportedWGSLLanguageFeaturesFreeMembers;
    }
    proc(supportedWGSLLanguageFeatures);
}
static void ThreadDispatchSurfaceConfigure(WGPUSurface surface, WGPUSurfaceConfiguration const * config) {
    auto* proc = perThreadProcs.surfaceConfigure;
    if (!proc) {
        proc = defaultProc.surfaceConfigure;
    }
    proc(surface, config);
}
static WGPUStatus ThreadDispatchSurfaceGetCapabilities(WGPUSurface surface, WGPUAdapter adapter, WGPUSurfaceCapabilities * capabilities) {
    auto* proc = perThreadProcs.surfaceGetCapabilities;
    if (!proc) {
        proc = defaultProc.surfaceGetCapabilities;
    }
return     proc(surface, adapter, capabilities);
}
static void ThreadDispatchSurfaceGetCurrentTexture(WGPUSurface surface, WGPUSurfaceTexture * surfaceTexture) {
    auto* proc = perThreadProcs.surfaceGetCurrentTexture;
    if (!proc) {
        proc = defaultProc.surfaceGetCurrentTexture;
    }
    proc(surface, surfaceTexture);
}
static WGPUStatus ThreadDispatchSurfacePresent(WGPUSurface surface) {
    auto* proc = perThreadProcs.surfacePresent;
    if (!proc) {
        proc = defaultProc.surfacePresent;
    }
return     proc(surface);
}
static void ThreadDispatchSurfaceSetLabel(WGPUSurface surface, WGPUStringView label) {
    auto* proc = perThreadProcs.surfaceSetLabel;
    if (!proc) {
        proc = defaultProc.surfaceSetLabel;
    }
    proc(surface, label);
}
static void ThreadDispatchSurfaceUnconfigure(WGPUSurface surface) {
    auto* proc = perThreadProcs.surfaceUnconfigure;
    if (!proc) {
        proc = defaultProc.surfaceUnconfigure;
    }
    proc(surface);
}
static void ThreadDispatchSurfaceAddRef(WGPUSurface surface) {
    auto* proc = perThreadProcs.surfaceAddRef;
    if (!proc) {
        proc = defaultProc.surfaceAddRef;
    }
    proc(surface);
}
static void ThreadDispatchSurfaceRelease(WGPUSurface surface) {
    auto* proc = perThreadProcs.surfaceRelease;
    if (!proc) {
        proc = defaultProc.surfaceRelease;
    }
    proc(surface);
}
static void ThreadDispatchSurfaceCapabilitiesFreeMembers(WGPUSurfaceCapabilities surfaceCapabilities) {
    auto* proc = perThreadProcs.surfaceCapabilitiesFreeMembers;
    if (!proc) {
        proc = defaultProc.surfaceCapabilitiesFreeMembers;
    }
    proc(surfaceCapabilities);
}
static void ThreadDispatchTexelBufferViewSetLabel(WGPUTexelBufferView texelBufferView, WGPUStringView label) {
    auto* proc = perThreadProcs.texelBufferViewSetLabel;
    if (!proc) {
        proc = defaultProc.texelBufferViewSetLabel;
    }
    proc(texelBufferView, label);
}
static void ThreadDispatchTexelBufferViewAddRef(WGPUTexelBufferView texelBufferView) {
    auto* proc = perThreadProcs.texelBufferViewAddRef;
    if (!proc) {
        proc = defaultProc.texelBufferViewAddRef;
    }
    proc(texelBufferView);
}
static void ThreadDispatchTexelBufferViewRelease(WGPUTexelBufferView texelBufferView) {
    auto* proc = perThreadProcs.texelBufferViewRelease;
    if (!proc) {
        proc = defaultProc.texelBufferViewRelease;
    }
    proc(texelBufferView);
}
static WGPUTextureView ThreadDispatchTextureCreateErrorView(WGPUTexture texture, WGPUTextureViewDescriptor const * descriptor) {
    auto* proc = perThreadProcs.textureCreateErrorView;
    if (!proc) {
        proc = defaultProc.textureCreateErrorView;
    }
return     proc(texture, descriptor);
}
static WGPUTextureView ThreadDispatchTextureCreateView(WGPUTexture texture, WGPUTextureViewDescriptor const * descriptor) {
    auto* proc = perThreadProcs.textureCreateView;
    if (!proc) {
        proc = defaultProc.textureCreateView;
    }
return     proc(texture, descriptor);
}
static void ThreadDispatchTextureDestroy(WGPUTexture texture) {
    auto* proc = perThreadProcs.textureDestroy;
    if (!proc) {
        proc = defaultProc.textureDestroy;
    }
    proc(texture);
}
static uint32_t ThreadDispatchTextureGetDepthOrArrayLayers(WGPUTexture texture) {
    auto* proc = perThreadProcs.textureGetDepthOrArrayLayers;
    if (!proc) {
        proc = defaultProc.textureGetDepthOrArrayLayers;
    }
return     proc(texture);
}
static WGPUTextureDimension ThreadDispatchTextureGetDimension(WGPUTexture texture) {
    auto* proc = perThreadProcs.textureGetDimension;
    if (!proc) {
        proc = defaultProc.textureGetDimension;
    }
return     proc(texture);
}
static WGPUTextureFormat ThreadDispatchTextureGetFormat(WGPUTexture texture) {
    auto* proc = perThreadProcs.textureGetFormat;
    if (!proc) {
        proc = defaultProc.textureGetFormat;
    }
return     proc(texture);
}
static uint32_t ThreadDispatchTextureGetHeight(WGPUTexture texture) {
    auto* proc = perThreadProcs.textureGetHeight;
    if (!proc) {
        proc = defaultProc.textureGetHeight;
    }
return     proc(texture);
}
static uint32_t ThreadDispatchTextureGetMipLevelCount(WGPUTexture texture) {
    auto* proc = perThreadProcs.textureGetMipLevelCount;
    if (!proc) {
        proc = defaultProc.textureGetMipLevelCount;
    }
return     proc(texture);
}
static uint32_t ThreadDispatchTextureGetSampleCount(WGPUTexture texture) {
    auto* proc = perThreadProcs.textureGetSampleCount;
    if (!proc) {
        proc = defaultProc.textureGetSampleCount;
    }
return     proc(texture);
}
static WGPUTextureViewDimension ThreadDispatchTextureGetTextureBindingViewDimension(WGPUTexture texture) {
    auto* proc = perThreadProcs.textureGetTextureBindingViewDimension;
    if (!proc) {
        proc = defaultProc.textureGetTextureBindingViewDimension;
    }
return     proc(texture);
}
static WGPUTextureUsage ThreadDispatchTextureGetUsage(WGPUTexture texture) {
    auto* proc = perThreadProcs.textureGetUsage;
    if (!proc) {
        proc = defaultProc.textureGetUsage;
    }
return     proc(texture);
}
static uint32_t ThreadDispatchTextureGetWidth(WGPUTexture texture) {
    auto* proc = perThreadProcs.textureGetWidth;
    if (!proc) {
        proc = defaultProc.textureGetWidth;
    }
return     proc(texture);
}
static void ThreadDispatchTextureSetLabel(WGPUTexture texture, WGPUStringView label) {
    auto* proc = perThreadProcs.textureSetLabel;
    if (!proc) {
        proc = defaultProc.textureSetLabel;
    }
    proc(texture, label);
}
static void ThreadDispatchTextureSetOwnershipForMemoryDump(WGPUTexture texture, uint64_t ownerGuid) {
    auto* proc = perThreadProcs.textureSetOwnershipForMemoryDump;
    if (!proc) {
        proc = defaultProc.textureSetOwnershipForMemoryDump;
    }
    proc(texture, ownerGuid);
}
static void ThreadDispatchTextureAddRef(WGPUTexture texture) {
    auto* proc = perThreadProcs.textureAddRef;
    if (!proc) {
        proc = defaultProc.textureAddRef;
    }
    proc(texture);
}
static void ThreadDispatchTextureRelease(WGPUTexture texture) {
    auto* proc = perThreadProcs.textureRelease;
    if (!proc) {
        proc = defaultProc.textureRelease;
    }
    proc(texture);
}
static void ThreadDispatchTextureViewSetLabel(WGPUTextureView textureView, WGPUStringView label) {
    auto* proc = perThreadProcs.textureViewSetLabel;
    if (!proc) {
        proc = defaultProc.textureViewSetLabel;
    }
    proc(textureView, label);
}
static void ThreadDispatchTextureViewAddRef(WGPUTextureView textureView) {
    auto* proc = perThreadProcs.textureViewAddRef;
    if (!proc) {
        proc = defaultProc.textureViewAddRef;
    }
    proc(textureView);
}
static void ThreadDispatchTextureViewRelease(WGPUTextureView textureView) {
    auto* proc = perThreadProcs.textureViewRelease;
    if (!proc) {
        proc = defaultProc.textureViewRelease;
    }
    proc(textureView);
}

    constexpr DawnProcTable MakeThreadDispatchProcTable() {
        DawnProcTable procs = {};
        std::ranges::copy(dawn::kDawnVersion, procs.version);
        procs.createInstance = ThreadDispatchCreateInstance;
        procs.getInstanceFeatures = ThreadDispatchGetInstanceFeatures;
        procs.getInstanceLimits = ThreadDispatchGetInstanceLimits;
        procs.hasInstanceFeature = ThreadDispatchHasInstanceFeature;
        procs.getProcAddress = ThreadDispatchGetProcAddress;
        procs.adapterCreateDevice = ThreadDispatchAdapterCreateDevice;
        procs.adapterGetFeatures = ThreadDispatchAdapterGetFeatures;
        procs.adapterGetFormatCapabilities = ThreadDispatchAdapterGetFormatCapabilities;
        procs.adapterGetInfo = ThreadDispatchAdapterGetInfo;
        procs.adapterGetInstance = ThreadDispatchAdapterGetInstance;
        procs.adapterGetLimits = ThreadDispatchAdapterGetLimits;
        procs.adapterHasFeature = ThreadDispatchAdapterHasFeature;
        procs.adapterRequestDevice = ThreadDispatchAdapterRequestDevice;
        procs.adapterAddRef = ThreadDispatchAdapterAddRef;
        procs.adapterRelease = ThreadDispatchAdapterRelease;
        procs.adapterInfoFreeMembers = ThreadDispatchAdapterInfoFreeMembers;
        procs.adapterPropertiesMemoryHeapsFreeMembers = ThreadDispatchAdapterPropertiesMemoryHeapsFreeMembers;
        procs.adapterPropertiesSubgroupMatrixConfigsFreeMembers = ThreadDispatchAdapterPropertiesSubgroupMatrixConfigsFreeMembers;
        procs.bindGroupSetLabel = ThreadDispatchBindGroupSetLabel;
        procs.bindGroupAddRef = ThreadDispatchBindGroupAddRef;
        procs.bindGroupRelease = ThreadDispatchBindGroupRelease;
        procs.bindGroupLayoutSetLabel = ThreadDispatchBindGroupLayoutSetLabel;
        procs.bindGroupLayoutAddRef = ThreadDispatchBindGroupLayoutAddRef;
        procs.bindGroupLayoutRelease = ThreadDispatchBindGroupLayoutRelease;
        procs.bufferCreateTexelView = ThreadDispatchBufferCreateTexelView;
        procs.bufferDestroy = ThreadDispatchBufferDestroy;
        procs.bufferGetConstMappedRange = ThreadDispatchBufferGetConstMappedRange;
        procs.bufferGetMappedRange = ThreadDispatchBufferGetMappedRange;
        procs.bufferGetMapState = ThreadDispatchBufferGetMapState;
        procs.bufferGetSize = ThreadDispatchBufferGetSize;
        procs.bufferGetUsage = ThreadDispatchBufferGetUsage;
        procs.bufferMapAsync = ThreadDispatchBufferMapAsync;
        procs.bufferReadMappedRange = ThreadDispatchBufferReadMappedRange;
        procs.bufferSetLabel = ThreadDispatchBufferSetLabel;
        procs.bufferUnmap = ThreadDispatchBufferUnmap;
        procs.bufferWriteMappedRange = ThreadDispatchBufferWriteMappedRange;
        procs.bufferAddRef = ThreadDispatchBufferAddRef;
        procs.bufferRelease = ThreadDispatchBufferRelease;
        procs.commandBufferSetLabel = ThreadDispatchCommandBufferSetLabel;
        procs.commandBufferAddRef = ThreadDispatchCommandBufferAddRef;
        procs.commandBufferRelease = ThreadDispatchCommandBufferRelease;
        procs.commandEncoderBeginComputePass = ThreadDispatchCommandEncoderBeginComputePass;
        procs.commandEncoderBeginRenderPass = ThreadDispatchCommandEncoderBeginRenderPass;
        procs.commandEncoderClearBuffer = ThreadDispatchCommandEncoderClearBuffer;
        procs.commandEncoderCopyBufferToBuffer = ThreadDispatchCommandEncoderCopyBufferToBuffer;
        procs.commandEncoderCopyBufferToTexture = ThreadDispatchCommandEncoderCopyBufferToTexture;
        procs.commandEncoderCopyTextureToBuffer = ThreadDispatchCommandEncoderCopyTextureToBuffer;
        procs.commandEncoderCopyTextureToTexture = ThreadDispatchCommandEncoderCopyTextureToTexture;
        procs.commandEncoderFinish = ThreadDispatchCommandEncoderFinish;
        procs.commandEncoderInjectValidationError = ThreadDispatchCommandEncoderInjectValidationError;
        procs.commandEncoderInsertDebugMarker = ThreadDispatchCommandEncoderInsertDebugMarker;
        procs.commandEncoderPopDebugGroup = ThreadDispatchCommandEncoderPopDebugGroup;
        procs.commandEncoderPushDebugGroup = ThreadDispatchCommandEncoderPushDebugGroup;
        procs.commandEncoderResolveQuerySet = ThreadDispatchCommandEncoderResolveQuerySet;
        procs.commandEncoderSetLabel = ThreadDispatchCommandEncoderSetLabel;
        procs.commandEncoderWriteBuffer = ThreadDispatchCommandEncoderWriteBuffer;
        procs.commandEncoderWriteTimestamp = ThreadDispatchCommandEncoderWriteTimestamp;
        procs.commandEncoderAddRef = ThreadDispatchCommandEncoderAddRef;
        procs.commandEncoderRelease = ThreadDispatchCommandEncoderRelease;
        procs.computePassEncoderDispatchWorkgroups = ThreadDispatchComputePassEncoderDispatchWorkgroups;
        procs.computePassEncoderDispatchWorkgroupsIndirect = ThreadDispatchComputePassEncoderDispatchWorkgroupsIndirect;
        procs.computePassEncoderEnd = ThreadDispatchComputePassEncoderEnd;
        procs.computePassEncoderInsertDebugMarker = ThreadDispatchComputePassEncoderInsertDebugMarker;
        procs.computePassEncoderPopDebugGroup = ThreadDispatchComputePassEncoderPopDebugGroup;
        procs.computePassEncoderPushDebugGroup = ThreadDispatchComputePassEncoderPushDebugGroup;
        procs.computePassEncoderSetBindGroup = ThreadDispatchComputePassEncoderSetBindGroup;
        procs.computePassEncoderSetImmediates = ThreadDispatchComputePassEncoderSetImmediates;
        procs.computePassEncoderSetLabel = ThreadDispatchComputePassEncoderSetLabel;
        procs.computePassEncoderSetPipeline = ThreadDispatchComputePassEncoderSetPipeline;
        procs.computePassEncoderSetResourceTable = ThreadDispatchComputePassEncoderSetResourceTable;
        procs.computePassEncoderWriteTimestamp = ThreadDispatchComputePassEncoderWriteTimestamp;
        procs.computePassEncoderAddRef = ThreadDispatchComputePassEncoderAddRef;
        procs.computePassEncoderRelease = ThreadDispatchComputePassEncoderRelease;
        procs.computePipelineGetBindGroupLayout = ThreadDispatchComputePipelineGetBindGroupLayout;
        procs.computePipelineSetLabel = ThreadDispatchComputePipelineSetLabel;
        procs.computePipelineAddRef = ThreadDispatchComputePipelineAddRef;
        procs.computePipelineRelease = ThreadDispatchComputePipelineRelease;
        procs.dawnDrmFormatCapabilitiesFreeMembers = ThreadDispatchDawnDrmFormatCapabilitiesFreeMembers;
        procs.deviceCreateBindGroup = ThreadDispatchDeviceCreateBindGroup;
        procs.deviceCreateBindGroupLayout = ThreadDispatchDeviceCreateBindGroupLayout;
        procs.deviceCreateBuffer = ThreadDispatchDeviceCreateBuffer;
        procs.deviceCreateCommandEncoder = ThreadDispatchDeviceCreateCommandEncoder;
        procs.deviceCreateComputePipeline = ThreadDispatchDeviceCreateComputePipeline;
        procs.deviceCreateComputePipelineAsync = ThreadDispatchDeviceCreateComputePipelineAsync;
        procs.deviceCreateErrorBuffer = ThreadDispatchDeviceCreateErrorBuffer;
        procs.deviceCreateErrorComputePipeline = ThreadDispatchDeviceCreateErrorComputePipeline;
        procs.deviceCreateErrorExternalTexture = ThreadDispatchDeviceCreateErrorExternalTexture;
        procs.deviceCreateErrorRenderPipeline = ThreadDispatchDeviceCreateErrorRenderPipeline;
        procs.deviceCreateErrorShaderModule = ThreadDispatchDeviceCreateErrorShaderModule;
        procs.deviceCreateErrorTexture = ThreadDispatchDeviceCreateErrorTexture;
        procs.deviceCreateExternalTexture = ThreadDispatchDeviceCreateExternalTexture;
        procs.deviceCreatePipelineLayout = ThreadDispatchDeviceCreatePipelineLayout;
        procs.deviceCreateQuerySet = ThreadDispatchDeviceCreateQuerySet;
        procs.deviceCreateRenderBundleEncoder = ThreadDispatchDeviceCreateRenderBundleEncoder;
        procs.deviceCreateRenderPipeline = ThreadDispatchDeviceCreateRenderPipeline;
        procs.deviceCreateRenderPipelineAsync = ThreadDispatchDeviceCreateRenderPipelineAsync;
        procs.deviceCreateResourceTable = ThreadDispatchDeviceCreateResourceTable;
        procs.deviceCreateSampler = ThreadDispatchDeviceCreateSampler;
        procs.deviceCreateShaderModule = ThreadDispatchDeviceCreateShaderModule;
        procs.deviceCreateTexture = ThreadDispatchDeviceCreateTexture;
        procs.deviceDestroy = ThreadDispatchDeviceDestroy;
        procs.deviceForceLoss = ThreadDispatchDeviceForceLoss;
        procs.deviceGetAdapter = ThreadDispatchDeviceGetAdapter;
        procs.deviceGetAdapterInfo = ThreadDispatchDeviceGetAdapterInfo;
        procs.deviceGetAHardwareBufferProperties = ThreadDispatchDeviceGetAHardwareBufferProperties;
        procs.deviceGetFeatures = ThreadDispatchDeviceGetFeatures;
        procs.deviceGetLimits = ThreadDispatchDeviceGetLimits;
        procs.deviceGetLostFuture = ThreadDispatchDeviceGetLostFuture;
        procs.deviceGetQueue = ThreadDispatchDeviceGetQueue;
        procs.deviceHasFeature = ThreadDispatchDeviceHasFeature;
        procs.deviceImportSharedBufferMemory = ThreadDispatchDeviceImportSharedBufferMemory;
        procs.deviceImportSharedFence = ThreadDispatchDeviceImportSharedFence;
        procs.deviceImportSharedTextureMemory = ThreadDispatchDeviceImportSharedTextureMemory;
        procs.deviceInjectError = ThreadDispatchDeviceInjectError;
        procs.devicePopErrorScope = ThreadDispatchDevicePopErrorScope;
        procs.devicePushErrorScope = ThreadDispatchDevicePushErrorScope;
        procs.deviceSetLabel = ThreadDispatchDeviceSetLabel;
        procs.deviceSetLoggingCallback = ThreadDispatchDeviceSetLoggingCallback;
        procs.deviceTick = ThreadDispatchDeviceTick;
        procs.deviceValidateTextureDescriptor = ThreadDispatchDeviceValidateTextureDescriptor;
        procs.deviceAddRef = ThreadDispatchDeviceAddRef;
        procs.deviceRelease = ThreadDispatchDeviceRelease;
        procs.externalTextureDestroy = ThreadDispatchExternalTextureDestroy;
        procs.externalTextureExpire = ThreadDispatchExternalTextureExpire;
        procs.externalTextureRefresh = ThreadDispatchExternalTextureRefresh;
        procs.externalTextureSetLabel = ThreadDispatchExternalTextureSetLabel;
        procs.externalTextureAddRef = ThreadDispatchExternalTextureAddRef;
        procs.externalTextureRelease = ThreadDispatchExternalTextureRelease;
        procs.instanceCreateSurface = ThreadDispatchInstanceCreateSurface;
        procs.instanceGetWGSLLanguageFeatures = ThreadDispatchInstanceGetWGSLLanguageFeatures;
        procs.instanceHasWGSLLanguageFeature = ThreadDispatchInstanceHasWGSLLanguageFeature;
        procs.instanceProcessEvents = ThreadDispatchInstanceProcessEvents;
        procs.instanceRequestAdapter = ThreadDispatchInstanceRequestAdapter;
        procs.instanceWaitAny = ThreadDispatchInstanceWaitAny;
        procs.instanceAddRef = ThreadDispatchInstanceAddRef;
        procs.instanceRelease = ThreadDispatchInstanceRelease;
        procs.pipelineLayoutSetLabel = ThreadDispatchPipelineLayoutSetLabel;
        procs.pipelineLayoutAddRef = ThreadDispatchPipelineLayoutAddRef;
        procs.pipelineLayoutRelease = ThreadDispatchPipelineLayoutRelease;
        procs.querySetDestroy = ThreadDispatchQuerySetDestroy;
        procs.querySetGetCount = ThreadDispatchQuerySetGetCount;
        procs.querySetGetType = ThreadDispatchQuerySetGetType;
        procs.querySetSetLabel = ThreadDispatchQuerySetSetLabel;
        procs.querySetAddRef = ThreadDispatchQuerySetAddRef;
        procs.querySetRelease = ThreadDispatchQuerySetRelease;
        procs.queueCopyExternalTextureForBrowser = ThreadDispatchQueueCopyExternalTextureForBrowser;
        procs.queueCopyTextureForBrowser = ThreadDispatchQueueCopyTextureForBrowser;
        procs.queueOnSubmittedWorkDone = ThreadDispatchQueueOnSubmittedWorkDone;
        procs.queueSetLabel = ThreadDispatchQueueSetLabel;
        procs.queueSubmit = ThreadDispatchQueueSubmit;
        procs.queueWriteBuffer = ThreadDispatchQueueWriteBuffer;
        procs.queueWriteTexture = ThreadDispatchQueueWriteTexture;
        procs.queueAddRef = ThreadDispatchQueueAddRef;
        procs.queueRelease = ThreadDispatchQueueRelease;
        procs.renderBundleSetLabel = ThreadDispatchRenderBundleSetLabel;
        procs.renderBundleAddRef = ThreadDispatchRenderBundleAddRef;
        procs.renderBundleRelease = ThreadDispatchRenderBundleRelease;
        procs.renderBundleEncoderDraw = ThreadDispatchRenderBundleEncoderDraw;
        procs.renderBundleEncoderDrawIndexed = ThreadDispatchRenderBundleEncoderDrawIndexed;
        procs.renderBundleEncoderDrawIndexedIndirect = ThreadDispatchRenderBundleEncoderDrawIndexedIndirect;
        procs.renderBundleEncoderDrawIndirect = ThreadDispatchRenderBundleEncoderDrawIndirect;
        procs.renderBundleEncoderFinish = ThreadDispatchRenderBundleEncoderFinish;
        procs.renderBundleEncoderInsertDebugMarker = ThreadDispatchRenderBundleEncoderInsertDebugMarker;
        procs.renderBundleEncoderPopDebugGroup = ThreadDispatchRenderBundleEncoderPopDebugGroup;
        procs.renderBundleEncoderPushDebugGroup = ThreadDispatchRenderBundleEncoderPushDebugGroup;
        procs.renderBundleEncoderSetBindGroup = ThreadDispatchRenderBundleEncoderSetBindGroup;
        procs.renderBundleEncoderSetImmediates = ThreadDispatchRenderBundleEncoderSetImmediates;
        procs.renderBundleEncoderSetIndexBuffer = ThreadDispatchRenderBundleEncoderSetIndexBuffer;
        procs.renderBundleEncoderSetLabel = ThreadDispatchRenderBundleEncoderSetLabel;
        procs.renderBundleEncoderSetPipeline = ThreadDispatchRenderBundleEncoderSetPipeline;
        procs.renderBundleEncoderSetResourceTable = ThreadDispatchRenderBundleEncoderSetResourceTable;
        procs.renderBundleEncoderSetVertexBuffer = ThreadDispatchRenderBundleEncoderSetVertexBuffer;
        procs.renderBundleEncoderAddRef = ThreadDispatchRenderBundleEncoderAddRef;
        procs.renderBundleEncoderRelease = ThreadDispatchRenderBundleEncoderRelease;
        procs.renderPassEncoderBeginOcclusionQuery = ThreadDispatchRenderPassEncoderBeginOcclusionQuery;
        procs.renderPassEncoderDraw = ThreadDispatchRenderPassEncoderDraw;
        procs.renderPassEncoderDrawIndexed = ThreadDispatchRenderPassEncoderDrawIndexed;
        procs.renderPassEncoderDrawIndexedIndirect = ThreadDispatchRenderPassEncoderDrawIndexedIndirect;
        procs.renderPassEncoderDrawIndirect = ThreadDispatchRenderPassEncoderDrawIndirect;
        procs.renderPassEncoderEnd = ThreadDispatchRenderPassEncoderEnd;
        procs.renderPassEncoderEndOcclusionQuery = ThreadDispatchRenderPassEncoderEndOcclusionQuery;
        procs.renderPassEncoderExecuteBundles = ThreadDispatchRenderPassEncoderExecuteBundles;
        procs.renderPassEncoderInsertDebugMarker = ThreadDispatchRenderPassEncoderInsertDebugMarker;
        procs.renderPassEncoderMultiDrawIndexedIndirect = ThreadDispatchRenderPassEncoderMultiDrawIndexedIndirect;
        procs.renderPassEncoderMultiDrawIndirect = ThreadDispatchRenderPassEncoderMultiDrawIndirect;
        procs.renderPassEncoderPixelLocalStorageBarrier = ThreadDispatchRenderPassEncoderPixelLocalStorageBarrier;
        procs.renderPassEncoderPopDebugGroup = ThreadDispatchRenderPassEncoderPopDebugGroup;
        procs.renderPassEncoderPushDebugGroup = ThreadDispatchRenderPassEncoderPushDebugGroup;
        procs.renderPassEncoderSetBindGroup = ThreadDispatchRenderPassEncoderSetBindGroup;
        procs.renderPassEncoderSetBlendConstant = ThreadDispatchRenderPassEncoderSetBlendConstant;
        procs.renderPassEncoderSetImmediates = ThreadDispatchRenderPassEncoderSetImmediates;
        procs.renderPassEncoderSetIndexBuffer = ThreadDispatchRenderPassEncoderSetIndexBuffer;
        procs.renderPassEncoderSetLabel = ThreadDispatchRenderPassEncoderSetLabel;
        procs.renderPassEncoderSetPipeline = ThreadDispatchRenderPassEncoderSetPipeline;
        procs.renderPassEncoderSetResourceTable = ThreadDispatchRenderPassEncoderSetResourceTable;
        procs.renderPassEncoderSetScissorRect = ThreadDispatchRenderPassEncoderSetScissorRect;
        procs.renderPassEncoderSetStencilReference = ThreadDispatchRenderPassEncoderSetStencilReference;
        procs.renderPassEncoderSetVertexBuffer = ThreadDispatchRenderPassEncoderSetVertexBuffer;
        procs.renderPassEncoderSetViewport = ThreadDispatchRenderPassEncoderSetViewport;
        procs.renderPassEncoderWriteTimestamp = ThreadDispatchRenderPassEncoderWriteTimestamp;
        procs.renderPassEncoderAddRef = ThreadDispatchRenderPassEncoderAddRef;
        procs.renderPassEncoderRelease = ThreadDispatchRenderPassEncoderRelease;
        procs.renderPipelineGetBindGroupLayout = ThreadDispatchRenderPipelineGetBindGroupLayout;
        procs.renderPipelineSetLabel = ThreadDispatchRenderPipelineSetLabel;
        procs.renderPipelineAddRef = ThreadDispatchRenderPipelineAddRef;
        procs.renderPipelineRelease = ThreadDispatchRenderPipelineRelease;
        procs.resourceTableDestroy = ThreadDispatchResourceTableDestroy;
        procs.resourceTableGetSize = ThreadDispatchResourceTableGetSize;
        procs.resourceTableInsert = ThreadDispatchResourceTableInsert;
        procs.resourceTableRemove = ThreadDispatchResourceTableRemove;
        procs.resourceTableSetLabel = ThreadDispatchResourceTableSetLabel;
        procs.resourceTableUpdate = ThreadDispatchResourceTableUpdate;
        procs.resourceTableAddRef = ThreadDispatchResourceTableAddRef;
        procs.resourceTableRelease = ThreadDispatchResourceTableRelease;
        procs.samplerSetLabel = ThreadDispatchSamplerSetLabel;
        procs.samplerAddRef = ThreadDispatchSamplerAddRef;
        procs.samplerRelease = ThreadDispatchSamplerRelease;
        procs.shaderModuleGetCompilationInfo = ThreadDispatchShaderModuleGetCompilationInfo;
        procs.shaderModuleSetLabel = ThreadDispatchShaderModuleSetLabel;
        procs.shaderModuleAddRef = ThreadDispatchShaderModuleAddRef;
        procs.shaderModuleRelease = ThreadDispatchShaderModuleRelease;
        procs.sharedBufferMemoryBeginAccess = ThreadDispatchSharedBufferMemoryBeginAccess;
        procs.sharedBufferMemoryCreateBuffer = ThreadDispatchSharedBufferMemoryCreateBuffer;
        procs.sharedBufferMemoryEndAccess = ThreadDispatchSharedBufferMemoryEndAccess;
        procs.sharedBufferMemoryGetProperties = ThreadDispatchSharedBufferMemoryGetProperties;
        procs.sharedBufferMemoryIsDeviceLost = ThreadDispatchSharedBufferMemoryIsDeviceLost;
        procs.sharedBufferMemorySetLabel = ThreadDispatchSharedBufferMemorySetLabel;
        procs.sharedBufferMemoryAddRef = ThreadDispatchSharedBufferMemoryAddRef;
        procs.sharedBufferMemoryRelease = ThreadDispatchSharedBufferMemoryRelease;
        procs.sharedBufferMemoryEndAccessStateFreeMembers = ThreadDispatchSharedBufferMemoryEndAccessStateFreeMembers;
        procs.sharedFenceExportInfo = ThreadDispatchSharedFenceExportInfo;
        procs.sharedFenceSetLabel = ThreadDispatchSharedFenceSetLabel;
        procs.sharedFenceAddRef = ThreadDispatchSharedFenceAddRef;
        procs.sharedFenceRelease = ThreadDispatchSharedFenceRelease;
        procs.sharedTextureMemoryBeginAccess = ThreadDispatchSharedTextureMemoryBeginAccess;
        procs.sharedTextureMemoryCreateTexture = ThreadDispatchSharedTextureMemoryCreateTexture;
        procs.sharedTextureMemoryEndAccess = ThreadDispatchSharedTextureMemoryEndAccess;
        procs.sharedTextureMemoryGetProperties = ThreadDispatchSharedTextureMemoryGetProperties;
        procs.sharedTextureMemoryIsDeviceLost = ThreadDispatchSharedTextureMemoryIsDeviceLost;
        procs.sharedTextureMemorySetLabel = ThreadDispatchSharedTextureMemorySetLabel;
        procs.sharedTextureMemoryAddRef = ThreadDispatchSharedTextureMemoryAddRef;
        procs.sharedTextureMemoryRelease = ThreadDispatchSharedTextureMemoryRelease;
        procs.sharedTextureMemoryEndAccessStateFreeMembers = ThreadDispatchSharedTextureMemoryEndAccessStateFreeMembers;
        procs.supportedFeaturesFreeMembers = ThreadDispatchSupportedFeaturesFreeMembers;
        procs.supportedInstanceFeaturesFreeMembers = ThreadDispatchSupportedInstanceFeaturesFreeMembers;
        procs.supportedWGSLLanguageFeaturesFreeMembers = ThreadDispatchSupportedWGSLLanguageFeaturesFreeMembers;
        procs.surfaceConfigure = ThreadDispatchSurfaceConfigure;
        procs.surfaceGetCapabilities = ThreadDispatchSurfaceGetCapabilities;
        procs.surfaceGetCurrentTexture = ThreadDispatchSurfaceGetCurrentTexture;
        procs.surfacePresent = ThreadDispatchSurfacePresent;
        procs.surfaceSetLabel = ThreadDispatchSurfaceSetLabel;
        procs.surfaceUnconfigure = ThreadDispatchSurfaceUnconfigure;
        procs.surfaceAddRef = ThreadDispatchSurfaceAddRef;
        procs.surfaceRelease = ThreadDispatchSurfaceRelease;
        procs.surfaceCapabilitiesFreeMembers = ThreadDispatchSurfaceCapabilitiesFreeMembers;
        procs.texelBufferViewSetLabel = ThreadDispatchTexelBufferViewSetLabel;
        procs.texelBufferViewAddRef = ThreadDispatchTexelBufferViewAddRef;
        procs.texelBufferViewRelease = ThreadDispatchTexelBufferViewRelease;
        procs.textureCreateErrorView = ThreadDispatchTextureCreateErrorView;
        procs.textureCreateView = ThreadDispatchTextureCreateView;
        procs.textureDestroy = ThreadDispatchTextureDestroy;
        procs.textureGetDepthOrArrayLayers = ThreadDispatchTextureGetDepthOrArrayLayers;
        procs.textureGetDimension = ThreadDispatchTextureGetDimension;
        procs.textureGetFormat = ThreadDispatchTextureGetFormat;
        procs.textureGetHeight = ThreadDispatchTextureGetHeight;
        procs.textureGetMipLevelCount = ThreadDispatchTextureGetMipLevelCount;
        procs.textureGetSampleCount = ThreadDispatchTextureGetSampleCount;
        procs.textureGetTextureBindingViewDimension = ThreadDispatchTextureGetTextureBindingViewDimension;
        procs.textureGetUsage = ThreadDispatchTextureGetUsage;
        procs.textureGetWidth = ThreadDispatchTextureGetWidth;
        procs.textureSetLabel = ThreadDispatchTextureSetLabel;
        procs.textureSetOwnershipForMemoryDump = ThreadDispatchTextureSetOwnershipForMemoryDump;
        procs.textureAddRef = ThreadDispatchTextureAddRef;
        procs.textureRelease = ThreadDispatchTextureRelease;
        procs.textureViewSetLabel = ThreadDispatchTextureViewSetLabel;
        procs.textureViewAddRef = ThreadDispatchTextureViewAddRef;
        procs.textureViewRelease = ThreadDispatchTextureViewRelease;
        return procs;
    }

extern "C" {
    DawnProcTable dawnThreadDispatchProcTable = MakeThreadDispatchProcTable();
}
