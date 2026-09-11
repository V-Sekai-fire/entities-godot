
#ifndef DAWNWIRE_CLIENT_APIOBJECTS_AUTOGEN_H_
#define DAWNWIRE_CLIENT_APIOBJECTS_AUTOGEN_H_

#include "dawn/wire/ObjectType_autogen.h"
#include "src/dawn/wire/client/ObjectBase.h"

namespace dawn::wire::client {

    template<typename T>
    inline constexpr ObjectType ObjectTypeToTypeEnum = static_cast<ObjectType>(-1);

    class Adapter;

    template <>
    inline constexpr ObjectType ObjectTypeToTypeEnum<Adapter> = ObjectType::Adapter;

    struct BindGroup final : ObjectBase {
        using ObjectBase::ObjectBase;

        ObjectType GetObjectType() const override {
            return ObjectType::BindGroup;
        }
    };

    template <>
    inline constexpr ObjectType ObjectTypeToTypeEnum<BindGroup> = ObjectType::BindGroup;

    struct BindGroupLayout final : ObjectBase {
        using ObjectBase::ObjectBase;

        ObjectType GetObjectType() const override {
            return ObjectType::BindGroupLayout;
        }
    };

    template <>
    inline constexpr ObjectType ObjectTypeToTypeEnum<BindGroupLayout> = ObjectType::BindGroupLayout;

    class Buffer;

    template <>
    inline constexpr ObjectType ObjectTypeToTypeEnum<Buffer> = ObjectType::Buffer;

    struct CommandBuffer final : ObjectBase {
        using ObjectBase::ObjectBase;

        ObjectType GetObjectType() const override {
            return ObjectType::CommandBuffer;
        }
    };

    template <>
    inline constexpr ObjectType ObjectTypeToTypeEnum<CommandBuffer> = ObjectType::CommandBuffer;

    struct CommandEncoder final : ObjectBase {
        using ObjectBase::ObjectBase;

        ObjectType GetObjectType() const override {
            return ObjectType::CommandEncoder;
        }
    };

    template <>
    inline constexpr ObjectType ObjectTypeToTypeEnum<CommandEncoder> = ObjectType::CommandEncoder;

    struct ComputePipeline final : ObjectBase {
        using ObjectBase::ObjectBase;

        ObjectType GetObjectType() const override {
            return ObjectType::ComputePipeline;
        }
    };

    template <>
    inline constexpr ObjectType ObjectTypeToTypeEnum<ComputePipeline> = ObjectType::ComputePipeline;

    class Device;

    template <>
    inline constexpr ObjectType ObjectTypeToTypeEnum<Device> = ObjectType::Device;

    struct ExternalTexture final : ObjectBase {
        using ObjectBase::ObjectBase;

        ObjectType GetObjectType() const override {
            return ObjectType::ExternalTexture;
        }
    };

    template <>
    inline constexpr ObjectType ObjectTypeToTypeEnum<ExternalTexture> = ObjectType::ExternalTexture;

    class Instance;

    template <>
    inline constexpr ObjectType ObjectTypeToTypeEnum<Instance> = ObjectType::Instance;

    struct PipelineLayout final : ObjectBase {
        using ObjectBase::ObjectBase;

        ObjectType GetObjectType() const override {
            return ObjectType::PipelineLayout;
        }
    };

    template <>
    inline constexpr ObjectType ObjectTypeToTypeEnum<PipelineLayout> = ObjectType::PipelineLayout;

    class QuerySet;

    template <>
    inline constexpr ObjectType ObjectTypeToTypeEnum<QuerySet> = ObjectType::QuerySet;

    class Queue;

    template <>
    inline constexpr ObjectType ObjectTypeToTypeEnum<Queue> = ObjectType::Queue;

    struct RenderBundle final : ObjectBase {
        using ObjectBase::ObjectBase;

        ObjectType GetObjectType() const override {
            return ObjectType::RenderBundle;
        }
    };

    template <>
    inline constexpr ObjectType ObjectTypeToTypeEnum<RenderBundle> = ObjectType::RenderBundle;

    struct RenderPipeline final : ObjectBase {
        using ObjectBase::ObjectBase;

        ObjectType GetObjectType() const override {
            return ObjectType::RenderPipeline;
        }
    };

    template <>
    inline constexpr ObjectType ObjectTypeToTypeEnum<RenderPipeline> = ObjectType::RenderPipeline;

    class ResourceTable;

    template <>
    inline constexpr ObjectType ObjectTypeToTypeEnum<ResourceTable> = ObjectType::ResourceTable;

    struct Sampler final : ObjectBase {
        using ObjectBase::ObjectBase;

        ObjectType GetObjectType() const override {
            return ObjectType::Sampler;
        }
    };

    template <>
    inline constexpr ObjectType ObjectTypeToTypeEnum<Sampler> = ObjectType::Sampler;

    class ShaderModule;

    template <>
    inline constexpr ObjectType ObjectTypeToTypeEnum<ShaderModule> = ObjectType::ShaderModule;

    class Surface;

    template <>
    inline constexpr ObjectType ObjectTypeToTypeEnum<Surface> = ObjectType::Surface;

    struct TexelBufferView final : ObjectBase {
        using ObjectBase::ObjectBase;

        ObjectType GetObjectType() const override {
            return ObjectType::TexelBufferView;
        }
    };

    template <>
    inline constexpr ObjectType ObjectTypeToTypeEnum<TexelBufferView> = ObjectType::TexelBufferView;

    class Texture;

    template <>
    inline constexpr ObjectType ObjectTypeToTypeEnum<Texture> = ObjectType::Texture;

    struct TextureView final : ObjectBase {
        using ObjectBase::ObjectBase;

        ObjectType GetObjectType() const override {
            return ObjectType::TextureView;
        }
    };

    template <>
    inline constexpr ObjectType ObjectTypeToTypeEnum<TextureView> = ObjectType::TextureView;

    class ComputePassEncoder;

    template <>
    inline constexpr ObjectType ObjectTypeToTypeEnum<ComputePassEncoder> = ObjectType::ComputePassEncoder;

    class RenderBundleEncoder;

    template <>
    inline constexpr ObjectType ObjectTypeToTypeEnum<RenderBundleEncoder> = ObjectType::RenderBundleEncoder;

    class RenderPassEncoder;

    template <>
    inline constexpr ObjectType ObjectTypeToTypeEnum<RenderPassEncoder> = ObjectType::RenderPassEncoder;

}  // namespace dawn::wire::client

#endif  // DAWNWIRE_CLIENT_APIOBJECTS_AUTOGEN_H_
