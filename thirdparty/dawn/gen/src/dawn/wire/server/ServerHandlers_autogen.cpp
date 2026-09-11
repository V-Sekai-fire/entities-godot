
#include "src/dawn/wire/server/Server.h"
#include "src/utils/assert.h"

namespace dawn::wire::server {

    WireResult Server::HandleAdapterRequestDevice(DeserializeBuffer* deserializeBuffer) {
        AdapterRequestDeviceCmd cmd;
        WIRE_TRY(cmd.Deserialize(deserializeBuffer, &mAllocator, *this));

        Known<WGPUAdapter> adapterIdHandle;
        WIRE_TRY(Get(cmd.adapterId, &adapterIdHandle));
        Known<WGPUInstance> instanceIdHandle;
        WIRE_TRY(Get(cmd.instanceId, &instanceIdHandle));

        WIRE_TRY(DoAdapterRequestDevice(adapterIdHandle, instanceIdHandle, cmd.future, cmd.deviceObjectHandle, cmd.deviceLostFuture, cmd.descriptor));

        return WireResult::Success;
    }

    WireResult Server::HandleBindGroupLayoutSetLabel(DeserializeBuffer* deserializeBuffer) {
        BindGroupLayoutSetLabelCmd cmd;
        WIRE_TRY(cmd.Deserialize(deserializeBuffer, &mAllocator, *this));


        WIRE_TRY(DoBindGroupLayoutSetLabel(cmd.self, cmd.label));

        return WireResult::Success;
    }

    WireResult Server::HandleBindGroupSetLabel(DeserializeBuffer* deserializeBuffer) {
        BindGroupSetLabelCmd cmd;
        WIRE_TRY(cmd.Deserialize(deserializeBuffer, &mAllocator, *this));


        WIRE_TRY(DoBindGroupSetLabel(cmd.self, cmd.label));

        return WireResult::Success;
    }

    WireResult Server::HandleBufferCreateTexelView(DeserializeBuffer* deserializeBuffer) {
        BufferCreateTexelViewCmd cmd;
        WIRE_TRY(cmd.Deserialize(deserializeBuffer, &mAllocator, *this));

        Reserved<WGPUTexelBufferView> resultData;
        WIRE_TRY(Allocate(&resultData, cmd.result));
        resultData->generation = cmd.result.generation;

        WIRE_TRY(DoBufferCreateTexelView(cmd.self, cmd.descriptor, &resultData->handle));

        return WireResult::Success;
    }

    WireResult Server::HandleBufferDestroy(DeserializeBuffer* deserializeBuffer) {
        BufferDestroyCmd cmd;
        WIRE_TRY(cmd.Deserialize(deserializeBuffer, &mAllocator, *this));
        WIRE_TRY(PreHandleBufferDestroy(cmd));

        WIRE_TRY(DoBufferDestroy(cmd.self));

        return WireResult::Success;
    }

    WireResult Server::HandleBufferMapAsync(DeserializeBuffer* deserializeBuffer) {
        BufferMapAsyncCmd cmd;
        WIRE_TRY(cmd.Deserialize(deserializeBuffer, &mAllocator));

        Known<WGPUBuffer> bufferIdHandle;
        WIRE_TRY(Get(cmd.bufferId, &bufferIdHandle));
        Known<WGPUInstance> instanceIdHandle;
        WIRE_TRY(Get(cmd.instanceId, &instanceIdHandle));

        WIRE_TRY(DoBufferMapAsync(bufferIdHandle, instanceIdHandle, cmd.future, cmd.mode, cmd.offset, cmd.size));

        return WireResult::Success;
    }

    WireResult Server::HandleBufferSetLabel(DeserializeBuffer* deserializeBuffer) {
        BufferSetLabelCmd cmd;
        WIRE_TRY(cmd.Deserialize(deserializeBuffer, &mAllocator, *this));


        WIRE_TRY(DoBufferSetLabel(cmd.self, cmd.label));

        return WireResult::Success;
    }

    WireResult Server::HandleBufferUnmap(DeserializeBuffer* deserializeBuffer) {
        BufferUnmapCmd cmd;
        WIRE_TRY(cmd.Deserialize(deserializeBuffer, &mAllocator, *this));
        WIRE_TRY(PreHandleBufferUnmap(cmd));

        WIRE_TRY(DoBufferUnmap(cmd.self));

        return WireResult::Success;
    }

    WireResult Server::HandleBufferUpdateMappedData(DeserializeBuffer* deserializeBuffer) {
        BufferUpdateMappedDataCmd cmd;
        WIRE_TRY(cmd.Deserialize(deserializeBuffer, &mAllocator));

        Known<WGPUBuffer> bufferIdHandle;
        WIRE_TRY(Get(cmd.bufferId, &bufferIdHandle));

        WIRE_TRY(DoBufferUpdateMappedData(bufferIdHandle, cmd.dataUpdateInfoLength, cmd.dataUpdateInfo, cmd.offset, cmd.size));

        return WireResult::Success;
    }

    WireResult Server::HandleCommandBufferSetLabel(DeserializeBuffer* deserializeBuffer) {
        CommandBufferSetLabelCmd cmd;
        WIRE_TRY(cmd.Deserialize(deserializeBuffer, &mAllocator, *this));


        WIRE_TRY(DoCommandBufferSetLabel(cmd.self, cmd.label));

        return WireResult::Success;
    }

    WireResult Server::HandleCommandEncoderBeginComputePass(DeserializeBuffer* deserializeBuffer) {
        CommandEncoderBeginComputePassCmd cmd;
        WIRE_TRY(cmd.Deserialize(deserializeBuffer, &mAllocator, *this));

        Reserved<WGPUComputePassEncoder> resultData;
        WIRE_TRY(Allocate(&resultData, cmd.result));
        resultData->generation = cmd.result.generation;

        WIRE_TRY(DoCommandEncoderBeginComputePass(cmd.self, cmd.descriptor, &resultData->handle));

        return WireResult::Success;
    }

    WireResult Server::HandleCommandEncoderBeginRenderPass(DeserializeBuffer* deserializeBuffer) {
        CommandEncoderBeginRenderPassCmd cmd;
        WIRE_TRY(cmd.Deserialize(deserializeBuffer, &mAllocator, *this));

        Reserved<WGPURenderPassEncoder> resultData;
        WIRE_TRY(Allocate(&resultData, cmd.result));
        resultData->generation = cmd.result.generation;

        WIRE_TRY(DoCommandEncoderBeginRenderPass(cmd.self, cmd.descriptor, &resultData->handle));

        return WireResult::Success;
    }

    WireResult Server::HandleCommandEncoderClearBuffer(DeserializeBuffer* deserializeBuffer) {
        CommandEncoderClearBufferCmd cmd;
        WIRE_TRY(cmd.Deserialize(deserializeBuffer, &mAllocator, *this));


        WIRE_TRY(DoCommandEncoderClearBuffer(cmd.self, cmd.buffer, cmd.offset, cmd.size));

        return WireResult::Success;
    }

    WireResult Server::HandleCommandEncoderCopyBufferToBuffer(DeserializeBuffer* deserializeBuffer) {
        CommandEncoderCopyBufferToBufferCmd cmd;
        WIRE_TRY(cmd.Deserialize(deserializeBuffer, &mAllocator, *this));


        WIRE_TRY(DoCommandEncoderCopyBufferToBuffer(cmd.self, cmd.source, cmd.sourceOffset, cmd.destination, cmd.destinationOffset, cmd.size));

        return WireResult::Success;
    }

    WireResult Server::HandleCommandEncoderCopyBufferToTexture(DeserializeBuffer* deserializeBuffer) {
        CommandEncoderCopyBufferToTextureCmd cmd;
        WIRE_TRY(cmd.Deserialize(deserializeBuffer, &mAllocator, *this));


        WIRE_TRY(DoCommandEncoderCopyBufferToTexture(cmd.self, cmd.source, cmd.destination, cmd.copySize));

        return WireResult::Success;
    }

    WireResult Server::HandleCommandEncoderCopyTextureToBuffer(DeserializeBuffer* deserializeBuffer) {
        CommandEncoderCopyTextureToBufferCmd cmd;
        WIRE_TRY(cmd.Deserialize(deserializeBuffer, &mAllocator, *this));


        WIRE_TRY(DoCommandEncoderCopyTextureToBuffer(cmd.self, cmd.source, cmd.destination, cmd.copySize));

        return WireResult::Success;
    }

    WireResult Server::HandleCommandEncoderCopyTextureToTexture(DeserializeBuffer* deserializeBuffer) {
        CommandEncoderCopyTextureToTextureCmd cmd;
        WIRE_TRY(cmd.Deserialize(deserializeBuffer, &mAllocator, *this));


        WIRE_TRY(DoCommandEncoderCopyTextureToTexture(cmd.self, cmd.source, cmd.destination, cmd.copySize));

        return WireResult::Success;
    }

    WireResult Server::HandleCommandEncoderFinish(DeserializeBuffer* deserializeBuffer) {
        CommandEncoderFinishCmd cmd;
        WIRE_TRY(cmd.Deserialize(deserializeBuffer, &mAllocator, *this));

        Reserved<WGPUCommandBuffer> resultData;
        WIRE_TRY(Allocate(&resultData, cmd.result));
        resultData->generation = cmd.result.generation;

        WIRE_TRY(DoCommandEncoderFinish(cmd.self, cmd.descriptor, &resultData->handle));

        return WireResult::Success;
    }

    WireResult Server::HandleCommandEncoderInjectValidationError(DeserializeBuffer* deserializeBuffer) {
        CommandEncoderInjectValidationErrorCmd cmd;
        WIRE_TRY(cmd.Deserialize(deserializeBuffer, &mAllocator, *this));


        WIRE_TRY(DoCommandEncoderInjectValidationError(cmd.self, cmd.message));

        return WireResult::Success;
    }

    WireResult Server::HandleCommandEncoderInsertDebugMarker(DeserializeBuffer* deserializeBuffer) {
        CommandEncoderInsertDebugMarkerCmd cmd;
        WIRE_TRY(cmd.Deserialize(deserializeBuffer, &mAllocator, *this));


        WIRE_TRY(DoCommandEncoderInsertDebugMarker(cmd.self, cmd.markerLabel));

        return WireResult::Success;
    }

    WireResult Server::HandleCommandEncoderPopDebugGroup(DeserializeBuffer* deserializeBuffer) {
        CommandEncoderPopDebugGroupCmd cmd;
        WIRE_TRY(cmd.Deserialize(deserializeBuffer, &mAllocator, *this));


        WIRE_TRY(DoCommandEncoderPopDebugGroup(cmd.self));

        return WireResult::Success;
    }

    WireResult Server::HandleCommandEncoderPushDebugGroup(DeserializeBuffer* deserializeBuffer) {
        CommandEncoderPushDebugGroupCmd cmd;
        WIRE_TRY(cmd.Deserialize(deserializeBuffer, &mAllocator, *this));


        WIRE_TRY(DoCommandEncoderPushDebugGroup(cmd.self, cmd.groupLabel));

        return WireResult::Success;
    }

    WireResult Server::HandleCommandEncoderResolveQuerySet(DeserializeBuffer* deserializeBuffer) {
        CommandEncoderResolveQuerySetCmd cmd;
        WIRE_TRY(cmd.Deserialize(deserializeBuffer, &mAllocator, *this));


        WIRE_TRY(DoCommandEncoderResolveQuerySet(cmd.self, cmd.querySet, cmd.firstQuery, cmd.queryCount, cmd.destination, cmd.destinationOffset));

        return WireResult::Success;
    }

    WireResult Server::HandleCommandEncoderSetLabel(DeserializeBuffer* deserializeBuffer) {
        CommandEncoderSetLabelCmd cmd;
        WIRE_TRY(cmd.Deserialize(deserializeBuffer, &mAllocator, *this));


        WIRE_TRY(DoCommandEncoderSetLabel(cmd.self, cmd.label));

        return WireResult::Success;
    }

    WireResult Server::HandleCommandEncoderWriteBuffer(DeserializeBuffer* deserializeBuffer) {
        CommandEncoderWriteBufferCmd cmd;
        WIRE_TRY(cmd.Deserialize(deserializeBuffer, &mAllocator, *this));


        WIRE_TRY(DoCommandEncoderWriteBuffer(cmd.self, cmd.buffer, cmd.bufferOffset, cmd.data, cmd.size));

        return WireResult::Success;
    }

