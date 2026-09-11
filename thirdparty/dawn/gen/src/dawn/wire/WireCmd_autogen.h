
#ifndef DAWNWIRE_WIRECMD_AUTOGEN_H_
#define DAWNWIRE_WIRECMD_AUTOGEN_H_

#include <webgpu/webgpu.h>

#include <optional>

#include "dawn/wire/ObjectType_autogen.h"
#include "src/dawn/wire/BufferConsumer.h"
#include "src/dawn/wire/ObjectHandle.h"
#include "src/dawn/wire/WireResult.h"
#include "src/utils/span.h"

namespace dawn::wire {

    // Interface to allocate more space to deserialize pointed-to data.
    // nullptr is treated as an error.
    class DeserializeAllocator {
        public:
            virtual std::optional<Span<std::byte>> TryGetSpace(size_t size) = 0;
    };

    // Interface to convert an ID to a server object, if possible.
    // Methods return FatalError if the ID is for a non-existent object and Success otherwise.
    class ObjectIdResolver {
        public:
            virtual WireResult GetFromId(ObjectId id, WGPUAdapter* out) const = 0;
            virtual WireResult GetOptionalFromId(ObjectId id, WGPUAdapter* out) const = 0;
            virtual WireResult GetFromId(ObjectId id, WGPUBindGroup* out) const = 0;
            virtual WireResult GetOptionalFromId(ObjectId id, WGPUBindGroup* out) const = 0;
            virtual WireResult GetFromId(ObjectId id, WGPUBindGroupLayout* out) const = 0;
            virtual WireResult GetOptionalFromId(ObjectId id, WGPUBindGroupLayout* out) const = 0;
            virtual WireResult GetFromId(ObjectId id, WGPUBuffer* out) const = 0;
            virtual WireResult GetOptionalFromId(ObjectId id, WGPUBuffer* out) const = 0;
            virtual WireResult GetFromId(ObjectId id, WGPUCommandBuffer* out) const = 0;
            virtual WireResult GetOptionalFromId(ObjectId id, WGPUCommandBuffer* out) const = 0;
            virtual WireResult GetFromId(ObjectId id, WGPUCommandEncoder* out) const = 0;
            virtual WireResult GetOptionalFromId(ObjectId id, WGPUCommandEncoder* out) const = 0;
            virtual WireResult GetFromId(ObjectId id, WGPUComputePipeline* out) const = 0;
            virtual WireResult GetOptionalFromId(ObjectId id, WGPUComputePipeline* out) const = 0;
            virtual WireResult GetFromId(ObjectId id, WGPUDevice* out) const = 0;
            virtual WireResult GetOptionalFromId(ObjectId id, WGPUDevice* out) const = 0;
            virtual WireResult GetFromId(ObjectId id, WGPUExternalTexture* out) const = 0;
            virtual WireResult GetOptionalFromId(ObjectId id, WGPUExternalTexture* out) const = 0;
            virtual WireResult GetFromId(ObjectId id, WGPUInstance* out) const = 0;
            virtual WireResult GetOptionalFromId(ObjectId id, WGPUInstance* out) const = 0;
            virtual WireResult GetFromId(ObjectId id, WGPUPipelineLayout* out) const = 0;
            virtual WireResult GetOptionalFromId(ObjectId id, WGPUPipelineLayout* out) const = 0;
            virtual WireResult GetFromId(ObjectId id, WGPUQuerySet* out) const = 0;
            virtual WireResult GetOptionalFromId(ObjectId id, WGPUQuerySet* out) const = 0;
            virtual WireResult GetFromId(ObjectId id, WGPUQueue* out) const = 0;
            virtual WireResult GetOptionalFromId(ObjectId id, WGPUQueue* out) const = 0;
            virtual WireResult GetFromId(ObjectId id, WGPURenderBundle* out) const = 0;
            virtual WireResult GetOptionalFromId(ObjectId id, WGPURenderBundle* out) const = 0;
            virtual WireResult GetFromId(ObjectId id, WGPURenderPipeline* out) const = 0;
            virtual WireResult GetOptionalFromId(ObjectId id, WGPURenderPipeline* out) const = 0;
            virtual WireResult GetFromId(ObjectId id, WGPUResourceTable* out) const = 0;
            virtual WireResult GetOptionalFromId(ObjectId id, WGPUResourceTable* out) const = 0;
            virtual WireResult GetFromId(ObjectId id, WGPUSampler* out) const = 0;
            virtual WireResult GetOptionalFromId(ObjectId id, WGPUSampler* out) const = 0;
            virtual WireResult GetFromId(ObjectId id, WGPUShaderModule* out) const = 0;
            virtual WireResult GetOptionalFromId(ObjectId id, WGPUShaderModule* out) const = 0;
            virtual WireResult GetFromId(ObjectId id, WGPUSurface* out) const = 0;
            virtual WireResult GetOptionalFromId(ObjectId id, WGPUSurface* out) const = 0;
            virtual WireResult GetFromId(ObjectId id, WGPUTexelBufferView* out) const = 0;
            virtual WireResult GetOptionalFromId(ObjectId id, WGPUTexelBufferView* out) const = 0;
            virtual WireResult GetFromId(ObjectId id, WGPUTexture* out) const = 0;
            virtual WireResult GetOptionalFromId(ObjectId id, WGPUTexture* out) const = 0;
            virtual WireResult GetFromId(ObjectId id, WGPUTextureView* out) const = 0;
            virtual WireResult GetOptionalFromId(ObjectId id, WGPUTextureView* out) const = 0;
            virtual WireResult GetFromId(ObjectId id, WGPUComputePassEncoder* out) const = 0;
            virtual WireResult GetOptionalFromId(ObjectId id, WGPUComputePassEncoder* out) const = 0;
            virtual WireResult GetFromId(ObjectId id, WGPURenderBundleEncoder* out) const = 0;
            virtual WireResult GetOptionalFromId(ObjectId id, WGPURenderBundleEncoder* out) const = 0;
            virtual WireResult GetFromId(ObjectId id, WGPURenderPassEncoder* out) const = 0;
            virtual WireResult GetOptionalFromId(ObjectId id, WGPURenderPassEncoder* out) const = 0;
    };

    // Interface to convert a client object to its ID for the wiring.
    class ObjectIdProvider {
        public:
            virtual WireResult GetId(WGPUAdapter object, volatile ObjectId* out) const = 0;
            virtual WireResult GetOptionalId(WGPUAdapter object, volatile ObjectId* out) const = 0;
            virtual WireResult GetId(WGPUBindGroup object, volatile ObjectId* out) const = 0;
            virtual WireResult GetOptionalId(WGPUBindGroup object, volatile ObjectId* out) const = 0;
            virtual WireResult GetId(WGPUBindGroupLayout object, volatile ObjectId* out) const = 0;
            virtual WireResult GetOptionalId(WGPUBindGroupLayout object, volatile ObjectId* out) const = 0;
            virtual WireResult GetId(WGPUBuffer object, volatile ObjectId* out) const = 0;
            virtual WireResult GetOptionalId(WGPUBuffer object, volatile ObjectId* out) const = 0;
            virtual WireResult GetId(WGPUCommandBuffer object, volatile ObjectId* out) const = 0;
            virtual WireResult GetOptionalId(WGPUCommandBuffer object, volatile ObjectId* out) const = 0;
            virtual WireResult GetId(WGPUCommandEncoder object, volatile ObjectId* out) const = 0;
            virtual WireResult GetOptionalId(WGPUCommandEncoder object, volatile ObjectId* out) const = 0;
            virtual WireResult GetId(WGPUComputePipeline object, volatile ObjectId* out) const = 0;
            virtual WireResult GetOptionalId(WGPUComputePipeline object, volatile ObjectId* out) const = 0;
            virtual WireResult GetId(WGPUDevice object, volatile ObjectId* out) const = 0;
            virtual WireResult GetOptionalId(WGPUDevice object, volatile ObjectId* out) const = 0;
            virtual WireResult GetId(WGPUExternalTexture object, volatile ObjectId* out) const = 0;
            virtual WireResult GetOptionalId(WGPUExternalTexture object, volatile ObjectId* out) const = 0;
            virtual WireResult GetId(WGPUInstance object, volatile ObjectId* out) const = 0;
            virtual WireResult GetOptionalId(WGPUInstance object, volatile ObjectId* out) const = 0;
            virtual WireResult GetId(WGPUPipelineLayout object, volatile ObjectId* out) const = 0;
            virtual WireResult GetOptionalId(WGPUPipelineLayout object, volatile ObjectId* out) const = 0;
            virtual WireResult GetId(WGPUQuerySet object, volatile ObjectId* out) const = 0;
            virtual WireResult GetOptionalId(WGPUQuerySet object, volatile ObjectId* out) const = 0;
            virtual WireResult GetId(WGPUQueue object, volatile ObjectId* out) const = 0;
            virtual WireResult GetOptionalId(WGPUQueue object, volatile ObjectId* out) const = 0;
            virtual WireResult GetId(WGPURenderBundle object, volatile ObjectId* out) const = 0;
            virtual WireResult GetOptionalId(WGPURenderBundle object, volatile ObjectId* out) const = 0;
            virtual WireResult GetId(WGPURenderPipeline object, volatile ObjectId* out) const = 0;
            virtual WireResult GetOptionalId(WGPURenderPipeline object, volatile ObjectId* out) const = 0;
            virtual WireResult GetId(WGPUResourceTable object, volatile ObjectId* out) const = 0;
            virtual WireResult GetOptionalId(WGPUResourceTable object, volatile ObjectId* out) const = 0;
            virtual WireResult GetId(WGPUSampler object, volatile ObjectId* out) const = 0;
            virtual WireResult GetOptionalId(WGPUSampler object, volatile ObjectId* out) const = 0;
            virtual WireResult GetId(WGPUShaderModule object, volatile ObjectId* out) const = 0;
            virtual WireResult GetOptionalId(WGPUShaderModule object, volatile ObjectId* out) const = 0;
            virtual WireResult GetId(WGPUSurface object, volatile ObjectId* out) const = 0;
            virtual WireResult GetOptionalId(WGPUSurface object, volatile ObjectId* out) const = 0;
            virtual WireResult GetId(WGPUTexelBufferView object, volatile ObjectId* out) const = 0;
            virtual WireResult GetOptionalId(WGPUTexelBufferView object, volatile ObjectId* out) const = 0;
            virtual WireResult GetId(WGPUTexture object, volatile ObjectId* out) const = 0;
            virtual WireResult GetOptionalId(WGPUTexture object, volatile ObjectId* out) const = 0;
            virtual WireResult GetId(WGPUTextureView object, volatile ObjectId* out) const = 0;
            virtual WireResult GetOptionalId(WGPUTextureView object, volatile ObjectId* out) const = 0;
            virtual WireResult GetId(WGPUComputePassEncoder object, volatile ObjectId* out) const = 0;
            virtual WireResult GetOptionalId(WGPUComputePassEncoder object, volatile ObjectId* out) const = 0;
            virtual WireResult GetId(WGPURenderBundleEncoder object, volatile ObjectId* out) const = 0;
            virtual WireResult GetOptionalId(WGPURenderBundleEncoder object, volatile ObjectId* out) const = 0;
            virtual WireResult GetId(WGPURenderPassEncoder object, volatile ObjectId* out) const = 0;
            virtual WireResult GetOptionalId(WGPURenderPassEncoder object, volatile ObjectId* out) const = 0;
    };

    enum class WireCmd : uint32_t {
        // Enums used in special wire commands.
        ChunkedCommand,

        // Enums used on the wire format.
        AdapterRequestDevice,
        BindGroupLayoutSetLabel,
        BindGroupSetLabel,
        BufferCreateTexelView,
        BufferDestroy,
        BufferMapAsync,
        BufferSetLabel,
        BufferUnmap,
        BufferUpdateMappedData,
        CommandBufferSetLabel,
        CommandEncoderBeginComputePass,
        CommandEncoderBeginRenderPass,
        CommandEncoderClearBuffer,
        CommandEncoderCopyBufferToBuffer,
        CommandEncoderCopyBufferToTexture,
        CommandEncoderCopyTextureToBuffer,
        CommandEncoderCopyTextureToTexture,
        CommandEncoderFinish,
        CommandEncoderInjectValidationError,
        CommandEncoderInsertDebugMarker,
        CommandEncoderPopDebugGroup,
        CommandEncoderPushDebugGroup,
        CommandEncoderResolveQuerySet,
        CommandEncoderSetLabel,
        CommandEncoderWriteBuffer,
        CommandEncoderWriteTimestamp,
        ComputePassEncoderDispatchWorkgroups,
        ComputePassEncoderDispatchWorkgroupsIndirect,
        ComputePassEncoderEnd,
        ComputePassEncoderInsertDebugMarker,
        ComputePassEncoderPopDebugGroup,
        ComputePassEncoderPushDebugGroup,
        ComputePassEncoderSetBindGroup,
        ComputePassEncoderSetImmediates,
        ComputePassEncoderSetLabel,
        ComputePassEncoderSetPipeline,
        ComputePassEncoderSetResourceTable,
        ComputePassEncoderWriteTimestamp,
        ComputePipelineGetBindGroupLayout,
        ComputePipelineSetLabel,
        DeviceCreateBindGroup,
        DeviceCreateBindGroupLayout,
        DeviceCreateBuffer,
        DeviceCreateCommandEncoder,
        DeviceCreateComputePipeline,
        DeviceCreateComputePipelineAsync,
        DeviceCreateErrorBuffer,
        DeviceCreateErrorComputePipeline,
        DeviceCreateErrorExternalTexture,
        DeviceCreateErrorRenderPipeline,
        DeviceCreateErrorShaderModule,
        DeviceCreateErrorTexture,
        DeviceCreateExternalTexture,
        DeviceCreatePipelineLayout,
        DeviceCreateQuerySet,
        DeviceCreateRenderBundleEncoder,
        DeviceCreateRenderPipeline,
        DeviceCreateRenderPipelineAsync,
        DeviceCreateResourceTable,
        DeviceCreateSampler,
        DeviceCreateShaderModule,
        DeviceCreateTexture,
        DeviceDestroy,
        DeviceForceLoss,
        DeviceGetQueue,
        DeviceInjectError,
        DevicePopErrorScope,
        DevicePushErrorScope,
        DeviceSetLabel,
        DeviceTick,
        DeviceValidateTextureDescriptor,
        ExternalTextureDestroy,
        ExternalTextureExpire,
        ExternalTextureRefresh,
        ExternalTextureSetLabel,
        InstanceProcessEvents,
        InstanceRequestAdapter,
        PipelineLayoutSetLabel,
        QuerySetDestroy,
        QuerySetSetLabel,
        QueueCopyExternalTextureForBrowser,
        QueueCopyTextureForBrowser,
        QueueOnSubmittedWorkDone,
        QueueSetLabel,
        QueueSubmit,
        QueueWriteBuffer,
        QueueWriteBufferXl,
        QueueWriteTexture,
        QueueWriteTextureXl,
        RenderBundleEncoderDraw,
        RenderBundleEncoderDrawIndexed,
        RenderBundleEncoderDrawIndexedIndirect,
        RenderBundleEncoderDrawIndirect,
        RenderBundleEncoderFinish,
        RenderBundleEncoderInsertDebugMarker,
        RenderBundleEncoderPopDebugGroup,
        RenderBundleEncoderPushDebugGroup,
        RenderBundleEncoderSetBindGroup,
        RenderBundleEncoderSetImmediates,
        RenderBundleEncoderSetIndexBuffer,
        RenderBundleEncoderSetLabel,
        RenderBundleEncoderSetPipeline,
        RenderBundleEncoderSetResourceTable,
        RenderBundleEncoderSetVertexBuffer,
        RenderBundleSetLabel,
        RenderPassEncoderBeginOcclusionQuery,
        RenderPassEncoderDraw,
        RenderPassEncoderDrawIndexed,
        RenderPassEncoderDrawIndexedIndirect,
        RenderPassEncoderDrawIndirect,
        RenderPassEncoderEnd,
        RenderPassEncoderEndOcclusionQuery,
        RenderPassEncoderExecuteBundles,
        RenderPassEncoderInsertDebugMarker,
        RenderPassEncoderMultiDrawIndexedIndirect,
        RenderPassEncoderMultiDrawIndirect,
        RenderPassEncoderPixelLocalStorageBarrier,
        RenderPassEncoderPopDebugGroup,
        RenderPassEncoderPushDebugGroup,
        RenderPassEncoderSetBindGroup,
        RenderPassEncoderSetBlendConstant,
        RenderPassEncoderSetImmediates,
        RenderPassEncoderSetIndexBuffer,
        RenderPassEncoderSetLabel,
        RenderPassEncoderSetPipeline,
        RenderPassEncoderSetResourceTable,
        RenderPassEncoderSetScissorRect,
        RenderPassEncoderSetStencilReference,
        RenderPassEncoderSetVertexBuffer,
        RenderPassEncoderSetViewport,
        RenderPassEncoderWriteTimestamp,
        RenderPipelineGetBindGroupLayout,
        RenderPipelineSetLabel,
        ResourceTableDestroy,
        ResourceTableRemove,
        ResourceTableSetLabel,
        ResourceTableUpdate,
        SamplerSetLabel,
        ShaderModuleGetCompilationInfo,
        ShaderModuleSetLabel,
        SurfaceConfigure,
        SurfaceGetCurrentTexture,
        SurfacePresent,
        SurfaceSetLabel,
        SurfaceUnconfigure,
        TexelBufferViewSetLabel,
        TextureCreateErrorView,
        TextureCreateView,
        TextureDestroy,
        TextureSetLabel,
        TextureViewSetLabel,
        UnregisterObject,

        // Enums used on the return wire format.
        ReturnAdapterRequestDeviceCallback,
        ReturnBufferMapAsyncCallback,
        ReturnDeviceCreateComputePipelineAsyncCallback,
        ReturnDeviceCreateRenderPipelineAsyncCallback,
        ReturnDeviceLoggingCallback,
        ReturnDeviceLostCallback,
        ReturnDevicePopErrorScopeCallback,
        ReturnDeviceUncapturedErrorCallback,
        ReturnInstanceRequestAdapterCallback,
        ReturnQueueWorkDoneCallback,
        ReturnShaderModuleGetCompilationInfoCallback,
    };

    struct CmdHeader {
        uint64_t commandSize;
        WireCmd commandId;

        CmdHeader() = default;
        CmdHeader(const CmdHeader&) = default;
        CmdHeader(CmdHeader&&) = default;

        // Volatile constructors and assignment operators are never expected to be called at
        // runtime when handling wire commands. They exist solely to satisfy C++20 iterator and
        // std::span requirements (e.g. std::indirectly_readable) when constructing views over
        // volatile shared memory buffers.
        [[noreturn]] CmdHeader(const volatile CmdHeader& other) {
            DAWN_UNREACHABLE();
        }
        [[noreturn]] CmdHeader(volatile CmdHeader&& other) {
            DAWN_UNREACHABLE();
        }
        [[noreturn]] CmdHeader(const volatile CmdHeader&& other) {
            DAWN_UNREACHABLE();
        }
        [[noreturn]] CmdHeader& operator=(const volatile CmdHeader& other) {
            DAWN_UNREACHABLE();
        }
    };


