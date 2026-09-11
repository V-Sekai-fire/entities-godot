
#ifndef DAWNWIRE_CLIENT_CLIENTBASE_AUTOGEN_H_
#define DAWNWIRE_CLIENT_CLIENTBASE_AUTOGEN_H_

#include "dawn/wire/WireCmd_autogen.h"
#include "src/dawn/wire/ChunkedCommandHandler.h"
#include "src/dawn/wire/client/ApiObjects.h"

namespace dawn::wire::client {

    class ClientBase : public ChunkedCommandHandler, public ObjectIdProvider {
      public:
        ClientBase() = default;
        ~ClientBase() override = default;

        // Implementation of the ObjectIdProvider interface
        WireResult GetId(WGPUAdapter object, volatile ObjectId* out) const final {
            DAWN_ASSERT(out != nullptr);
            if (object == nullptr) {
                return WireResult::FatalError;
            }
            *out = reinterpret_cast<Adapter*>(object)->GetWireHandle(this).id;
            return WireResult::Success;
        }
        WireResult GetOptionalId(WGPUAdapter object, volatile ObjectId* out) const final {
            DAWN_ASSERT(out != nullptr);
            *out = (object == nullptr ? 0 : reinterpret_cast<Adapter*>(object)->GetWireHandle(this).id);
            return WireResult::Success;
        }
        WireResult GetId(WGPUBindGroup object, volatile ObjectId* out) const final {
            DAWN_ASSERT(out != nullptr);
            if (object == nullptr) {
                return WireResult::FatalError;
            }
            *out = reinterpret_cast<BindGroup*>(object)->GetWireHandle(this).id;
            return WireResult::Success;
        }
        WireResult GetOptionalId(WGPUBindGroup object, volatile ObjectId* out) const final {
            DAWN_ASSERT(out != nullptr);
            *out = (object == nullptr ? 0 : reinterpret_cast<BindGroup*>(object)->GetWireHandle(this).id);
            return WireResult::Success;
        }
        WireResult GetId(WGPUBindGroupLayout object, volatile ObjectId* out) const final {
            DAWN_ASSERT(out != nullptr);
            if (object == nullptr) {
                return WireResult::FatalError;
            }
            *out = reinterpret_cast<BindGroupLayout*>(object)->GetWireHandle(this).id;
            return WireResult::Success;
        }
        WireResult GetOptionalId(WGPUBindGroupLayout object, volatile ObjectId* out) const final {
            DAWN_ASSERT(out != nullptr);
            *out = (object == nullptr ? 0 : reinterpret_cast<BindGroupLayout*>(object)->GetWireHandle(this).id);
            return WireResult::Success;
        }
        WireResult GetId(WGPUBuffer object, volatile ObjectId* out) const final {
            DAWN_ASSERT(out != nullptr);
            if (object == nullptr) {
                return WireResult::FatalError;
            }
            *out = reinterpret_cast<Buffer*>(object)->GetWireHandle(this).id;
            return WireResult::Success;
        }
        WireResult GetOptionalId(WGPUBuffer object, volatile ObjectId* out) const final {
            DAWN_ASSERT(out != nullptr);
            *out = (object == nullptr ? 0 : reinterpret_cast<Buffer*>(object)->GetWireHandle(this).id);
            return WireResult::Success;
        }
        WireResult GetId(WGPUCommandBuffer object, volatile ObjectId* out) const final {
            DAWN_ASSERT(out != nullptr);
            if (object == nullptr) {
                return WireResult::FatalError;
            }
            *out = reinterpret_cast<CommandBuffer*>(object)->GetWireHandle(this).id;
            return WireResult::Success;
        }
        WireResult GetOptionalId(WGPUCommandBuffer object, volatile ObjectId* out) const final {
            DAWN_ASSERT(out != nullptr);
            *out = (object == nullptr ? 0 : reinterpret_cast<CommandBuffer*>(object)->GetWireHandle(this).id);
            return WireResult::Success;
        }
        WireResult GetId(WGPUCommandEncoder object, volatile ObjectId* out) const final {
            DAWN_ASSERT(out != nullptr);
            if (object == nullptr) {
                return WireResult::FatalError;
            }
            *out = reinterpret_cast<CommandEncoder*>(object)->GetWireHandle(this).id;
            return WireResult::Success;
        }
        WireResult GetOptionalId(WGPUCommandEncoder object, volatile ObjectId* out) const final {
            DAWN_ASSERT(out != nullptr);
            *out = (object == nullptr ? 0 : reinterpret_cast<CommandEncoder*>(object)->GetWireHandle(this).id);
            return WireResult::Success;
        }
        WireResult GetId(WGPUComputePipeline object, volatile ObjectId* out) const final {
            DAWN_ASSERT(out != nullptr);
            if (object == nullptr) {
                return WireResult::FatalError;
            }
            *out = reinterpret_cast<ComputePipeline*>(object)->GetWireHandle(this).id;
            return WireResult::Success;
        }
        WireResult GetOptionalId(WGPUComputePipeline object, volatile ObjectId* out) const final {
            DAWN_ASSERT(out != nullptr);
            *out = (object == nullptr ? 0 : reinterpret_cast<ComputePipeline*>(object)->GetWireHandle(this).id);
            return WireResult::Success;
        }
        WireResult GetId(WGPUDevice object, volatile ObjectId* out) const final {
            DAWN_ASSERT(out != nullptr);
            if (object == nullptr) {
                return WireResult::FatalError;
            }
            *out = reinterpret_cast<Device*>(object)->GetWireHandle(this).id;
            return WireResult::Success;
        }
        WireResult GetOptionalId(WGPUDevice object, volatile ObjectId* out) const final {
            DAWN_ASSERT(out != nullptr);
            *out = (object == nullptr ? 0 : reinterpret_cast<Device*>(object)->GetWireHandle(this).id);
            return WireResult::Success;
        }
        WireResult GetId(WGPUExternalTexture object, volatile ObjectId* out) const final {
            DAWN_ASSERT(out != nullptr);
            if (object == nullptr) {
                return WireResult::FatalError;
            }
            *out = reinterpret_cast<ExternalTexture*>(object)->GetWireHandle(this).id;
            return WireResult::Success;
        }
        WireResult GetOptionalId(WGPUExternalTexture object, volatile ObjectId* out) const final {
            DAWN_ASSERT(out != nullptr);
            *out = (object == nullptr ? 0 : reinterpret_cast<ExternalTexture*>(object)->GetWireHandle(this).id);
            return WireResult::Success;
        }
        WireResult GetId(WGPUInstance object, volatile ObjectId* out) const final {
            DAWN_ASSERT(out != nullptr);
            if (object == nullptr) {
                return WireResult::FatalError;
            }
            *out = reinterpret_cast<Instance*>(object)->GetWireHandle(this).id;
            return WireResult::Success;
        }
        WireResult GetOptionalId(WGPUInstance object, volatile ObjectId* out) const final {
            DAWN_ASSERT(out != nullptr);
            *out = (object == nullptr ? 0 : reinterpret_cast<Instance*>(object)->GetWireHandle(this).id);
            return WireResult::Success;
        }
        WireResult GetId(WGPUPipelineLayout object, volatile ObjectId* out) const final {
            DAWN_ASSERT(out != nullptr);
            if (object == nullptr) {
                return WireResult::FatalError;
            }
            *out = reinterpret_cast<PipelineLayout*>(object)->GetWireHandle(this).id;
            return WireResult::Success;
        }
        WireResult GetOptionalId(WGPUPipelineLayout object, volatile ObjectId* out) const final {
            DAWN_ASSERT(out != nullptr);
            *out = (object == nullptr ? 0 : reinterpret_cast<PipelineLayout*>(object)->GetWireHandle(this).id);
            return WireResult::Success;
        }
        WireResult GetId(WGPUQuerySet object, volatile ObjectId* out) const final {
            DAWN_ASSERT(out != nullptr);
            if (object == nullptr) {
                return WireResult::FatalError;
            }
            *out = reinterpret_cast<QuerySet*>(object)->GetWireHandle(this).id;
            return WireResult::Success;
        }
        WireResult GetOptionalId(WGPUQuerySet object, volatile ObjectId* out) const final {
            DAWN_ASSERT(out != nullptr);
            *out = (object == nullptr ? 0 : reinterpret_cast<QuerySet*>(object)->GetWireHandle(this).id);
            return WireResult::Success;
        }
        WireResult GetId(WGPUQueue object, volatile ObjectId* out) const final {
            DAWN_ASSERT(out != nullptr);
            if (object == nullptr) {
                return WireResult::FatalError;
            }
            *out = reinterpret_cast<Queue*>(object)->GetWireHandle(this).id;
            return WireResult::Success;
        }
        WireResult GetOptionalId(WGPUQueue object, volatile ObjectId* out) const final {
            DAWN_ASSERT(out != nullptr);
            *out = (object == nullptr ? 0 : reinterpret_cast<Queue*>(object)->GetWireHandle(this).id);
            return WireResult::Success;
        }
        WireResult GetId(WGPURenderBundle object, volatile ObjectId* out) const final {
            DAWN_ASSERT(out != nullptr);
            if (object == nullptr) {
                return WireResult::FatalError;
            }
            *out = reinterpret_cast<RenderBundle*>(object)->GetWireHandle(this).id;
            return WireResult::Success;
        }
        WireResult GetOptionalId(WGPURenderBundle object, volatile ObjectId* out) const final {
            DAWN_ASSERT(out != nullptr);
            *out = (object == nullptr ? 0 : reinterpret_cast<RenderBundle*>(object)->GetWireHandle(this).id);
            return WireResult::Success;
        }
        WireResult GetId(WGPURenderPipeline object, volatile ObjectId* out) const final {
            DAWN_ASSERT(out != nullptr);
            if (object == nullptr) {
                return WireResult::FatalError;
            }
            *out = reinterpret_cast<RenderPipeline*>(object)->GetWireHandle(this).id;
            return WireResult::Success;
        }
        WireResult GetOptionalId(WGPURenderPipeline object, volatile ObjectId* out) const final {
            DAWN_ASSERT(out != nullptr);
            *out = (object == nullptr ? 0 : reinterpret_cast<RenderPipeline*>(object)->GetWireHandle(this).id);
            return WireResult::Success;
        }
        WireResult GetId(WGPUResourceTable object, volatile ObjectId* out) const final {
            DAWN_ASSERT(out != nullptr);
            if (object == nullptr) {
                return WireResult::FatalError;
            }
            *out = reinterpret_cast<ResourceTable*>(object)->GetWireHandle(this).id;
            return WireResult::Success;
        }
        WireResult GetOptionalId(WGPUResourceTable object, volatile ObjectId* out) const final {
            DAWN_ASSERT(out != nullptr);
            *out = (object == nullptr ? 0 : reinterpret_cast<ResourceTable*>(object)->GetWireHandle(this).id);
            return WireResult::Success;
        }
        WireResult GetId(WGPUSampler object, volatile ObjectId* out) const final {
            DAWN_ASSERT(out != nullptr);
            if (object == nullptr) {
                return WireResult::FatalError;
            }
            *out = reinterpret_cast<Sampler*>(object)->GetWireHandle(this).id;
            return WireResult::Success;
        }
        WireResult GetOptionalId(WGPUSampler object, volatile ObjectId* out) const final {
            DAWN_ASSERT(out != nullptr);
            *out = (object == nullptr ? 0 : reinterpret_cast<Sampler*>(object)->GetWireHandle(this).id);
            return WireResult::Success;
        }
        WireResult GetId(WGPUShaderModule object, volatile ObjectId* out) const final {
            DAWN_ASSERT(out != nullptr);
            if (object == nullptr) {
                return WireResult::FatalError;
            }
            *out = reinterpret_cast<ShaderModule*>(object)->GetWireHandle(this).id;
            return WireResult::Success;
        }
        WireResult GetOptionalId(WGPUShaderModule object, volatile ObjectId* out) const final {
            DAWN_ASSERT(out != nullptr);
            *out = (object == nullptr ? 0 : reinterpret_cast<ShaderModule*>(object)->GetWireHandle(this).id);
            return WireResult::Success;
        }
        WireResult GetId(WGPUSurface object, volatile ObjectId* out) const final {
            DAWN_ASSERT(out != nullptr);
            if (object == nullptr) {
                return WireResult::FatalError;
            }
            *out = reinterpret_cast<Surface*>(object)->GetWireHandle(this).id;
            return WireResult::Success;
        }
        WireResult GetOptionalId(WGPUSurface object, volatile ObjectId* out) const final {
            DAWN_ASSERT(out != nullptr);
            *out = (object == nullptr ? 0 : reinterpret_cast<Surface*>(object)->GetWireHandle(this).id);
            return WireResult::Success;
        }
        WireResult GetId(WGPUTexelBufferView object, volatile ObjectId* out) const final {
            DAWN_ASSERT(out != nullptr);
            if (object == nullptr) {
                return WireResult::FatalError;
            }
            *out = reinterpret_cast<TexelBufferView*>(object)->GetWireHandle(this).id;
            return WireResult::Success;
        }
        WireResult GetOptionalId(WGPUTexelBufferView object, volatile ObjectId* out) const final {
            DAWN_ASSERT(out != nullptr);
            *out = (object == nullptr ? 0 : reinterpret_cast<TexelBufferView*>(object)->GetWireHandle(this).id);
            return WireResult::Success;
        }
        WireResult GetId(WGPUTexture object, volatile ObjectId* out) const final {
            DAWN_ASSERT(out != nullptr);
            if (object == nullptr) {
                return WireResult::FatalError;
            }
            *out = reinterpret_cast<Texture*>(object)->GetWireHandle(this).id;
            return WireResult::Success;
        }
        WireResult GetOptionalId(WGPUTexture object, volatile ObjectId* out) const final {
            DAWN_ASSERT(out != nullptr);
            *out = (object == nullptr ? 0 : reinterpret_cast<Texture*>(object)->GetWireHandle(this).id);
            return WireResult::Success;
        }
        WireResult GetId(WGPUTextureView object, volatile ObjectId* out) const final {
            DAWN_ASSERT(out != nullptr);
            if (object == nullptr) {
                return WireResult::FatalError;
            }
            *out = reinterpret_cast<TextureView*>(object)->GetWireHandle(this).id;
            return WireResult::Success;
        }
        WireResult GetOptionalId(WGPUTextureView object, volatile ObjectId* out) const final {
            DAWN_ASSERT(out != nullptr);
            *out = (object == nullptr ? 0 : reinterpret_cast<TextureView*>(object)->GetWireHandle(this).id);
            return WireResult::Success;
        }
        WireResult GetId(WGPUComputePassEncoder object, volatile ObjectId* out) const final {
            DAWN_ASSERT(out != nullptr);
            if (object == nullptr) {
                return WireResult::FatalError;
            }
            *out = reinterpret_cast<ComputePassEncoder*>(object)->GetWireHandle(this).id;
            return WireResult::Success;
        }
        WireResult GetOptionalId(WGPUComputePassEncoder object, volatile ObjectId* out) const final {
            DAWN_ASSERT(out != nullptr);
            *out = (object == nullptr ? 0 : reinterpret_cast<ComputePassEncoder*>(object)->GetWireHandle(this).id);
            return WireResult::Success;
        }
        WireResult GetId(WGPURenderBundleEncoder object, volatile ObjectId* out) const final {
            DAWN_ASSERT(out != nullptr);
            if (object == nullptr) {
                return WireResult::FatalError;
            }
            *out = reinterpret_cast<RenderBundleEncoder*>(object)->GetWireHandle(this).id;
            return WireResult::Success;
        }
        WireResult GetOptionalId(WGPURenderBundleEncoder object, volatile ObjectId* out) const final {
            DAWN_ASSERT(out != nullptr);
            *out = (object == nullptr ? 0 : reinterpret_cast<RenderBundleEncoder*>(object)->GetWireHandle(this).id);
            return WireResult::Success;
        }
        WireResult GetId(WGPURenderPassEncoder object, volatile ObjectId* out) const final {
            DAWN_ASSERT(out != nullptr);
            if (object == nullptr) {
                return WireResult::FatalError;
            }
            *out = reinterpret_cast<RenderPassEncoder*>(object)->GetWireHandle(this).id;
            return WireResult::Success;
        }
        WireResult GetOptionalId(WGPURenderPassEncoder object, volatile ObjectId* out) const final {
            DAWN_ASSERT(out != nullptr);
            *out = (object == nullptr ? 0 : reinterpret_cast<RenderPassEncoder*>(object)->GetWireHandle(this).id);
            return WireResult::Success;
        }
    };

}  // namespace dawn::wire::client

#endif  // DAWNWIRE_CLIENT_CLIENTBASE_AUTOGEN_H_