    WireResult Server::HandleCommandEncoderWriteTimestamp(DeserializeBuffer* deserializeBuffer) {
        CommandEncoderWriteTimestampCmd cmd;
        WIRE_TRY(cmd.Deserialize(deserializeBuffer, &mAllocator, *this));


        WIRE_TRY(DoCommandEncoderWriteTimestamp(cmd.self, cmd.querySet, cmd.queryIndex));

        return WireResult::Success;
    }

    WireResult Server::HandleComputePassEncoderDispatchWorkgroups(DeserializeBuffer* deserializeBuffer) {
        ComputePassEncoderDispatchWorkgroupsCmd cmd;
        WIRE_TRY(cmd.Deserialize(deserializeBuffer, &mAllocator, *this));


        WIRE_TRY(DoComputePassEncoderDispatchWorkgroups(cmd.self, cmd.workgroupCountX, cmd.workgroupCountY, cmd.workgroupCountZ));

        return WireResult::Success;
    }

    WireResult Server::HandleComputePassEncoderDispatchWorkgroupsIndirect(DeserializeBuffer* deserializeBuffer) {
        ComputePassEncoderDispatchWorkgroupsIndirectCmd cmd;
        WIRE_TRY(cmd.Deserialize(deserializeBuffer, &mAllocator, *this));


        WIRE_TRY(DoComputePassEncoderDispatchWorkgroupsIndirect(cmd.self, cmd.indirectBuffer, cmd.indirectOffset));

        return WireResult::Success;
    }

    WireResult Server::HandleComputePassEncoderEnd(DeserializeBuffer* deserializeBuffer) {
        ComputePassEncoderEndCmd cmd;
        WIRE_TRY(cmd.Deserialize(deserializeBuffer, &mAllocator, *this));


        WIRE_TRY(DoComputePassEncoderEnd(cmd.self));

        return WireResult::Success;
    }

    WireResult Server::HandleComputePassEncoderInsertDebugMarker(DeserializeBuffer* deserializeBuffer) {
        ComputePassEncoderInsertDebugMarkerCmd cmd;
        WIRE_TRY(cmd.Deserialize(deserializeBuffer, &mAllocator, *this));


        WIRE_TRY(DoComputePassEncoderInsertDebugMarker(cmd.self, cmd.markerLabel));

        return WireResult::Success;
    }

    WireResult Server::HandleComputePassEncoderPopDebugGroup(DeserializeBuffer* deserializeBuffer) {
        ComputePassEncoderPopDebugGroupCmd cmd;
        WIRE_TRY(cmd.Deserialize(deserializeBuffer, &mAllocator, *this));


        WIRE_TRY(DoComputePassEncoderPopDebugGroup(cmd.self));

        return WireResult::Success;
    }

    WireResult Server::HandleComputePassEncoderPushDebugGroup(DeserializeBuffer* deserializeBuffer) {
        ComputePassEncoderPushDebugGroupCmd cmd;
        WIRE_TRY(cmd.Deserialize(deserializeBuffer, &mAllocator, *this));


        WIRE_TRY(DoComputePassEncoderPushDebugGroup(cmd.self, cmd.groupLabel));

        return WireResult::Success;
    }

    WireResult Server::HandleComputePassEncoderSetBindGroup(DeserializeBuffer* deserializeBuffer) {
        ComputePassEncoderSetBindGroupCmd cmd;
        WIRE_TRY(cmd.Deserialize(deserializeBuffer, &mAllocator, *this));


        WIRE_TRY(DoComputePassEncoderSetBindGroup(cmd.self, cmd.groupIndex, cmd.group, cmd.dynamicOffsetCount, cmd.dynamicOffsets));

        return WireResult::Success;
    }

    WireResult Server::HandleComputePassEncoderSetImmediates(DeserializeBuffer* deserializeBuffer) {
        ComputePassEncoderSetImmediatesCmd cmd;
        WIRE_TRY(cmd.Deserialize(deserializeBuffer, &mAllocator));

        Known<WGPUComputePassEncoder> computePassEncoderIdHandle;
        WIRE_TRY(Get(cmd.computePassEncoderId, &computePassEncoderIdHandle));

        WIRE_TRY(DoComputePassEncoderSetImmediates(computePassEncoderIdHandle, cmd.offset, cmd.data, cmd.size));

        return WireResult::Success;
    }

    WireResult Server::HandleComputePassEncoderSetLabel(DeserializeBuffer* deserializeBuffer) {
        ComputePassEncoderSetLabelCmd cmd;
        WIRE_TRY(cmd.Deserialize(deserializeBuffer, &mAllocator, *this));


        WIRE_TRY(DoComputePassEncoderSetLabel(cmd.self, cmd.label));

        return WireResult::Success;
    }

    WireResult Server::HandleComputePassEncoderSetPipeline(DeserializeBuffer* deserializeBuffer) {
        ComputePassEncoderSetPipelineCmd cmd;
        WIRE_TRY(cmd.Deserialize(deserializeBuffer, &mAllocator, *this));


        WIRE_TRY(DoComputePassEncoderSetPipeline(cmd.self, cmd.pipeline));

        return WireResult::Success;
    }

    WireResult Server::HandleComputePassEncoderSetResourceTable(DeserializeBuffer* deserializeBuffer) {
        ComputePassEncoderSetResourceTableCmd cmd;
        WIRE_TRY(cmd.Deserialize(deserializeBuffer, &mAllocator, *this));


        WIRE_TRY(DoComputePassEncoderSetResourceTable(cmd.self, cmd.table));

        return WireResult::Success;
    }

    WireResult Server::HandleComputePassEncoderWriteTimestamp(DeserializeBuffer* deserializeBuffer) {
        ComputePassEncoderWriteTimestampCmd cmd;
        WIRE_TRY(cmd.Deserialize(deserializeBuffer, &mAllocator, *this));


        WIRE_TRY(DoComputePassEncoderWriteTimestamp(cmd.self, cmd.querySet, cmd.queryIndex));

        return WireResult::Success;
    }

    WireResult Server::HandleComputePipelineGetBindGroupLayout(DeserializeBuffer* deserializeBuffer) {
        ComputePipelineGetBindGroupLayoutCmd cmd;
        WIRE_TRY(cmd.Deserialize(deserializeBuffer, &mAllocator, *this));

        Reserved<WGPUBindGroupLayout> resultData;
        WIRE_TRY(Allocate(&resultData, cmd.result));
        resultData->generation = cmd.result.generation;

        WIRE_TRY(DoComputePipelineGetBindGroupLayout(cmd.self, cmd.groupIndex, &resultData->handle));

        return WireResult::Success;
    }

    WireResult Server::HandleComputePipelineSetLabel(DeserializeBuffer* deserializeBuffer) {
        ComputePipelineSetLabelCmd cmd;
        WIRE_TRY(cmd.Deserialize(deserializeBuffer, &mAllocator, *this));


        WIRE_TRY(DoComputePipelineSetLabel(cmd.self, cmd.label));

        return WireResult::Success;
    }

    WireResult Server::HandleDeviceCreateBindGroup(DeserializeBuffer* deserializeBuffer) {
        DeviceCreateBindGroupCmd cmd;
        WIRE_TRY(cmd.Deserialize(deserializeBuffer, &mAllocator, *this));

        Reserved<WGPUBindGroup> resultData;
        WIRE_TRY(Allocate(&resultData, cmd.result));
        resultData->generation = cmd.result.generation;

        WIRE_TRY(DoDeviceCreateBindGroup(cmd.self, cmd.descriptor, &resultData->handle));

        return WireResult::Success;
    }

    WireResult Server::HandleDeviceCreateBindGroupLayout(DeserializeBuffer* deserializeBuffer) {
        DeviceCreateBindGroupLayoutCmd cmd;
        WIRE_TRY(cmd.Deserialize(deserializeBuffer, &mAllocator, *this));

        Reserved<WGPUBindGroupLayout> resultData;
        WIRE_TRY(Allocate(&resultData, cmd.result));
        resultData->generation = cmd.result.generation;

        WIRE_TRY(DoDeviceCreateBindGroupLayout(cmd.self, cmd.descriptor, &resultData->handle));

        return WireResult::Success;
    }

    WireResult Server::HandleDeviceCreateBuffer(DeserializeBuffer* deserializeBuffer) {
        DeviceCreateBufferCmd cmd;
        WIRE_TRY(cmd.Deserialize(deserializeBuffer, &mAllocator, *this));

        Known<WGPUDevice> deviceIdHandle;
        WIRE_TRY(Get(cmd.deviceId, &deviceIdHandle));

        WIRE_TRY(DoDeviceCreateBuffer(deviceIdHandle, cmd.descriptor, cmd.result, cmd.memoryHandleCreateInfoLength, cmd.memoryHandleCreateInfo));

        return WireResult::Success;
    }

    WireResult Server::HandleDeviceCreateCommandEncoder(DeserializeBuffer* deserializeBuffer) {
        DeviceCreateCommandEncoderCmd cmd;
        WIRE_TRY(cmd.Deserialize(deserializeBuffer, &mAllocator, *this));

        Reserved<WGPUCommandEncoder> resultData;
        WIRE_TRY(Allocate(&resultData, cmd.result));
        resultData->generation = cmd.result.generation;

        WIRE_TRY(DoDeviceCreateCommandEncoder(cmd.self, cmd.descriptor, &resultData->handle));

        return WireResult::Success;
    }

    WireResult Server::HandleDeviceCreateComputePipeline(DeserializeBuffer* deserializeBuffer) {
        DeviceCreateComputePipelineCmd cmd;
        WIRE_TRY(cmd.Deserialize(deserializeBuffer, &mAllocator, *this));

        Reserved<WGPUComputePipeline> resultData;
        WIRE_TRY(Allocate(&resultData, cmd.result));
        resultData->generation = cmd.result.generation;

        WIRE_TRY(DoDeviceCreateComputePipeline(cmd.self, cmd.descriptor, &resultData->handle));

        return WireResult::Success;
    }

    WireResult Server::HandleDeviceCreateComputePipelineAsync(DeserializeBuffer* deserializeBuffer) {
        DeviceCreateComputePipelineAsyncCmd cmd;
        WIRE_TRY(cmd.Deserialize(deserializeBuffer, &mAllocator, *this));

        Known<WGPUDevice> deviceIdHandle;
        WIRE_TRY(Get(cmd.deviceId, &deviceIdHandle));
        Known<WGPUInstance> instanceIdHandle;
        WIRE_TRY(Get(cmd.instanceId, &instanceIdHandle));

        WIRE_TRY(DoDeviceCreateComputePipelineAsync(deviceIdHandle, instanceIdHandle, cmd.future, cmd.pipelineObjectHandle, cmd.descriptor));

        return WireResult::Success;
    }

    WireResult Server::HandleDeviceCreateErrorBuffer(DeserializeBuffer* deserializeBuffer) {
        DeviceCreateErrorBufferCmd cmd;
        WIRE_TRY(cmd.Deserialize(deserializeBuffer, &mAllocator, *this));
        WIRE_TRY(PreHandleDeviceCreateErrorBuffer(cmd));
        Reserved<WGPUBuffer> resultData;
        WIRE_TRY(Allocate(&resultData, cmd.result));
        resultData->generation = cmd.result.generation;

        WIRE_TRY(DoDeviceCreateErrorBuffer(cmd.self, cmd.descriptor, &resultData->handle));

        return WireResult::Success;
    }

    WireResult Server::HandleDeviceCreateErrorComputePipeline(DeserializeBuffer* deserializeBuffer) {
        DeviceCreateErrorComputePipelineCmd cmd;
        WIRE_TRY(cmd.Deserialize(deserializeBuffer, &mAllocator, *this));

        Reserved<WGPUComputePipeline> resultData;
        WIRE_TRY(Allocate(&resultData, cmd.result));
        resultData->generation = cmd.result.generation;

        WIRE_TRY(DoDeviceCreateErrorComputePipeline(cmd.self, cmd.label, &resultData->handle));

        return WireResult::Success;
    }