    struct ChunkedCommandCmd {
    size_t GetRequiredSize() const;

    WireResult Serialize(size_t commandSize, SerializeBuffer* serializeBuffer, const ObjectIdProvider& objectIdProvider) const;
    // Override which produces a FatalError if any object is used.
    WireResult Serialize(size_t commandSize, SerializeBuffer* serializeBuffer) const;

    WireResult Deserialize(DeserializeBuffer* deserializeBuffer, DeserializeAllocator* allocator, const ObjectIdResolver& resolver);
    // Override which produces a FatalError if any object is used.
    WireResult Deserialize(DeserializeBuffer* deserializeBuffer, DeserializeAllocator* allocator);


    uint64_t id;
    size_t size;
    std::byte const * chunkData;
    size_t chunkSize;
};


    struct AdapterRequestDeviceCmd {
    size_t GetRequiredSize() const;

    WireResult Serialize(size_t commandSize, SerializeBuffer* serializeBuffer, const ObjectIdProvider& objectIdProvider) const;
    // Override which produces a FatalError if any object is used.
    WireResult Serialize(size_t commandSize, SerializeBuffer* serializeBuffer) const;

    WireResult Deserialize(DeserializeBuffer* deserializeBuffer, DeserializeAllocator* allocator, const ObjectIdResolver& resolver);
    // Override which produces a FatalError if any object is used.
    WireResult Deserialize(DeserializeBuffer* deserializeBuffer, DeserializeAllocator* allocator);


    ObjectId adapterId;
    ObjectId instanceId;
    WGPUFuture future;
    ObjectHandle deviceObjectHandle;
    WGPUFuture deviceLostFuture;
    WGPUDeviceDescriptor const * descriptor;
};

    struct BindGroupLayoutSetLabelCmd {
    size_t GetRequiredSize() const;

    WireResult Serialize(size_t commandSize, SerializeBuffer* serializeBuffer, const ObjectIdProvider& objectIdProvider) const;
    // Override which produces a FatalError if any object is used.
    WireResult Serialize(size_t commandSize, SerializeBuffer* serializeBuffer) const;

    WireResult Deserialize(DeserializeBuffer* deserializeBuffer, DeserializeAllocator* allocator, const ObjectIdResolver& resolver);
    // Override which produces a FatalError if any object is used.
    WireResult Deserialize(DeserializeBuffer* deserializeBuffer, DeserializeAllocator* allocator);

    ObjectId selfId;

    WGPUBindGroupLayout self;
    WGPUStringView label;
};

    struct BindGroupSetLabelCmd {
    size_t GetRequiredSize() const;

    WireResult Serialize(size_t commandSize, SerializeBuffer* serializeBuffer, const ObjectIdProvider& objectIdProvider) const;
    // Override which produces a FatalError if any object is used.
    WireResult Serialize(size_t commandSize, SerializeBuffer* serializeBuffer) const;

    WireResult Deserialize(DeserializeBuffer* deserializeBuffer, DeserializeAllocator* allocator, const ObjectIdResolver& resolver);
    // Override which produces a FatalError if any object is used.
    WireResult Deserialize(DeserializeBuffer* deserializeBuffer, DeserializeAllocator* allocator);

    ObjectId selfId;

    WGPUBindGroup self;
    WGPUStringView label;
};

    struct BufferCreateTexelViewCmd {
    size_t GetRequiredSize() const;

    WireResult Serialize(size_t commandSize, SerializeBuffer* serializeBuffer, const ObjectIdProvider& objectIdProvider) const;
    // Override which produces a FatalError if any object is used.
    WireResult Serialize(size_t commandSize, SerializeBuffer* serializeBuffer) const;

    WireResult Deserialize(DeserializeBuffer* deserializeBuffer, DeserializeAllocator* allocator, const ObjectIdResolver& resolver);
    // Override which produces a FatalError if any object is used.
    WireResult Deserialize(DeserializeBuffer* deserializeBuffer, DeserializeAllocator* allocator);

    ObjectId selfId;

    WGPUBuffer self;
    WGPUTexelBufferViewDescriptor const * descriptor;
    ObjectHandle result;
};

    struct BufferDestroyCmd {
    size_t GetRequiredSize() const;

    WireResult Serialize(size_t commandSize, SerializeBuffer* serializeBuffer, const ObjectIdProvider& objectIdProvider) const;
    // Override which produces a FatalError if any object is used.
    WireResult Serialize(size_t commandSize, SerializeBuffer* serializeBuffer) const;

    WireResult Deserialize(DeserializeBuffer* deserializeBuffer, DeserializeAllocator* allocator, const ObjectIdResolver& resolver);
    // Override which produces a FatalError if any object is used.
    WireResult Deserialize(DeserializeBuffer* deserializeBuffer, DeserializeAllocator* allocator);

    ObjectId selfId;

    WGPUBuffer self;
};

    struct BufferMapAsyncCmd {
    size_t GetRequiredSize() const;

    WireResult Serialize(size_t commandSize, SerializeBuffer* serializeBuffer, const ObjectIdProvider& objectIdProvider) const;
    // Override which produces a FatalError if any object is used.
    WireResult Serialize(size_t commandSize, SerializeBuffer* serializeBuffer) const;

    WireResult Deserialize(DeserializeBuffer* deserializeBuffer, DeserializeAllocator* allocator, const ObjectIdResolver& resolver);
    // Override which produces a FatalError if any object is used.
    WireResult Deserialize(DeserializeBuffer* deserializeBuffer, DeserializeAllocator* allocator);


    ObjectId bufferId;
    ObjectId instanceId;
    WGPUFuture future;
    WGPUMapMode mode;
    size_t offset;
    size_t size;
};

    struct BufferSetLabelCmd {
    size_t GetRequiredSize() const;

    WireResult Serialize(size_t commandSize, SerializeBuffer* serializeBuffer, const ObjectIdProvider& objectIdProvider) const;
    // Override which produces a FatalError if any object is used.
    WireResult Serialize(size_t commandSize, SerializeBuffer* serializeBuffer) const;

    WireResult Deserialize(DeserializeBuffer* deserializeBuffer, DeserializeAllocator* allocator, const ObjectIdResolver& resolver);
    // Override which produces a FatalError if any object is used.
    WireResult Deserialize(DeserializeBuffer* deserializeBuffer, DeserializeAllocator* allocator);

    ObjectId selfId;

    WGPUBuffer self;
    WGPUStringView label;
};

    struct BufferUnmapCmd {
    size_t GetRequiredSize() const;

    WireResult Serialize(size_t commandSize, SerializeBuffer* serializeBuffer, const ObjectIdProvider& objectIdProvider) const;
    // Override which produces a FatalError if any object is used.
    WireResult Serialize(size_t commandSize, SerializeBuffer* serializeBuffer) const;

    WireResult Deserialize(DeserializeBuffer* deserializeBuffer, DeserializeAllocator* allocator, const ObjectIdResolver& resolver);
    // Override which produces a FatalError if any object is used.
    WireResult Deserialize(DeserializeBuffer* deserializeBuffer, DeserializeAllocator* allocator);

    ObjectId selfId;

    WGPUBuffer self;
};

    struct BufferUpdateMappedDataCmd {
    size_t GetRequiredSize() const;

    WireResult Serialize(size_t commandSize, SerializeBuffer* serializeBuffer, const ObjectIdProvider& objectIdProvider) const;
    // Override which produces a FatalError if any object is used.
    WireResult Serialize(size_t commandSize, SerializeBuffer* serializeBuffer) const;

    WireResult Deserialize(DeserializeBuffer* deserializeBuffer, DeserializeAllocator* allocator, const ObjectIdResolver& resolver);
    // Override which produces a FatalError if any object is used.
    WireResult Deserialize(DeserializeBuffer* deserializeBuffer, DeserializeAllocator* allocator);


    ObjectId bufferId;
    size_t dataUpdateInfoLength;
    std::byte const * dataUpdateInfo;
    size_t offset;
    size_t size;
};

    struct CommandBufferSetLabelCmd {
    size_t GetRequiredSize() const;

    WireResult Serialize(size_t commandSize, SerializeBuffer* serializeBuffer, const ObjectIdProvider& objectIdProvider) const;
    // Override which produces a FatalError if any object is used.
    WireResult Serialize(size_t commandSize, SerializeBuffer* serializeBuffer) const;

    WireResult Deserialize(DeserializeBuffer* deserializeBuffer, DeserializeAllocator* allocator, const ObjectIdResolver& resolver);
    // Override which produces a FatalError if any object is used.
    WireResult Deserialize(DeserializeBuffer* deserializeBuffer, DeserializeAllocator* allocator);

    ObjectId selfId;

    WGPUCommandBuffer self;
    WGPUStringView label;
};

    struct CommandEncoderBeginComputePassCmd {
    size_t GetRequiredSize() const;

    WireResult Serialize(size_t commandSize, SerializeBuffer* serializeBuffer, const ObjectIdProvider& objectIdProvider) const;
    // Override which produces a FatalError if any object is used.
    WireResult Serialize(size_t commandSize, SerializeBuffer* serializeBuffer) const;

    WireResult Deserialize(DeserializeBuffer* deserializeBuffer, DeserializeAllocator* allocator, const ObjectIdResolver& resolver);
    // Override which produces a FatalError if any object is used.
    WireResult Deserialize(DeserializeBuffer* deserializeBuffer, DeserializeAllocator* allocator);

    ObjectId selfId;

    WGPUCommandEncoder self;
    WGPUComputePassDescriptor const * descriptor;
    ObjectHandle result;
};

    struct CommandEncoderBeginRenderPassCmd {
    size_t GetRequiredSize() const;

    WireResult Serialize(size_t commandSize, SerializeBuffer* serializeBuffer, const ObjectIdProvider& objectIdProvider) const;
    // Override which produces a FatalError if any object is used.
    WireResult Serialize(size_t commandSize, SerializeBuffer* serializeBuffer) const;

    WireResult Deserialize(DeserializeBuffer* deserializeBuffer, DeserializeAllocator* allocator, const ObjectIdResolver& resolver);
    // Override which produces a FatalError if any object is used.
    WireResult Deserialize(DeserializeBuffer* deserializeBuffer, DeserializeAllocator* allocator);

    ObjectId selfId;

    WGPUCommandEncoder self;
    WGPURenderPassDescriptor const * descriptor;
    ObjectHandle result;
};

    struct CommandEncoderClearBufferCmd {
    size_t GetRequiredSize() const;

    WireResult Serialize(size_t commandSize, SerializeBuffer* serializeBuffer, const ObjectIdProvider& objectIdProvider) const;
    // Override which produces a FatalError if any object is used.
    WireResult Serialize(size_t commandSize, SerializeBuffer* serializeBuffer) const;

    WireResult Deserialize(DeserializeBuffer* deserializeBuffer, DeserializeAllocator* allocator, const ObjectIdResolver& resolver);
    // Override which produces a FatalError if any object is used.
    WireResult Deserialize(DeserializeBuffer* deserializeBuffer, DeserializeAllocator* allocator);

    ObjectId selfId;

    WGPUCommandEncoder self;
    WGPUBuffer buffer;
    uint64_t offset;
    uint64_t size;
};

    struct CommandEncoderCopyBufferToBufferCmd {
    size_t GetRequiredSize() const;

    WireResult Serialize(size_t commandSize, SerializeBuffer* serializeBuffer, const ObjectIdProvider& objectIdProvider) const;
    // Override which produces a FatalError if any object is used.
    WireResult Serialize(size_t commandSize, SerializeBuffer* serializeBuffer) const;

    WireResult Deserialize(DeserializeBuffer* deserializeBuffer, DeserializeAllocator* allocator, const ObjectIdResolver& resolver);
    // Override which produces a FatalError if any object is used.
    WireResult Deserialize(DeserializeBuffer* deserializeBuffer, DeserializeAllocator* allocator);

    ObjectId selfId;

    WGPUCommandEncoder self;
    WGPUBuffer source;
    uint64_t sourceOffset;
    WGPUBuffer destination;
    uint64_t destinationOffset;
    uint64_t size;
};

    struct CommandEncoderCopyBufferToTextureCmd {
    size_t GetRequiredSize() const;

    WireResult Serialize(size_t commandSize, SerializeBuffer* serializeBuffer, const ObjectIdProvider& objectIdProvider) const;
    // Override which produces a FatalError if any object is used.
    WireResult Serialize(size_t commandSize, SerializeBuffer* serializeBuffer) const;

    WireResult Deserialize(DeserializeBuffer* deserializeBuffer, DeserializeAllocator* allocator, const ObjectIdResolver& resolver);
    // Override which produces a FatalError if any object is used.
    WireResult Deserialize(DeserializeBuffer* deserializeBuffer, DeserializeAllocator* allocator);

    ObjectId selfId;

    WGPUCommandEncoder self;
    WGPUTexelCopyBufferInfo const * source;
    WGPUTexelCopyTextureInfo const * destination;
    WGPUExtent3D const * copySize;
};

    struct CommandEncoderCopyTextureToBufferCmd {
    size_t GetRequiredSize() const;

    WireResult Serialize(size_t commandSize, SerializeBuffer* serializeBuffer, const ObjectIdProvider& objectIdProvider) const;
    // Override which produces a FatalError if any object is used.
    WireResult Serialize(size_t commandSize, SerializeBuffer* serializeBuffer) const;

    WireResult Deserialize(DeserializeBuffer* deserializeBuffer, DeserializeAllocator* allocator, const ObjectIdResolver& resolver);
    // Override which produces a FatalError if any object is used.
    WireResult Deserialize(DeserializeBuffer* deserializeBuffer, DeserializeAllocator* allocator);

    ObjectId selfId;

    WGPUCommandEncoder self;
    WGPUTexelCopyTextureInfo const * source;
    WGPUTexelCopyBufferInfo const * destination;
    WGPUExtent3D const * copySize;
};

    struct CommandEncoderCopyTextureToTextureCmd {
    size_t GetRequiredSize() const;

    WireResult Serialize(size_t commandSize, SerializeBuffer* serializeBuffer, const ObjectIdProvider& objectIdProvider) const;
    // Override which produces a FatalError if any object is used.
    WireResult Serialize(size_t commandSize, SerializeBuffer* serializeBuffer) const;

    WireResult Deserialize(DeserializeBuffer* deserializeBuffer, DeserializeAllocator* allocator, const ObjectIdResolver& resolver);
    // Override which produces a FatalError if any object is used.
    WireResult Deserialize(DeserializeBuffer* deserializeBuffer, DeserializeAllocator* allocator);

    ObjectId selfId;

    WGPUCommandEncoder self;
    WGPUTexelCopyTextureInfo const * source;
    WGPUTexelCopyTextureInfo const * destination;
    WGPUExtent3D const * copySize;
};

    struct CommandEncoderFinishCmd {
    size_t GetRequiredSize() const;

    WireResult Serialize(size_t commandSize, SerializeBuffer* serializeBuffer, const ObjectIdProvider& objectIdProvider) const;
    // Override which produces a FatalError if any object is used.
    WireResult Serialize(size_t commandSize, SerializeBuffer* serializeBuffer) const;

    WireResult Deserialize(DeserializeBuffer* deserializeBuffer, DeserializeAllocator* allocator, const ObjectIdResolver& resolver);
    // Override which produces a FatalError if any object is used.
    WireResult Deserialize(DeserializeBuffer* deserializeBuffer, DeserializeAllocator* allocator);

    ObjectId selfId;

    WGPUCommandEncoder self;
    WGPUCommandBufferDescriptor const * descriptor;
    ObjectHandle result;
};

    struct CommandEncoderInjectValidationErrorCmd {
    size_t GetRequiredSize() const;

    WireResult Serialize(size_t commandSize, SerializeBuffer* serializeBuffer, const ObjectIdProvider& objectIdProvider) const;
    // Override which produces a FatalError if any object is used.
    WireResult Serialize(size_t commandSize, SerializeBuffer* serializeBuffer) const;

    WireResult Deserialize(DeserializeBuffer* deserializeBuffer, DeserializeAllocator* allocator, const ObjectIdResolver& resolver);
    // Override which produces a FatalError if any object is used.
    WireResult Deserialize(DeserializeBuffer* deserializeBuffer, DeserializeAllocator* allocator);

    ObjectId selfId;

    WGPUCommandEncoder self;
    WGPUStringView message;
};

    struct CommandEncoderInsertDebugMarkerCmd {
    size_t GetRequiredSize() const;

    WireResult Serialize(size_t commandSize, SerializeBuffer* serializeBuffer, const ObjectIdProvider& objectIdProvider) const;
    // Override which produces a FatalError if any object is used.
    WireResult Serialize(size_t commandSize, SerializeBuffer* serializeBuffer) const;

    WireResult Deserialize(DeserializeBuffer* deserializeBuffer, DeserializeAllocator* allocator, const ObjectIdResolver& resolver);
    // Override which produces a FatalError if any object is used.
    WireResult Deserialize(DeserializeBuffer* deserializeBuffer, DeserializeAllocator* allocator);

    ObjectId selfId;

    WGPUCommandEncoder self;
    WGPUStringView markerLabel;
};

    struct CommandEncoderPopDebugGroupCmd {
    size_t GetRequiredSize() const;

    WireResult Serialize(size_t commandSize, SerializeBuffer* serializeBuffer, const ObjectIdProvider& objectIdProvider) const;
    // Override which produces a FatalError if any object is used.
    WireResult Serialize(size_t commandSize, SerializeBuffer* serializeBuffer) const;

    WireResult Deserialize(DeserializeBuffer* deserializeBuffer, DeserializeAllocator* allocator, const ObjectIdResolver& resolver);
    // Override which produces a FatalError if any object is used.
    WireResult Deserialize(DeserializeBuffer* deserializeBuffer, DeserializeAllocator* allocator);

    ObjectId selfId;

    WGPUCommandEncoder self;
};

    struct CommandEncoderPushDebugGroupCmd {
    size_t GetRequiredSize() const;

    WireResult Serialize(size_t commandSize, SerializeBuffer* serializeBuffer, const ObjectIdProvider& objectIdProvider) const;
    // Override which produces a FatalError if any object is used.
    WireResult Serialize(size_t commandSize, SerializeBuffer* serializeBuffer) const;

    WireResult Deserialize(DeserializeBuffer* deserializeBuffer, DeserializeAllocator* allocator, const ObjectIdResolver& resolver);
    // Override which produces a FatalError if any object is used.
    WireResult Deserialize(DeserializeBuffer* deserializeBuffer, DeserializeAllocator* allocator);

    ObjectId selfId;

    WGPUCommandEncoder self;
    WGPUStringView groupLabel;
};

    struct CommandEncoderResolveQuerySetCmd {
    size_t GetRequiredSize() const;

