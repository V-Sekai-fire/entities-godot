
// Command handlers & doers
WireResult HandleAdapterRequestDevice(DeserializeBuffer* deserializeBuffer);
WireResult DoAdapterRequestDevice(Known<WGPUAdapter> adapterId, Known<WGPUInstance> instanceId, WGPUFuture future, ObjectHandle deviceObjectHandle, WGPUFuture deviceLostFuture, WGPUDeviceDescriptor const * descriptor);

WireResult HandleBindGroupLayoutSetLabel(DeserializeBuffer* deserializeBuffer);
WireResult DoBindGroupLayoutSetLabel(WGPUBindGroupLayout self, WGPUStringView label);

WireResult HandleBindGroupSetLabel(DeserializeBuffer* deserializeBuffer);
WireResult DoBindGroupSetLabel(WGPUBindGroup self, WGPUStringView label);

WireResult HandleBufferCreateTexelView(DeserializeBuffer* deserializeBuffer);
WireResult DoBufferCreateTexelView(WGPUBuffer self, WGPUTexelBufferViewDescriptor const * descriptor, WGPUTexelBufferView* result);

WireResult HandleBufferDestroy(DeserializeBuffer* deserializeBuffer);
WireResult DoBufferDestroy(WGPUBuffer self);

WireResult HandleBufferMapAsync(DeserializeBuffer* deserializeBuffer);
WireResult DoBufferMapAsync(Known<WGPUBuffer> bufferId, Known<WGPUInstance> instanceId, WGPUFuture future, WGPUMapMode mode, size_t offset, size_t size);

WireResult HandleBufferSetLabel(DeserializeBuffer* deserializeBuffer);
WireResult DoBufferSetLabel(WGPUBuffer self, WGPUStringView label);

WireResult HandleBufferUnmap(DeserializeBuffer* deserializeBuffer);
WireResult DoBufferUnmap(WGPUBuffer self);

WireResult HandleBufferUpdateMappedData(DeserializeBuffer* deserializeBuffer);
WireResult DoBufferUpdateMappedData(Known<WGPUBuffer> bufferId, size_t dataUpdateInfoLength, std::byte const * dataUpdateInfo, size_t offset, size_t size);

WireResult HandleCommandBufferSetLabel(DeserializeBuffer* deserializeBuffer);
WireResult DoCommandBufferSetLabel(WGPUCommandBuffer self, WGPUStringView label);

WireResult HandleCommandEncoderBeginComputePass(DeserializeBuffer* deserializeBuffer);
WireResult DoCommandEncoderBeginComputePass(WGPUCommandEncoder self, WGPUComputePassDescriptor const * descriptor, WGPUComputePassEncoder* result);

WireResult HandleCommandEncoderBeginRenderPass(DeserializeBuffer* deserializeBuffer);
WireResult DoCommandEncoderBeginRenderPass(WGPUCommandEncoder self, WGPURenderPassDescriptor const * descriptor, WGPURenderPassEncoder* result);

WireResult HandleCommandEncoderClearBuffer(DeserializeBuffer* deserializeBuffer);
WireResult DoCommandEncoderClearBuffer(WGPUCommandEncoder self, WGPUBuffer buffer, uint64_t offset, uint64_t size);

WireResult HandleCommandEncoderCopyBufferToBuffer(DeserializeBuffer* deserializeBuffer);
WireResult DoCommandEncoderCopyBufferToBuffer(WGPUCommandEncoder self, WGPUBuffer source, uint64_t sourceOffset, WGPUBuffer destination, uint64_t destinationOffset, uint64_t size);

WireResult HandleCommandEncoderCopyBufferToTexture(DeserializeBuffer* deserializeBuffer);
WireResult DoCommandEncoderCopyBufferToTexture(WGPUCommandEncoder self, WGPUTexelCopyBufferInfo const * source, WGPUTexelCopyTextureInfo const * destination, WGPUExtent3D const * copySize);

WireResult HandleCommandEncoderCopyTextureToBuffer(DeserializeBuffer* deserializeBuffer);
WireResult DoCommandEncoderCopyTextureToBuffer(WGPUCommandEncoder self, WGPUTexelCopyTextureInfo const * source, WGPUTexelCopyBufferInfo const * destination, WGPUExtent3D const * copySize);

WireResult HandleCommandEncoderCopyTextureToTexture(DeserializeBuffer* deserializeBuffer);
WireResult DoCommandEncoderCopyTextureToTexture(WGPUCommandEncoder self, WGPUTexelCopyTextureInfo const * source, WGPUTexelCopyTextureInfo const * destination, WGPUExtent3D const * copySize);

WireResult HandleCommandEncoderFinish(DeserializeBuffer* deserializeBuffer);
WireResult DoCommandEncoderFinish(WGPUCommandEncoder self, WGPUCommandBufferDescriptor const * descriptor, WGPUCommandBuffer* result);