    WireResult Server::HandleDeviceCreateErrorExternalTexture(DeserializeBuffer* deserializeBuffer) {
        DeviceCreateErrorExternalTextureCmd cmd;
        WIRE_TRY(cmd.Deserialize(deserializeBuffer, &mAllocator, *this));

        Reserved<WGPUExternalTexture> resultData;
        WIRE_TRY(Allocate(&resultData, cmd.result));
        resultData->generation = cmd.result.generation;

        WIRE_TRY(DoDeviceCreateErrorExternalTexture(cmd.self, &resultData->handle));

        return WireResult::Success;
    }

    WireResult Server::HandleDeviceCreateErrorRenderPipeline(DeserializeBuffer* deserializeBuffer) {
        DeviceCreateErrorRenderPipelineCmd cmd;
        WIRE_TRY(cmd.Deserialize(deserializeBuffer, &mAllocator, *this));

        Reserved<WGPURenderPipeline> resultData;
        WIRE_TRY(Allocate(&resultData, cmd.result));
        resultData->generation = cmd.result.generation;

        WIRE_TRY(DoDeviceCreateErrorRenderPipeline(cmd.self, cmd.label, &resultData->handle));

        return WireResult::Success;
    }

    WireResult Server::HandleDeviceCreateErrorShaderModule(DeserializeBuffer* deserializeBuffer) {
        DeviceCreateErrorShaderModuleCmd cmd;
        WIRE_TRY(cmd.Deserialize(deserializeBuffer, &mAllocator, *this));

        Reserved<WGPUShaderModule> resultData;
        WIRE_TRY(Allocate(&resultData, cmd.result));
        resultData->generation = cmd.result.generation;

        WIRE_TRY(DoDeviceCreateErrorShaderModule(cmd.self, cmd.descriptor, cmd.errorMessage, &resultData->handle));

        return WireResult::Success;
    }

    WireResult Server::HandleDeviceCreateErrorTexture(DeserializeBuffer* deserializeBuffer) {
        DeviceCreateErrorTextureCmd cmd;
        WIRE_TRY(cmd.Deserialize(deserializeBuffer, &mAllocator, *this));

        Reserved<WGPUTexture> resultData;
        WIRE_TRY(Allocate(&resultData, cmd.result));
        resultData->generation = cmd.result.generation;

        WIRE_TRY(DoDeviceCreateErrorTexture(cmd.self, cmd.descriptor, &resultData->handle));

        return WireResult::Success;
    }

    WireResult Server::HandleDeviceCreateExternalTexture(DeserializeBuffer* deserializeBuffer) {
        DeviceCreateExternalTextureCmd cmd;
        WIRE_TRY(cmd.Deserialize(deserializeBuffer, &mAllocator, *this));

        Reserved<WGPUExternalTexture> resultData;
        WIRE_TRY(Allocate(&resultData, cmd.result));
        resultData->generation = cmd.result.generation;

        WIRE_TRY(DoDeviceCreateExternalTexture(cmd.self, cmd.externalTextureDescriptor, &resultData->handle));

        return WireResult::Success;
    }

    WireResult Server::HandleDeviceCreatePipelineLayout(DeserializeBuffer* deserializeBuffer) {
        DeviceCreatePipelineLayoutCmd cmd;
        WIRE_TRY(cmd.Deserialize(deserializeBuffer, &mAllocator, *this));

        Reserved<WGPUPipelineLayout> resultData;
        WIRE_TRY(Allocate(&resultData, cmd.result));
        resultData->generation = cmd.result.generation;

        WIRE_TRY(DoDeviceCreatePipelineLayout(cmd.self, cmd.descriptor, &resultData->handle));

        return WireResult::Success;
    }

    WireResult Server::HandleDeviceCreateQuerySet(DeserializeBuffer* deserializeBuffer) {
        DeviceCreateQuerySetCmd cmd;
        WIRE_TRY(cmd.Deserialize(deserializeBuffer, &mAllocator, *this));

        Reserved<WGPUQuerySet> resultData;
        WIRE_TRY(Allocate(&resultData, cmd.result));
        resultData->generation = cmd.result.generation;

        WIRE_TRY(DoDeviceCreateQuerySet(cmd.self, cmd.descriptor, &resultData->handle));

        return WireResult::Success;
    }

    WireResult Server::HandleDeviceCreateRenderBundleEncoder(DeserializeBuffer* deserializeBuffer) {
        DeviceCreateRenderBundleEncoderCmd cmd;
        WIRE_TRY(cmd.Deserialize(deserializeBuffer, &mAllocator, *this));

        Reserved<WGPURenderBundleEncoder> resultData;
        WIRE_TRY(Allocate(&resultData, cmd.result));
        resultData->generation = cmd.result.generation;

        WIRE_TRY(DoDeviceCreateRenderBundleEncoder(cmd.self, cmd.descriptor, &resultData->handle));

        return WireResult::Success;
    }

    WireResult Server::HandleDeviceCreateRenderPipeline(DeserializeBuffer* deserializeBuffer) {
        DeviceCreateRenderPipelineCmd cmd;
        WIRE_TRY(cmd.Deserialize(deserializeBuffer, &mAllocator, *this));

        Reserved<WGPURenderPipeline> resultData;
        WIRE_TRY(Allocate(&resultData, cmd.result));
        resultData->generation = cmd.result.generation;

        WIRE_TRY(DoDeviceCreateRenderPipeline(cmd.self, cmd.descriptor, &resultData->handle));

        return WireResult::Success;
    }

    WireResult Server::HandleDeviceCreateRenderPipelineAsync(DeserializeBuffer* deserializeBuffer) {
        DeviceCreateRenderPipelineAsyncCmd cmd;
        WIRE_TRY(cmd.Deserialize(deserializeBuffer, &mAllocator, *this));

        Known<WGPUDevice> deviceIdHandle;
        WIRE_TRY(Get(cmd.deviceId, &deviceIdHandle));
        Known<WGPUInstance> instanceIdHandle;
        WIRE_TRY(Get(cmd.instanceId, &instanceIdHandle));

        WIRE_TRY(DoDeviceCreateRenderPipelineAsync(deviceIdHandle, instanceIdHandle, cmd.future, cmd.pipelineObjectHandle, cmd.descriptor));

        return WireResult::Success;
    }

    WireResult Server::HandleDeviceCreateResourceTable(DeserializeBuffer* deserializeBuffer) {
        DeviceCreateResourceTableCmd cmd;
        WIRE_TRY(cmd.Deserialize(deserializeBuffer, &mAllocator, *this));

        Reserved<WGPUResourceTable> resultData;
        WIRE_TRY(Allocate(&resultData, cmd.result));
        resultData->generation = cmd.result.generation;

        WIRE_TRY(DoDeviceCreateResourceTable(cmd.self, cmd.descriptor, &resultData->handle));

        return WireResult::Success;
    }

    WireResult Server::HandleDeviceCreateSampler(DeserializeBuffer* deserializeBuffer) {
        DeviceCreateSamplerCmd cmd;
        WIRE_TRY(cmd.Deserialize(deserializeBuffer, &mAllocator, *this));

        Reserved<WGPUSampler> resultData;
        WIRE_TRY(Allocate(&resultData, cmd.result));
        resultData->generation = cmd.result.generation;

        WIRE_TRY(DoDeviceCreateSampler(cmd.self, cmd.descriptor, &resultData->handle));

        return WireResult::Success;
    }

    WireResult Server::HandleDeviceCreateShaderModule(DeserializeBuffer* deserializeBuffer) {
        DeviceCreateShaderModuleCmd cmd;
        WIRE_TRY(cmd.Deserialize(deserializeBuffer, &mAllocator, *this));

        Reserved<WGPUShaderModule> resultData;
        WIRE_TRY(Allocate(&resultData, cmd.result));
        resultData->generation = cmd.result.generation;

        WIRE_TRY(DoDeviceCreateShaderModule(cmd.self, cmd.descriptor, &resultData->handle));

        return WireResult::Success;
    }

    WireResult Server::HandleDeviceCreateTexture(DeserializeBuffer* deserializeBuffer) {
        DeviceCreateTextureCmd cmd;
        WIRE_TRY(cmd.Deserialize(deserializeBuffer, &mAllocator, *this));

        Reserved<WGPUTexture> resultData;
        WIRE_TRY(Allocate(&resultData, cmd.result));
        resultData->generation = cmd.result.generation;

        WIRE_TRY(DoDeviceCreateTexture(cmd.self, cmd.descriptor, &resultData->handle));

        return WireResult::Success;
    }

    WireResult Server::HandleDeviceDestroy(DeserializeBuffer* deserializeBuffer) {
        DeviceDestroyCmd cmd;
        WIRE_TRY(cmd.Deserialize(deserializeBuffer, &mAllocator, *this));


        WIRE_TRY(DoDeviceDestroy(cmd.self));

        return WireResult::Success;
    }

    WireResult Server::HandleDeviceForceLoss(DeserializeBuffer* deserializeBuffer) {
        DeviceForceLossCmd cmd;
        WIRE_TRY(cmd.Deserialize(deserializeBuffer, &mAllocator, *this));


        WIRE_TRY(DoDeviceForceLoss(cmd.self, cmd.type, cmd.message));

        return WireResult::Success;
    }

    WireResult Server::HandleDeviceGetQueue(DeserializeBuffer* deserializeBuffer) {
        DeviceGetQueueCmd cmd;
        WIRE_TRY(cmd.Deserialize(deserializeBuffer, &mAllocator, *this));

        Reserved<WGPUQueue> resultData;
        WIRE_TRY(Allocate(&resultData, cmd.result));
        resultData->generation = cmd.result.generation;

        WIRE_TRY(DoDeviceGetQueue(cmd.self, &resultData->handle));

        return WireResult::Success;
    }

    WireResult Server::HandleDeviceInjectError(DeserializeBuffer* deserializeBuffer) {
        DeviceInjectErrorCmd cmd;
        WIRE_TRY(cmd.Deserialize(deserializeBuffer, &mAllocator, *this));


        WIRE_TRY(DoDeviceInjectError(cmd.self, cmd.type, cmd.message));

        return WireResult::Success;
    }

    WireResult Server::HandleDevicePopErrorScope(DeserializeBuffer* deserializeBuffer) {
        DevicePopErrorScopeCmd cmd;
        WIRE_TRY(cmd.Deserialize(deserializeBuffer, &mAllocator));

        Known<WGPUDevice> deviceIdHandle;
        WIRE_TRY(Get(cmd.deviceId, &deviceIdHandle));
        Known<WGPUInstance> instanceIdHandle;
        WIRE_TRY(Get(cmd.instanceId, &instanceIdHandle));

        WIRE_TRY(DoDevicePopErrorScope(deviceIdHandle, instanceIdHandle, cmd.future));

        return WireResult::Success;
    }

    WireResult Server::HandleDevicePushErrorScope(DeserializeBuffer* deserializeBuffer) {
        DevicePushErrorScopeCmd cmd;
        WIRE_TRY(cmd.Deserialize(deserializeBuffer, &mAllocator, *this));


        WIRE_TRY(DoDevicePushErrorScope(cmd.self, cmd.filter));

        return WireResult::Success;
    }

    WireResult Server::HandleDeviceSetLabel(DeserializeBuffer* deserializeBuffer) {
        DeviceSetLabelCmd cmd;
        WIRE_TRY(cmd.Deserialize(deserializeBuffer, &mAllocator, *this));


        WIRE_TRY(DoDeviceSetLabel(cmd.self, cmd.label));

        return WireResult::Success;
    }

    WireResult Server::HandleDeviceTick(DeserializeBuffer* deserializeBuffer) {
        DeviceTickCmd cmd;
        WIRE_TRY(cmd.Deserialize(deserializeBuffer, &mAllocator, *this));


        WIRE_TRY(DoDeviceTick(cmd.self));

        return WireResult::Success;
    }

    WireResult Server::HandleDeviceValidateTextureDescriptor(DeserializeBuffer* deserializeBuffer) {
        DeviceValidateTextureDescriptorCmd cmd;
        WIRE_TRY(cmd.Deserialize(deserializeBuffer, &mAllocator, *this));


        WIRE_TRY(DoDeviceValidateTextureDescriptor(cmd.self, cmd.descriptor));

        return WireResult::Success;
    }

    WireResult Server::HandleExternalTextureDestroy(DeserializeBuffer* deserializeBuffer) {
        ExternalTextureDestroyCmd cmd;
        WIRE_TRY(cmd.Deserialize(deserializeBuffer, &mAllocator, *this));


        WIRE_TRY(DoExternalTextureDestroy(cmd.self));

        return WireResult::Success;
    }