    WireResult Serialize(size_t commandSize, SerializeBuffer* serializeBuffer, const ObjectIdProvider& objectIdProvider) const;
    // Override which produces a FatalError if any object is used.
    WireResult Serialize(size_t commandSize, SerializeBuffer* serializeBuffer) const;

    WireResult Deserialize(DeserializeBuffer* deserializeBuffer, DeserializeAllocator* allocator, const ObjectIdResolver& resolver);
    // Override which produces a FatalError if any object is used.
    WireResult Deserialize(DeserializeBuffer* deserializeBuffer, DeserializeAllocator* allocator);

    ObjectId selfId;

    WGPUCommandEncoder self;
    WGPUQuerySet querySet;
    uint32_t firstQuery;
    uint32_t queryCount;
    WGPUBuffer destination;
    uint64_t destinationOffset;
};

    struct CommandEncoderSetLabelCmd {
    size_t GetRequiredSize() const;

    WireResult Serialize(size_t commandSize, SerializeBuffer* serializeBuffer, const ObjectIdProvider& objectIdProvider) const;
    // Override which produces a FatalError if any object is used.
    WireResult Serialize(size_t commandSize, SerializeBuffer* serializeBuffer) const;

    WireResult Deserialize(DeserializeBuffer* deserializeBuffer, DeserializeAllocator* allocator, const ObjectIdResolver& resolver);
    // Override which produces a FatalError if any object is used.
    WireResult Deserialize(DeserializeBuffer* deserializeBuffer, DeserializeAllocator* allocator);

    ObjectId selfId;

    WGPUCommandEncoder self;
    WGPUStringView label;
};

    struct CommandEncoderWriteBufferCmd {
    size_t GetRequiredSize() const;

    WireResult Serialize(size_t commandSize, SerializeBuffer* serializeBuffer, const ObjectIdProvider& objectIdProvider) const;
    // Override which produces a FatalError if any object is used.
    WireResult Serialize(size_t commandSize, SerializeBuffer* serializeBuffer) const;

    WireResult Deserialize(DeserializeBuffer* deserializeBuffer, DeserializeAllocator* allocator, const ObjectIdResolver& resolver);
    // Override which produces a FatalError if any object is used.
    WireResult Deserialize(DeserializeBuffer* deserializeBuffer, DeserializeAllocator* allocator);

    ObjectId selfId;

    WGPUCommandEncoder self;
    WGPUBuffer buffer;
    uint64_t bufferOffset;
    uint8_t const * data;
    uint64_t size;
};

    struct CommandEncoderWriteTimestampCmd {
    size_t GetRequiredSize() const;

    WireResult Serialize(size_t commandSize, SerializeBuffer* serializeBuffer, const ObjectIdProvider& objectIdProvider) const;
    // Override which produces a FatalError if any object is used.
    WireResult Serialize(size_t commandSize, SerializeBuffer* serializeBuffer) const;

    WireResult Deserialize(DeserializeBuffer* deserializeBuffer, DeserializeAllocator* allocator, const ObjectIdResolver& resolver);
    // Override which produces a FatalError if any object is used.
    WireResult Deserialize(DeserializeBuffer* deserializeBuffer, DeserializeAllocator* allocator);

    ObjectId selfId;

    WGPUCommandEncoder self;
    WGPUQuerySet querySet;
    uint32_t queryIndex;
};

    struct ComputePassEncoderDispatchWorkgroupsCmd {
    size_t GetRequiredSize() const;

    WireResult Serialize(size_t commandSize, SerializeBuffer* serializeBuffer, const ObjectIdProvider& objectIdProvider) const;
    // Override which produces a FatalError if any object is used.
    WireResult Serialize(size_t commandSize, SerializeBuffer* serializeBuffer) const;

    WireResult Deserialize(DeserializeBuffer* deserializeBuffer, DeserializeAllocator* allocator, const ObjectIdResolver& resolver);
    // Override which produces a FatalError if any object is used.
    WireResult Deserialize(DeserializeBuffer* deserializeBuffer, DeserializeAllocator* allocator);

    ObjectId selfId;

    WGPUComputePassEncoder self;
    uint32_t workgroupCountX;
    uint32_t workgroupCountY;
    uint32_t workgroupCountZ;
};

    struct ComputePassEncoderDispatchWorkgroupsIndirectCmd {
    size_t GetRequiredSize() const;

    WireResult Serialize(size_t commandSize, SerializeBuffer* serializeBuffer, const ObjectIdProvider& objectIdProvider) const;
    // Override which produces a FatalError if any object is used.
    WireResult Serialize(size_t commandSize, SerializeBuffer* serializeBuffer) const;

    WireResult Deserialize(DeserializeBuffer* deserializeBuffer, DeserializeAllocator* allocator, const ObjectIdResolver& resolver);
    // Override which produces a FatalError if any object is used.
    WireResult Deserialize(DeserializeBuffer* deserializeBuffer, DeserializeAllocator* allocator);

    ObjectId selfId;

    WGPUComputePassEncoder self;
    WGPUBuffer indirectBuffer;
    uint64_t indirectOffset;
};

    struct ComputePassEncoderEndCmd {
    size_t GetRequiredSize() const;

    WireResult Serialize(size_t commandSize, SerializeBuffer* serializeBuffer, const ObjectIdProvider& objectIdProvider) const;
    // Override which produces a FatalError if any object is used.
    WireResult Serialize(size_t commandSize, SerializeBuffer* serializeBuffer) const;

    WireResult Deserialize(DeserializeBuffer* deserializeBuffer, DeserializeAllocator* allocator, const ObjectIdResolver& resolver);
    // Override which produces a FatalError if any object is used.
    WireResult Deserialize(DeserializeBuffer* deserializeBuffer, DeserializeAllocator* allocator);

    ObjectId selfId;

    WGPUComputePassEncoder self;
};

    struct ComputePassEncoderInsertDebugMarkerCmd {
    size_t GetRequiredSize() const;

    WireResult Serialize(size_t commandSize, SerializeBuffer* serializeBuffer, const ObjectIdProvider& objectIdProvider) const;
    // Override which produces a FatalError if any object is used.
    WireResult Serialize(size_t commandSize, SerializeBuffer* serializeBuffer) const;

    WireResult Deserialize(DeserializeBuffer* deserializeBuffer, DeserializeAllocator* allocator, const ObjectIdResolver& resolver);
    // Override which produces a FatalError if any object is used.
    WireResult Deserialize(DeserializeBuffer* deserializeBuffer, DeserializeAllocator* allocator);

    ObjectId selfId;

    WGPUComputePassEncoder self;
    WGPUStringView markerLabel;
};

    struct ComputePassEncoderPopDebugGroupCmd {
    size_t GetRequiredSize() const;

    WireResult Serialize(size_t commandSize, SerializeBuffer* serializeBuffer, const ObjectIdProvider& objectIdProvider) const;
    // Override which produces a FatalError if any object is used.
    WireResult Serialize(size_t commandSize, SerializeBuffer* serializeBuffer) const;

    WireResult Deserialize(DeserializeBuffer* deserializeBuffer, DeserializeAllocator* allocator, const ObjectIdResolver& resolver);
    // Override which produces a FatalError if any object is used.
    WireResult Deserialize(DeserializeBuffer* deserializeBuffer, DeserializeAllocator* allocator);

    ObjectId selfId;

    WGPUComputePassEncoder self;
};

    struct ComputePassEncoderPushDebugGroupCmd {
    size_t GetRequiredSize() const;

    WireResult Serialize(size_t commandSize, SerializeBuffer* serializeBuffer, const ObjectIdProvider& objectIdProvider) const;
    // Override which produces a FatalError if any object is used.
    WireResult Serialize(size_t commandSize, SerializeBuffer* serializeBuffer) const;

    WireResult Deserialize(DeserializeBuffer* deserializeBuffer, DeserializeAllocator* allocator, const ObjectIdResolver& resolver);
    // Override which produces a FatalError if any object is used.
    WireResult Deserialize(DeserializeBuffer* deserializeBuffer, DeserializeAllocator* allocator);

    ObjectId selfId;

    WGPUComputePassEncoder self;
    WGPUStringView groupLabel;
};

    struct ComputePassEncoderSetBindGroupCmd {
    size_t GetRequiredSize() const;

    WireResult Serialize(size_t commandSize, SerializeBuffer* serializeBuffer, const ObjectIdProvider& objectIdProvider) const;
    // Override which produces a FatalError if any object is used.
    WireResult Serialize(size_t commandSize, SerializeBuffer* serializeBuffer) const;

    WireResult Deserialize(DeserializeBuffer* deserializeBuffer, DeserializeAllocator* allocator, const ObjectIdResolver& resolver);
    // Override which produces a FatalError if any object is used.
    WireResult Deserialize(DeserializeBuffer* deserializeBuffer, DeserializeAllocator* allocator);

    ObjectId selfId;

    WGPUComputePassEncoder self;
    uint32_t groupIndex;
    WGPUBindGroup group;
    size_t dynamicOffsetCount;
    uint32_t const * dynamicOffsets;
};

    struct ComputePassEncoderSetImmediatesCmd {
    size_t GetRequiredSize() const;

    WireResult Serialize(size_t commandSize, SerializeBuffer* serializeBuffer, const ObjectIdProvider& objectIdProvider) const;
    // Override which produces a FatalError if any object is used.
    WireResult Serialize(size_t commandSize, SerializeBuffer* serializeBuffer) const;

    WireResult Deserialize(DeserializeBuffer* deserializeBuffer, DeserializeAllocator* allocator, const ObjectIdResolver& resolver);
    // Override which produces a FatalError if any object is used.
    WireResult Deserialize(DeserializeBuffer* deserializeBuffer, DeserializeAllocator* allocator);


    ObjectId computePassEncoderId;
    uint32_t offset;
    uint8_t const * data;
    size_t size;
};

    struct ComputePassEncoderSetLabelCmd {
    size_t GetRequiredSize() const;

    WireResult Serialize(size_t commandSize, SerializeBuffer* serializeBuffer, const ObjectIdProvider& objectIdProvider) const;
    // Override which produces a FatalError if any object is used.
    WireResult Serialize(size_t commandSize, SerializeBuffer* serializeBuffer) const;

    WireResult Deserialize(DeserializeBuffer* deserializeBuffer, DeserializeAllocator* allocator, const ObjectIdResolver& resolver);
    // Override which produces a FatalError if any object is used.
    WireResult Deserialize(DeserializeBuffer* deserializeBuffer, DeserializeAllocator* allocator);

    ObjectId selfId;

    WGPUComputePassEncoder self;
    WGPUStringView label;
};

    struct ComputePassEncoderSetPipelineCmd {
    size_t GetRequiredSize() const;

    WireResult Serialize(size_t commandSize, SerializeBuffer* serializeBuffer, const ObjectIdProvider& objectIdProvider) const;
    // Override which produces a FatalError if any object is used.
    WireResult Serialize(size_t commandSize, SerializeBuffer* serializeBuffer) const;

    WireResult Deserialize(DeserializeBuffer* deserializeBuffer, DeserializeAllocator* allocator, const ObjectIdResolver& resolver);
    // Override which produces a FatalError if any object is used.
    WireResult Deserialize(DeserializeBuffer* deserializeBuffer, DeserializeAllocator* allocator);

    ObjectId selfId;

    WGPUComputePassEncoder self;
    WGPUComputePipeline pipeline;
};

    struct ComputePassEncoderSetResourceTableCmd {
    size_t GetRequiredSize() const;

    WireResult Serialize(size_t commandSize, SerializeBuffer* serializeBuffer, const ObjectIdProvider& objectIdProvider) const;
    // Override which produces a FatalError if any object is used.
    WireResult Serialize(size_t commandSize, SerializeBuffer* serializeBuffer) const;

    WireResult Deserialize(DeserializeBuffer* deserializeBuffer, DeserializeAllocator* allocator, const ObjectIdResolver& resolver);
    // Override which produces a FatalError if any object is used.
    WireResult Deserialize(DeserializeBuffer* deserializeBuffer, DeserializeAllocator* allocator);

    ObjectId selfId;

    WGPUComputePassEncoder self;
    WGPUResourceTable table;
};

    struct ComputePassEncoderWriteTimestampCmd {
    size_t GetRequiredSize() const;

    WireResult Serialize(size_t commandSize, SerializeBuffer* serializeBuffer, const ObjectIdProvider& objectIdProvider) const;
    // Override which produces a FatalError if any object is used.
    WireResult Serialize(size_t commandSize, SerializeBuffer* serializeBuffer) const;

    WireResult Deserialize(DeserializeBuffer* deserializeBuffer, DeserializeAllocator* allocator, const ObjectIdResolver& resolver);
    // Override which produces a FatalError if any object is used.
    WireResult Deserialize(DeserializeBuffer* deserializeBuffer, DeserializeAllocator* allocator);

    ObjectId selfId;

    WGPUComputePassEncoder self;
    WGPUQuerySet querySet;
    uint32_t queryIndex;
};

    struct ComputePipelineGetBindGroupLayoutCmd {
    size_t GetRequiredSize() const;

    WireResult Serialize(size_t commandSize, SerializeBuffer* serializeBuffer, const ObjectIdProvider& objectIdProvider) const;
    // Override which produces a FatalError if any object is used.
    WireResult Serialize(size_t commandSize, SerializeBuffer* serializeBuffer) const;

    WireResult Deserialize(DeserializeBuffer* deserializeBuffer, DeserializeAllocator* allocator, const ObjectIdResolver& resolver);
    // Override which produces a FatalError if any object is used.
    WireResult Deserialize(DeserializeBuffer* deserializeBuffer, DeserializeAllocator* allocator);

    ObjectId selfId;

    WGPUComputePipeline self;
    uint32_t groupIndex;
    ObjectHandle result;
};

    struct ComputePipelineSetLabelCmd {
    size_t GetRequiredSize() const;

    WireResult Serialize(size_t commandSize, SerializeBuffer* serializeBuffer, const ObjectIdProvider& objectIdProvider) const;
    // Override which produces a FatalError if any object is used.
    WireResult Serialize(size_t commandSize, SerializeBuffer* serializeBuffer) const;

    WireResult Deserialize(DeserializeBuffer* deserializeBuffer, DeserializeAllocator* allocator, const ObjectIdResolver& resolver);
    // Override which produces a FatalError if any object is used.
    WireResult Deserialize(DeserializeBuffer* deserializeBuffer, DeserializeAllocator* allocator);

    ObjectId selfId;

    WGPUComputePipeline self;
    WGPUStringView label;
};

    struct DeviceCreateBindGroupCmd {
    size_t GetRequiredSize() const;

    WireResult Serialize(size_t commandSize, SerializeBuffer* serializeBuffer, const ObjectIdProvider& objectIdProvider) const;
    // Override which produces a FatalError if any object is used.
    WireResult Serialize(size_t commandSize, SerializeBuffer* serializeBuffer) const;

    WireResult Deserialize(DeserializeBuffer* deserializeBuffer, DeserializeAllocator* allocator, const ObjectIdResolver& resolver);
    // Override which produces a FatalError if any object is used.
    WireResult Deserialize(DeserializeBuffer* deserializeBuffer, DeserializeAllocator* allocator);

    ObjectId selfId;

    WGPUDevice self;
    WGPUBindGroupDescriptor const * descriptor;
    ObjectHandle result;
};

    struct DeviceCreateBindGroupLayoutCmd {
    size_t GetRequiredSize() const;

    WireResult Serialize(size_t commandSize, SerializeBuffer* serializeBuffer, const ObjectIdProvider& objectIdProvider) const;
    // Override which produces a FatalError if any object is used.
    WireResult Serialize(size_t commandSize, SerializeBuffer* serializeBuffer) const;

    WireResult Deserialize(DeserializeBuffer* deserializeBuffer, DeserializeAllocator* allocator, const ObjectIdResolver& resolver);
    // Override which produces a FatalError if any object is used.
    WireResult Deserialize(DeserializeBuffer* deserializeBuffer, DeserializeAllocator* allocator);

    ObjectId selfId;

    WGPUDevice self;
    WGPUBindGroupLayoutDescriptor const * descriptor;
    ObjectHandle result;
};

    struct DeviceCreateBufferCmd {
    size_t GetRequiredSize() const;

    WireResult Serialize(size_t commandSize, SerializeBuffer* serializeBuffer, const ObjectIdProvider& objectIdProvider) const;
    // Override which produces a FatalError if any object is used.
    WireResult Serialize(size_t commandSize, SerializeBuffer* serializeBuffer) const;

    WireResult Deserialize(DeserializeBuffer* deserializeBuffer, DeserializeAllocator* allocator, const ObjectIdResolver& resolver);
    // Override which produces a FatalError if any object is used.
    WireResult Deserialize(DeserializeBuffer* deserializeBuffer, DeserializeAllocator* allocator);


    ObjectId deviceId;
    WGPUBufferDescriptor const * descriptor;
    ObjectHandle result;
    size_t memoryHandleCreateInfoLength;
    std::byte const * memoryHandleCreateInfo;
};

    struct DeviceCreateCommandEncoderCmd {
    size_t GetRequiredSize() const;

    WireResult Serialize(size_t commandSize, SerializeBuffer* serializeBuffer, const ObjectIdProvider& objectIdProvider) const;
    // Override which produces a FatalError if any object is used.
    WireResult Serialize(size_t commandSize, SerializeBuffer* serializeBuffer) const;

    WireResult Deserialize(DeserializeBuffer* deserializeBuffer, DeserializeAllocator* allocator, const ObjectIdResolver& resolver);
    // Override which produces a FatalError if any object is used.
    WireResult Deserialize(DeserializeBuffer* deserializeBuffer, DeserializeAllocator* allocator);

    ObjectId selfId;

    WGPUDevice self;
    WGPUCommandEncoderDescriptor const * descriptor;
    ObjectHandle result;
};

    struct DeviceCreateComputePipelineCmd {
    size_t GetRequiredSize() const;

    WireResult Serialize(size_t commandSize, SerializeBuffer* serializeBuffer, const ObjectIdProvider& objectIdProvider) const;
    // Override which produces a FatalError if any object is used.
    WireResult Serialize(size_t commandSize, SerializeBuffer* serializeBuffer) const;

    WireResult Deserialize(DeserializeBuffer* deserializeBuffer, DeserializeAllocator* allocator, const ObjectIdResolver& resolver);
    // Override which produces a FatalError if any object is used.
    WireResult Deserialize(DeserializeBuffer* deserializeBuffer, DeserializeAllocator* allocator);

    ObjectId selfId;

    WGPUDevice self;
    WGPUComputePipelineDescriptor const * descriptor;
    ObjectHandle result;
};

    struct DeviceCreateComputePipelineAsyncCmd {
    size_t GetRequiredSize() const;

    WireResult Serialize(size_t commandSize, SerializeBuffer* serializeBuffer, const ObjectIdProvider& objectIdProvider) const;
    // Override which produces a FatalError if any object is used.
    WireResult Serialize(size_t commandSize, SerializeBuffer* serializeBuffer) const;

