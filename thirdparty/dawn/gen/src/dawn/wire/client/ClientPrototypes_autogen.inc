
// Command handlers & doers
WireResult HandleAdapterRequestDeviceCallback(DeserializeBuffer* deserializeBuffer);
WireResult DoAdapterRequestDeviceCallback(ObjectId instanceId, WGPUFuture future, WGPURequestDeviceStatus status, WGPUStringView message, WGPULimits const * limits, uint32_t featuresCount, WGPUFeatureName const * features);

WireResult HandleBufferMapAsyncCallback(DeserializeBuffer* deserializeBuffer);
WireResult DoBufferMapAsyncCallback(ObjectId instanceId, WGPUFuture future, WGPUMapAsyncStatus status, WGPUStringView message, size_t readDataUpdateInfoLength, std::byte const * readDataUpdateInfo);

WireResult HandleDeviceCreateComputePipelineAsyncCallback(DeserializeBuffer* deserializeBuffer);
WireResult DoDeviceCreateComputePipelineAsyncCallback(ObjectId instanceId, WGPUFuture future, WGPUCreatePipelineAsyncStatus status, WGPUStringView message);

WireResult HandleDeviceCreateRenderPipelineAsyncCallback(DeserializeBuffer* deserializeBuffer);
WireResult DoDeviceCreateRenderPipelineAsyncCallback(ObjectId instanceId, WGPUFuture future, WGPUCreatePipelineAsyncStatus status, WGPUStringView message);

WireResult HandleDeviceLoggingCallback(DeserializeBuffer* deserializeBuffer);
WireResult DoDeviceLoggingCallback(Device* device, WGPULoggingType type, WGPUStringView message);

WireResult HandleDeviceLostCallback(DeserializeBuffer* deserializeBuffer);
WireResult DoDeviceLostCallback(ObjectId instanceId, WGPUFuture future, WGPUDeviceLostReason reason, WGPUStringView message);

WireResult HandleDevicePopErrorScopeCallback(DeserializeBuffer* deserializeBuffer);
WireResult DoDevicePopErrorScopeCallback(ObjectId instanceId, WGPUFuture future, WGPUPopErrorScopeStatus status, WGPUErrorType type, WGPUStringView message);

WireResult HandleDeviceUncapturedErrorCallback(DeserializeBuffer* deserializeBuffer);
WireResult DoDeviceUncapturedErrorCallback(Device* device, WGPUErrorType type, WGPUStringView message);

WireResult HandleInstanceRequestAdapterCallback(DeserializeBuffer* deserializeBuffer);
WireResult DoInstanceRequestAdapterCallback(ObjectId instanceId, WGPUFuture future, WGPURequestAdapterStatus status, WGPUStringView message, WGPUAdapterInfo const * info, WGPULimits const * limits, uint32_t featuresCount, WGPUFeatureName const * features);

WireResult HandleQueueWorkDoneCallback(DeserializeBuffer* deserializeBuffer);
WireResult DoQueueWorkDoneCallback(ObjectId instanceId, WGPUFuture future, WGPUQueueWorkDoneStatus status, WGPUStringView message);

WireResult HandleShaderModuleGetCompilationInfoCallback(DeserializeBuffer* deserializeBuffer);
WireResult DoShaderModuleGetCompilationInfoCallback(ObjectId instanceId, WGPUFuture future, WGPUCompilationInfoRequestStatus status, WGPUCompilationInfo const * info);