    WireResult Server::HandleExternalTextureExpire(DeserializeBuffer* deserializeBuffer) {
        ExternalTextureExpireCmd cmd;
        WIRE_TRY(cmd.Deserialize(deserializeBuffer, &mAllocator, *this));


        WIRE_TRY(DoExternalTextureExpire(cmd.self));

        return WireResult::Success;
    }

    WireResult Server::HandleExternalTextureRefresh(DeserializeBuffer* deserializeBuffer) {
        ExternalTextureRefreshCmd cmd;
        WIRE_TRY(cmd.Deserialize(deserializeBuffer, &mAllocator, *this));


        WIRE_TRY(DoExternalTextureRefresh(cmd.self));

        return WireResult::Success;
    }

    WireResult Server::HandleExternalTextureSetLabel(DeserializeBuffer* deserializeBuffer) {
        ExternalTextureSetLabelCmd cmd;
        WIRE_TRY(cmd.Deserialize(deserializeBuffer, &mAllocator, *this));


        WIRE_TRY(DoExternalTextureSetLabel(cmd.self, cmd.label));

        return WireResult::Success;
    }

    WireResult Server::HandleInstanceProcessEvents(DeserializeBuffer* deserializeBuffer) {
        InstanceProcessEventsCmd cmd;
        WIRE_TRY(cmd.Deserialize(deserializeBuffer, &mAllocator, *this));


        WIRE_TRY(DoInstanceProcessEvents(cmd.self));

        return WireResult::Success;
    }

    WireResult Server::HandleInstanceRequestAdapter(DeserializeBuffer* deserializeBuffer) {
        InstanceRequestAdapterCmd cmd;
        WIRE_TRY(cmd.Deserialize(deserializeBuffer, &mAllocator, *this));

        Known<WGPUInstance> instanceIdHandle;
        WIRE_TRY(Get(cmd.instanceId, &instanceIdHandle));

        WIRE_TRY(DoInstanceRequestAdapter(instanceIdHandle, cmd.future, cmd.adapterObjectHandle, cmd.options));

        return WireResult::Success;
    }

    WireResult Server::HandlePipelineLayoutSetLabel(DeserializeBuffer* deserializeBuffer) {
        PipelineLayoutSetLabelCmd cmd;
        WIRE_TRY(cmd.Deserialize(deserializeBuffer, &mAllocator, *this));


        WIRE_TRY(DoPipelineLayoutSetLabel(cmd.self, cmd.label));

        return WireResult::Success;
    }

    WireResult Server::HandleQuerySetDestroy(DeserializeBuffer* deserializeBuffer) {
        QuerySetDestroyCmd cmd;
        WIRE_TRY(cmd.Deserialize(deserializeBuffer, &mAllocator, *this));


        WIRE_TRY(DoQuerySetDestroy(cmd.self));

        return WireResult::Success;
    }

    WireResult Server::HandleQuerySetSetLabel(DeserializeBuffer* deserializeBuffer) {
        QuerySetSetLabelCmd cmd;
        WIRE_TRY(cmd.Deserialize(deserializeBuffer, &mAllocator, *this));


        WIRE_TRY(DoQuerySetSetLabel(cmd.self, cmd.label));

        return WireResult::Success;
    }

    WireResult Server::HandleQueueCopyExternalTextureForBrowser(DeserializeBuffer* deserializeBuffer) {
        QueueCopyExternalTextureForBrowserCmd cmd;
        WIRE_TRY(cmd.Deserialize(deserializeBuffer, &mAllocator, *this));


        WIRE_TRY(DoQueueCopyExternalTextureForBrowser(cmd.self, cmd.source, cmd.destination, cmd.copySize, cmd.options));

        return WireResult::Success;
    }

    WireResult Server::HandleQueueCopyTextureForBrowser(DeserializeBuffer* deserializeBuffer) {
        QueueCopyTextureForBrowserCmd cmd;
        WIRE_TRY(cmd.Deserialize(deserializeBuffer, &mAllocator, *this));


        WIRE_TRY(DoQueueCopyTextureForBrowser(cmd.self, cmd.source, cmd.destination, cmd.copySize, cmd.options));

        return WireResult::Success;
    }

    WireResult Server::HandleQueueOnSubmittedWorkDone(DeserializeBuffer* deserializeBuffer) {
        QueueOnSubmittedWorkDoneCmd cmd;
        WIRE_TRY(cmd.Deserialize(deserializeBuffer, &mAllocator));

        Known<WGPUQueue> queueIdHandle;
        WIRE_TRY(Get(cmd.queueId, &queueIdHandle));
        Known<WGPUInstance> instanceIdHandle;
        WIRE_TRY(Get(cmd.instanceId, &instanceIdHandle));

        WIRE_TRY(DoQueueOnSubmittedWorkDone(queueIdHandle, instanceIdHandle, cmd.future));

        return WireResult::Success;
    }

    WireResult Server::HandleQueueSetLabel(DeserializeBuffer* deserializeBuffer) {
        QueueSetLabelCmd cmd;
        WIRE_TRY(cmd.Deserialize(deserializeBuffer, &mAllocator, *this));


        WIRE_TRY(DoQueueSetLabel(cmd.self, cmd.label));

        return WireResult::Success;
    }

    WireResult Server::HandleQueueSubmit(DeserializeBuffer* deserializeBuffer) {
        QueueSubmitCmd cmd;
        WIRE_TRY(cmd.Deserialize(deserializeBuffer, &mAllocator, *this));


        WIRE_TRY(DoQueueSubmit(cmd.self, cmd.commandCount, cmd.commands));

        return WireResult::Success;
    }

    WireResult Server::HandleQueueWriteBuffer(DeserializeBuffer* deserializeBuffer) {
        QueueWriteBufferCmd cmd;
        WIRE_TRY(cmd.Deserialize(deserializeBuffer, &mAllocator));

        Known<WGPUQueue> queueIdHandle;
        WIRE_TRY(Get(cmd.queueId, &queueIdHandle));
        Known<WGPUBuffer> bufferIdHandle;
        WIRE_TRY(Get(cmd.bufferId, &bufferIdHandle));

        WIRE_TRY(DoQueueWriteBuffer(queueIdHandle, bufferIdHandle, cmd.bufferOffset, cmd.data, cmd.size));

        return WireResult::Success;
    }

    WireResult Server::HandleQueueWriteBufferXl(DeserializeBuffer* deserializeBuffer) {
        QueueWriteBufferXlCmd cmd;
        WIRE_TRY(cmd.Deserialize(deserializeBuffer, &mAllocator));

        Known<WGPUQueue> queueIdHandle;
        WIRE_TRY(Get(cmd.queueId, &queueIdHandle));
        Known<WGPUBuffer> bufferIdHandle;
        WIRE_TRY(Get(cmd.bufferId, &bufferIdHandle));

        WIRE_TRY(DoQueueWriteBufferXl(queueIdHandle, bufferIdHandle, cmd.bufferOffset, cmd.size, cmd.memoryHandleCreateInfoLength, cmd.memoryHandleCreateInfo, cmd.memoryDataUpdateInfoLength, cmd.memoryDataUpdateInfo));

        return WireResult::Success;
    }

    WireResult Server::HandleQueueWriteTexture(DeserializeBuffer* deserializeBuffer) {
        QueueWriteTextureCmd cmd;
        WIRE_TRY(cmd.Deserialize(deserializeBuffer, &mAllocator, *this));

        Known<WGPUQueue> queueIdHandle;
        WIRE_TRY(Get(cmd.queueId, &queueIdHandle));

        WIRE_TRY(DoQueueWriteTexture(queueIdHandle, cmd.destination, cmd.data, cmd.dataSize, cmd.dataLayout, cmd.writeSize));

        return WireResult::Success;
    }

    WireResult Server::HandleQueueWriteTextureXl(DeserializeBuffer* deserializeBuffer) {
        QueueWriteTextureXlCmd cmd;
        WIRE_TRY(cmd.Deserialize(deserializeBuffer, &mAllocator, *this));

        Known<WGPUQueue> queueIdHandle;
        WIRE_TRY(Get(cmd.queueId, &queueIdHandle));

        WIRE_TRY(DoQueueWriteTextureXl(queueIdHandle, cmd.destination, cmd.dataSize, cmd.dataLayout, cmd.writeSize, cmd.memoryHandleCreateInfoLength, cmd.memoryHandleCreateInfo, cmd.memoryDataUpdateInfoLength, cmd.memoryDataUpdateInfo));

        return WireResult::Success;
    }

    WireResult Server::HandleRenderBundleEncoderDraw(DeserializeBuffer* deserializeBuffer) {
        RenderBundleEncoderDrawCmd cmd;
        WIRE_TRY(cmd.Deserialize(deserializeBuffer, &mAllocator, *this));


        WIRE_TRY(DoRenderBundleEncoderDraw(cmd.self, cmd.vertexCount, cmd.instanceCount, cmd.firstVertex, cmd.firstInstance));

        return WireResult::Success;
    }

    WireResult Server::HandleRenderBundleEncoderDrawIndexed(DeserializeBuffer* deserializeBuffer) {
        RenderBundleEncoderDrawIndexedCmd cmd;
        WIRE_TRY(cmd.Deserialize(deserializeBuffer, &mAllocator, *this));


        WIRE_TRY(DoRenderBundleEncoderDrawIndexed(cmd.self, cmd.indexCount, cmd.instanceCount, cmd.firstIndex, cmd.baseVertex, cmd.firstInstance));

        return WireResult::Success;
    }

    WireResult Server::HandleRenderBundleEncoderDrawIndexedIndirect(DeserializeBuffer* deserializeBuffer) {
        RenderBundleEncoderDrawIndexedIndirectCmd cmd;
        WIRE_TRY(cmd.Deserialize(deserializeBuffer, &mAllocator, *this));


        WIRE_TRY(DoRenderBundleEncoderDrawIndexedIndirect(cmd.self, cmd.indirectBuffer, cmd.indirectOffset));

        return WireResult::Success;
    }

    WireResult Server::HandleRenderBundleEncoderDrawIndirect(DeserializeBuffer* deserializeBuffer) {
        RenderBundleEncoderDrawIndirectCmd cmd;
        WIRE_TRY(cmd.Deserialize(deserializeBuffer, &mAllocator, *this));


        WIRE_TRY(DoRenderBundleEncoderDrawIndirect(cmd.self, cmd.indirectBuffer, cmd.indirectOffset));

        return WireResult::Success;
    }

    WireResult Server::HandleRenderBundleEncoderFinish(DeserializeBuffer* deserializeBuffer) {
        RenderBundleEncoderFinishCmd cmd;
        WIRE_TRY(cmd.Deserialize(deserializeBuffer, &mAllocator, *this));

        Reserved<WGPURenderBundle> resultData;
        WIRE_TRY(Allocate(&resultData, cmd.result));
        resultData->generation = cmd.result.generation;

        WIRE_TRY(DoRenderBundleEncoderFinish(cmd.self, cmd.descriptor, &resultData->handle));

        return WireResult::Success;
    }

    WireResult Server::HandleRenderBundleEncoderInsertDebugMarker(DeserializeBuffer* deserializeBuffer) {
        RenderBundleEncoderInsertDebugMarkerCmd cmd;
        WIRE_TRY(cmd.Deserialize(deserializeBuffer, &mAllocator, *this));


        WIRE_TRY(DoRenderBundleEncoderInsertDebugMarker(cmd.self, cmd.markerLabel));

        return WireResult::Success;
    }

    WireResult Server::HandleRenderBundleEncoderPopDebugGroup(DeserializeBuffer* deserializeBuffer) {
        RenderBundleEncoderPopDebugGroupCmd cmd;
        WIRE_TRY(cmd.Deserialize(deserializeBuffer, &mAllocator, *this));


        WIRE_TRY(DoRenderBundleEncoderPopDebugGroup(cmd.self));

        return WireResult::Success;
    }

    WireResult Server::HandleRenderBundleEncoderPushDebugGroup(DeserializeBuffer* deserializeBuffer) {
        RenderBundleEncoderPushDebugGroupCmd cmd;
        WIRE_TRY(cmd.Deserialize(deserializeBuffer, &mAllocator, *this));


        WIRE_TRY(DoRenderBundleEncoderPushDebugGroup(cmd.self, cmd.groupLabel));

        return WireResult::Success;
    }