    WireResult Deserialize(DeserializeBuffer* deserializeBuffer, DeserializeAllocator* allocator, const ObjectIdResolver& resolver);
    // Override which produces a FatalError if any object is used.
    WireResult Deserialize(DeserializeBuffer* deserializeBuffer, DeserializeAllocator* allocator);


    ObjectId deviceId;
    ObjectId instanceId;
    WGPUFuture future;
    ObjectHandle pipelineObjectHandle;
    WGPUComputePipelineDescriptor const * descriptor;
};

    struct DeviceCreateErrorBufferCmd {
    size_t GetRequiredSize() const;

    WireResult Serialize(size_t commandSize, SerializeBuffer* serializeBuffer, const ObjectIdProvider& objectIdProvider) const;
    // Override which produces a FatalError if any object is used.
    WireResult Serialize(size_t commandSize, SerializeBuffer* serializeBuffer) const;

    WireResult Deserialize(DeserializeBuffer* deserializeBuffer, DeserializeAllocator* allocator, const ObjectIdResolver& resolver);
    // Override which produces a FatalError if any object is used.
    WireResult Deserialize(DeserializeBuffer* deserializeBuffer, DeserializeAllocator* allocator);

    ObjectId selfId;

    WGPUDevice self;
    WGPUBufferDescriptor const * descriptor;
    ObjectHandle result;
};

    struct DeviceCreateErrorComputePipelineCmd {
    size_t GetRequiredSize() const;

    WireResult Serialize(size_t commandSize, SerializeBuffer* serializeBuffer, const ObjectIdProvider& objectIdProvider) const;
    // Override which produces a FatalError if any object is used.
    WireResult Serialize(size_t commandSize, SerializeBuffer* serializeBuffer) const;

    WireResult Deserialize(DeserializeBuffer* deserializeBuffer, DeserializeAllocator* allocator, const ObjectIdResolver& resolver);
    // Override which produces a FatalError if any object is used.
    WireResult Deserialize(DeserializeBuffer* deserializeBuffer, DeserializeAllocator* allocator);

    ObjectId selfId;

    WGPUDevice self;
    WGPUStringView label;
    ObjectHandle result;
};

    struct DeviceCreateErrorExternalTextureCmd {
    size_t GetRequiredSize() const;

    WireResult Serialize(size_t commandSize, SerializeBuffer* serializeBuffer, const ObjectIdProvider& objectIdProvider) const;
    // Override which produces a FatalError if any object is used.
    WireResult Serialize(size_t commandSize, SerializeBuffer* serializeBuffer) const;

    WireResult Deserialize(DeserializeBuffer* deserializeBuffer, DeserializeAllocator* allocator, const ObjectIdResolver& resolver);
    // Override which produces a FatalError if any object is used.
    WireResult Deserialize(DeserializeBuffer* deserializeBuffer, DeserializeAllocator* allocator);

    ObjectId selfId;

    WGPUDevice self;
    ObjectHandle result;
};

    struct DeviceCreateErrorRenderPipelineCmd {
    size_t GetRequiredSize() const;

    WireResult Serialize(size_t commandSize, SerializeBuffer* serializeBuffer, const ObjectIdProvider& objectIdProvider) const;
    // Override which produces a FatalError if any object is used.
    WireResult Serialize(size_t commandSize, SerializeBuffer* serializeBuffer) const;

    WireResult Deserialize(DeserializeBuffer* deserializeBuffer, DeserializeAllocator* allocator, const ObjectIdResolver& resolver);
    // Override which produces a FatalError if any object is used.
    WireResult Deserialize(DeserializeBuffer* deserializeBuffer, DeserializeAllocator* allocator);

    ObjectId selfId;

    WGPUDevice self;
    WGPUStringView label;
    ObjectHandle result;
};

    struct DeviceCreateErrorShaderModuleCmd {
    size_t GetRequiredSize() const;

    WireResult Serialize(size_t commandSize, SerializeBuffer* serializeBuffer, const ObjectIdProvider& objectIdProvider) const;
    // Override which produces a FatalError if any object is used.
    WireResult Serialize(size_t commandSize, SerializeBuffer* serializeBuffer) const;

    WireResult Deserialize(DeserializeBuffer* deserializeBuffer, DeserializeAllocator* allocator, const ObjectIdResolver& resolver);
    // Override which produces a FatalError if any object is used.
    WireResult Deserialize(DeserializeBuffer* deserializeBuffer, DeserializeAllocator* allocator);

    ObjectId selfId;

    WGPUDevice self;
    WGPUShaderModuleDescriptor const * descriptor;
    WGPUStringView errorMessage;
    ObjectHandle result;
};

    struct DeviceCreateErrorTextureCmd {
    size_t GetRequiredSize() const;

    WireResult Serialize(size_t commandSize, SerializeBuffer* serializeBuffer, const ObjectIdProvider& objectIdProvider) const;
    // Override which produces a FatalError if any object is used.
    WireResult Serialize(size_t commandSize, SerializeBuffer* serializeBuffer) const;

    WireResult Deserialize(DeserializeBuffer* deserializeBuffer, DeserializeAllocator* allocator, const ObjectIdResolver& resolver);
    // Override which produces a FatalError if any object is used.
    WireResult Deserialize(DeserializeBuffer* deserializeBuffer, DeserializeAllocator* allocator);

    ObjectId selfId;

    WGPUDevice self;
    WGPUTextureDescriptor const * descriptor;
    ObjectHandle result;
};

    struct DeviceCreateExternalTextureCmd {
    size_t GetRequiredSize() const;

    WireResult Serialize(size_t commandSize, SerializeBuffer* serializeBuffer, const ObjectIdProvider& objectIdProvider) const;
    // Override which produces a FatalError if any object is used.
    WireResult Serialize(size_t commandSize, SerializeBuffer* serializeBuffer) const;

    WireResult Deserialize(DeserializeBuffer* deserializeBuffer, DeserializeAllocator* allocator, const ObjectIdResolver& resolver);
    // Override which produces a FatalError if any object is used.
    WireResult Deserialize(DeserializeBuffer* deserializeBuffer, DeserializeAllocator* allocator);

    ObjectId selfId;

    WGPUDevice self;
    WGPUExternalTextureDescriptor const * externalTextureDescriptor;
    ObjectHandle result;
};

    struct DeviceCreatePipelineLayoutCmd {
    size_t GetRequiredSize() const;

    WireResult Serialize(size_t commandSize, SerializeBuffer* serializeBuffer, const ObjectIdProvider& objectIdProvider) const;
    // Override which produces a FatalError if any object is used.
    WireResult Serialize(size_t commandSize, SerializeBuffer* serializeBuffer) const;

    WireResult Deserialize(DeserializeBuffer* deserializeBuffer, DeserializeAllocator* allocator, const ObjectIdResolver& resolver);
    // Override which produces a FatalError if any object is used.
    WireResult Deserialize(DeserializeBuffer* deserializeBuffer, DeserializeAllocator* allocator);

    ObjectId selfId;

    WGPUDevice self;
    WGPUPipelineLayoutDescriptor const * descriptor;
    ObjectHandle result;
};

    struct DeviceCreateQuerySetCmd {
    size_t GetRequiredSize() const;

    WireResult Serialize(size_t commandSize, SerializeBuffer* serializeBuffer, const ObjectIdProvider& objectIdProvider) const;
    // Override which produces a FatalError if any object is used.
    WireResult Serialize(size_t commandSize, SerializeBuffer* serializeBuffer) const;

    WireResult Deserialize(DeserializeBuffer* deserializeBuffer, DeserializeAllocator* allocator, const ObjectIdResolver& resolver);
    // Override which produces a FatalError if any object is used.
    WireResult Deserialize(DeserializeBuffer* deserializeBuffer, DeserializeAllocator* allocator);

    ObjectId selfId;

    WGPUDevice self;
    WGPUQuerySetDescriptor const * descriptor;
    ObjectHandle result;
};

    struct DeviceCreateRenderBundleEncoderCmd {
    size_t GetRequiredSize() const;

    WireResult Serialize(size_t commandSize, SerializeBuffer* serializeBuffer, const ObjectIdProvider& objectIdProvider) const;
    // Override which produces a FatalError if any object is used.
    WireResult Serialize(size_t commandSize, SerializeBuffer* serializeBuffer) const;

    WireResult Deserialize(DeserializeBuffer* deserializeBuffer, DeserializeAllocator* allocator, const ObjectIdResolver& resolver);
    // Override which produces a FatalError if any object is used.
    WireResult Deserialize(DeserializeBuffer* deserializeBuffer, DeserializeAllocator* allocator);

    ObjectId selfId;

    WGPUDevice self;
    WGPURenderBundleEncoderDescriptor const * descriptor;
    ObjectHandle result;
};

    struct DeviceCreateRenderPipelineCmd {
    size_t GetRequiredSize() const;

    WireResult Serialize(size_t commandSize, SerializeBuffer* serializeBuffer, const ObjectIdProvider& objectIdProvider) const;
    // Override which produces a FatalError if any object is used.
    WireResult Serialize(size_t commandSize, SerializeBuffer* serializeBuffer) const;

    WireResult Deserialize(DeserializeBuffer* deserializeBuffer, DeserializeAllocator* allocator, const ObjectIdResolver& resolver);
    // Override which produces a FatalError if any object is used.
    WireResult Deserialize(DeserializeBuffer* deserializeBuffer, DeserializeAllocator* allocator);

    ObjectId selfId;

    WGPUDevice self;
    WGPURenderPipelineDescriptor const * descriptor;
    ObjectHandle result;
};

    struct DeviceCreateRenderPipelineAsyncCmd {
    size_t GetRequiredSize() const;

    WireResult Serialize(size_t commandSize, SerializeBuffer* serializeBuffer, const ObjectIdProvider& objectIdProvider) const;
    // Override which produces a FatalError if any object is used.
    WireResult Serialize(size_t commandSize, SerializeBuffer* serializeBuffer) const;

    WireResult Deserialize(DeserializeBuffer* deserializeBuffer, DeserializeAllocator* allocator, const ObjectIdResolver& resolver);
    // Override which produces a FatalError if any object is used.
    WireResult Deserialize(DeserializeBuffer* deserializeBuffer, DeserializeAllocator* allocator);


    ObjectId deviceId;
    ObjectId instanceId;
    WGPUFuture future;
    ObjectHandle pipelineObjectHandle;
    WGPURenderPipelineDescriptor const * descriptor;
};

    struct DeviceCreateResourceTableCmd {
    size_t GetRequiredSize() const;

    WireResult Serialize(size_t commandSize, SerializeBuffer* serializeBuffer, const ObjectIdProvider& objectIdProvider) const;
    // Override which produces a FatalError if any object is used.
    WireResult Serialize(size_t commandSize, SerializeBuffer* serializeBuffer) const;

    WireResult Deserialize(DeserializeBuffer* deserializeBuffer, DeserializeAllocator* allocator, const ObjectIdResolver& resolver);
    // Override which produces a FatalError if any object is used.
    WireResult Deserialize(DeserializeBuffer* deserializeBuffer, DeserializeAllocator* allocator);

    ObjectId selfId;

    WGPUDevice self;
    WGPUResourceTableDescriptor const * descriptor;
    ObjectHandle result;
};

    struct DeviceCreateSamplerCmd {
    size_t GetRequiredSize() const;

    WireResult Serialize(size_t commandSize, SerializeBuffer* serializeBuffer, const ObjectIdProvider& objectIdProvider) const;
    // Override which produces a FatalError if any object is used.
    WireResult Serialize(size_t commandSize, SerializeBuffer* serializeBuffer) const;

    WireResult Deserialize(DeserializeBuffer* deserializeBuffer, DeserializeAllocator* allocator, const ObjectIdResolver& resolver);
    // Override which produces a FatalError if any object is used.
    WireResult Deserialize(DeserializeBuffer* deserializeBuffer, DeserializeAllocator* allocator);

    ObjectId selfId;

    WGPUDevice self;
    WGPUSamplerDescriptor const * descriptor;
    ObjectHandle result;
};

    struct DeviceCreateShaderModuleCmd {
    size_t GetRequiredSize() const;

    WireResult Serialize(size_t commandSize, SerializeBuffer* serializeBuffer, const ObjectIdProvider& objectIdProvider) const;
    // Override which produces a FatalError if any object is used.
    WireResult Serialize(size_t commandSize, SerializeBuffer* serializeBuffer) const;

    WireResult Deserialize(DeserializeBuffer* deserializeBuffer, DeserializeAllocator* allocator, const ObjectIdResolver& resolver);
    // Override which produces a FatalError if any object is used.
    WireResult Deserialize(DeserializeBuffer* deserializeBuffer, DeserializeAllocator* allocator);

    ObjectId selfId;

    WGPUDevice self;
    WGPUShaderModuleDescriptor const * descriptor;
    ObjectHandle result;
};

    struct DeviceCreateTextureCmd {
    size_t GetRequiredSize() const;

    WireResult Serialize(size_t commandSize, SerializeBuffer* serializeBuffer, const ObjectIdProvider& objectIdProvider) const;
    // Override which produces a FatalError if any object is used.
    WireResult Serialize(size_t commandSize, SerializeBuffer* serializeBuffer) const;

    WireResult Deserialize(DeserializeBuffer* deserializeBuffer, DeserializeAllocator* allocator, const ObjectIdResolver& resolver);
    // Override which produces a FatalError if any object is used.
    WireResult Deserialize(DeserializeBuffer* deserializeBuffer, DeserializeAllocator* allocator);

    ObjectId selfId;

    WGPUDevice self;
    WGPUTextureDescriptor const * descriptor;
    ObjectHandle result;
};

    struct DeviceDestroyCmd {
    size_t GetRequiredSize() const;

    WireResult Serialize(size_t commandSize, SerializeBuffer* serializeBuffer, const ObjectIdProvider& objectIdProvider) const;
    // Override which produces a FatalError if any object is used.
    WireResult Serialize(size_t commandSize, SerializeBuffer* serializeBuffer) const;

    WireResult Deserialize(DeserializeBuffer* deserializeBuffer, DeserializeAllocator* allocator, const ObjectIdResolver& resolver);
    // Override which produces a FatalError if any object is used.
    WireResult Deserialize(DeserializeBuffer* deserializeBuffer, DeserializeAllocator* allocator);

    ObjectId selfId;

    WGPUDevice self;
};

    struct DeviceForceLossCmd {
    size_t GetRequiredSize() const;

    WireResult Serialize(size_t commandSize, SerializeBuffer* serializeBuffer, const ObjectIdProvider& objectIdProvider) const;
    // Override which produces a FatalError if any object is used.
    WireResult Serialize(size_t commandSize, SerializeBuffer* serializeBuffer) const;

    WireResult Deserialize(DeserializeBuffer* deserializeBuffer, DeserializeAllocator* allocator, const ObjectIdResolver& resolver);
    // Override which produces a FatalError if any object is used.
    WireResult Deserialize(DeserializeBuffer* deserializeBuffer, DeserializeAllocator* allocator);

    ObjectId selfId;

    WGPUDevice self;
    WGPUDeviceLostReason type;
    WGPUStringView message;
};

    struct DeviceGetQueueCmd {
    size_t GetRequiredSize() const;

    WireResult Serialize(size_t commandSize, SerializeBuffer* serializeBuffer, const ObjectIdProvider& objectIdProvider) const;
    // Override which produces a FatalError if any object is used.
    WireResult Serialize(size_t commandSize, SerializeBuffer* serializeBuffer) const;

    WireResult Deserialize(DeserializeBuffer* deserializeBuffer, DeserializeAllocator* allocator, const ObjectIdResolver& resolver);
    // Override which produces a FatalError if any object is used.
    WireResult Deserialize(DeserializeBuffer* deserializeBuffer, DeserializeAllocator* allocator);

    ObjectId selfId;

    WGPUDevice self;
    ObjectHandle result;
};

    struct DeviceInjectErrorCmd {
    size_t GetRequiredSize() const;

    WireResult Serialize(size_t commandSize, SerializeBuffer* serializeBuffer, const ObjectIdProvider& objectIdProvider) const;
    // Override which produces a FatalError if any object is used.
    WireResult Serialize(size_t commandSize, SerializeBuffer* serializeBuffer) const;

    WireResult Deserialize(DeserializeBuffer* deserializeBuffer, DeserializeAllocator* allocator, const ObjectIdResolver& resolver);
    // Override which produces a FatalError if any object is used.
    WireResult Deserialize(DeserializeBuffer* deserializeBuffer, DeserializeAllocator* allocator);

    ObjectId selfId;

    WGPUDevice self;
    WGPUErrorType type;
    WGPUStringView message;
};

    struct DevicePopErrorScopeCmd {
    size_t GetRequiredSize() const;

    WireResult Serialize(size_t commandSize, SerializeBuffer* serializeBuffer, const ObjectIdProvider& objectIdProvider) const;
    // Override which produces a FatalError if any object is used.
    WireResult Serialize(size_t commandSize, SerializeBuffer* serializeBuffer) const;

    WireResult Deserialize(DeserializeBuffer* deserializeBuffer, DeserializeAllocator* allocator, const ObjectIdResolver& resolver);
    // Override which produces a FatalError if any object is used.
    WireResult Deserialize(DeserializeBuffer* deserializeBuffer, DeserializeAllocator* allocator);


    ObjectId deviceId;
    ObjectId instanceId;
    WGPUFuture future;
};

    struct DevicePushErrorScopeCmd {
    size_t GetRequiredSize() const;

    WireResult Serialize(size_t commandSize, SerializeBuffer* serializeBuffer, const ObjectIdProvider& objectIdProvider) const;
    // Override which produces a FatalError if any object is used.
    WireResult Serialize(size_t commandSize, SerializeBuffer* serializeBuffer) const;

    WireResult Deserialize(DeserializeBuffer* deserializeBuffer, DeserializeAllocator* allocator, const ObjectIdResolver& resolver);
    // Override which produces a FatalError if any object is used.
    WireResult Deserialize(DeserializeBuffer* deserializeBuffer, DeserializeAllocator* allocator);

    ObjectId selfId;

    WGPUDevice self;
    WGPUErrorFilter filter;
};

    struct DeviceSetLabelCmd {
    size_t GetRequiredSize() const;

    WireResult Serialize(size_t commandSize, SerializeBuffer* serializeBuffer, const ObjectIdProvider& objectIdProvider) const;
    // Override which produces a FatalError if any object is used.
    WireResult Serialize(size_t commandSize, SerializeBuffer* serializeBuffer) const;

    WireResult Deserialize(DeserializeBuffer* deserializeBuffer, DeserializeAllocator* allocator, const ObjectIdResolver& resolver);
    // Override which produces a FatalError if any object is used.
    WireResult Deserialize(DeserializeBuffer* deserializeBuffer, DeserializeAllocator* allocator);

    ObjectId selfId;

    WGPUDevice self;
    WGPUStringView label;
};

    struct DeviceTickCmd {
    size_t GetRequiredSize() const;

