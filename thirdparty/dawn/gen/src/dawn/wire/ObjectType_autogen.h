
#ifndef DAWNWIRE_OBJECTTYPE_AUTOGEN_H_
#define DAWNWIRE_OBJECTTYPE_AUTOGEN_H_

#include "src/dawn/common/ityp_array.h"

namespace dawn::wire {

    constexpr uint32_t kObjectTypes = 25;

    enum class ObjectType : uint32_t {
        Adapter,
        BindGroup,
        BindGroupLayout,
        Buffer,
        CommandBuffer,
        CommandEncoder,
        ComputePipeline,
        Device,
        ExternalTexture,
        Instance,
        PipelineLayout,
        QuerySet,
        Queue,
        RenderBundle,
        RenderPipeline,
        ResourceTable,
        Sampler,
        ShaderModule,
        Surface,
        TexelBufferView,
        Texture,
        TextureView,
        ComputePassEncoder,
        RenderBundleEncoder,
        RenderPassEncoder,
    };

    template <typename T>
    using PerObjectType = ityp::array<ObjectType, T, 25>;

} // namespace dawn::wire


#endif  // DAWNWIRE_OBJECTTYPE_AUTOGEN_H_