WireResult HandleCommandEncoderInjectValidationError(DeserializeBuffer* deserializeBuffer);
WireResult DoCommandEncoderInjectValidationError(WGPUCommandEncoder self, WGPUStringView message);

WireResult HandleCommandEncoderInsertDebugMarker(DeserializeBuffer* deserializeBuffer);
WireResult DoCommandEncoderInsertDebugMarker(WGPUCommandEncoder self, WGPUStringView markerLabel);

WireResult HandleCommandEncoderPopDebugGroup(DeserializeBuffer* deserializeBuffer);
WireResult DoCommandEncoderPopDebugGroup(WGPUCommandEncoder self);

WireResult HandleCommandEncoderPushDebugGroup(DeserializeBuffer* deserializeBuffer);
WireResult DoCommandEncoderPushDebugGroup(WGPUCommandEncoder self, WGPUStringView groupLabel);

WireResult HandleCommandEncoderResolveQuerySet(DeserializeBuffer* deserializeBuffer);
WireResult DoCommandEncoderResolveQuerySet(WGPUCommandEncoder self, WGPUQuerySet querySet, uint32_t firstQuery, uint32_t queryCount, WGPUBuffer destination, uint64_t destinationOffset);

WireResult HandleCommandEncoderSetLabel(DeserializeBuffer* deserializeBuffer);
WireResult DoCommandEncoderSetLabel(WGPUCommandEncoder self, WGPUStringView label);

WireResult HandleCommandEncoderWriteBuffer(DeserializeBuffer* deserializeBuffer);
WireResult DoCommandEncoderWriteBuffer(WGPUCommandEncoder self, WGPUBuffer buffer, uint64_t bufferOffset, uint8_t const * data, uint64_t size);

WireResult HandleCommandEncoderWriteTimestamp(DeserializeBuffer* deserializeBuffer);
WireResult DoCommandEncoderWriteTimestamp(WGPUCommandEncoder self, WGPUQuerySet querySet, uint32_t queryIndex);

WireResult HandleComputePassEncoderDispatchWorkgroups(DeserializeBuffer* deserializeBuffer);
WireResult DoComputePassEncoderDispatchWorkgroups(WGPUComputePassEncoder self, uint32_t workgroupCountX, uint32_t workgroupCountY, uint32_t workgroupCountZ);

WireResult HandleComputePassEncoderDispatchWorkgroupsIndirect(DeserializeBuffer* deserializeBuffer);
WireResult DoComputePassEncoderDispatchWorkgroupsIndirect(WGPUComputePassEncoder self, WGPUBuffer indirectBuffer, uint64_t indirectOffset);

WireResult HandleComputePassEncoderEnd(DeserializeBuffer* deserializeBuffer);
WireResult DoComputePassEncoderEnd(WGPUComputePassEncoder self);

WireResult HandleComputePassEncoderInsertDebugMarker(DeserializeBuffer* deserializeBuffer);
WireResult DoComputePassEncoderInsertDebugMarker(WGPUComputePassEncoder self, WGPUStringView markerLabel);

WireResult HandleComputePassEncoderPopDebugGroup(DeserializeBuffer* deserializeBuffer);
WireResult DoComputePassEncoderPopDebugGroup(WGPUComputePassEncoder self);

WireResult HandleComputePassEncoderPushDebugGroup(DeserializeBuffer* deserializeBuffer);
WireResult DoComputePassEncoderPushDebugGroup(WGPUComputePassEncoder self, WGPUStringView groupLabel);

WireResult HandleComputePassEncoderSetBindGroup(DeserializeBuffer* deserializeBuffer);
WireResult DoComputePassEncoderSetBindGroup(WGPUComputePassEncoder self, uint32_t groupIndex, WGPUBindGroup group, size_t dynamicOffsetCount, uint32_t const * dynamicOffsets);

WireResult HandleComputePassEncoderSetImmediates(DeserializeBuffer* deserializeBuffer);
WireResult DoComputePassEncoderSetImmediates(Known<WGPUComputePassEncoder> computePassEncoderId, uint32_t offset, uint8_t const * data, size_t size);

WireResult HandleComputePassEncoderSetLabel(DeserializeBuffer* deserializeBuffer);
WireResult DoComputePassEncoderSetLabel(WGPUComputePassEncoder self, WGPUStringView label);

WireResult HandleComputePassEncoderSetPipeline(DeserializeBuffer* deserializeBuffer);
WireResult DoComputePassEncoderSetPipeline(WGPUComputePassEncoder self, WGPUComputePipeline pipeline);

WireResult HandleComputePassEncoderSetResourceTable(DeserializeBuffer* deserializeBuffer);
WireResult DoComputePassEncoderSetResourceTable(WGPUComputePassEncoder self, WGPUResourceTable table);