    WireResult Serialize(size_t commandSize, SerializeBuffer* serializeBuffer, const ObjectIdProvider& objectIdProvider) const;
    // Override which produces a FatalError if any object is used.
    WireResult Serialize(size_t commandSize, SerializeBuffer* serializeBuffer) const;

    WireResult Deserialize(DeserializeBuffer* deserializeBuffer, DeserializeAllocator* allocator, const ObjectIdResolver& resolver);
    // Override which produces a FatalError if any object is used.
    WireResult Deserialize(DeserializeBuffer* deserializeBuffer, DeserializeAllocator* allocator);

    ObjectId selfId;

    WGPUDevice self;
};

    struct DeviceValidateTextureDescriptorCmd {
    size_t GetRequiredSize() const;

    WireResult Serialize(size_t commandSize, SerializeBuffer* serializeBuffer, const ObjectIdProvider& objectIdProvider) const;
    // Override which produces a FatalError if any object is used.
    WireResult Serialize(size_t commandSize, SerializeBuffer* serializeBuffer) const;

    WireResult Deserialize(DeserializeBuffer* deserializeBuffer, DeserializeAllocator* allocator, const ObjectIdResolver& resolver);
    // Override which produces a FatalError if any object is used.
    WireResult Deserialize(DeserializeBuffer* deserializeBuffer, DeserializeAllocator* allocator);

    ObjectId selfId;

    WGPUDevice self;
    WGPUTextureDescriptor const * descriptor;
};

    struct ExternalTextureDestroyCmd {
    size_t GetRequiredSize() const;

    WireResult Serialize(size_t commandSize, SerializeBuffer* serializeBuffer, const ObjectIdProvider& objectIdProvider) const;
    // Override which produces a FatalError if any object is used.
    WireResult Serialize(size_t commandSize, SerializeBuffer* serializeBuffer) const;

    WireResult Deserialize(DeserializeBuffer* deserializeBuffer, DeserializeAllocator* allocator, const ObjectIdResolver& resolver);
    // Override which produces a FatalError if any object is used.
    WireResult Deserialize(DeserializeBuffer* deserializeBuffer, DeserializeAllocator* allocator);

    ObjectId selfId;

    WGPUExternalTexture self;
};

    struct ExternalTextureExpireCmd {
    size_t GetRequiredSize() const;

    WireResult Serialize(size_t commandSize, SerializeBuffer* serializeBuffer, const ObjectIdProvider& objectIdProvider) const;
    // Override which produces a FatalError if any object is used.
    WireResult Serialize(size_t commandSize, SerializeBuffer* serializeBuffer) const;

    WireResult Deserialize(DeserializeBuffer* deserializeBuffer, DeserializeAllocator* allocator, const ObjectIdResolver& resolver);
    // Override which produces a FatalError if any object is used.
    WireResult Deserialize(DeserializeBuffer* deserializeBuffer, DeserializeAllocator* allocator);

    ObjectId selfId;

    WGPUExternalTexture self;
};

    struct ExternalTextureRefreshCmd {
    size_t GetRequiredSize() const;

    WireResult Serialize(size_t commandSize, SerializeBuffer* serializeBuffer, const ObjectIdProvider& objectIdProvider) const;
    // Override which produces a FatalError if any object is used.
    WireResult Serialize(size_t commandSize, SerializeBuffer* serializeBuffer) const;

    WireResult Deserialize(DeserializeBuffer* deserializeBuffer, DeserializeAllocator* allocator, const ObjectIdResolver& resolver);
    // Override which produces a FatalError if any object is used.
    WireResult Deserialize(DeserializeBuffer* deserializeBuffer, DeserializeAllocator* allocator);

    ObjectId selfId;

    WGPUExternalTexture self;
};

    struct ExternalTextureSetLabelCmd {
    size_t GetRequiredSize() const;

    WireResult Serialize(size_t commandSize, SerializeBuffer* serializeBuffer, const ObjectIdProvider& objectIdProvider) const;
    // Override which produces a FatalError if any object is used.
    WireResult Serialize(size_t commandSize, SerializeBuffer* serializeBuffer) const;

    WireResult Deserialize(DeserializeBuffer* deserializeBuffer, DeserializeAllocator* allocator, const ObjectIdResolver& resolver);
    // Override which produces a FatalError if any object is used.
    WireResult Deserialize(DeserializeBuffer* deserializeBuffer, DeserializeAllocator* allocator);

    ObjectId selfId;

    WGPUExternalTexture self;
    WGPUStringView label;
};

    struct InstanceProcessEventsCmd {
    size_t GetRequiredSize() const;

    WireResult Serialize(size_t commandSize, SerializeBuffer* serializeBuffer, const ObjectIdProvider& objectIdProvider) const;
    // Override which produces a FatalError if any object is used.
    WireResult Serialize(size_t commandSize, SerializeBuffer* serializeBuffer) const;

    WireResult Deserialize(DeserializeBuffer* deserializeBuffer, DeserializeAllocator* allocator, const ObjectIdResolver& resolver);
    // Override which produces a FatalError if any object is used.
    WireResult Deserialize(DeserializeBuffer* deserializeBuffer, DeserializeAllocator* allocator);

    ObjectId selfId;

    WGPUInstance self;
};

    struct InstanceRequestAdapterCmd {
    size_t GetRequiredSize() const;

    WireResult Serialize(size_t commandSize, SerializeBuffer* serializeBuffer, const ObjectIdProvider& objectIdProvider) const;
    // Override which produces a FatalError if any object is used.
    WireResult Serialize(size_t commandSize, SerializeBuffer* serializeBuffer) const;

    WireResult Deserialize(DeserializeBuffer* deserializeBuffer, DeserializeAllocator* allocator, const ObjectIdResolver& resolver);
    // Override which produces a FatalError if any object is used.
    WireResult Deserialize(DeserializeBuffer* deserializeBuffer, DeserializeAllocator* allocator);


    ObjectId instanceId;
    WGPUFuture future;
    ObjectHandle adapterObjectHandle;
    WGPURequestAdapterOptions const * options;
};

    struct PipelineLayoutSetLabelCmd {
    size_t GetRequiredSize() const;

    WireResult Serialize(size_t commandSize, SerializeBuffer* serializeBuffer, const ObjectIdProvider& objectIdProvider) const;
    // Override which produces a FatalError if any object is used.
    WireResult Serialize(size_t commandSize, SerializeBuffer* serializeBuffer) const;

    WireResult Deserialize(DeserializeBuffer* deserializeBuffer, DeserializeAllocator* allocator, const ObjectIdResolver& resolver);
    // Override which produces a FatalError if any object is used.
    WireResult Deserialize(DeserializeBuffer* deserializeBuffer, DeserializeAllocator* allocator);

    ObjectId selfId;

    WGPUPipelineLayout self;
    WGPUStringView label;
};

    struct QuerySetDestroyCmd {
    size_t GetRequiredSize() const;

    WireResult Serialize(size_t commandSize, SerializeBuffer* serializeBuffer, const ObjectIdProvider& objectIdProvider) const;
    // Override which produces a FatalError if any object is used.
    WireResult Serialize(size_t commandSize, SerializeBuffer* serializeBuffer) const;

    WireResult Deserialize(DeserializeBuffer* deserializeBuffer, DeserializeAllocator* allocator, const ObjectIdResolver& resolver);
    // Override which produces a FatalError if any object is used.
    WireResult Deserialize(DeserializeBuffer* deserializeBuffer, DeserializeAllocator* allocator);

    ObjectId selfId;

    WGPUQuerySet self;
};

    struct QuerySetSetLabelCmd {
    size_t GetRequiredSize() const;

    WireResult Serialize(size_t commandSize, SerializeBuffer* serializeBuffer, const ObjectIdProvider& objectIdProvider) const;
    // Override which produces a FatalError if any object is used.
    WireResult Serialize(size_t commandSize, SerializeBuffer* serializeBuffer) const;

    WireResult Deserialize(DeserializeBuffer* deserializeBuffer, DeserializeAllocator* allocator, const ObjectIdResolver& resolver);
    // Override which produces a FatalError if any object is used.
    WireResult Deserialize(DeserializeBuffer* deserializeBuffer, DeserializeAllocator* allocator);

    ObjectId selfId;

    WGPUQuerySet self;
    WGPUStringView label;
};

    struct QueueCopyExternalTextureForBrowserCmd {
    size_t GetRequiredSize() const;

    WireResult Serialize(size_t commandSize, SerializeBuffer* serializeBuffer, const ObjectIdProvider& objectIdProvider) const;
    // Override which produces a FatalError if any object is used.
    WireResult Serialize(size_t commandSize, SerializeBuffer* serializeBuffer) const;

    WireResult Deserialize(DeserializeBuffer* deserializeBuffer, DeserializeAllocator* allocator, const ObjectIdResolver& resolver);
    // Override which produces a FatalError if any object is used.
    WireResult Deserialize(DeserializeBuffer* deserializeBuffer, DeserializeAllocator* allocator);

    ObjectId selfId;

    WGPUQueue self;
    WGPUImageCopyExternalTexture const * source;
    WGPUTexelCopyTextureInfo const * destination;
    WGPUExtent3D const * copySize;
    WGPUCopyTextureForBrowserOptions const * options;
};

    struct QueueCopyTextureForBrowserCmd {
    size_t GetRequiredSize() const;

    WireResult Serialize(size_t commandSize, SerializeBuffer* serializeBuffer, const ObjectIdProvider& objectIdProvider) const;
    // Override which produces a FatalError if any object is used.
    WireResult Serialize(size_t commandSize, SerializeBuffer* serializeBuffer) const;

    WireResult Deserialize(DeserializeBuffer* deserializeBuffer, DeserializeAllocator* allocator, const ObjectIdResolver& resolver);
    // Override which produces a FatalError if any object is used.
    WireResult Deserialize(DeserializeBuffer* deserializeBuffer, DeserializeAllocator* allocator);

    ObjectId selfId;

    WGPUQueue self;
    WGPUTexelCopyTextureInfo const * source;
    WGPUTexelCopyTextureInfo const * destination;
    WGPUExtent3D const * copySize;
    WGPUCopyTextureForBrowserOptions const * options;
};

    struct QueueOnSubmittedWorkDoneCmd {
    size_t GetRequiredSize() const;

    WireResult Serialize(size_t commandSize, SerializeBuffer* serializeBuffer, const ObjectIdProvider& objectIdProvider) const;
    // Override which produces a FatalError if any object is used.
    WireResult Serialize(size_t commandSize, SerializeBuffer* serializeBuffer) const;

    WireResult Deserialize(DeserializeBuffer* deserializeBuffer, DeserializeAllocator* allocator, const ObjectIdResolver& resolver);
    // Override which produces a FatalError if any object is used.
    WireResult Deserialize(DeserializeBuffer* deserializeBuffer, DeserializeAllocator* allocator);


    ObjectId queueId;
    ObjectId instanceId;
    WGPUFuture future;
};

    struct QueueSetLabelCmd {
    size_t GetRequiredSize() const;

    WireResult Serialize(size_t commandSize, SerializeBuffer* serializeBuffer, const ObjectIdProvider& objectIdProvider) const;
    // Override which produces a FatalError if any object is used.
    WireResult Serialize(size_t commandSize, SerializeBuffer* serializeBuffer) const;

    WireResult Deserialize(DeserializeBuffer* deserializeBuffer, DeserializeAllocator* allocator, const ObjectIdResolver& resolver);
    // Override which produces a FatalError if any object is used.
    WireResult Deserialize(DeserializeBuffer* deserializeBuffer, DeserializeAllocator* allocator);

    ObjectId selfId;

    WGPUQueue self;
    WGPUStringView label;
};

    struct QueueSubmitCmd {
    size_t GetRequiredSize() const;

    WireResult Serialize(size_t commandSize, SerializeBuffer* serializeBuffer, const ObjectIdProvider& objectIdProvider) const;
    // Override which produces a FatalError if any object is used.
    WireResult Serialize(size_t commandSize, SerializeBuffer* serializeBuffer) const;

    WireResult Deserialize(DeserializeBuffer* deserializeBuffer, DeserializeAllocator* allocator, const ObjectIdResolver& resolver);
    // Override which produces a FatalError if any object is used.
    WireResult Deserialize(DeserializeBuffer* deserializeBuffer, DeserializeAllocator* allocator);

    ObjectId selfId;

    WGPUQueue self;
    size_t commandCount;
    WGPUCommandBuffer const * commands;
};

    struct QueueWriteBufferCmd {
    size_t GetRequiredSize() const;

    WireResult Serialize(size_t commandSize, SerializeBuffer* serializeBuffer, const ObjectIdProvider& objectIdProvider) const;
    // Override which produces a FatalError if any object is used.
    WireResult Serialize(size_t commandSize, SerializeBuffer* serializeBuffer) const;

    WireResult Deserialize(DeserializeBuffer* deserializeBuffer, DeserializeAllocator* allocator, const ObjectIdResolver& resolver);
    // Override which produces a FatalError if any object is used.
    WireResult Deserialize(DeserializeBuffer* deserializeBuffer, DeserializeAllocator* allocator);


    ObjectId queueId;
    ObjectId bufferId;
    uint64_t bufferOffset;
    uint8_t const * data;
    size_t size;
};

    struct QueueWriteBufferXlCmd {
    size_t GetRequiredSize() const;

    WireResult Serialize(size_t commandSize, SerializeBuffer* serializeBuffer, const ObjectIdProvider& objectIdProvider) const;
    // Override which produces a FatalError if any object is used.
    WireResult Serialize(size_t commandSize, SerializeBuffer* serializeBuffer) const;

    WireResult Deserialize(DeserializeBuffer* deserializeBuffer, DeserializeAllocator* allocator, const ObjectIdResolver& resolver);
    // Override which produces a FatalError if any object is used.
    WireResult Deserialize(DeserializeBuffer* deserializeBuffer, DeserializeAllocator* allocator);


    ObjectId queueId;
    ObjectId bufferId;
    uint64_t bufferOffset;
    size_t size;
    size_t memoryHandleCreateInfoLength;
    std::byte const * memoryHandleCreateInfo;
    size_t memoryDataUpdateInfoLength;
    std::byte const * memoryDataUpdateInfo;
};

    struct QueueWriteTextureCmd {
    size_t GetRequiredSize() const;

    WireResult Serialize(size_t commandSize, SerializeBuffer* serializeBuffer, const ObjectIdProvider& objectIdProvider) const;
    // Override which produces a FatalError if any object is used.
    WireResult Serialize(size_t commandSize, SerializeBuffer* serializeBuffer) const;

    WireResult Deserialize(DeserializeBuffer* deserializeBuffer, DeserializeAllocator* allocator, const ObjectIdResolver& resolver);
    // Override which produces a FatalError if any object is used.
    WireResult Deserialize(DeserializeBuffer* deserializeBuffer, DeserializeAllocator* allocator);


    ObjectId queueId;
    WGPUTexelCopyTextureInfo const * destination;
    uint8_t const * data;
    size_t dataSize;
    WGPUTexelCopyBufferLayout const * dataLayout;
    WGPUExtent3D const * writeSize;
};

    struct QueueWriteTextureXlCmd {
    size_t GetRequiredSize() const;

    WireResult Serialize(size_t commandSize, SerializeBuffer* serializeBuffer, const ObjectIdProvider& objectIdProvider) const;
    // Override which produces a FatalError if any object is used.
    WireResult Serialize(size_t commandSize, SerializeBuffer* serializeBuffer) const;

    WireResult Deserialize(DeserializeBuffer* deserializeBuffer, DeserializeAllocator* allocator, const ObjectIdResolver& resolver);
    // Override which produces a FatalError if any object is used.
    WireResult Deserialize(DeserializeBuffer* deserializeBuffer, DeserializeAllocator* allocator);


    ObjectId queueId;
    WGPUTexelCopyTextureInfo const * destination;
    size_t dataSize;
    WGPUTexelCopyBufferLayout const * dataLayout;
    WGPUExtent3D const * writeSize;
    size_t memoryHandleCreateInfoLength;
    std::byte const * memoryHandleCreateInfo;
    size_t memoryDataUpdateInfoLength;
    std::byte const * memoryDataUpdateInfo;
};

    struct RenderBundleEncoderDrawCmd {
    size_t GetRequiredSize() const;

    WireResult Serialize(size_t commandSize, SerializeBuffer* serializeBuffer, const ObjectIdProvider& objectIdProvider) const;
    // Override which produces a FatalError if any object is used.
    WireResult Serialize(size_t commandSize, SerializeBuffer* serializeBuffer) const;

    WireResult Deserialize(DeserializeBuffer* deserializeBuffer, DeserializeAllocator* allocator, const ObjectIdResolver& resolver);
    // Override which produces a FatalError if any object is used.
    WireResult Deserialize(DeserializeBuffer* deserializeBuffer, DeserializeAllocator* allocator);

    ObjectId selfId;

    WGPURenderBundleEncoder self;
    uint32_t vertexCount;
    uint32_t instanceCount;
    uint32_t firstVertex;
    uint32_t firstInstance;
};

    struct RenderBundleEncoderDrawIndexedCmd {
    size_t GetRequiredSize() const;

    WireResult Serialize(size_t commandSize, SerializeBuffer* serializeBuffer, const ObjectIdProvider& objectIdProvider) const;
    // Override which produces a FatalError if any object is used.
    WireResult Serialize(size_t commandSize, SerializeBuffer* serializeBuffer) const;

    WireResult Deserialize(DeserializeBuffer* deserializeBuffer, DeserializeAllocator* allocator, const ObjectIdResolver& resolver);
    // Override which produces a FatalError if any object is used.
    WireResult Deserialize(DeserializeBuffer* deserializeBuffer, DeserializeAllocator* allocator);

    ObjectId selfId;

    WGPURenderBundleEncoder self;
    uint32_t indexCount;
    uint32_t instanceCount;
    uint32_t firstIndex;
    int32_t baseVertex;
    uint32_t firstInstance;
};

    struct RenderBundleEncoderDrawIndexedIndirectCmd {
    size_t GetRequiredSize() const;

    WireResult Serialize(size_t commandSize, SerializeBuffer* serializeBuffer, const ObjectIdProvider& objectIdProvider) const;
    // Override which produces a FatalError if any object is used.
    WireResult Serialize(size_t commandSize, SerializeBuffer* serializeBuffer) const;

    WireResult Deserialize(DeserializeBuffer* deserializeBuffer, DeserializeAllocator* allocator, const ObjectIdResolver& resolver);
    // Override which produces a FatalError if any object is used.
    WireResult Deserialize(DeserializeBuffer* deserializeBuffer, DeserializeAllocator* allocator);

    ObjectId selfId;

    WGPURenderBundleEncoder self;
    WGPUBuffer indirectBuffer;
    uint64_t indirectOffset;
};

    struct RenderBundleEncoderDrawIndirectCmd {
    size_t GetRequiredSize() const;

    WireResult Serialize(size_t commandSize, SerializeBuffer* serializeBuffer, const ObjectIdProvider& objectIdProvider) const;
    // Override which produces a FatalError if any object is used.
    WireResult Serialize(size_t commandSize, SerializeBuffer* serializeBuffer) const;