    WireResult Server::HandleRenderBundleEncoderSetBindGroup(DeserializeBuffer* deserializeBuffer) {
        RenderBundleEncoderSetBindGroupCmd cmd;
        WIRE_TRY(cmd.Deserialize(deserializeBuffer, &mAllocator, *this));


        WIRE_TRY(DoRenderBundleEncoderSetBindGroup(cmd.self, cmd.groupIndex, cmd.group, cmd.dynamicOffsetCount, cmd.dynamicOffsets));

        return WireResult::Success;
    }

    WireResult Server::HandleRenderBundleEncoderSetImmediates(DeserializeBuffer* deserializeBuffer) {
        RenderBundleEncoderSetImmediatesCmd cmd;
        WIRE_TRY(cmd.Deserialize(deserializeBuffer, &mAllocator));

        Known<WGPURenderBundleEncoder> renderBundleEncoderIdHandle;
        WIRE_TRY(Get(cmd.renderBundleEncoderId, &renderBundleEncoderIdHandle));

        WIRE_TRY(DoRenderBundleEncoderSetImmediates(renderBundleEncoderIdHandle, cmd.offset, cmd.data, cmd.size));

        return WireResult::Success;
    }

    WireResult Server::HandleRenderBundleEncoderSetIndexBuffer(DeserializeBuffer* deserializeBuffer) {
        RenderBundleEncoderSetIndexBufferCmd cmd;
        WIRE_TRY(cmd.Deserialize(deserializeBuffer, &mAllocator, *this));


        WIRE_TRY(DoRenderBundleEncoderSetIndexBuffer(cmd.self, cmd.buffer, cmd.format, cmd.offset, cmd.size));

        return WireResult::Success;
    }

    WireResult Server::HandleRenderBundleEncoderSetLabel(DeserializeBuffer* deserializeBuffer) {
        RenderBundleEncoderSetLabelCmd cmd;
        WIRE_TRY(cmd.Deserialize(deserializeBuffer, &mAllocator, *this));


        WIRE_TRY(DoRenderBundleEncoderSetLabel(cmd.self, cmd.label));

        return WireResult::Success;
    }

    WireResult Server::HandleRenderBundleEncoderSetPipeline(DeserializeBuffer* deserializeBuffer) {
        RenderBundleEncoderSetPipelineCmd cmd;
        WIRE_TRY(cmd.Deserialize(deserializeBuffer, &mAllocator, *this));


        WIRE_TRY(DoRenderBundleEncoderSetPipeline(cmd.self, cmd.pipeline));

        return WireResult::Success;
    }

    WireResult Server::HandleRenderBundleEncoderSetResourceTable(DeserializeBuffer* deserializeBuffer) {
        RenderBundleEncoderSetResourceTableCmd cmd;
        WIRE_TRY(cmd.Deserialize(deserializeBuffer, &mAllocator, *this));


        WIRE_TRY(DoRenderBundleEncoderSetResourceTable(cmd.self, cmd.table));

        return WireResult::Success;
    }

    WireResult Server::HandleRenderBundleEncoderSetVertexBuffer(DeserializeBuffer* deserializeBuffer) {
        RenderBundleEncoderSetVertexBufferCmd cmd;
        WIRE_TRY(cmd.Deserialize(deserializeBuffer, &mAllocator, *this));


        WIRE_TRY(DoRenderBundleEncoderSetVertexBuffer(cmd.self, cmd.slot, cmd.buffer, cmd.offset, cmd.size));

        return WireResult::Success;
    }

    WireResult Server::HandleRenderBundleSetLabel(DeserializeBuffer* deserializeBuffer) {
        RenderBundleSetLabelCmd cmd;
        WIRE_TRY(cmd.Deserialize(deserializeBuffer, &mAllocator, *this));


        WIRE_TRY(DoRenderBundleSetLabel(cmd.self, cmd.label));

        return WireResult::Success;
    }

    WireResult Server::HandleRenderPassEncoderBeginOcclusionQuery(DeserializeBuffer* deserializeBuffer) {
        RenderPassEncoderBeginOcclusionQueryCmd cmd;
        WIRE_TRY(cmd.Deserialize(deserializeBuffer, &mAllocator, *this));


        WIRE_TRY(DoRenderPassEncoderBeginOcclusionQuery(cmd.self, cmd.queryIndex));

        return WireResult::Success;
    }

    WireResult Server::HandleRenderPassEncoderDraw(DeserializeBuffer* deserializeBuffer) {
        RenderPassEncoderDrawCmd cmd;
        WIRE_TRY(cmd.Deserialize(deserializeBuffer, &mAllocator, *this));


        WIRE_TRY(DoRenderPassEncoderDraw(cmd.self, cmd.vertexCount, cmd.instanceCount, cmd.firstVertex, cmd.firstInstance));

        return WireResult::Success;
    }

    WireResult Server::HandleRenderPassEncoderDrawIndexed(DeserializeBuffer* deserializeBuffer) {
        RenderPassEncoderDrawIndexedCmd cmd;
        WIRE_TRY(cmd.Deserialize(deserializeBuffer, &mAllocator, *this));


        WIRE_TRY(DoRenderPassEncoderDrawIndexed(cmd.self, cmd.indexCount, cmd.instanceCount, cmd.firstIndex, cmd.baseVertex, cmd.firstInstance));

        return WireResult::Success;
    }

    WireResult Server::HandleRenderPassEncoderDrawIndexedIndirect(DeserializeBuffer* deserializeBuffer) {
        RenderPassEncoderDrawIndexedIndirectCmd cmd;
        WIRE_TRY(cmd.Deserialize(deserializeBuffer, &mAllocator, *this));


        WIRE_TRY(DoRenderPassEncoderDrawIndexedIndirect(cmd.self, cmd.indirectBuffer, cmd.indirectOffset));

        return WireResult::Success;
    }

    WireResult Server::HandleRenderPassEncoderDrawIndirect(DeserializeBuffer* deserializeBuffer) {
        RenderPassEncoderDrawIndirectCmd cmd;
        WIRE_TRY(cmd.Deserialize(deserializeBuffer, &mAllocator, *this));


        WIRE_TRY(DoRenderPassEncoderDrawIndirect(cmd.self, cmd.indirectBuffer, cmd.indirectOffset));

        return WireResult::Success;
    }

    WireResult Server::HandleRenderPassEncoderEnd(DeserializeBuffer* deserializeBuffer) {
        RenderPassEncoderEndCmd cmd;
        WIRE_TRY(cmd.Deserialize(deserializeBuffer, &mAllocator, *this));


        WIRE_TRY(DoRenderPassEncoderEnd(cmd.self));

        return WireResult::Success;
    }

    WireResult Server::HandleRenderPassEncoderEndOcclusionQuery(DeserializeBuffer* deserializeBuffer) {
        RenderPassEncoderEndOcclusionQueryCmd cmd;
        WIRE_TRY(cmd.Deserialize(deserializeBuffer, &mAllocator, *this));


        WIRE_TRY(DoRenderPassEncoderEndOcclusionQuery(cmd.self));

        return WireResult::Success;
    }

    WireResult Server::HandleRenderPassEncoderExecuteBundles(DeserializeBuffer* deserializeBuffer) {
        RenderPassEncoderExecuteBundlesCmd cmd;
        WIRE_TRY(cmd.Deserialize(deserializeBuffer, &mAllocator, *this));


        WIRE_TRY(DoRenderPassEncoderExecuteBundles(cmd.self, cmd.bundleCount, cmd.bundles));

        return WireResult::Success;
    }

    WireResult Server::HandleRenderPassEncoderInsertDebugMarker(DeserializeBuffer* deserializeBuffer) {
        RenderPassEncoderInsertDebugMarkerCmd cmd;
        WIRE_TRY(cmd.Deserialize(deserializeBuffer, &mAllocator, *this));


        WIRE_TRY(DoRenderPassEncoderInsertDebugMarker(cmd.self, cmd.markerLabel));

        return WireResult::Success;
    }

    WireResult Server::HandleRenderPassEncoderMultiDrawIndexedIndirect(DeserializeBuffer* deserializeBuffer) {
        RenderPassEncoderMultiDrawIndexedIndirectCmd cmd;
        WIRE_TRY(cmd.Deserialize(deserializeBuffer, &mAllocator, *this));


        WIRE_TRY(DoRenderPassEncoderMultiDrawIndexedIndirect(cmd.self, cmd.indirectBuffer, cmd.indirectOffset, cmd.maxDrawCount, cmd.drawCountBuffer, cmd.drawCountBufferOffset));

        return WireResult::Success;
    }

    WireResult Server::HandleRenderPassEncoderMultiDrawIndirect(DeserializeBuffer* deserializeBuffer) {
        RenderPassEncoderMultiDrawIndirectCmd cmd;
        WIRE_TRY(cmd.Deserialize(deserializeBuffer, &mAllocator, *this));


        WIRE_TRY(DoRenderPassEncoderMultiDrawIndirect(cmd.self, cmd.indirectBuffer, cmd.indirectOffset, cmd.maxDrawCount, cmd.drawCountBuffer, cmd.drawCountBufferOffset));

        return WireResult::Success;
    }

    WireResult Server::HandleRenderPassEncoderPixelLocalStorageBarrier(DeserializeBuffer* deserializeBuffer) {
        RenderPassEncoderPixelLocalStorageBarrierCmd cmd;
        WIRE_TRY(cmd.Deserialize(deserializeBuffer, &mAllocator, *this));


        WIRE_TRY(DoRenderPassEncoderPixelLocalStorageBarrier(cmd.self));

        return WireResult::Success;
    }

    WireResult Server::HandleRenderPassEncoderPopDebugGroup(DeserializeBuffer* deserializeBuffer) {
        RenderPassEncoderPopDebugGroupCmd cmd;
        WIRE_TRY(cmd.Deserialize(deserializeBuffer, &mAllocator, *this));


        WIRE_TRY(DoRenderPassEncoderPopDebugGroup(cmd.self));

        return WireResult::Success;
    }

    WireResult Server::HandleRenderPassEncoderPushDebugGroup(DeserializeBuffer* deserializeBuffer) {
        RenderPassEncoderPushDebugGroupCmd cmd;
        WIRE_TRY(cmd.Deserialize(deserializeBuffer, &mAllocator, *this));


        WIRE_TRY(DoRenderPassEncoderPushDebugGroup(cmd.self, cmd.groupLabel));

        return WireResult::Success;
    }

    WireResult Server::HandleRenderPassEncoderSetBindGroup(DeserializeBuffer* deserializeBuffer) {
        RenderPassEncoderSetBindGroupCmd cmd;
        WIRE_TRY(cmd.Deserialize(deserializeBuffer, &mAllocator, *this));


        WIRE_TRY(DoRenderPassEncoderSetBindGroup(cmd.self, cmd.groupIndex, cmd.group, cmd.dynamicOffsetCount, cmd.dynamicOffsets));

        return WireResult::Success;
    }

    WireResult Server::HandleRenderPassEncoderSetBlendConstant(DeserializeBuffer* deserializeBuffer) {
        RenderPassEncoderSetBlendConstantCmd cmd;
        WIRE_TRY(cmd.Deserialize(deserializeBuffer, &mAllocator, *this));


        WIRE_TRY(DoRenderPassEncoderSetBlendConstant(cmd.self, cmd.color));

        return WireResult::Success;
    }

    WireResult Server::HandleRenderPassEncoderSetImmediates(DeserializeBuffer* deserializeBuffer) {
        RenderPassEncoderSetImmediatesCmd cmd;
        WIRE_TRY(cmd.Deserialize(deserializeBuffer, &mAllocator));

        Known<WGPURenderPassEncoder> renderPassEncoderIdHandle;
        WIRE_TRY(Get(cmd.renderPassEncoderId, &renderPassEncoderIdHandle));

        WIRE_TRY(DoRenderPassEncoderSetImmediates(renderPassEncoderIdHandle, cmd.offset, cmd.data, cmd.size));

        return WireResult::Success;
    }