WireResult HandleComputePassEncoderWriteTimestamp(DeserializeBuffer* deserializeBuffer);
WireResult DoComputePassEncoderWriteTimestamp(WGPUComputePassEncoder self, WGPUQuerySet querySet, uint32_t queryIndex);

WireResult HandleComputePipelineGetBindGroupLayout(DeserializeBuffer* deserializeBuffer);
WireResult DoComputePipelineGetBindGroupLayout(WGPUComputePipeline self, uint32_t groupIndex, WGPUBindGroupLayout* result);

WireResult HandleComputePipelineSetLabel(DeserializeBuffer* deserializeBuffer);
WireResult DoComputePipelineSetLabel(WGPUComputePipeline self, WGPUStringView label);

WireResult HandleDeviceCreateBindGroup(DeserializeBuffer* deserializeBuffer);
WireResult DoDeviceCreateBindGroup(WGPUDevice self, WGPUBindGroupDescriptor const * descriptor, WGPUBindGroup* result);

WireResult HandleDeviceCreateBindGroupLayout(DeserializeBuffer* deserializeBuffer);
WireResult DoDeviceCreateBindGroupLayout(WGPUDevice self, WGPUBindGroupLayoutDescriptor const * descriptor, WGPUBindGroupLayout* result);

WireResult HandleDeviceCreateBuffer(DeserializeBuffer* deserializeBuffer);
WireResult DoDeviceCreateBuffer(Known<WGPUDevice> deviceId, WGPUBufferDescriptor const * descriptor, ObjectHandle result, size_t memoryHandleCreateInfoLength, std::byte const * memoryHandleCreateInfo);

WireResult HandleDeviceCreateCommandEncoder(DeserializeBuffer* deserializeBuffer);
WireResult DoDeviceCreateCommandEncoder(WGPUDevice self, WGPUCommandEncoderDescriptor const * descriptor, WGPUCommandEncoder* result);

WireResult HandleDeviceCreateComputePipeline(DeserializeBuffer* deserializeBuffer);
WireResult DoDeviceCreateComputePipeline(WGPUDevice self, WGPUComputePipelineDescriptor const * descriptor, WGPUComputePipeline* result);

WireResult HandleDeviceCreateComputePipelineAsync(DeserializeBuffer* deserializeBuffer);
WireResult DoDeviceCreateComputePipelineAsync(Known<WGPUDevice> deviceId, Known<WGPUInstance> instanceId, WGPUFuture future, ObjectHandle pipelineObjectHandle, WGPUComputePipelineDescriptor const * descriptor);

WireResult HandleDeviceCreateErrorBuffer(DeserializeBuffer* deserializeBuffer);
WireResult DoDeviceCreateErrorBuffer(WGPUDevice self, WGPUBufferDescriptor const * descriptor, WGPUBuffer* result);

WireResult HandleDeviceCreateErrorComputePipeline(DeserializeBuffer* deserializeBuffer);
WireResult DoDeviceCreateErrorComputePipeline(WGPUDevice self, WGPUStringView label, WGPUComputePipeline* result);

WireResult HandleDeviceCreateErrorExternalTexture(DeserializeBuffer* deserializeBuffer);
WireResult DoDeviceCreateErrorExternalTexture(WGPUDevice self, WGPUExternalTexture* result);

WireResult HandleDeviceCreateErrorRenderPipeline(DeserializeBuffer* deserializeBuffer);
WireResult DoDeviceCreateErrorRenderPipeline(WGPUDevice self, WGPUStringView label, WGPURenderPipeline* result);

WireResult HandleDeviceCreateErrorShaderModule(DeserializeBuffer* deserializeBuffer);
WireResult DoDeviceCreateErrorShaderModule(WGPUDevice self, WGPUShaderModuleDescriptor const * descriptor, WGPUStringView errorMessage, WGPUShaderModule* result);

WireResult HandleDeviceCreateErrorTexture(DeserializeBuffer* deserializeBuffer);
WireResult DoDeviceCreateErrorTexture(WGPUDevice self, WGPUTextureDescriptor const * descriptor, WGPUTexture* result);

WireResult HandleDeviceCreateExternalTexture(DeserializeBuffer* deserializeBuffer);
WireResult DoDeviceCreateExternalTexture(WGPUDevice self, WGPUExternalTextureDescriptor const * externalTextureDescriptor, WGPUExternalTexture* result);

WireResult HandleDeviceCreatePipelineLayout(DeserializeBuffer* deserializeBuffer);
WireResult DoDeviceCreatePipelineLayout(WGPUDevice self, WGPUPipelineLayoutDescriptor const * descriptor, WGPUPipelineLayout* result);

WireResult HandleDeviceCreateQuerySet(DeserializeBuffer* deserializeBuffer);
WireResult DoDeviceCreateQuerySet(WGPUDevice self, WGPUQuerySetDescriptor const * descriptor, WGPUQuerySet* result);