    WireResult Deserialize(DeserializeBuffer* deserializeBuffer, DeserializeAllocator* allocator, const ObjectIdResolver& resolver);
    // Override which produces a FatalError if any object is used.
    WireResult Deserialize(DeserializeBuffer* deserializeBuffer, DeserializeAllocator* allocator);

    ObjectId selfId;

    WGPURenderBundleEncoder self;
    WGPUBuffer indirectBuffer;
    uint64_t indirectOffset;
};

    struct RenderBundleEncoderFinishCmd {
    size_t GetRequiredSize() const;

    WireResult Serialize(size_t commandSize, SerializeBuffer* serializeBuffer, const ObjectIdProvider& objectIdProvider) const;
    // Override which produces a FatalError if any object is used.
    WireResult Serialize(size_t commandSize, SerializeBuffer* serializeBuffer) const;

    WireResult Deserialize(DeserializeBuffer* deserializeBuffer, DeserializeAllocator* allocator, const ObjectIdResolver& resolver);
    // Override which produces a FatalError if any object is used.
    WireResult Deserialize(DeserializeBuffer* deserializeBuffer, DeserializeAllocator* allocator);

    ObjectId selfId;

    WGPURenderBundleEncoder self;
    WGPURenderBundleDescriptor const * descriptor;
    ObjectHandle result;
};

    struct RenderBundleEncoderInsertDebugMarkerCmd {
    size_t GetRequiredSize() const;

    WireResult Serialize(size_t commandSize, SerializeBuffer* serializeBuffer, const ObjectIdProvider& objectIdProvider) const;
    // Override which produces a FatalError if any object is used.
    WireResult Serialize(size_t commandSize, SerializeBuffer* serializeBuffer) const;

    WireResult Deserialize(DeserializeBuffer* deserializeBuffer, DeserializeAllocator* allocator, const ObjectIdResolver& resolver);
    // Override which produces a FatalError if any object is used.
    WireResult Deserialize(DeserializeBuffer* deserializeBuffer, DeserializeAllocator* allocator);

    ObjectId selfId;

    WGPURenderBundleEncoder self;
    WGPUStringView markerLabel;
};

    struct RenderBundleEncoderPopDebugGroupCmd {
    size_t GetRequiredSize() const;

    WireResult Serialize(size_t commandSize, SerializeBuffer* serializeBuffer, const ObjectIdProvider& objectIdProvider) const;
    // Override which produces a FatalError if any object is used.
    WireResult Serialize(size_t commandSize, SerializeBuffer* serializeBuffer) const;

    WireResult Deserialize(DeserializeBuffer* deserializeBuffer, DeserializeAllocator* allocator, const ObjectIdResolver& resolver);
    // Override which produces a FatalError if any object is used.
    WireResult Deserialize(DeserializeBuffer* deserializeBuffer, DeserializeAllocator* allocator);

    ObjectId selfId;

    WGPURenderBundleEncoder self;
};

    struct RenderBundleEncoderPushDebugGroupCmd {
    size_t GetRequiredSize() const;

    WireResult Serialize(size_t commandSize, SerializeBuffer* serializeBuffer, const ObjectIdProvider& objectIdProvider) const;
    // Override which produces a FatalError if any object is used.
    WireResult Serialize(size_t commandSize, SerializeBuffer* serializeBuffer) const;

    WireResult Deserialize(DeserializeBuffer* deserializeBuffer, DeserializeAllocator* allocator, const ObjectIdResolver& resolver);
    // Override which produces a FatalError if any object is used.
    WireResult Deserialize(DeserializeBuffer* deserializeBuffer, DeserializeAllocator* allocator);

    ObjectId selfId;

    WGPURenderBundleEncoder self;
    WGPUStringView groupLabel;
};

    struct RenderBundleEncoderSetBindGroupCmd {
    size_t GetRequiredSize() const;

    WireResult Serialize(size_t commandSize, SerializeBuffer* serializeBuffer, const ObjectIdProvider& objectIdProvider) const;
    // Override which produces a FatalError if any object is used.
    WireResult Serialize(size_t commandSize, SerializeBuffer* serializeBuffer) const;

    WireResult Deserialize(DeserializeBuffer* deserializeBuffer, DeserializeAllocator* allocator, const ObjectIdResolver& resolver);
    // Override which produces a FatalError if any object is used.
    WireResult Deserialize(DeserializeBuffer* deserializeBuffer, DeserializeAllocator* allocator);

    ObjectId selfId;

    WGPURenderBundleEncoder self;
    uint32_t groupIndex;
    WGPUBindGroup group;
    size_t dynamicOffsetCount;
    uint32_t const * dynamicOffsets;
};

    struct RenderBundleEncoderSetImmediatesCmd {
    size_t GetRequiredSize() const;

    WireResult Serialize(size_t commandSize, SerializeBuffer* serializeBuffer, const ObjectIdProvider& objectIdProvider) const;
    // Override which produces a FatalError if any object is used.
    WireResult Serialize(size_t commandSize, SerializeBuffer* serializeBuffer) const;

    WireResult Deserialize(DeserializeBuffer* deserializeBuffer, DeserializeAllocator* allocator, const ObjectIdResolver& resolver);
    // Override which produces a FatalError if any object is used.
    WireResult Deserialize(DeserializeBuffer* deserializeBuffer, DeserializeAllocator* allocator);


    ObjectId renderBundleEncoderId;
    uint32_t offset;
    uint8_t const * data;
    size_t size;
};

    struct RenderBundleEncoderSetIndexBufferCmd {
    size_t GetRequiredSize() const;

    WireResult Serialize(size_t commandSize, SerializeBuffer* serializeBuffer, const ObjectIdProvider& objectIdProvider) const;
    // Override which produces a FatalError if any object is used.
    WireResult Serialize(size_t commandSize, SerializeBuffer* serializeBuffer) const;

    WireResult Deserialize(DeserializeBuffer* deserializeBuffer, DeserializeAllocator* allocator, const ObjectIdResolver& resolver);
    // Override which produces a FatalError if any object is used.
    WireResult Deserialize(DeserializeBuffer* deserializeBuffer, DeserializeAllocator* allocator);

    ObjectId selfId;

    WGPURenderBundleEncoder self;
    WGPUBuffer buffer;
    WGPUIndexFormat format;
    uint64_t offset;
    uint64_t size;
};

    struct RenderBundleEncoderSetLabelCmd {
    size_t GetRequiredSize() const;

    WireResult Serialize(size_t commandSize, SerializeBuffer* serializeBuffer, const ObjectIdProvider& objectIdProvider) const;
    // Override which produces a FatalError if any object is used.
    WireResult Serialize(size_t commandSize, SerializeBuffer* serializeBuffer) const;

    WireResult Deserialize(DeserializeBuffer* deserializeBuffer, DeserializeAllocator* allocator, const ObjectIdResolver& resolver);
    // Override which produces a FatalError if any object is used.
    WireResult Deserialize(DeserializeBuffer* deserializeBuffer, DeserializeAllocator* allocator);

    ObjectId selfId;

    WGPURenderBundleEncoder self;
    WGPUStringView label;
};

    struct RenderBundleEncoderSetPipelineCmd {
    size_t GetRequiredSize() const;

    WireResult Serialize(size_t commandSize, SerializeBuffer* serializeBuffer, const ObjectIdProvider& objectIdProvider) const;
    // Override which produces a FatalError if any object is used.
    WireResult Serialize(size_t commandSize, SerializeBuffer* serializeBuffer) const;

    WireResult Deserialize(DeserializeBuffer* deserializeBuffer, DeserializeAllocator* allocator, const ObjectIdResolver& resolver);
    // Override which produces a FatalError if any object is used.
    WireResult Deserialize(DeserializeBuffer* deserializeBuffer, DeserializeAllocator* allocator);

    ObjectId selfId;

    WGPURenderBundleEncoder self;
    WGPURenderPipeline pipeline;
};

    struct RenderBundleEncoderSetResourceTableCmd {
    size_t GetRequiredSize() const;

    WireResult Serialize(size_t commandSize, SerializeBuffer* serializeBuffer, const ObjectIdProvider& objectIdProvider) const;
    // Override which produces a FatalError if any object is used.
    WireResult Serialize(size_t commandSize, SerializeBuffer* serializeBuffer) const;

    WireResult Deserialize(DeserializeBuffer* deserializeBuffer, DeserializeAllocator* allocator, const ObjectIdResolver& resolver);
    // Override which produces a FatalError if any object is used.
    WireResult Deserialize(DeserializeBuffer* deserializeBuffer, DeserializeAllocator* allocator);

    ObjectId selfId;

    WGPURenderBundleEncoder self;
    WGPUResourceTable table;
};

    struct RenderBundleEncoderSetVertexBufferCmd {
    size_t GetRequiredSize() const;

    WireResult Serialize(size_t commandSize, SerializeBuffer* serializeBuffer, const ObjectIdProvider& objectIdProvider) const;
    // Override which produces a FatalError if any object is used.
    WireResult Serialize(size_t commandSize, SerializeBuffer* serializeBuffer) const;

    WireResult Deserialize(DeserializeBuffer* deserializeBuffer, DeserializeAllocator* allocator, const ObjectIdResolver& resolver);
    // Override which produces a FatalError if any object is used.
    WireResult Deserialize(DeserializeBuffer* deserializeBuffer, DeserializeAllocator* allocator);

    ObjectId selfId;

    WGPURenderBundleEncoder self;
    uint32_t slot;
    WGPUBuffer buffer;
    uint64_t offset;
    uint64_t size;
};

    struct RenderBundleSetLabelCmd {
    size_t GetRequiredSize() const;

    WireResult Serialize(size_t commandSize, SerializeBuffer* serializeBuffer, const ObjectIdProvider& objectIdProvider) const;
    // Override which produces a FatalError if any object is used.
    WireResult Serialize(size_t commandSize, SerializeBuffer* serializeBuffer) const;

    WireResult Deserialize(DeserializeBuffer* deserializeBuffer, DeserializeAllocator* allocator, const ObjectIdResolver& resolver);
    // Override which produces a FatalError if any object is used.
    WireResult Deserialize(DeserializeBuffer* deserializeBuffer, DeserializeAllocator* allocator);

    ObjectId selfId;

    WGPURenderBundle self;
    WGPUStringView label;
};

    struct RenderPassEncoderBeginOcclusionQueryCmd {
    size_t GetRequiredSize() const;

    WireResult Serialize(size_t commandSize, SerializeBuffer* serializeBuffer, const ObjectIdProvider& objectIdProvider) const;
    // Override which produces a FatalError if any object is used.
    WireResult Serialize(size_t commandSize, SerializeBuffer* serializeBuffer) const;

    WireResult Deserialize(DeserializeBuffer* deserializeBuffer, DeserializeAllocator* allocator, const ObjectIdResolver& resolver);
    // Override which produces a FatalError if any object is used.
    WireResult Deserialize(DeserializeBuffer* deserializeBuffer, DeserializeAllocator* allocator);

    ObjectId selfId;

    WGPURenderPassEncoder self;
    uint32_t queryIndex;
};

    struct RenderPassEncoderDrawCmd {
    size_t GetRequiredSize() const;

    WireResult Serialize(size_t commandSize, SerializeBuffer* serializeBuffer, const ObjectIdProvider& objectIdProvider) const;
    // Override which produces a FatalError if any object is used.
    WireResult Serialize(size_t commandSize, SerializeBuffer* serializeBuffer) const;

    WireResult Deserialize(DeserializeBuffer* deserializeBuffer, DeserializeAllocator* allocator, const ObjectIdResolver& resolver);
    // Override which produces a FatalError if any object is used.
    WireResult Deserialize(DeserializeBuffer* deserializeBuffer, DeserializeAllocator* allocator);

    ObjectId selfId;

    WGPURenderPassEncoder self;
    uint32_t vertexCount;
    uint32_t instanceCount;
    uint32_t firstVertex;
    uint32_t firstInstance;
};

    struct RenderPassEncoderDrawIndexedCmd {
    size_t GetRequiredSize() const;

    WireResult Serialize(size_t commandSize, SerializeBuffer* serializeBuffer, const ObjectIdProvider& objectIdProvider) const;
    // Override which produces a FatalError if any object is used.
    WireResult Serialize(size_t commandSize, SerializeBuffer* serializeBuffer) const;

    WireResult Deserialize(DeserializeBuffer* deserializeBuffer, DeserializeAllocator* allocator, const ObjectIdResolver& resolver);
    // Override which produces a FatalError if any object is used.
    WireResult Deserialize(DeserializeBuffer* deserializeBuffer, DeserializeAllocator* allocator);

    ObjectId selfId;

    WGPURenderPassEncoder self;
    uint32_t indexCount;
    uint32_t instanceCount;
    uint32_t firstIndex;
    int32_t baseVertex;
    uint32_t firstInstance;
};

    struct RenderPassEncoderDrawIndexedIndirectCmd {
    size_t GetRequiredSize() const;

    WireResult Serialize(size_t commandSize, SerializeBuffer* serializeBuffer, const ObjectIdProvider& objectIdProvider) const;
    // Override which produces a FatalError if any object is used.
    WireResult Serialize(size_t commandSize, SerializeBuffer* serializeBuffer) const;

    WireResult Deserialize(DeserializeBuffer* deserializeBuffer, DeserializeAllocator* allocator, const ObjectIdResolver& resolver);
    // Override which produces a FatalError if any object is used.
    WireResult Deserialize(DeserializeBuffer* deserializeBuffer, DeserializeAllocator* allocator);

    ObjectId selfId;

    WGPURenderPassEncoder self;
    WGPUBuffer indirectBuffer;
    uint64_t indirectOffset;
};

    struct RenderPassEncoderDrawIndirectCmd {
    size_t GetRequiredSize() const;

    WireResult Serialize(size_t commandSize, SerializeBuffer* serializeBuffer, const ObjectIdProvider& objectIdProvider) const;
    // Override which produces a FatalError if any object is used.
    WireResult Serialize(size_t commandSize, SerializeBuffer* serializeBuffer) const;

    WireResult Deserialize(DeserializeBuffer* deserializeBuffer, DeserializeAllocator* allocator, const ObjectIdResolver& resolver);
    // Override which produces a FatalError if any object is used.
    WireResult Deserialize(DeserializeBuffer* deserializeBuffer, DeserializeAllocator* allocator);

    ObjectId selfId;

    WGPURenderPassEncoder self;
    WGPUBuffer indirectBuffer;
    uint64_t indirectOffset;
};

    struct RenderPassEncoderEndCmd {
    size_t GetRequiredSize() const;

    WireResult Serialize(size_t commandSize, SerializeBuffer* serializeBuffer, const ObjectIdProvider& objectIdProvider) const;
    // Override which produces a FatalError if any object is used.
    WireResult Serialize(size_t commandSize, SerializeBuffer* serializeBuffer) const;

    WireResult Deserialize(DeserializeBuffer* deserializeBuffer, DeserializeAllocator* allocator, const ObjectIdResolver& resolver);
    // Override which produces a FatalError if any object is used.
    WireResult Deserialize(DeserializeBuffer* deserializeBuffer, DeserializeAllocator* allocator);

    ObjectId selfId;

    WGPURenderPassEncoder self;
};

    struct RenderPassEncoderEndOcclusionQueryCmd {
    size_t GetRequiredSize() const;

    WireResult Serialize(size_t commandSize, SerializeBuffer* serializeBuffer, const ObjectIdProvider& objectIdProvider) const;
    // Override which produces a FatalError if any object is used.
    WireResult Serialize(size_t commandSize, SerializeBuffer* serializeBuffer) const;

    WireResult Deserialize(DeserializeBuffer* deserializeBuffer, DeserializeAllocator* allocator, const ObjectIdResolver& resolver);
    // Override which produces a FatalError if any object is used.
    WireResult Deserialize(DeserializeBuffer* deserializeBuffer, DeserializeAllocator* allocator);

    ObjectId selfId;

    WGPURenderPassEncoder self;
};

    struct RenderPassEncoderExecuteBundlesCmd {
    size_t GetRequiredSize() const;

    WireResult Serialize(size_t commandSize, SerializeBuffer* serializeBuffer, const ObjectIdProvider& objectIdProvider) const;
    // Override which produces a FatalError if any object is used.
    WireResult Serialize(size_t commandSize, SerializeBuffer* serializeBuffer) const;

    WireResult Deserialize(DeserializeBuffer* deserializeBuffer, DeserializeAllocator* allocator, const ObjectIdResolver& resolver);
    // Override which produces a FatalError if any object is used.
    WireResult Deserialize(DeserializeBuffer* deserializeBuffer, DeserializeAllocator* allocator);

    ObjectId selfId;

    WGPURenderPassEncoder self;
    size_t bundleCount;
    WGPURenderBundle const * bundles;
};

    struct RenderPassEncoderInsertDebugMarkerCmd {
    size_t GetRequiredSize() const;

    WireResult Serialize(size_t commandSize, SerializeBuffer* serializeBuffer, const ObjectIdProvider& objectIdProvider) const;
    // Override which produces a FatalError if any object is used.
    WireResult Serialize(size_t commandSize, SerializeBuffer* serializeBuffer) const;

    WireResult Deserialize(DeserializeBuffer* deserializeBuffer, DeserializeAllocator* allocator, const ObjectIdResolver& resolver);
    // Override which produces a FatalError if any object is used.
    WireResult Deserialize(DeserializeBuffer* deserializeBuffer, DeserializeAllocator* allocator);

    ObjectId selfId;

    WGPURenderPassEncoder self;
    WGPUStringView markerLabel;
};

    struct RenderPassEncoderMultiDrawIndexedIndirectCmd {
    size_t GetRequiredSize() const;

    WireResult Serialize(size_t commandSize, SerializeBuffer* serializeBuffer, const ObjectIdProvider& objectIdProvider) const;
    // Override which produces a FatalError if any object is used.
    WireResult Serialize(size_t commandSize, SerializeBuffer* serializeBuffer) const;

    WireResult Deserialize(DeserializeBuffer* deserializeBuffer, DeserializeAllocator* allocator, const ObjectIdResolver& resolver);
    // Override which produces a FatalError if any object is used.
    WireResult Deserialize(DeserializeBuffer* deserializeBuffer, DeserializeAllocator* allocator);

    ObjectId selfId;

    WGPURenderPassEncoder self;
    WGPUBuffer indirectBuffer;
    uint64_t indirectOffset;
    uint32_t maxDrawCount;
    WGPUBuffer drawCountBuffer;
    uint64_t drawCountBufferOffset;
};

    struct RenderPassEncoderMultiDrawIndirectCmd {
    size_t GetRequiredSize() const;

    WireResult Serialize(size_t commandSize, SerializeBuffer* serializeBuffer, const ObjectIdProvider& objectIdProvider) const;
    // Override which produces a FatalError if any object is used.
    WireResult Serialize(size_t commandSize, SerializeBuffer* serializeBuffer) const;