    WireResult Server::HandleRenderPassEncoderSetIndexBuffer(DeserializeBuffer* deserializeBuffer) {
        RenderPassEncoderSetIndexBufferCmd cmd;
        WIRE_TRY(cmd.Deserialize(deserializeBuffer, &mAllocator, *this));


        WIRE_TRY(DoRenderPassEncoderSetIndexBuffer(cmd.self, cmd.buffer, cmd.format, cmd.offset, cmd.size));

        return WireResult::Success;
    }

    WireResult Server::HandleRenderPassEncoderSetLabel(DeserializeBuffer* deserializeBuffer) {
        RenderPassEncoderSetLabelCmd cmd;
        WIRE_TRY(cmd.Deserialize(deserializeBuffer, &mAllocator, *this));


        WIRE_TRY(DoRenderPassEncoderSetLabel(cmd.self, cmd.label));

        return WireResult::Success;
    }

    WireResult Server::HandleRenderPassEncoderSetPipeline(DeserializeBuffer* deserializeBuffer) {
        RenderPassEncoderSetPipelineCmd cmd;
        WIRE_TRY(cmd.Deserialize(deserializeBuffer, &mAllocator, *this));


        WIRE_TRY(DoRenderPassEncoderSetPipeline(cmd.self, cmd.pipeline));

        return WireResult::Success;
    }

    WireResult Server::HandleRenderPassEncoderSetResourceTable(DeserializeBuffer* deserializeBuffer) {
        RenderPassEncoderSetResourceTableCmd cmd;
        WIRE_TRY(cmd.Deserialize(deserializeBuffer, &mAllocator, *this));


        WIRE_TRY(DoRenderPassEncoderSetResourceTable(cmd.self, cmd.table));

        return WireResult::Success;
    }

    WireResult Server::HandleRenderPassEncoderSetScissorRect(DeserializeBuffer* deserializeBuffer) {
        RenderPassEncoderSetScissorRectCmd cmd;
        WIRE_TRY(cmd.Deserialize(deserializeBuffer, &mAllocator, *this));


        WIRE_TRY(DoRenderPassEncoderSetScissorRect(cmd.self, cmd.x, cmd.y, cmd.width, cmd.height));

        return WireResult::Success;
    }

    WireResult Server::HandleRenderPassEncoderSetStencilReference(DeserializeBuffer* deserializeBuffer) {
        RenderPassEncoderSetStencilReferenceCmd cmd;
        WIRE_TRY(cmd.Deserialize(deserializeBuffer, &mAllocator, *this));


        WIRE_TRY(DoRenderPassEncoderSetStencilReference(cmd.self, cmd.reference));

        return WireResult::Success;
    }

    WireResult Server::HandleRenderPassEncoderSetVertexBuffer(DeserializeBuffer* deserializeBuffer) {
        RenderPassEncoderSetVertexBufferCmd cmd;
        WIRE_TRY(cmd.Deserialize(deserializeBuffer, &mAllocator, *this));


        WIRE_TRY(DoRenderPassEncoderSetVertexBuffer(cmd.self, cmd.slot, cmd.buffer, cmd.offset, cmd.size));

        return WireResult::Success;
    }

    WireResult Server::HandleRenderPassEncoderSetViewport(DeserializeBuffer* deserializeBuffer) {
        RenderPassEncoderSetViewportCmd cmd;
        WIRE_TRY(cmd.Deserialize(deserializeBuffer, &mAllocator, *this));


        WIRE_TRY(DoRenderPassEncoderSetViewport(cmd.self, cmd.x, cmd.y, cmd.width, cmd.height, cmd.minDepth, cmd.maxDepth));

        return WireResult::Success;
    }

    WireResult Server::HandleRenderPassEncoderWriteTimestamp(DeserializeBuffer* deserializeBuffer) {
        RenderPassEncoderWriteTimestampCmd cmd;
        WIRE_TRY(cmd.Deserialize(deserializeBuffer, &mAllocator, *this));


        WIRE_TRY(DoRenderPassEncoderWriteTimestamp(cmd.self, cmd.querySet, cmd.queryIndex));

        return WireResult::Success;
    }

    WireResult Server::HandleRenderPipelineGetBindGroupLayout(DeserializeBuffer* deserializeBuffer) {
        RenderPipelineGetBindGroupLayoutCmd cmd;
        WIRE_TRY(cmd.Deserialize(deserializeBuffer, &mAllocator, *this));

        Reserved<WGPUBindGroupLayout> resultData;
        WIRE_TRY(Allocate(&resultData, cmd.result));
        resultData->generation = cmd.result.generation;

        WIRE_TRY(DoRenderPipelineGetBindGroupLayout(cmd.self, cmd.groupIndex, &resultData->handle));

        return WireResult::Success;
    }

    WireResult Server::HandleRenderPipelineSetLabel(DeserializeBuffer* deserializeBuffer) {
        RenderPipelineSetLabelCmd cmd;
        WIRE_TRY(cmd.Deserialize(deserializeBuffer, &mAllocator, *this));


        WIRE_TRY(DoRenderPipelineSetLabel(cmd.self, cmd.label));

        return WireResult::Success;
    }

    WireResult Server::HandleResourceTableDestroy(DeserializeBuffer* deserializeBuffer) {
        ResourceTableDestroyCmd cmd;
        WIRE_TRY(cmd.Deserialize(deserializeBuffer, &mAllocator, *this));


        WIRE_TRY(DoResourceTableDestroy(cmd.self));

        return WireResult::Success;
    }

    WireResult Server::HandleResourceTableRemove(DeserializeBuffer* deserializeBuffer) {
        ResourceTableRemoveCmd cmd;
        WIRE_TRY(cmd.Deserialize(deserializeBuffer, &mAllocator, *this));


        WIRE_TRY(DoResourceTableRemove(cmd.self, cmd.slot));

        return WireResult::Success;
    }

    WireResult Server::HandleResourceTableSetLabel(DeserializeBuffer* deserializeBuffer) {
        ResourceTableSetLabelCmd cmd;
        WIRE_TRY(cmd.Deserialize(deserializeBuffer, &mAllocator, *this));


        WIRE_TRY(DoResourceTableSetLabel(cmd.self, cmd.label));

        return WireResult::Success;
    }

    WireResult Server::HandleResourceTableUpdate(DeserializeBuffer* deserializeBuffer) {
        ResourceTableUpdateCmd cmd;
        WIRE_TRY(cmd.Deserialize(deserializeBuffer, &mAllocator, *this));


        WIRE_TRY(DoResourceTableUpdate(cmd.self, cmd.slot, cmd.resource));

        return WireResult::Success;
    }

    WireResult Server::HandleSamplerSetLabel(DeserializeBuffer* deserializeBuffer) {
        SamplerSetLabelCmd cmd;
        WIRE_TRY(cmd.Deserialize(deserializeBuffer, &mAllocator, *this));


        WIRE_TRY(DoSamplerSetLabel(cmd.self, cmd.label));

        return WireResult::Success;
    }

    WireResult Server::HandleShaderModuleGetCompilationInfo(DeserializeBuffer* deserializeBuffer) {
        ShaderModuleGetCompilationInfoCmd cmd;
        WIRE_TRY(cmd.Deserialize(deserializeBuffer, &mAllocator));

        Known<WGPUShaderModule> shaderModuleIdHandle;
        WIRE_TRY(Get(cmd.shaderModuleId, &shaderModuleIdHandle));
        Known<WGPUInstance> instanceIdHandle;
        WIRE_TRY(Get(cmd.instanceId, &instanceIdHandle));

        WIRE_TRY(DoShaderModuleGetCompilationInfo(shaderModuleIdHandle, instanceIdHandle, cmd.future));

        return WireResult::Success;
    }

    WireResult Server::HandleShaderModuleSetLabel(DeserializeBuffer* deserializeBuffer) {
        ShaderModuleSetLabelCmd cmd;
        WIRE_TRY(cmd.Deserialize(deserializeBuffer, &mAllocator, *this));


        WIRE_TRY(DoShaderModuleSetLabel(cmd.self, cmd.label));

        return WireResult::Success;
    }

    WireResult Server::HandleSurfaceConfigure(DeserializeBuffer* deserializeBuffer) {
        SurfaceConfigureCmd cmd;
        WIRE_TRY(cmd.Deserialize(deserializeBuffer, &mAllocator, *this));


        WIRE_TRY(DoSurfaceConfigure(cmd.self, cmd.config));

        return WireResult::Success;
    }

    WireResult Server::HandleSurfaceGetCurrentTexture(DeserializeBuffer* deserializeBuffer) {
        SurfaceGetCurrentTextureCmd cmd;
        WIRE_TRY(cmd.Deserialize(deserializeBuffer, &mAllocator));

        Known<WGPUSurface> surfaceIdHandle;
        WIRE_TRY(Get(cmd.surfaceId, &surfaceIdHandle));
        Known<WGPUDevice> configuredDeviceIdHandle;
        WIRE_TRY(Get(cmd.configuredDeviceId, &configuredDeviceIdHandle));

        WIRE_TRY(DoSurfaceGetCurrentTexture(surfaceIdHandle, configuredDeviceIdHandle, cmd.textureHandle));

        return WireResult::Success;
    }

    WireResult Server::HandleSurfacePresent(DeserializeBuffer* deserializeBuffer) {
        SurfacePresentCmd cmd;
        WIRE_TRY(cmd.Deserialize(deserializeBuffer, &mAllocator, *this));


        WIRE_TRY(DoSurfacePresent(cmd.self));

        return WireResult::Success;
    }

    WireResult Server::HandleSurfaceSetLabel(DeserializeBuffer* deserializeBuffer) {
        SurfaceSetLabelCmd cmd;
        WIRE_TRY(cmd.Deserialize(deserializeBuffer, &mAllocator, *this));


        WIRE_TRY(DoSurfaceSetLabel(cmd.self, cmd.label));

        return WireResult::Success;
    }

    WireResult Server::HandleSurfaceUnconfigure(DeserializeBuffer* deserializeBuffer) {
        SurfaceUnconfigureCmd cmd;
        WIRE_TRY(cmd.Deserialize(deserializeBuffer, &mAllocator, *this));


        WIRE_TRY(DoSurfaceUnconfigure(cmd.self));

        return WireResult::Success;
    }

    WireResult Server::HandleTexelBufferViewSetLabel(DeserializeBuffer* deserializeBuffer) {
        TexelBufferViewSetLabelCmd cmd;
        WIRE_TRY(cmd.Deserialize(deserializeBuffer, &mAllocator, *this));


        WIRE_TRY(DoTexelBufferViewSetLabel(cmd.self, cmd.label));

        return WireResult::Success;
    }

    WireResult Server::HandleTextureCreateErrorView(DeserializeBuffer* deserializeBuffer) {
        TextureCreateErrorViewCmd cmd;
        WIRE_TRY(cmd.Deserialize(deserializeBuffer, &mAllocator, *this));

        Reserved<WGPUTextureView> resultData;
        WIRE_TRY(Allocate(&resultData, cmd.result));
        resultData->generation = cmd.result.generation;

        WIRE_TRY(DoTextureCreateErrorView(cmd.self, cmd.descriptor, &resultData->handle));

        return WireResult::Success;
    }

    WireResult Server::HandleTextureCreateView(DeserializeBuffer* deserializeBuffer) {
        TextureCreateViewCmd cmd;
        WIRE_TRY(cmd.Deserialize(deserializeBuffer, &mAllocator, *this));

        Reserved<WGPUTextureView> resultData;
        WIRE_TRY(Allocate(&resultData, cmd.result));
        resultData->generation = cmd.result.generation;

        WIRE_TRY(DoTextureCreateView(cmd.self, cmd.descriptor, &resultData->handle));

        return WireResult::Success;
    }

    WireResult Server::HandleTextureDestroy(DeserializeBuffer* deserializeBuffer) {
        TextureDestroyCmd cmd;
        WIRE_TRY(cmd.Deserialize(deserializeBuffer, &mAllocator, *this));


        WIRE_TRY(DoTextureDestroy(cmd.self));

        return WireResult::Success;
    }

    WireResult Server::HandleTextureSetLabel(DeserializeBuffer* deserializeBuffer) {
        TextureSetLabelCmd cmd;
        WIRE_TRY(cmd.Deserialize(deserializeBuffer, &mAllocator, *this));


        WIRE_TRY(DoTextureSetLabel(cmd.self, cmd.label));

        return WireResult::Success;
    }