WireResult HandleDeviceCreateRenderBundleEncoder(DeserializeBuffer* deserializeBuffer);
WireResult DoDeviceCreateRenderBundleEncoder(WGPUDevice self, WGPURenderBundleEncoderDescriptor const * descriptor, WGPURenderBundleEncoder* result);

WireResult HandleDeviceCreateRenderPipeline(DeserializeBuffer* deserializeBuffer);
WireResult DoDeviceCreateRenderPipeline(WGPUDevice self, WGPURenderPipelineDescriptor const * descriptor, WGPURenderPipeline* result);

WireResult HandleDeviceCreateRenderPipelineAsync(DeserializeBuffer* deserializeBuffer);
WireResult DoDeviceCreateRenderPipelineAsync(Known<WGPUDevice> deviceId, Known<WGPUInstance> instanceId, WGPUFuture future, ObjectHandle pipelineObjectHandle, WGPURenderPipelineDescriptor const * descriptor);

WireResult HandleDeviceCreateResourceTable(DeserializeBuffer* deserializeBuffer);
WireResult DoDeviceCreateResourceTable(WGPUDevice self, WGPUResourceTableDescriptor const * descriptor, WGPUResourceTable* result);

WireResult HandleDeviceCreateSampler(DeserializeBuffer* deserializeBuffer);
WireResult DoDeviceCreateSampler(WGPUDevice self, WGPUSamplerDescriptor const * descriptor, WGPUSampler* result);

WireResult HandleDeviceCreateShaderModule(DeserializeBuffer* deserializeBuffer);
WireResult DoDeviceCreateShaderModule(WGPUDevice self, WGPUShaderModuleDescriptor const * descriptor, WGPUShaderModule* result);

WireResult HandleDeviceCreateTexture(DeserializeBuffer* deserializeBuffer);
WireResult DoDeviceCreateTexture(WGPUDevice self, WGPUTextureDescriptor const * descriptor, WGPUTexture* result);

WireResult HandleDeviceDestroy(DeserializeBuffer* deserializeBuffer);
WireResult DoDeviceDestroy(WGPUDevice self);

WireResult HandleDeviceForceLoss(DeserializeBuffer* deserializeBuffer);
WireResult DoDeviceForceLoss(WGPUDevice self, WGPUDeviceLostReason type, WGPUStringView message);

WireResult HandleDeviceGetQueue(DeserializeBuffer* deserializeBuffer);
WireResult DoDeviceGetQueue(WGPUDevice self, WGPUQueue* result);

WireResult HandleDeviceInjectError(DeserializeBuffer* deserializeBuffer);
WireResult DoDeviceInjectError(WGPUDevice self, WGPUErrorType type, WGPUStringView message);

WireResult HandleDevicePopErrorScope(DeserializeBuffer* deserializeBuffer);
WireResult DoDevicePopErrorScope(Known<WGPUDevice> deviceId, Known<WGPUInstance> instanceId, WGPUFuture future);

WireResult HandleDevicePushErrorScope(DeserializeBuffer* deserializeBuffer);
WireResult DoDevicePushErrorScope(WGPUDevice self, WGPUErrorFilter filter);

WireResult HandleDeviceSetLabel(DeserializeBuffer* deserializeBuffer);
WireResult DoDeviceSetLabel(WGPUDevice self, WGPUStringView label);

WireResult HandleDeviceTick(DeserializeBuffer* deserializeBuffer);
WireResult DoDeviceTick(WGPUDevice self);

WireResult HandleDeviceValidateTextureDescriptor(DeserializeBuffer* deserializeBuffer);
WireResult DoDeviceValidateTextureDescriptor(WGPUDevice self, WGPUTextureDescriptor const * descriptor);

WireResult HandleExternalTextureDestroy(DeserializeBuffer* deserializeBuffer);
WireResult DoExternalTextureDestroy(WGPUExternalTexture self);

WireResult HandleExternalTextureExpire(DeserializeBuffer* deserializeBuffer);
WireResult DoExternalTextureExpire(WGPUExternalTexture self);

WireResult HandleExternalTextureRefresh(DeserializeBuffer* deserializeBuffer);
WireResult DoExternalTextureRefresh(WGPUExternalTexture self);

WireResult HandleExternalTextureSetLabel(DeserializeBuffer* deserializeBuffer);
WireResult DoExternalTextureSetLabel(WGPUExternalTexture self, WGPUStringView label);

WireResult HandleInstanceProcessEvents(DeserializeBuffer* deserializeBuffer);
WireResult DoInstanceProcessEvents(WGPUInstance self);

WireResult HandleInstanceRequestAdapter(DeserializeBuffer* deserializeBuffer);
WireResult DoInstanceRequestAdapter(Known<WGPUInstance> instanceId, WGPUFuture future, ObjectHandle adapterObjectHandle, WGPURequestAdapterOptions const * options);