    WireResult Deserialize(DeserializeBuffer* deserializeBuffer, DeserializeAllocator* allocator, const ObjectIdResolver& resolver);
    // Override which produces a FatalError if any object is used.
    WireResult Deserialize(DeserializeBuffer* deserializeBuffer, DeserializeAllocator* allocator);

    ObjectId selfId;

    WGPURenderPassEncoder self;
    WGPUBuffer indirectBuffer;
    uint64_t indirectOffset;
    uint32_t maxDrawCount;
    WGPUBuffer drawCountBuffer;
    uint64_t drawCountBufferOffset;
};

    struct RenderPassEncoderPixelLocalStorageBarrierCmd {
    size_t GetRequiredSize() const;

    WireResult Serialize(size_t commandSize, SerializeBuffer* serializeBuffer, const ObjectIdProvider& objectIdProvider) const;
    // Override which produces a FatalError if any object is used.
    WireResult Serialize(size_t commandSize, SerializeBuffer* serializeBuffer) const;

    WireResult Deserialize(DeserializeBuffer* deserializeBuffer, DeserializeAllocator* allocator, const ObjectIdResolver& resolver);
    // Override which produces a FatalError if any object is used.
    WireResult Deserialize(DeserializeBuffer* deserializeBuffer, DeserializeAllocator* allocator);

    ObjectId selfId;

    WGPURenderPassEncoder self;
};

    struct RenderPassEncoderPopDebugGroupCmd {
    size_t GetRequiredSize() const;

    WireResult Serialize(size_t commandSize, SerializeBuffer* serializeBuffer, const ObjectIdProvider& objectIdProvider) const;
    // Override which produces a FatalError if any object is used.
    WireResult Serialize(size_t commandSize, SerializeBuffer* serializeBuffer) const;

    WireResult Deserialize(DeserializeBuffer* deserializeBuffer, DeserializeAllocator* allocator, const ObjectIdResolver& resolver);
    // Override which produces a FatalError if any object is used.
    WireResult Deserialize(DeserializeBuffer* deserializeBuffer, DeserializeAllocator* allocator);

    ObjectId selfId;

    WGPURenderPassEncoder self;
};

    struct RenderPassEncoderPushDebugGroupCmd {
    size_t GetRequiredSize() const;

    WireResult Serialize(size_t commandSize, SerializeBuffer* serializeBuffer, const ObjectIdProvider& objectIdProvider) const;
    // Override which produces a FatalError if any object is used.
    WireResult Serialize(size_t commandSize, SerializeBuffer* serializeBuffer) const;

    WireResult Deserialize(DeserializeBuffer* deserializeBuffer, DeserializeAllocator* allocator, const ObjectIdResolver& resolver);
    // Override which produces a FatalError if any object is used.
    WireResult Deserialize(DeserializeBuffer* deserializeBuffer, DeserializeAllocator* allocator);

    ObjectId selfId;

    WGPURenderPassEncoder self;
    WGPUStringView groupLabel;
};

    struct RenderPassEncoderSetBindGroupCmd {
    size_t GetRequiredSize() const;

    WireResult Serialize(size_t commandSize, SerializeBuffer* serializeBuffer, const ObjectIdProvider& objectIdProvider) const;
    // Override which produces a FatalError if any object is used.
    WireResult Serialize(size_t commandSize, SerializeBuffer* serializeBuffer) const;

    WireResult Deserialize(DeserializeBuffer* deserializeBuffer, DeserializeAllocator* allocator, const ObjectIdResolver& resolver);
    // Override which produces a FatalError if any object is used.
    WireResult Deserialize(DeserializeBuffer* deserializeBuffer, DeserializeAllocator* allocator);

    ObjectId selfId;

    WGPURenderPassEncoder self;
    uint32_t groupIndex;
    WGPUBindGroup group;
    size_t dynamicOffsetCount;
    uint32_t const * dynamicOffsets;
};

    struct RenderPassEncoderSetBlendConstantCmd {
    size_t GetRequiredSize() const;

    WireResult Serialize(size_t commandSize, SerializeBuffer* serializeBuffer, const ObjectIdProvider& objectIdProvider) const;
    // Override which produces a FatalError if any object is used.
    WireResult Serialize(size_t commandSize, SerializeBuffer* serializeBuffer) const;

    WireResult Deserialize(DeserializeBuffer* deserializeBuffer, DeserializeAllocator* allocator, const ObjectIdResolver& resolver);
    // Override which produces a FatalError if any object is used.
    WireResult Deserialize(DeserializeBuffer* deserializeBuffer, DeserializeAllocator* allocator);

    ObjectId selfId;

    WGPURenderPassEncoder self;
    WGPUColor const * color;
};

    struct RenderPassEncoderSetImmediatesCmd {
    size_t GetRequiredSize() const;

    WireResult Serialize(size_t commandSize, SerializeBuffer* serializeBuffer, const ObjectIdProvider& objectIdProvider) const;
    // Override which produces a FatalError if any object is used.
    WireResult Serialize(size_t commandSize, SerializeBuffer* serializeBuffer) const;

    WireResult Deserialize(DeserializeBuffer* deserializeBuffer, DeserializeAllocator* allocator, const ObjectIdResolver& resolver);
    // Override which produces a FatalError if any object is used.
    WireResult Deserialize(DeserializeBuffer* deserializeBuffer, DeserializeAllocator* allocator);


    ObjectId renderPassEncoderId;
    uint32_t offset;
    uint8_t const * data;
    size_t size;
};

    struct RenderPassEncoderSetIndexBufferCmd {
    size_t GetRequiredSize() const;

    WireResult Serialize(size_t commandSize, SerializeBuffer* serializeBuffer, const ObjectIdProvider& objectIdProvider) const;
    // Override which produces a FatalError if any object is used.
    WireResult Serialize(size_t commandSize, SerializeBuffer* serializeBuffer) const;

    WireResult Deserialize(DeserializeBuffer* deserializeBuffer, DeserializeAllocator* allocator, const ObjectIdResolver& resolver);
    // Override which produces a FatalError if any object is used.
    WireResult Deserialize(DeserializeBuffer* deserializeBuffer, DeserializeAllocator* allocator);

    ObjectId selfId;

    WGPURenderPassEncoder self;
    WGPUBuffer buffer;
    WGPUIndexFormat format;
    uint64_t offset;
    uint64_t size;
};

    struct RenderPassEncoderSetLabelCmd {
    size_t GetRequiredSize() const;

    WireResult Serialize(size_t commandSize, SerializeBuffer* serializeBuffer, const ObjectIdProvider& objectIdProvider) const;
    // Override which produces a FatalError if any object is used.
    WireResult Serialize(size_t commandSize, SerializeBuffer* serializeBuffer) const;

    WireResult Deserialize(DeserializeBuffer* deserializeBuffer, DeserializeAllocator* allocator, const ObjectIdResolver& resolver);
    // Override which produces a FatalError if any object is used.
    WireResult Deserialize(DeserializeBuffer* deserializeBuffer, DeserializeAllocator* allocator);

    ObjectId selfId;

    WGPURenderPassEncoder self;
    WGPUStringView label;
};

    struct RenderPassEncoderSetPipelineCmd {
    size_t GetRequiredSize() const;

    WireResult Serialize(size_t commandSize, SerializeBuffer* serializeBuffer, const ObjectIdProvider& objectIdProvider) const;
    // Override which produces a FatalError if any object is used.
    WireResult Serialize(size_t commandSize, SerializeBuffer* serializeBuffer) const;

    WireResult Deserialize(DeserializeBuffer* deserializeBuffer, DeserializeAllocator* allocator, const ObjectIdResolver& resolver);
    // Override which produces a FatalError if any object is used.
    WireResult Deserialize(DeserializeBuffer* deserializeBuffer, DeserializeAllocator* allocator);

    ObjectId selfId;

    WGPURenderPassEncoder self;
    WGPURenderPipeline pipeline;
};

    struct RenderPassEncoderSetResourceTableCmd {
    size_t GetRequiredSize() const;

    WireResult Serialize(size_t commandSize, SerializeBuffer* serializeBuffer, const ObjectIdProvider& objectIdProvider) const;
    // Override which produces a FatalError if any object is used.
    WireResult Serialize(size_t commandSize, SerializeBuffer* serializeBuffer) const;

    WireResult Deserialize(DeserializeBuffer* deserializeBuffer, DeserializeAllocator* allocator, const ObjectIdResolver& resolver);
    // Override which produces a FatalError if any object is used.
    WireResult Deserialize(DeserializeBuffer* deserializeBuffer, DeserializeAllocator* allocator);

    ObjectId selfId;

    WGPURenderPassEncoder self;
    WGPUResourceTable table;
};

    struct RenderPassEncoderSetScissorRectCmd {
    size_t GetRequiredSize() const;

    WireResult Serialize(size_t commandSize, SerializeBuffer* serializeBuffer, const ObjectIdProvider& objectIdProvider) const;
    // Override which produces a FatalError if any object is used.
    WireResult Serialize(size_t commandSize, SerializeBuffer* serializeBuffer) const;

    WireResult Deserialize(DeserializeBuffer* deserializeBuffer, DeserializeAllocator* allocator, const ObjectIdResolver& resolver);
    // Override which produces a FatalError if any object is used.
    WireResult Deserialize(DeserializeBuffer* deserializeBuffer, DeserializeAllocator* allocator);

    ObjectId selfId;

    WGPURenderPassEncoder self;
    uint32_t x;
    uint32_t y;
    uint32_t width;
    uint32_t height;
};

    struct RenderPassEncoderSetStencilReferenceCmd {
    size_t GetRequiredSize() const;

    WireResult Serialize(size_t commandSize, SerializeBuffer* serializeBuffer, const ObjectIdProvider& objectIdProvider) const;
    // Override which produces a FatalError if any object is used.
    WireResult Serialize(size_t commandSize, SerializeBuffer* serializeBuffer) const;

    WireResult Deserialize(DeserializeBuffer* deserializeBuffer, DeserializeAllocator* allocator, const ObjectIdResolver& resolver);
    // Override which produces a FatalError if any object is used.
    WireResult Deserialize(DeserializeBuffer* deserializeBuffer, DeserializeAllocator* allocator);

    ObjectId selfId;

    WGPURenderPassEncoder self;
    uint32_t reference;
};

    struct RenderPassEncoderSetVertexBufferCmd {
    size_t GetRequiredSize() const;

    WireResult Serialize(size_t commandSize, SerializeBuffer* serializeBuffer, const ObjectIdProvider& objectIdProvider) const;
    // Override which produces a FatalError if any object is used.
    WireResult Serialize(size_t commandSize, SerializeBuffer* serializeBuffer) const;

    WireResult Deserialize(DeserializeBuffer* deserializeBuffer, DeserializeAllocator* allocator, const ObjectIdResolver& resolver);
    // Override which produces a FatalError if any object is used.
    WireResult Deserialize(DeserializeBuffer* deserializeBuffer, DeserializeAllocator* allocator);

    ObjectId selfId;

    WGPURenderPassEncoder self;
    uint32_t slot;
    WGPUBuffer buffer;
    uint64_t offset;
    uint64_t size;
};

    struct RenderPassEncoderSetViewportCmd {
    size_t GetRequiredSize() const;

    WireResult Serialize(size_t commandSize, SerializeBuffer* serializeBuffer, const ObjectIdProvider& objectIdProvider) const;
    // Override which produces a FatalError if any object is used.
    WireResult Serialize(size_t commandSize, SerializeBuffer* serializeBuffer) const;

    WireResult Deserialize(DeserializeBuffer* deserializeBuffer, DeserializeAllocator* allocator, const ObjectIdResolver& resolver);
    // Override which produces a FatalError if any object is used.
    WireResult Deserialize(DeserializeBuffer* deserializeBuffer, DeserializeAllocator* allocator);

    ObjectId selfId;

    WGPURenderPassEncoder self;
    float x;
    float y;
    float width;
    float height;
    float minDepth;
    float maxDepth;
};

    struct RenderPassEncoderWriteTimestampCmd {
    size_t GetRequiredSize() const;

    WireResult Serialize(size_t commandSize, SerializeBuffer* serializeBuffer, const ObjectIdProvider& objectIdProvider) const;
    // Override which produces a FatalError if any object is used.
    WireResult Serialize(size_t commandSize, SerializeBuffer* serializeBuffer) const;

    WireResult Deserialize(DeserializeBuffer* deserializeBuffer, DeserializeAllocator* allocator, const ObjectIdResolver& resolver);
    // Override which produces a FatalError if any object is used.
    WireResult Deserialize(DeserializeBuffer* deserializeBuffer, DeserializeAllocator* allocator);

    ObjectId selfId;

    WGPURenderPassEncoder self;
    WGPUQuerySet querySet;
    uint32_t queryIndex;
};

    struct RenderPipelineGetBindGroupLayoutCmd {
    size_t GetRequiredSize() const;

    WireResult Serialize(size_t commandSize, SerializeBuffer* serializeBuffer, const ObjectIdProvider& objectIdProvider) const;
    // Override which produces a FatalError if any object is used.
    WireResult Serialize(size_t commandSize, SerializeBuffer* serializeBuffer) const;

    WireResult Deserialize(DeserializeBuffer* deserializeBuffer, DeserializeAllocator* allocator, const ObjectIdResolver& resolver);
    // Override which produces a FatalError if any object is used.
    WireResult Deserialize(DeserializeBuffer* deserializeBuffer, DeserializeAllocator* allocator);

    ObjectId selfId;

    WGPURenderPipeline self;
    uint32_t groupIndex;
    ObjectHandle result;
};

    struct RenderPipelineSetLabelCmd {
    size_t GetRequiredSize() const;

    WireResult Serialize(size_t commandSize, SerializeBuffer* serializeBuffer, const ObjectIdProvider& objectIdProvider) const;
    // Override which produces a FatalError if any object is used.
    WireResult Serialize(size_t commandSize, SerializeBuffer* serializeBuffer) const;

    WireResult Deserialize(DeserializeBuffer* deserializeBuffer, DeserializeAllocator* allocator, const ObjectIdResolver& resolver);
    // Override which produces a FatalError if any object is used.
    WireResult Deserialize(DeserializeBuffer* deserializeBuffer, DeserializeAllocator* allocator);

    ObjectId selfId;

    WGPURenderPipeline self;
    WGPUStringView label;
};

    struct ResourceTableDestroyCmd {
    size_t GetRequiredSize() const;

    WireResult Serialize(size_t commandSize, SerializeBuffer* serializeBuffer, const ObjectIdProvider& objectIdProvider) const;
    // Override which produces a FatalError if any object is used.
    WireResult Serialize(size_t commandSize, SerializeBuffer* serializeBuffer) const;

    WireResult Deserialize(DeserializeBuffer* deserializeBuffer, DeserializeAllocator* allocator, const ObjectIdResolver& resolver);
    // Override which produces a FatalError if any object is used.
    WireResult Deserialize(DeserializeBuffer* deserializeBuffer, DeserializeAllocator* allocator);

    ObjectId selfId;

    WGPUResourceTable self;
};

    struct ResourceTableRemoveCmd {
    size_t GetRequiredSize() const;

    WireResult Serialize(size_t commandSize, SerializeBuffer* serializeBuffer, const ObjectIdProvider& objectIdProvider) const;
    // Override which produces a FatalError if any object is used.
    WireResult Serialize(size_t commandSize, SerializeBuffer* serializeBuffer) const;

    WireResult Deserialize(DeserializeBuffer* deserializeBuffer, DeserializeAllocator* allocator, const ObjectIdResolver& resolver);
    // Override which produces a FatalError if any object is used.
    WireResult Deserialize(DeserializeBuffer* deserializeBuffer, DeserializeAllocator* allocator);

    ObjectId selfId;

    WGPUResourceTable self;
    uint32_t slot;
};

    struct ResourceTableSetLabelCmd {
    size_t GetRequiredSize() const;

    WireResult Serialize(size_t commandSize, SerializeBuffer* serializeBuffer, const ObjectIdProvider& objectIdProvider) const;
    // Override which produces a FatalError if any object is used.
    WireResult Serialize(size_t commandSize, SerializeBuffer* serializeBuffer) const;

    WireResult Deserialize(DeserializeBuffer* deserializeBuffer, DeserializeAllocator* allocator, const ObjectIdResolver& resolver);
    // Override which produces a FatalError if any object is used.
    WireResult Deserialize(DeserializeBuffer* deserializeBuffer, DeserializeAllocator* allocator);

    ObjectId selfId;

    WGPUResourceTable self;
    WGPUStringView label;
};

    struct ResourceTableUpdateCmd {
    size_t GetRequiredSize() const;

    WireResult Serialize(size_t commandSize, SerializeBuffer* serializeBuffer, const ObjectIdProvider& objectIdProvider) const;
    // Override which produces a FatalError if any object is used.
    WireResult Serialize(size_t commandSize, SerializeBuffer* serializeBuffer) const;

    WireResult Deserialize(DeserializeBuffer* deserializeBuffer, DeserializeAllocator* allocator, const ObjectIdResolver& resolver);
    // Override which produces a FatalError if any object is used.
    WireResult Deserialize(DeserializeBuffer* deserializeBuffer, DeserializeAllocator* allocator);

    ObjectId selfId;

    WGPUResourceTable self;
    uint32_t slot;
    WGPUBindingResource const * resource;
};

    struct SamplerSetLabelCmd {
    size_t GetRequiredSize() const;

    WireResult Serialize(size_t commandSize, SerializeBuffer* serializeBuffer, const ObjectIdProvider& objectIdProvider) const;
    // Override which produces a FatalError if any object is used.
    WireResult Serialize(size_t commandSize, SerializeBuffer* serializeBuffer) const;

    WireResult Deserialize(DeserializeBuffer* deserializeBuffer, DeserializeAllocator* allocator, const ObjectIdResolver& resolver);
    // Override which produces a FatalError if any object is used.
    WireResult Deserialize(DeserializeBuffer* deserializeBuffer, DeserializeAllocator* allocator);

    ObjectId selfId;

    WGPUSampler self;
    WGPUStringView label;
};

    struct ShaderModuleGetCompilationInfoCmd {
    size_t GetRequiredSize() const;

    WireResult Serialize(size_t commandSize, SerializeBuffer* serializeBuffer, const ObjectIdProvider& objectIdProvider) const;
    // Override which produces a FatalError if any object is used.
    WireResult Serialize(size_t commandSize, SerializeBuffer* serializeBuffer) const;

    WireResult Deserialize(DeserializeBuffer* deserializeBuffer, DeserializeAllocator* allocator, const ObjectIdResolver& resolver);
    // Override which produces a FatalError if any object is used.
    WireResult Deserialize(DeserializeBuffer* deserializeBuffer, DeserializeAllocator* allocator);


    ObjectId shaderModuleId;
    ObjectId instanceId;
    WGPUFuture future;
};

    struct ShaderModuleSetLabelCmd {
    size_t GetRequiredSize() const;

    WireResult Serialize(size_t commandSize, SerializeBuffer* serializeBuffer, const ObjectIdProvider& objectIdProvider) const;
    // Override which produces a FatalError if any object is used.
    WireResult Serialize(size_t commandSize, SerializeBuffer* serializeBuffer) const;