    WireResult Server::HandleTextureViewSetLabel(DeserializeBuffer* deserializeBuffer) {
        TextureViewSetLabelCmd cmd;
        WIRE_TRY(cmd.Deserialize(deserializeBuffer, &mAllocator, *this));


        WIRE_TRY(DoTextureViewSetLabel(cmd.self, cmd.label));

        return WireResult::Success;
    }

    WireResult Server::HandleUnregisterObject(DeserializeBuffer* deserializeBuffer) {
        UnregisterObjectCmd cmd;
        WIRE_TRY(cmd.Deserialize(deserializeBuffer, &mAllocator));


        WIRE_TRY(DoUnregisterObject(cmd.objectType, cmd.objectId));

        return WireResult::Success;
    }

    bool Server::HandleCommands(Span<const volatile std::byte> commands) {
        DeserializeBuffer deserializeBuffer(commands);

        const volatile CmdHeader* cmdHeader;
        while (deserializeBuffer.Peek(&cmdHeader) != WireResult::FatalError) {
            WireCmd cmdId = cmdHeader->commandId;
            WireResult result;
            switch (cmdId) {
                case WireCmd::ChunkedCommand:
                    result = HandleChunkedCommand(&deserializeBuffer);
                    break;
                case WireCmd::AdapterRequestDevice:
                    result = HandleAdapterRequestDevice(&deserializeBuffer);
                    break;
                case WireCmd::BindGroupLayoutSetLabel:
                    result = HandleBindGroupLayoutSetLabel(&deserializeBuffer);
                    break;
                case WireCmd::BindGroupSetLabel:
                    result = HandleBindGroupSetLabel(&deserializeBuffer);
                    break;
                case WireCmd::BufferCreateTexelView:
                    result = HandleBufferCreateTexelView(&deserializeBuffer);
                    break;
                case WireCmd::BufferDestroy:
                    result = HandleBufferDestroy(&deserializeBuffer);
                    break;
                case WireCmd::BufferMapAsync:
                    result = HandleBufferMapAsync(&deserializeBuffer);
                    break;
                case WireCmd::BufferSetLabel:
                    result = HandleBufferSetLabel(&deserializeBuffer);
                    break;
                case WireCmd::BufferUnmap:
                    result = HandleBufferUnmap(&deserializeBuffer);
                    break;
                case WireCmd::BufferUpdateMappedData:
                    result = HandleBufferUpdateMappedData(&deserializeBuffer);
                    break;
                case WireCmd::CommandBufferSetLabel:
                    result = HandleCommandBufferSetLabel(&deserializeBuffer);
                    break;
                case WireCmd::CommandEncoderBeginComputePass:
                    result = HandleCommandEncoderBeginComputePass(&deserializeBuffer);
                    break;
                case WireCmd::CommandEncoderBeginRenderPass:
                    result = HandleCommandEncoderBeginRenderPass(&deserializeBuffer);
                    break;
                case WireCmd::CommandEncoderClearBuffer:
                    result = HandleCommandEncoderClearBuffer(&deserializeBuffer);
                    break;
                case WireCmd::CommandEncoderCopyBufferToBuffer:
                    result = HandleCommandEncoderCopyBufferToBuffer(&deserializeBuffer);
                    break;
                case WireCmd::CommandEncoderCopyBufferToTexture:
                    result = HandleCommandEncoderCopyBufferToTexture(&deserializeBuffer);
                    break;
                case WireCmd::CommandEncoderCopyTextureToBuffer:
                    result = HandleCommandEncoderCopyTextureToBuffer(&deserializeBuffer);
                    break;
                case WireCmd::CommandEncoderCopyTextureToTexture:
                    result = HandleCommandEncoderCopyTextureToTexture(&deserializeBuffer);
                    break;
                case WireCmd::CommandEncoderFinish:
                    result = HandleCommandEncoderFinish(&deserializeBuffer);
                    break;
                case WireCmd::CommandEncoderInjectValidationError:
                    result = HandleCommandEncoderInjectValidationError(&deserializeBuffer);
                    break;
                case WireCmd::CommandEncoderInsertDebugMarker:
                    result = HandleCommandEncoderInsertDebugMarker(&deserializeBuffer);
                    break;
                case WireCmd::CommandEncoderPopDebugGroup:
                    result = HandleCommandEncoderPopDebugGroup(&deserializeBuffer);
                    break;
                case WireCmd::CommandEncoderPushDebugGroup:
                    result = HandleCommandEncoderPushDebugGroup(&deserializeBuffer);
                    break;
                case WireCmd::CommandEncoderResolveQuerySet:
                    result = HandleCommandEncoderResolveQuerySet(&deserializeBuffer);
                    break;
                case WireCmd::CommandEncoderSetLabel:
                    result = HandleCommandEncoderSetLabel(&deserializeBuffer);
                    break;
                case WireCmd::CommandEncoderWriteBuffer:
                    result = HandleCommandEncoderWriteBuffer(&deserializeBuffer);
                    break;
                case WireCmd::CommandEncoderWriteTimestamp:
                    result = HandleCommandEncoderWriteTimestamp(&deserializeBuffer);
                    break;
                case WireCmd::ComputePassEncoderDispatchWorkgroups:
                    result = HandleComputePassEncoderDispatchWorkgroups(&deserializeBuffer);
                    break;
                case WireCmd::ComputePassEncoderDispatchWorkgroupsIndirect:
                    result = HandleComputePassEncoderDispatchWorkgroupsIndirect(&deserializeBuffer);
                    break;
                case WireCmd::ComputePassEncoderEnd:
                    result = HandleComputePassEncoderEnd(&deserializeBuffer);
                    break;
                case WireCmd::ComputePassEncoderInsertDebugMarker:
                    result = HandleComputePassEncoderInsertDebugMarker(&deserializeBuffer);
                    break;
                case WireCmd::ComputePassEncoderPopDebugGroup:
                    result = HandleComputePassEncoderPopDebugGroup(&deserializeBuffer);
                    break;
                case WireCmd::ComputePassEncoderPushDebugGroup:
                    result = HandleComputePassEncoderPushDebugGroup(&deserializeBuffer);
                    break;
                case WireCmd::ComputePassEncoderSetBindGroup:
                    result = HandleComputePassEncoderSetBindGroup(&deserializeBuffer);
                    break;
                case WireCmd::ComputePassEncoderSetImmediates:
                    result = HandleComputePassEncoderSetImmediates(&deserializeBuffer);
                    break;
                case WireCmd::ComputePassEncoderSetLabel:
                    result = HandleComputePassEncoderSetLabel(&deserializeBuffer);
                    break;
                case WireCmd::ComputePassEncoderSetPipeline:
                    result = HandleComputePassEncoderSetPipeline(&deserializeBuffer);
                    break;
                case WireCmd::ComputePassEncoderSetResourceTable:
                    result = HandleComputePassEncoderSetResourceTable(&deserializeBuffer);
                    break;
                case WireCmd::ComputePassEncoderWriteTimestamp:
                    result = HandleComputePassEncoderWriteTimestamp(&deserializeBuffer);
                    break;
                case WireCmd::ComputePipelineGetBindGroupLayout:
                    result = HandleComputePipelineGetBindGroupLayout(&deserializeBuffer);
                    break;
                case WireCmd::ComputePipelineSetLabel:
                    result = HandleComputePipelineSetLabel(&deserializeBuffer);
                    break;
                case WireCmd::DeviceCreateBindGroup:
                    result = HandleDeviceCreateBindGroup(&deserializeBuffer);
                    break;
                case WireCmd::DeviceCreateBindGroupLayout:
                    result = HandleDeviceCreateBindGroupLayout(&deserializeBuffer);
                    break;
                case WireCmd::DeviceCreateBuffer:
                    result = HandleDeviceCreateBuffer(&deserializeBuffer);
                    break;
                case WireCmd::DeviceCreateCommandEncoder:
                    result = HandleDeviceCreateCommandEncoder(&deserializeBuffer);
                    break;
                case WireCmd::DeviceCreateComputePipeline:
                    result = HandleDeviceCreateComputePipeline(&deserializeBuffer);
                    break;
                case WireCmd::DeviceCreateComputePipelineAsync:
                    result = HandleDeviceCreateComputePipelineAsync(&deserializeBuffer);
                    break;
                case WireCmd::DeviceCreateErrorBuffer:
                    result = HandleDeviceCreateErrorBuffer(&deserializeBuffer);
                    break;
                case WireCmd::DeviceCreateErrorComputePipeline:
                    result = HandleDeviceCreateErrorComputePipeline(&deserializeBuffer);
                    break;
                case WireCmd::DeviceCreateErrorExternalTexture:
                    result = HandleDeviceCreateErrorExternalTexture(&deserializeBuffer);
                    break;
                case WireCmd::DeviceCreateErrorRenderPipeline:
                    result = HandleDeviceCreateErrorRenderPipeline(&deserializeBuffer);
                    break;
                case WireCmd::DeviceCreateErrorShaderModule:
                    result = HandleDeviceCreateErrorShaderModule(&deserializeBuffer);
                    break;
                case WireCmd::DeviceCreateErrorTexture:
                    result = HandleDeviceCreateErrorTexture(&deserializeBuffer);
                    break;
                case WireCmd::DeviceCreateExternalTexture:
                    result = HandleDeviceCreateExternalTexture(&deserializeBuffer);
                    break;
                case WireCmd::DeviceCreatePipelineLayout:
                    result = HandleDeviceCreatePipelineLayout(&deserializeBuffer);
                    break;
                case WireCmd::DeviceCreateQuerySet:
                    result = HandleDeviceCreateQuerySet(&deserializeBuffer);
                    break;
                case WireCmd::DeviceCreateRenderBundleEncoder:
                    result = HandleDeviceCreateRenderBundleEncoder(&deserializeBuffer);
                    break;
                case WireCmd::DeviceCreateRenderPipeline:
                    result = HandleDeviceCreateRenderPipeline(&deserializeBuffer);
                    break;
                case WireCmd::DeviceCreateRenderPipelineAsync:
                    result = HandleDeviceCreateRenderPipelineAsync(&deserializeBuffer);
                    break;
                case WireCmd::DeviceCreateResourceTable:
                    result = HandleDeviceCreateResourceTable(&deserializeBuffer);
                    break;
                case WireCmd::DeviceCreateSampler:
                    result = HandleDeviceCreateSampler(&deserializeBuffer);
                    break;
                case WireCmd::DeviceCreateShaderModule:
                    result = HandleDeviceCreateShaderModule(&deserializeBuffer);
                    break;
                case WireCmd::DeviceCreateTexture:
                    result = HandleDeviceCreateTexture(&deserializeBuffer);
                    break;
                case WireCmd::DeviceDestroy:
                    result = HandleDeviceDestroy(&deserializeBuffer);
                    break;
                case WireCmd::DeviceForceLoss:
                    result = HandleDeviceForceLoss(&deserializeBuffer);
                    break;
                case WireCmd::DeviceGetQueue:
                    result = HandleDeviceGetQueue(&deserializeBuffer);
                    break;
                case WireCmd::DeviceInjectError:
                    result = HandleDeviceInjectError(&deserializeBuffer);
                    break;
                case WireCmd::DevicePopErrorScope:
                    result = HandleDevicePopErrorScope(&deserializeBuffer);
                    break;
                case WireCmd::DevicePushErrorScope:
                    result = HandleDevicePushErrorScope(&deserializeBuffer);
                    break;
                case WireCmd::DeviceSetLabel:
                    result = HandleDeviceSetLabel(&deserializeBuffer);
                    break;
                case WireCmd::DeviceTick:
                    result = HandleDeviceTick(&deserializeBuffer);
                    break;
                case WireCmd::DeviceValidateTextureDescriptor:
                    result = HandleDeviceValidateTextureDescriptor(&deserializeBuffer);
                    break;
                case WireCmd::ExternalTextureDestroy:
                    result = HandleExternalTextureDestroy(&deserializeBuffer);
                    break;
                case WireCmd::ExternalTextureExpire:
                    result = HandleExternalTextureExpire(&deserializeBuffer);
                    break;
                case WireCmd::ExternalTextureRefresh:
                    result = HandleExternalTextureRefresh(&deserializeBuffer);
                    break;
                case WireCmd::ExternalTextureSetLabel:
                    result = HandleExternalTextureSetLabel(&deserializeBuffer);
                    break;
                case WireCmd::InstanceProcessEvents:
                    result = HandleInstanceProcessEvents(&deserializeBuffer);
                    break;
                case WireCmd::InstanceRequestAdapter:
                    result = HandleInstanceRequestAdapter(&deserializeBuffer);
                    break;
                case WireCmd::PipelineLayoutSetLabel:
                    result = HandlePipelineLayoutSetLabel(&deserializeBuffer);
                    break;
                case WireCmd::QuerySetDestroy:
                    result = HandleQuerySetDestroy(&deserializeBuffer);
                    break;
                case WireCmd::QuerySetSetLabel:
                    result = HandleQuerySetSetLabel(&deserializeBuffer);
                    break;
                case WireCmd::QueueCopyExternalTextureForBrowser:
                    result = HandleQueueCopyExternalTextureForBrowser(&deserializeBuffer);
                    break;
                case WireCmd::QueueCopyTextureForBrowser:
                    result = HandleQueueCopyTextureForBrowser(&deserializeBuffer);
                    break;
                case WireCmd::QueueOnSubmittedWorkDone:
                    result = HandleQueueOnSubmittedWorkDone(&deserializeBuffer);
                    break;
                case WireCmd::QueueSetLabel:
                    result = HandleQueueSetLabel(&deserializeBuffer);
                    break;
                case WireCmd::QueueSubmit:
                    result = HandleQueueSubmit(&deserializeBuffer);
                    break;
                case WireCmd::QueueWriteBuffer:
                    result = HandleQueueWriteBuffer(&deserializeBuffer);
                    break;
                case WireCmd::QueueWriteBufferXl:
                    result = HandleQueueWriteBufferXl(&deserializeBuffer);
                    break;
                case WireCmd::QueueWriteTexture:
                    result = HandleQueueWriteTexture(&deserializeBuffer);
                    break;
                case WireCmd::QueueWriteTextureXl:
                    result = HandleQueueWriteTextureXl(&deserializeBuffer);
                    break;
                case WireCmd::RenderBundleEncoderDraw:
                    result = HandleRenderBundleEncoderDraw(&deserializeBuffer);
                    break;
                case WireCmd::RenderBundleEncoderDrawIndexed:
                    result = HandleRenderBundleEncoderDrawIndexed(&deserializeBuffer);
                    break;
                case WireCmd::RenderBundleEncoderDrawIndexedIndirect:
                    result = HandleRenderBundleEncoderDrawIndexedIndirect(&deserializeBuffer);
                    break;
                case WireCmd::RenderBundleEncoderDrawIndirect:
                    result = HandleRenderBundleEncoderDrawIndirect(&deserializeBuffer);
                    break;
                case WireCmd::RenderBundleEncoderFinish:
                    result = HandleRenderBundleEncoderFinish(&deserializeBuffer);
                    break;
                case WireCmd::RenderBundleEncoderInsertDebugMarker:
                    result = HandleRenderBundleEncoderInsertDebugMarker(&deserializeBuffer);
                    break;
                case WireCmd::RenderBundleEncoderPopDebugGroup:
                    result = HandleRenderBundleEncoderPopDebugGroup(&deserializeBuffer);
                    break;
                case WireCmd::RenderBundleEncoderPushDebugGroup:
                    result = HandleRenderBundleEncoderPushDebugGroup(&deserializeBuffer);
                    break;
                case WireCmd::RenderBundleEncoderSetBindGroup:
                    result = HandleRenderBundleEncoderSetBindGroup(&deserializeBuffer);
                    break;
                case WireCmd::RenderBundleEncoderSetImmediates:
                    result = HandleRenderBundleEncoderSetImmediates(&deserializeBuffer);
                    break;
                case WireCmd::RenderBundleEncoderSetIndexBuffer:
                    result = HandleRenderBundleEncoderSetIndexBuffer(&deserializeBuffer);
                    break;
                case WireCmd::RenderBundleEncoderSetLabel:
                    result = HandleRenderBundleEncoderSetLabel(&deserializeBuffer);
                    break;
                case WireCmd::RenderBundleEncoderSetPipeline:
                    result = HandleRenderBundleEncoderSetPipeline(&deserializeBuffer);
                    break;
                case WireCmd::RenderBundleEncoderSetResourceTable:
                    result = HandleRenderBundleEncoderSetResourceTable(&deserializeBuffer);
                    break;
                case WireCmd::RenderBundleEncoderSetVertexBuffer:
                    result = HandleRenderBundleEncoderSetVertexBuffer(&deserializeBuffer);
                    break;
                case WireCmd::RenderBundleSetLabel:
                    result = HandleRenderBundleSetLabel(&deserializeBuffer);
                    break;
                case WireCmd::RenderPassEncoderBeginOcclusionQuery:
                    result = HandleRenderPassEncoderBeginOcclusionQuery(&deserializeBuffer);
                    break;
                case WireCmd::RenderPassEncoderDraw:
                    result = HandleRenderPassEncoderDraw(&deserializeBuffer);
                    break;
                case WireCmd::RenderPassEncoderDrawIndexed:
                    result = HandleRenderPassEncoderDrawIndexed(&deserializeBuffer);
                    break;
                case WireCmd::RenderPassEncoderDrawIndexedIndirect:
                    result = HandleRenderPassEncoderDrawIndexedIndirect(&deserializeBuffer);
                    break;
                case WireCmd::RenderPassEncoderDrawIndirect:
                    result = HandleRenderPassEncoderDrawIndirect(&deserializeBuffer);
                    break;
                case WireCmd::RenderPassEncoderEnd:
                    result = HandleRenderPassEncoderEnd(&deserializeBuffer);
                    break;
                case WireCmd::RenderPassEncoderEndOcclusionQuery:
                    result = HandleRenderPassEncoderEndOcclusionQuery(&deserializeBuffer);
                    break;
                case WireCmd::RenderPassEncoderExecuteBundles:
                    result = HandleRenderPassEncoderExecuteBundles(&deserializeBuffer);
                    break;
                case WireCmd::RenderPassEncoderInsertDebugMarker:
                    result = HandleRenderPassEncoderInsertDebugMarker(&deserializeBuffer);
                    break;
                case WireCmd::RenderPassEncoderMultiDrawIndexedIndirect:
                    result = HandleRenderPassEncoderMultiDrawIndexedIndirect(&deserializeBuffer);
                    break;
                case WireCmd::RenderPassEncoderMultiDrawIndirect:
                    result = HandleRenderPassEncoderMultiDrawIndirect(&deserializeBuffer);
                    break;
                case WireCmd::RenderPassEncoderPixelLocalStorageBarrier:
                    result = HandleRenderPassEncoderPixelLocalStorageBarrier(&deserializeBuffer);
                    break;
                case WireCmd::RenderPassEncoderPopDebugGroup:
                    result = HandleRenderPassEncoderPopDebugGroup(&deserializeBuffer);
                    break;
                case WireCmd::RenderPassEncoderPushDebugGroup:
                    result = HandleRenderPassEncoderPushDebugGroup(&deserializeBuffer);
                    break;
                case WireCmd::RenderPassEncoderSetBindGroup:
                    result = HandleRenderPassEncoderSetBindGroup(&deserializeBuffer);
                    break;
                case WireCmd::RenderPassEncoderSetBlendConstant:
                    result = HandleRenderPassEncoderSetBlendConstant(&deserializeBuffer);
                    break;
                case WireCmd::RenderPassEncoderSetImmediates:
                    result = HandleRenderPassEncoderSetImmediates(&deserializeBuffer);
                    break;
                case WireCmd::RenderPassEncoderSetIndexBuffer:
                    result = HandleRenderPassEncoderSetIndexBuffer(&deserializeBuffer);
                    break;
                case WireCmd::RenderPassEncoderSetLabel:
                    result = HandleRenderPassEncoderSetLabel(&deserializeBuffer);
                    break;
                case WireCmd::RenderPassEncoderSetPipeline:
                    result = HandleRenderPassEncoderSetPipeline(&deserializeBuffer);
                    break;
                case WireCmd::RenderPassEncoderSetResourceTable:
                    result = HandleRenderPassEncoderSetResourceTable(&deserializeBuffer);
                    break;
                case WireCmd::RenderPassEncoderSetScissorRect:
                    result = HandleRenderPassEncoderSetScissorRect(&deserializeBuffer);
                    break;
                case WireCmd::RenderPassEncoderSetStencilReference:
                    result = HandleRenderPassEncoderSetStencilReference(&deserializeBuffer);
                    break;
                case WireCmd::RenderPassEncoderSetVertexBuffer:
                    result = HandleRenderPassEncoderSetVertexBuffer(&deserializeBuffer);
                    break;
                case WireCmd::RenderPassEncoderSetViewport:
                    result = HandleRenderPassEncoderSetViewport(&deserializeBuffer);
                    break;
                case WireCmd::RenderPassEncoderWriteTimestamp:
                    result = HandleRenderPassEncoderWriteTimestamp(&deserializeBuffer);
                    break;
                case WireCmd::RenderPipelineGetBindGroupLayout:
                    result = HandleRenderPipelineGetBindGroupLayout(&deserializeBuffer);
                    break;
                case WireCmd::RenderPipelineSetLabel:
                    result = HandleRenderPipelineSetLabel(&deserializeBuffer);
                    break;
                case WireCmd::ResourceTableDestroy:
                    result = HandleResourceTableDestroy(&deserializeBuffer);
                    break;
                case WireCmd::ResourceTableRemove:
                    result = HandleResourceTableRemove(&deserializeBuffer);
                    break;
                case WireCmd::ResourceTableSetLabel:
                    result = HandleResourceTableSetLabel(&deserializeBuffer);
                    break;
                case WireCmd::ResourceTableUpdate:
                    result = HandleResourceTableUpdate(&deserializeBuffer);
                    break;
                case WireCmd::SamplerSetLabel:
                    result = HandleSamplerSetLabel(&deserializeBuffer);
                    break;
                case WireCmd::ShaderModuleGetCompilationInfo:
                    result = HandleShaderModuleGetCompilationInfo(&deserializeBuffer);
                    break;
                case WireCmd::ShaderModuleSetLabel:
                    result = HandleShaderModuleSetLabel(&deserializeBuffer);
                    break;
                case WireCmd::SurfaceConfigure:
                    result = HandleSurfaceConfigure(&deserializeBuffer);
                    break;
                case WireCmd::SurfaceGetCurrentTexture:
                    result = HandleSurfaceGetCurrentTexture(&deserializeBuffer);
                    break;
                case WireCmd::SurfacePresent:
                    result = HandleSurfacePresent(&deserializeBuffer);
                    break;
                case WireCmd::SurfaceSetLabel:
                    result = HandleSurfaceSetLabel(&deserializeBuffer);
                    break;
                case WireCmd::SurfaceUnconfigure:
                    result = HandleSurfaceUnconfigure(&deserializeBuffer);
                    break;
                case WireCmd::TexelBufferViewSetLabel:
                    result = HandleTexelBufferViewSetLabel(&deserializeBuffer);
                    break;
                case WireCmd::TextureCreateErrorView:
                    result = HandleTextureCreateErrorView(&deserializeBuffer);
                    break;
                case WireCmd::TextureCreateView:
                    result = HandleTextureCreateView(&deserializeBuffer);
                    break;
                case WireCmd::TextureDestroy:
                    result = HandleTextureDestroy(&deserializeBuffer);
                    break;
                case WireCmd::TextureSetLabel:
                    result = HandleTextureSetLabel(&deserializeBuffer);
                    break;
                case WireCmd::TextureViewSetLabel:
                    result = HandleTextureViewSetLabel(&deserializeBuffer);
                    break;
                case WireCmd::UnregisterObject:
                    result = HandleUnregisterObject(&deserializeBuffer);
                    break;
                default:
                    result = WireResult::FatalError;
            }

            if (result != WireResult::Success) {
                return false;
            }
            mAllocator.Reset();
        }

        // After the server handles all the commands from the stream, we additionally run
        // ProcessEvents on all known Instances so that any work done on the server side can be
        // forwarded through to the client.
        for (auto instance : GetAllInstanceHandles()) {
            if (DoInstanceProcessEvents(instance) != WireResult::Success) {
                return false;
            }
        }

        if (!deserializeBuffer.Empty()) {
            return false;
        }

        return true;
    }

}  // namespace dawn::wire::server