WireResult HandlePipelineLayoutSetLabel(DeserializeBuffer* deserializeBuffer);
WireResult DoPipelineLayoutSetLabel(WGPUPipelineLayout self, WGPUStringView label);

WireResult HandleQuerySetDestroy(DeserializeBuffer* deserializeBuffer);
WireResult DoQuerySetDestroy(WGPUQuerySet self);

WireResult HandleQuerySetSetLabel(DeserializeBuffer* deserializeBuffer);
WireResult DoQuerySetSetLabel(WGPUQuerySet self, WGPUStringView label);

WireResult HandleQueueCopyExternalTextureForBrowser(DeserializeBuffer* deserializeBuffer);
WireResult DoQueueCopyExternalTextureForBrowser(WGPUQueue self, WGPUImageCopyExternalTexture const * source, WGPUTexelCopyTextureInfo const * destination, WGPUExtent3D const * copySize, WGPUCopyTextureForBrowserOptions const * options);

WireResult HandleQueueCopyTextureForBrowser(DeserializeBuffer* deserializeBuffer);
WireResult DoQueueCopyTextureForBrowser(WGPUQueue self, WGPUTexelCopyTextureInfo const * source, WGPUTexelCopyTextureInfo const * destination, WGPUExtent3D const * copySize, WGPUCopyTextureForBrowserOptions const * options);

WireResult HandleQueueOnSubmittedWorkDone(DeserializeBuffer* deserializeBuffer);
WireResult DoQueueOnSubmittedWorkDone(Known<WGPUQueue> queueId, Known<WGPUInstance> instanceId, WGPUFuture future);

WireResult HandleQueueSetLabel(DeserializeBuffer* deserializeBuffer);
WireResult DoQueueSetLabel(WGPUQueue self, WGPUStringView label);

WireResult HandleQueueSubmit(DeserializeBuffer* deserializeBuffer);
WireResult DoQueueSubmit(WGPUQueue self, size_t commandCount, WGPUCommandBuffer const * commands);

WireResult HandleQueueWriteBuffer(DeserializeBuffer* deserializeBuffer);
WireResult DoQueueWriteBuffer(Known<WGPUQueue> queueId, Known<WGPUBuffer> bufferId, uint64_t bufferOffset, uint8_t const * data, size_t size);

WireResult HandleQueueWriteBufferXl(DeserializeBuffer* deserializeBuffer);
WireResult DoQueueWriteBufferXl(Known<WGPUQueue> queueId, Known<WGPUBuffer> bufferId, uint64_t bufferOffset, size_t size, size_t memoryHandleCreateInfoLength, std::byte const * memoryHandleCreateInfo, size_t memoryDataUpdateInfoLength, std::byte const * memoryDataUpdateInfo);

WireResult HandleQueueWriteTexture(DeserializeBuffer* deserializeBuffer);
WireResult DoQueueWriteTexture(Known<WGPUQueue> queueId, WGPUTexelCopyTextureInfo const * destination, uint8_t const * data, size_t dataSize, WGPUTexelCopyBufferLayout const * dataLayout, WGPUExtent3D const * writeSize);

WireResult HandleQueueWriteTextureXl(DeserializeBuffer* deserializeBuffer);
WireResult DoQueueWriteTextureXl(Known<WGPUQueue> queueId, WGPUTexelCopyTextureInfo const * destination, size_t dataSize, WGPUTexelCopyBufferLayout const * dataLayout, WGPUExtent3D const * writeSize, size_t memoryHandleCreateInfoLength, std::byte const * memoryHandleCreateInfo, size_t memoryDataUpdateInfoLength, std::byte const * memoryDataUpdateInfo);

WireResult HandleRenderBundleEncoderDraw(DeserializeBuffer* deserializeBuffer);
WireResult DoRenderBundleEncoderDraw(WGPURenderBundleEncoder self, uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance);

WireResult HandleRenderBundleEncoderDrawIndexed(DeserializeBuffer* deserializeBuffer);
WireResult DoRenderBundleEncoderDrawIndexed(WGPURenderBundleEncoder self, uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, int32_t baseVertex, uint32_t firstInstance);

WireResult HandleRenderBundleEncoderDrawIndexedIndirect(DeserializeBuffer* deserializeBuffer);
WireResult DoRenderBundleEncoderDrawIndexedIndirect(WGPURenderBundleEncoder self, WGPUBuffer indirectBuffer, uint64_t indirectOffset);

WireResult HandleRenderBundleEncoderDrawIndirect(DeserializeBuffer* deserializeBuffer);
WireResult DoRenderBundleEncoderDrawIndirect(WGPURenderBundleEncoder self, WGPUBuffer indirectBuffer, uint64_t indirectOffset);