    WireResult Deserialize(DeserializeBuffer* deserializeBuffer, DeserializeAllocator* allocator, const ObjectIdResolver& resolver);
    // Override which produces a FatalError if any object is used.
    WireResult Deserialize(DeserializeBuffer* deserializeBuffer, DeserializeAllocator* allocator);

    ObjectId selfId;

    WGPUShaderModule self;
    WGPUStringView label;
};

    struct SurfaceConfigureCmd {
    size_t GetRequiredSize() const;

    WireResult Serialize(size_t commandSize, SerializeBuffer* serializeBuffer, const ObjectIdProvider& objectIdProvider) const;
    // Override which produces a FatalError if any object is used.
    WireResult Serialize(size_t commandSize, SerializeBuffer* serializeBuffer) const;

    WireResult Deserialize(DeserializeBuffer* deserializeBuffer, DeserializeAllocator* allocator, const ObjectIdResolver& resolver);
    // Override which produces a FatalError if any object is used.
    WireResult Deserialize(DeserializeBuffer* deserializeBuffer, DeserializeAllocator* allocator);

    ObjectId selfId;

    WGPUSurface self;
    WGPUSurfaceConfiguration const * config;
};

    struct SurfaceGetCurrentTextureCmd {
    size_t GetRequiredSize() const;

    WireResult Serialize(size_t commandSize, SerializeBuffer* serializeBuffer, const ObjectIdProvider& objectIdProvider) const;
    // Override which produces a FatalError if any object is used.
    WireResult Serialize(size_t commandSize, SerializeBuffer* serializeBuffer) const;

    WireResult Deserialize(DeserializeBuffer* deserializeBuffer, DeserializeAllocator* allocator, const ObjectIdResolver& resolver);
    // Override which produces a FatalError if any object is used.
    WireResult Deserialize(DeserializeBuffer* deserializeBuffer, DeserializeAllocator* allocator);


    ObjectId surfaceId;
    ObjectId configuredDeviceId;
    ObjectHandle textureHandle;
};

    struct SurfacePresentCmd {
    size_t GetRequiredSize() const;

    WireResult Serialize(size_t commandSize, SerializeBuffer* serializeBuffer, const ObjectIdProvider& objectIdProvider) const;
    // Override which produces a FatalError if any object is used.
    WireResult Serialize(size_t commandSize, SerializeBuffer* serializeBuffer) const;

    WireResult Deserialize(DeserializeBuffer* deserializeBuffer, DeserializeAllocator* allocator, const ObjectIdResolver& resolver);
    // Override which produces a FatalError if any object is used.
    WireResult Deserialize(DeserializeBuffer* deserializeBuffer, DeserializeAllocator* allocator);

    ObjectId selfId;

    WGPUSurface self;
};

    struct SurfaceSetLabelCmd {
    size_t GetRequiredSize() const;

    WireResult Serialize(size_t commandSize, SerializeBuffer* serializeBuffer, const ObjectIdProvider& objectIdProvider) const;
    // Override which produces a FatalError if any object is used.
    WireResult Serialize(size_t commandSize, SerializeBuffer* serializeBuffer) const;

    WireResult Deserialize(DeserializeBuffer* deserializeBuffer, DeserializeAllocator* allocator, const ObjectIdResolver& resolver);
    // Override which produces a FatalError if any object is used.
    WireResult Deserialize(DeserializeBuffer* deserializeBuffer, DeserializeAllocator* allocator);

    ObjectId selfId;

    WGPUSurface self;
    WGPUStringView label;
};

    struct SurfaceUnconfigureCmd {
    size_t GetRequiredSize() const;

    WireResult Serialize(size_t commandSize, SerializeBuffer* serializeBuffer, const ObjectIdProvider& objectIdProvider) const;
    // Override which produces a FatalError if any object is used.
    WireResult Serialize(size_t commandSize, SerializeBuffer* serializeBuffer) const;

    WireResult Deserialize(DeserializeBuffer* deserializeBuffer, DeserializeAllocator* allocator, const ObjectIdResolver& resolver);
    // Override which produces a FatalError if any object is used.
    WireResult Deserialize(DeserializeBuffer* deserializeBuffer, DeserializeAllocator* allocator);

    ObjectId selfId;

    WGPUSurface self;
};

    struct TexelBufferViewSetLabelCmd {
    size_t GetRequiredSize() const;

    WireResult Serialize(size_t commandSize, SerializeBuffer* serializeBuffer, const ObjectIdProvider& objectIdProvider) const;
    // Override which produces a FatalError if any object is used.
    WireResult Serialize(size_t commandSize, SerializeBuffer* serializeBuffer) const;

    WireResult Deserialize(DeserializeBuffer* deserializeBuffer, DeserializeAllocator* allocator, const ObjectIdResolver& resolver);
    // Override which produces a FatalError if any object is used.
    WireResult Deserialize(DeserializeBuffer* deserializeBuffer, DeserializeAllocator* allocator);

    ObjectId selfId;

    WGPUTexelBufferView self;
    WGPUStringView label;
};

    struct TextureCreateErrorViewCmd {
    size_t GetRequiredSize() const;

    WireResult Serialize(size_t commandSize, SerializeBuffer* serializeBuffer, const ObjectIdProvider& objectIdProvider) const;
    // Override which produces a FatalError if any object is used.
    WireResult Serialize(size_t commandSize, SerializeBuffer* serializeBuffer) const;

    WireResult Deserialize(DeserializeBuffer* deserializeBuffer, DeserializeAllocator* allocator, const ObjectIdResolver& resolver);
    // Override which produces a FatalError if any object is used.
    WireResult Deserialize(DeserializeBuffer* deserializeBuffer, DeserializeAllocator* allocator);

    ObjectId selfId;

    WGPUTexture self;
    WGPUTextureViewDescriptor const * descriptor;
    ObjectHandle result;
};

    struct TextureCreateViewCmd {
    size_t GetRequiredSize() const;

    WireResult Serialize(size_t commandSize, SerializeBuffer* serializeBuffer, const ObjectIdProvider& objectIdProvider) const;
    // Override which produces a FatalError if any object is used.
    WireResult Serialize(size_t commandSize, SerializeBuffer* serializeBuffer) const;

    WireResult Deserialize(DeserializeBuffer* deserializeBuffer, DeserializeAllocator* allocator, const ObjectIdResolver& resolver);
    // Override which produces a FatalError if any object is used.
    WireResult Deserialize(DeserializeBuffer* deserializeBuffer, DeserializeAllocator* allocator);

    ObjectId selfId;

    WGPUTexture self;
    WGPUTextureViewDescriptor const * descriptor;
    ObjectHandle result;
};

    struct TextureDestroyCmd {
    size_t GetRequiredSize() const;

    WireResult Serialize(size_t commandSize, SerializeBuffer* serializeBuffer, const ObjectIdProvider& objectIdProvider) const;
    // Override which produces a FatalError if any object is used.
    WireResult Serialize(size_t commandSize, SerializeBuffer* serializeBuffer) const;

    WireResult Deserialize(DeserializeBuffer* deserializeBuffer, DeserializeAllocator* allocator, const ObjectIdResolver& resolver);
    // Override which produces a FatalError if any object is used.
    WireResult Deserialize(DeserializeBuffer* deserializeBuffer, DeserializeAllocator* allocator);

    ObjectId selfId;

    WGPUTexture self;
};

    struct TextureSetLabelCmd {
    size_t GetRequiredSize() const;

    WireResult Serialize(size_t commandSize, SerializeBuffer* serializeBuffer, const ObjectIdProvider& objectIdProvider) const;
    // Override which produces a FatalError if any object is used.
    WireResult Serialize(size_t commandSize, SerializeBuffer* serializeBuffer) const;

    WireResult Deserialize(DeserializeBuffer* deserializeBuffer, DeserializeAllocator* allocator, const ObjectIdResolver& resolver);
    // Override which produces a FatalError if any object is used.
    WireResult Deserialize(DeserializeBuffer* deserializeBuffer, DeserializeAllocator* allocator);

    ObjectId selfId;

    WGPUTexture self;
    WGPUStringView label;
};

    struct TextureViewSetLabelCmd {
    size_t GetRequiredSize() const;

    WireResult Serialize(size_t commandSize, SerializeBuffer* serializeBuffer, const ObjectIdProvider& objectIdProvider) const;
    // Override which produces a FatalError if any object is used.
    WireResult Serialize(size_t commandSize, SerializeBuffer* serializeBuffer) const;

    WireResult Deserialize(DeserializeBuffer* deserializeBuffer, DeserializeAllocator* allocator, const ObjectIdResolver& resolver);
    // Override which produces a FatalError if any object is used.
    WireResult Deserialize(DeserializeBuffer* deserializeBuffer, DeserializeAllocator* allocator);

    ObjectId selfId;

    WGPUTextureView self;
    WGPUStringView label;
};

    struct UnregisterObjectCmd {
    size_t GetRequiredSize() const;

    WireResult Serialize(size_t commandSize, SerializeBuffer* serializeBuffer, const ObjectIdProvider& objectIdProvider) const;
    // Override which produces a FatalError if any object is used.
    WireResult Serialize(size_t commandSize, SerializeBuffer* serializeBuffer) const;

    WireResult Deserialize(DeserializeBuffer* deserializeBuffer, DeserializeAllocator* allocator, const ObjectIdResolver& resolver);
    // Override which produces a FatalError if any object is used.
    WireResult Deserialize(DeserializeBuffer* deserializeBuffer, DeserializeAllocator* allocator);


    ObjectType objectType;
    ObjectId objectId;
};


    struct ReturnAdapterRequestDeviceCallbackCmd {
    size_t GetRequiredSize() const;

    WireResult Serialize(size_t commandSize, SerializeBuffer* serializeBuffer, const ObjectIdProvider& objectIdProvider) const;
    // Override which produces a FatalError if any object is used.
    WireResult Serialize(size_t commandSize, SerializeBuffer* serializeBuffer) const;

    WireResult Deserialize(DeserializeBuffer* deserializeBuffer, DeserializeAllocator* allocator, const ObjectIdResolver& resolver);
    // Override which produces a FatalError if any object is used.
    WireResult Deserialize(DeserializeBuffer* deserializeBuffer, DeserializeAllocator* allocator);


    ObjectId instanceId;
    WGPUFuture future;
    WGPURequestDeviceStatus status;
    WGPUStringView message;
    WGPULimits const * limits;
    uint32_t featuresCount;
    WGPUFeatureName const * features;
};

    struct ReturnBufferMapAsyncCallbackCmd {
    size_t GetRequiredSize() const;

    WireResult Serialize(size_t commandSize, SerializeBuffer* serializeBuffer, const ObjectIdProvider& objectIdProvider) const;
    // Override which produces a FatalError if any object is used.
    WireResult Serialize(size_t commandSize, SerializeBuffer* serializeBuffer) const;

    WireResult Deserialize(DeserializeBuffer* deserializeBuffer, DeserializeAllocator* allocator, const ObjectIdResolver& resolver);
    // Override which produces a FatalError if any object is used.
    WireResult Deserialize(DeserializeBuffer* deserializeBuffer, DeserializeAllocator* allocator);


    ObjectId instanceId;
    WGPUFuture future;
    WGPUMapAsyncStatus status;
    WGPUStringView message;
    size_t readDataUpdateInfoLength;
    std::byte const * readDataUpdateInfo;
};

    struct ReturnDeviceCreateComputePipelineAsyncCallbackCmd {
    size_t GetRequiredSize() const;

    WireResult Serialize(size_t commandSize, SerializeBuffer* serializeBuffer, const ObjectIdProvider& objectIdProvider) const;
    // Override which produces a FatalError if any object is used.
    WireResult Serialize(size_t commandSize, SerializeBuffer* serializeBuffer) const;

    WireResult Deserialize(DeserializeBuffer* deserializeBuffer, DeserializeAllocator* allocator, const ObjectIdResolver& resolver);
    // Override which produces a FatalError if any object is used.
    WireResult Deserialize(DeserializeBuffer* deserializeBuffer, DeserializeAllocator* allocator);


    ObjectId instanceId;
    WGPUFuture future;
    WGPUCreatePipelineAsyncStatus status;
    WGPUStringView message;
};

    struct ReturnDeviceCreateRenderPipelineAsyncCallbackCmd {
    size_t GetRequiredSize() const;

    WireResult Serialize(size_t commandSize, SerializeBuffer* serializeBuffer, const ObjectIdProvider& objectIdProvider) const;
    // Override which produces a FatalError if any object is used.
    WireResult Serialize(size_t commandSize, SerializeBuffer* serializeBuffer) const;

    WireResult Deserialize(DeserializeBuffer* deserializeBuffer, DeserializeAllocator* allocator, const ObjectIdResolver& resolver);
    // Override which produces a FatalError if any object is used.
    WireResult Deserialize(DeserializeBuffer* deserializeBuffer, DeserializeAllocator* allocator);


    ObjectId instanceId;
    WGPUFuture future;
    WGPUCreatePipelineAsyncStatus status;
    WGPUStringView message;
};

    struct ReturnDeviceLoggingCallbackCmd {
    size_t GetRequiredSize() const;

    WireResult Serialize(size_t commandSize, SerializeBuffer* serializeBuffer, const ObjectIdProvider& objectIdProvider) const;
    // Override which produces a FatalError if any object is used.
    WireResult Serialize(size_t commandSize, SerializeBuffer* serializeBuffer) const;

    WireResult Deserialize(DeserializeBuffer* deserializeBuffer, DeserializeAllocator* allocator, const ObjectIdResolver& resolver);
    // Override which produces a FatalError if any object is used.
    WireResult Deserialize(DeserializeBuffer* deserializeBuffer, DeserializeAllocator* allocator);


    ObjectHandle device;
    WGPULoggingType type;
    WGPUStringView message;
};

    struct ReturnDeviceLostCallbackCmd {
    size_t GetRequiredSize() const;

    WireResult Serialize(size_t commandSize, SerializeBuffer* serializeBuffer, const ObjectIdProvider& objectIdProvider) const;
    // Override which produces a FatalError if any object is used.
    WireResult Serialize(size_t commandSize, SerializeBuffer* serializeBuffer) const;

    WireResult Deserialize(DeserializeBuffer* deserializeBuffer, DeserializeAllocator* allocator, const ObjectIdResolver& resolver);
    // Override which produces a FatalError if any object is used.
    WireResult Deserialize(DeserializeBuffer* deserializeBuffer, DeserializeAllocator* allocator);


    ObjectId instanceId;
    WGPUFuture future;
    WGPUDeviceLostReason reason;
    WGPUStringView message;
};

    struct ReturnDevicePopErrorScopeCallbackCmd {
    size_t GetRequiredSize() const;

    WireResult Serialize(size_t commandSize, SerializeBuffer* serializeBuffer, const ObjectIdProvider& objectIdProvider) const;
    // Override which produces a FatalError if any object is used.
    WireResult Serialize(size_t commandSize, SerializeBuffer* serializeBuffer) const;

    WireResult Deserialize(DeserializeBuffer* deserializeBuffer, DeserializeAllocator* allocator, const ObjectIdResolver& resolver);
    // Override which produces a FatalError if any object is used.
    WireResult Deserialize(DeserializeBuffer* deserializeBuffer, DeserializeAllocator* allocator);


    ObjectId instanceId;
    WGPUFuture future;
    WGPUPopErrorScopeStatus status;
    WGPUErrorType type;
    WGPUStringView message;
};

    struct ReturnDeviceUncapturedErrorCallbackCmd {
    size_t GetRequiredSize() const;

    WireResult Serialize(size_t commandSize, SerializeBuffer* serializeBuffer, const ObjectIdProvider& objectIdProvider) const;
    // Override which produces a FatalError if any object is used.
    WireResult Serialize(size_t commandSize, SerializeBuffer* serializeBuffer) const;

    WireResult Deserialize(DeserializeBuffer* deserializeBuffer, DeserializeAllocator* allocator, const ObjectIdResolver& resolver);
    // Override which produces a FatalError if any object is used.
    WireResult Deserialize(DeserializeBuffer* deserializeBuffer, DeserializeAllocator* allocator);


    ObjectHandle device;
    WGPUErrorType type;
    WGPUStringView message;
};

    struct ReturnInstanceRequestAdapterCallbackCmd {
    size_t GetRequiredSize() const;

    WireResult Serialize(size_t commandSize, SerializeBuffer* serializeBuffer, const ObjectIdProvider& objectIdProvider) const;
    // Override which produces a FatalError if any object is used.
    WireResult Serialize(size_t commandSize, SerializeBuffer* serializeBuffer) const;

    WireResult Deserialize(DeserializeBuffer* deserializeBuffer, DeserializeAllocator* allocator, const ObjectIdResolver& resolver);
    // Override which produces a FatalError if any object is used.
    WireResult Deserialize(DeserializeBuffer* deserializeBuffer, DeserializeAllocator* allocator);


    ObjectId instanceId;
    WGPUFuture future;
    WGPURequestAdapterStatus status;
    WGPUStringView message;
    WGPUAdapterInfo const * info;
    WGPULimits const * limits;
    uint32_t featuresCount;
    WGPUFeatureName const * features;
};

    struct ReturnQueueWorkDoneCallbackCmd {
    size_t GetRequiredSize() const;

    WireResult Serialize(size_t commandSize, SerializeBuffer* serializeBuffer, const ObjectIdProvider& objectIdProvider) const;
    // Override which produces a FatalError if any object is used.
    WireResult Serialize(size_t commandSize, SerializeBuffer* serializeBuffer) const;

    WireResult Deserialize(DeserializeBuffer* deserializeBuffer, DeserializeAllocator* allocator, const ObjectIdResolver& resolver);
    // Override which produces a FatalError if any object is used.
    WireResult Deserialize(DeserializeBuffer* deserializeBuffer, DeserializeAllocator* allocator);


    ObjectId instanceId;
    WGPUFuture future;
    WGPUQueueWorkDoneStatus status;
    WGPUStringView message;
};

    struct ReturnShaderModuleGetCompilationInfoCallbackCmd {
    size_t GetRequiredSize() const;

    WireResult Serialize(size_t commandSize, SerializeBuffer* serializeBuffer, const ObjectIdProvider& objectIdProvider) const;
    // Override which produces a FatalError if any object is used.
    WireResult Serialize(size_t commandSize, SerializeBuffer* serializeBuffer) const;

    WireResult Deserialize(DeserializeBuffer* deserializeBuffer, DeserializeAllocator* allocator, const ObjectIdResolver& resolver);
    // Override which produces a FatalError if any object is used.
    WireResult Deserialize(DeserializeBuffer* deserializeBuffer, DeserializeAllocator* allocator);


    ObjectId instanceId;
    WGPUFuture future;
    WGPUCompilationInfoRequestStatus status;
    WGPUCompilationInfo const * info;
};


}  // namespace dawn::wire

#endif // DAWNWIRE_WIRECMD_AUTOGEN_H_