WireResult HandleRenderBundleEncoderFinish(DeserializeBuffer* deserializeBuffer);
WireResult DoRenderBundleEncoderFinish(WGPURenderBundleEncoder self, WGPURenderBundleDescriptor const * descriptor, WGPURenderBundle* result);

WireResult HandleRenderBundleEncoderInsertDebugMarker(DeserializeBuffer* deserializeBuffer);
WireResult DoRenderBundleEncoderInsertDebugMarker(WGPURenderBundleEncoder self, WGPUStringView markerLabel);

WireResult HandleRenderBundleEncoderPopDebugGroup(DeserializeBuffer* deserializeBuffer);
WireResult DoRenderBundleEncoderPopDebugGroup(WGPURenderBundleEncoder self);

WireResult HandleRenderBundleEncoderPushDebugGroup(DeserializeBuffer* deserializeBuffer);
WireResult DoRenderBundleEncoderPushDebugGroup(WGPURenderBundleEncoder self, WGPUStringView groupLabel);

WireResult HandleRenderBundleEncoderSetBindGroup(DeserializeBuffer* deserializeBuffer);
WireResult DoRenderBundleEncoderSetBindGroup(WGPURenderBundleEncoder self, uint32_t groupIndex, WGPUBindGroup group, size_t dynamicOffsetCount, uint32_t const * dynamicOffsets);

WireResult HandleRenderBundleEncoderSetImmediates(DeserializeBuffer* deserializeBuffer);
WireResult DoRenderBundleEncoderSetImmediates(Known<WGPURenderBundleEncoder> renderBundleEncoderId, uint32_t offset, uint8_t const * data, size_t size);

WireResult HandleRenderBundleEncoderSetIndexBuffer(DeserializeBuffer* deserializeBuffer);
WireResult DoRenderBundleEncoderSetIndexBuffer(WGPURenderBundleEncoder self, WGPUBuffer buffer, WGPUIndexFormat format, uint64_t offset, uint64_t size);

WireResult HandleRenderBundleEncoderSetLabel(DeserializeBuffer* deserializeBuffer);
WireResult DoRenderBundleEncoderSetLabel(WGPURenderBundleEncoder self, WGPUStringView label);

WireResult HandleRenderBundleEncoderSetPipeline(DeserializeBuffer* deserializeBuffer);
WireResult DoRenderBundleEncoderSetPipeline(WGPURenderBundleEncoder self, WGPURenderPipeline pipeline);

WireResult HandleRenderBundleEncoderSetResourceTable(DeserializeBuffer* deserializeBuffer);
WireResult DoRenderBundleEncoderSetResourceTable(WGPURenderBundleEncoder self, WGPUResourceTable table);

WireResult HandleRenderBundleEncoderSetVertexBuffer(DeserializeBuffer* deserializeBuffer);
WireResult DoRenderBundleEncoderSetVertexBuffer(WGPURenderBundleEncoder self, uint32_t slot, WGPUBuffer buffer, uint64_t offset, uint64_t size);

WireResult HandleRenderBundleSetLabel(DeserializeBuffer* deserializeBuffer);
WireResult DoRenderBundleSetLabel(WGPURenderBundle self, WGPUStringView label);

WireResult HandleRenderPassEncoderBeginOcclusionQuery(DeserializeBuffer* deserializeBuffer);
WireResult DoRenderPassEncoderBeginOcclusionQuery(WGPURenderPassEncoder self, uint32_t queryIndex);

WireResult HandleRenderPassEncoderDraw(DeserializeBuffer* deserializeBuffer);
WireResult DoRenderPassEncoderDraw(WGPURenderPassEncoder self, uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance);

WireResult HandleRenderPassEncoderDrawIndexed(DeserializeBuffer* deserializeBuffer);
WireResult DoRenderPassEncoderDrawIndexed(WGPURenderPassEncoder self, uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, int32_t baseVertex, uint32_t firstInstance);

WireResult HandleRenderPassEncoderDrawIndexedIndirect(DeserializeBuffer* deserializeBuffer);
WireResult DoRenderPassEncoderDrawIndexedIndirect(WGPURenderPassEncoder self, WGPUBuffer indirectBuffer, uint64_t indirectOffset);

WireResult HandleRenderPassEncoderDrawIndirect(DeserializeBuffer* deserializeBuffer);
WireResult DoRenderPassEncoderDrawIndirect(WGPURenderPassEncoder self, WGPUBuffer indirectBuffer, uint64_t indirectOffset);

WireResult HandleRenderPassEncoderEnd(DeserializeBuffer* deserializeBuffer);
WireResult DoRenderPassEncoderEnd(WGPURenderPassEncoder self);

WireResult HandleRenderPassEncoderEndOcclusionQuery(DeserializeBuffer* deserializeBuffer);
WireResult DoRenderPassEncoderEndOcclusionQuery(WGPURenderPassEncoder self);

WireResult HandleRenderPassEncoderExecuteBundles(DeserializeBuffer* deserializeBuffer);
WireResult DoRenderPassEncoderExecuteBundles(WGPURenderPassEncoder self, size_t bundleCount, WGPURenderBundle const * bundles);

WireResult HandleRenderPassEncoderInsertDebugMarker(DeserializeBuffer* deserializeBuffer);
WireResult DoRenderPassEncoderInsertDebugMarker(WGPURenderPassEncoder self, WGPUStringView markerLabel);

WireResult HandleRenderPassEncoderMultiDrawIndexedIndirect(DeserializeBuffer* deserializeBuffer);
WireResult DoRenderPassEncoderMultiDrawIndexedIndirect(WGPURenderPassEncoder self, WGPUBuffer indirectBuffer, uint64_t indirectOffset, uint32_t maxDrawCount, WGPUBuffer drawCountBuffer, uint64_t drawCountBufferOffset);

WireResult HandleRenderPassEncoderMultiDrawIndirect(DeserializeBuffer* deserializeBuffer);
WireResult DoRenderPassEncoderMultiDrawIndirect(WGPURenderPassEncoder self, WGPUBuffer indirectBuffer, uint64_t indirectOffset, uint32_t maxDrawCount, WGPUBuffer drawCountBuffer, uint64_t drawCountBufferOffset);

WireResult HandleRenderPassEncoderPixelLocalStorageBarrier(DeserializeBuffer* deserializeBuffer);
WireResult DoRenderPassEncoderPixelLocalStorageBarrier(WGPURenderPassEncoder self);

WireResult HandleRenderPassEncoderPopDebugGroup(DeserializeBuffer* deserializeBuffer);
WireResult DoRenderPassEncoderPopDebugGroup(WGPURenderPassEncoder self);

WireResult HandleRenderPassEncoderPushDebugGroup(DeserializeBuffer* deserializeBuffer);
WireResult DoRenderPassEncoderPushDebugGroup(WGPURenderPassEncoder self, WGPUStringView groupLabel);

WireResult HandleRenderPassEncoderSetBindGroup(DeserializeBuffer* deserializeBuffer);
WireResult DoRenderPassEncoderSetBindGroup(WGPURenderPassEncoder self, uint32_t groupIndex, WGPUBindGroup group, size_t dynamicOffsetCount, uint32_t const * dynamicOffsets);

WireResult HandleRenderPassEncoderSetBlendConstant(DeserializeBuffer* deserializeBuffer);
WireResult DoRenderPassEncoderSetBlendConstant(WGPURenderPassEncoder self, WGPUColor const * color);

WireResult HandleRenderPassEncoderSetImmediates(DeserializeBuffer* deserializeBuffer);
WireResult DoRenderPassEncoderSetImmediates(Known<WGPURenderPassEncoder> renderPassEncoderId, uint32_t offset, uint8_t const * data, size_t size);

WireResult HandleRenderPassEncoderSetIndexBuffer(DeserializeBuffer* deserializeBuffer);
WireResult DoRenderPassEncoderSetIndexBuffer(WGPURenderPassEncoder self, WGPUBuffer buffer, WGPUIndexFormat format, uint64_t offset, uint64_t size);

WireResult HandleRenderPassEncoderSetLabel(DeserializeBuffer* deserializeBuffer);
WireResult DoRenderPassEncoderSetLabel(WGPURenderPassEncoder self, WGPUStringView label);

WireResult HandleRenderPassEncoderSetPipeline(DeserializeBuffer* deserializeBuffer);
WireResult DoRenderPassEncoderSetPipeline(WGPURenderPassEncoder self, WGPURenderPipeline pipeline);

WireResult HandleRenderPassEncoderSetResourceTable(DeserializeBuffer* deserializeBuffer);
WireResult DoRenderPassEncoderSetResourceTable(WGPURenderPassEncoder self, WGPUResourceTable table);

WireResult HandleRenderPassEncoderSetScissorRect(DeserializeBuffer* deserializeBuffer);
WireResult DoRenderPassEncoderSetScissorRect(WGPURenderPassEncoder self, uint32_t x, uint32_t y, uint32_t width, uint32_t height);

WireResult HandleRenderPassEncoderSetStencilReference(DeserializeBuffer* deserializeBuffer);
WireResult DoRenderPassEncoderSetStencilReference(WGPURenderPassEncoder self, uint32_t reference);

WireResult HandleRenderPassEncoderSetVertexBuffer(DeserializeBuffer* deserializeBuffer);
WireResult DoRenderPassEncoderSetVertexBuffer(WGPURenderPassEncoder self, uint32_t slot, WGPUBuffer buffer, uint64_t offset, uint64_t size);

WireResult HandleRenderPassEncoderSetViewport(DeserializeBuffer* deserializeBuffer);
WireResult DoRenderPassEncoderSetViewport(WGPURenderPassEncoder self, float x, float y, float width, float height, float minDepth, float maxDepth);

WireResult HandleRenderPassEncoderWriteTimestamp(DeserializeBuffer* deserializeBuffer);
WireResult DoRenderPassEncoderWriteTimestamp(WGPURenderPassEncoder self, WGPUQuerySet querySet, uint32_t queryIndex);

WireResult HandleRenderPipelineGetBindGroupLayout(DeserializeBuffer* deserializeBuffer);
WireResult DoRenderPipelineGetBindGroupLayout(WGPURenderPipeline self, uint32_t groupIndex, WGPUBindGroupLayout* result);

WireResult HandleRenderPipelineSetLabel(DeserializeBuffer* deserializeBuffer);
WireResult DoRenderPipelineSetLabel(WGPURenderPipeline self, WGPUStringView label);

WireResult HandleResourceTableDestroy(DeserializeBuffer* deserializeBuffer);
WireResult DoResourceTableDestroy(WGPUResourceTable self);

WireResult HandleResourceTableRemove(DeserializeBuffer* deserializeBuffer);
WireResult DoResourceTableRemove(WGPUResourceTable self, uint32_t slot);

WireResult HandleResourceTableSetLabel(DeserializeBuffer* deserializeBuffer);
WireResult DoResourceTableSetLabel(WGPUResourceTable self, WGPUStringView label);

WireResult HandleResourceTableUpdate(DeserializeBuffer* deserializeBuffer);
WireResult DoResourceTableUpdate(WGPUResourceTable self, uint32_t slot, WGPUBindingResource const * resource);

WireResult HandleSamplerSetLabel(DeserializeBuffer* deserializeBuffer);
WireResult DoSamplerSetLabel(WGPUSampler self, WGPUStringView label);

WireResult HandleShaderModuleGetCompilationInfo(DeserializeBuffer* deserializeBuffer);
WireResult DoShaderModuleGetCompilationInfo(Known<WGPUShaderModule> shaderModuleId, Known<WGPUInstance> instanceId, WGPUFuture future);

WireResult HandleShaderModuleSetLabel(DeserializeBuffer* deserializeBuffer);
WireResult DoShaderModuleSetLabel(WGPUShaderModule self, WGPUStringView label);

WireResult HandleSurfaceConfigure(DeserializeBuffer* deserializeBuffer);
WireResult DoSurfaceConfigure(WGPUSurface self, WGPUSurfaceConfiguration const * config);

WireResult HandleSurfaceGetCurrentTexture(DeserializeBuffer* deserializeBuffer);
WireResult DoSurfaceGetCurrentTexture(Known<WGPUSurface> surfaceId, Known<WGPUDevice> configuredDeviceId, ObjectHandle textureHandle);

WireResult HandleSurfacePresent(DeserializeBuffer* deserializeBuffer);
WireResult DoSurfacePresent(WGPUSurface self);

WireResult HandleSurfaceSetLabel(DeserializeBuffer* deserializeBuffer);
WireResult DoSurfaceSetLabel(WGPUSurface self, WGPUStringView label);

WireResult HandleSurfaceUnconfigure(DeserializeBuffer* deserializeBuffer);
WireResult DoSurfaceUnconfigure(WGPUSurface self);

WireResult HandleTexelBufferViewSetLabel(DeserializeBuffer* deserializeBuffer);
WireResult DoTexelBufferViewSetLabel(WGPUTexelBufferView self, WGPUStringView label);

WireResult HandleTextureCreateErrorView(DeserializeBuffer* deserializeBuffer);
WireResult DoTextureCreateErrorView(WGPUTexture self, WGPUTextureViewDescriptor const * descriptor, WGPUTextureView* result);

WireResult HandleTextureCreateView(DeserializeBuffer* deserializeBuffer);
WireResult DoTextureCreateView(WGPUTexture self, WGPUTextureViewDescriptor const * descriptor, WGPUTextureView* result);

WireResult HandleTextureDestroy(DeserializeBuffer* deserializeBuffer);
WireResult DoTextureDestroy(WGPUTexture self);

WireResult HandleTextureSetLabel(DeserializeBuffer* deserializeBuffer);
WireResult DoTextureSetLabel(WGPUTexture self, WGPUStringView label);

WireResult HandleTextureViewSetLabel(DeserializeBuffer* deserializeBuffer);
WireResult DoTextureViewSetLabel(WGPUTextureView self, WGPUStringView label);

WireResult HandleUnregisterObject(DeserializeBuffer* deserializeBuffer);
WireResult DoUnregisterObject(ObjectType objectType, ObjectId objectId);


WireResult PreHandleDeviceCreateErrorBuffer(const DeviceCreateErrorBufferCmd& cmd);
WireResult PreHandleBufferDestroy(const BufferDestroyCmd& cmd);
WireResult PreHandleBufferUnmap(const BufferUnmapCmd& cmd);
