

#ifndef DAWNWIRE_CLIENT_DAWN_PLATFORM_AUTOGEN_H_
#define DAWNWIRE_CLIENT_DAWN_PLATFORM_AUTOGEN_H_

#include "dawn/webgpu_cpp.h"

#include "dawn/wire/client/wgpu_structs_autogen.h"

namespace dawn::wire::client {
    class Adapter;
    struct BindGroup;
    struct BindGroupLayout;
    class Buffer;
    struct CommandBuffer;
    struct CommandEncoder;
    struct ComputePipeline;
    class Device;
    struct ExternalTexture;
    class Instance;
    struct PipelineLayout;
    class QuerySet;
    class Queue;
    struct RenderBundle;
    struct RenderPipeline;
    class ResourceTable;
    struct Sampler;
    class ShaderModule;
    class Surface;
    struct TexelBufferView;
    class Texture;
    struct TextureView;
    class ComputePassEncoder;
    class RenderBundleEncoder;
    class RenderPassEncoder;
}


namespace dawn::wire::client {

    inline const WGPUChainedStruct* ToAPI(const ChainedStruct* rhs) {
        return reinterpret_cast<const WGPUChainedStruct*>(rhs);
    }
    inline WGPUChainedStruct* ToAPI(ChainedStruct* rhs) {
        return reinterpret_cast<WGPUChainedStruct*>(rhs);
    }
    inline const ChainedStruct* FromAPI(const WGPUChainedStruct* rhs) {
        return reinterpret_cast<const ChainedStruct*>(rhs);
    }
    inline ChainedStruct* FromAPI(WGPUChainedStruct* rhs) {
        return reinterpret_cast<ChainedStruct*>(rhs);
    }
    inline const ChainedStruct& FromAPI(const WGPUChainedStruct& rhs) {
        return *reinterpret_cast<const ChainedStruct*>(&rhs);
    }
    inline ChainedStruct& FromAPI(WGPUChainedStruct& rhs) {
        return *reinterpret_cast<ChainedStruct*>(&rhs);
    }

    inline const WGPUAdapterPropertiesD3D* ToAPI(const AdapterPropertiesD3D* rhs) {
        return reinterpret_cast<const WGPUAdapterPropertiesD3D*>(rhs);
    }
    inline WGPUAdapterPropertiesD3D* ToAPI(AdapterPropertiesD3D* rhs) {
        return reinterpret_cast<WGPUAdapterPropertiesD3D*>(rhs);
    }
    inline const AdapterPropertiesD3D* FromAPI(const WGPUAdapterPropertiesD3D* rhs) {
        return reinterpret_cast<const AdapterPropertiesD3D*>(rhs);
    }
    inline AdapterPropertiesD3D* FromAPI(WGPUAdapterPropertiesD3D* rhs) {
        return reinterpret_cast<AdapterPropertiesD3D*>(rhs);
    }
    inline const AdapterPropertiesD3D& FromAPI(const WGPUAdapterPropertiesD3D& rhs) {
        return *reinterpret_cast<const AdapterPropertiesD3D*>(&rhs);
    }
    inline AdapterPropertiesD3D& FromAPI(WGPUAdapterPropertiesD3D& rhs) {
        return *reinterpret_cast<AdapterPropertiesD3D*>(&rhs);
    }

    inline const WGPUAdapterPropertiesVk* ToAPI(const AdapterPropertiesVk* rhs) {
        return reinterpret_cast<const WGPUAdapterPropertiesVk*>(rhs);
    }
    inline WGPUAdapterPropertiesVk* ToAPI(AdapterPropertiesVk* rhs) {
        return reinterpret_cast<WGPUAdapterPropertiesVk*>(rhs);
    }
    inline const AdapterPropertiesVk* FromAPI(const WGPUAdapterPropertiesVk* rhs) {
        return reinterpret_cast<const AdapterPropertiesVk*>(rhs);
    }
    inline AdapterPropertiesVk* FromAPI(WGPUAdapterPropertiesVk* rhs) {
        return reinterpret_cast<AdapterPropertiesVk*>(rhs);
    }
    inline const AdapterPropertiesVk& FromAPI(const WGPUAdapterPropertiesVk& rhs) {
        return *reinterpret_cast<const AdapterPropertiesVk*>(&rhs);
    }
    inline AdapterPropertiesVk& FromAPI(WGPUAdapterPropertiesVk& rhs) {
        return *reinterpret_cast<AdapterPropertiesVk*>(&rhs);
    }

    inline const WGPUAdapterPropertiesWGPU* ToAPI(const AdapterPropertiesWGPU* rhs) {
        return reinterpret_cast<const WGPUAdapterPropertiesWGPU*>(rhs);
    }
    inline WGPUAdapterPropertiesWGPU* ToAPI(AdapterPropertiesWGPU* rhs) {
        return reinterpret_cast<WGPUAdapterPropertiesWGPU*>(rhs);
    }
    inline const AdapterPropertiesWGPU* FromAPI(const WGPUAdapterPropertiesWGPU* rhs) {
        return reinterpret_cast<const AdapterPropertiesWGPU*>(rhs);
    }
    inline AdapterPropertiesWGPU* FromAPI(WGPUAdapterPropertiesWGPU* rhs) {
        return reinterpret_cast<AdapterPropertiesWGPU*>(rhs);
    }
    inline const AdapterPropertiesWGPU& FromAPI(const WGPUAdapterPropertiesWGPU& rhs) {
        return *reinterpret_cast<const AdapterPropertiesWGPU*>(&rhs);
    }
    inline AdapterPropertiesWGPU& FromAPI(WGPUAdapterPropertiesWGPU& rhs) {
        return *reinterpret_cast<AdapterPropertiesWGPU*>(&rhs);
    }

    inline const WGPUBindingResource* ToAPI(const BindingResource* rhs) {
        return reinterpret_cast<const WGPUBindingResource*>(rhs);
    }
    inline WGPUBindingResource* ToAPI(BindingResource* rhs) {
        return reinterpret_cast<WGPUBindingResource*>(rhs);
    }
    inline const BindingResource* FromAPI(const WGPUBindingResource* rhs) {
        return reinterpret_cast<const BindingResource*>(rhs);
    }
    inline BindingResource* FromAPI(WGPUBindingResource* rhs) {
        return reinterpret_cast<BindingResource*>(rhs);
    }
    inline const BindingResource& FromAPI(const WGPUBindingResource& rhs) {
        return *reinterpret_cast<const BindingResource*>(&rhs);
    }
    inline BindingResource& FromAPI(WGPUBindingResource& rhs) {
        return *reinterpret_cast<BindingResource*>(&rhs);
    }

    inline const WGPUBlendComponent* ToAPI(const BlendComponent* rhs) {
        return reinterpret_cast<const WGPUBlendComponent*>(rhs);
    }
    inline WGPUBlendComponent* ToAPI(BlendComponent* rhs) {
        return reinterpret_cast<WGPUBlendComponent*>(rhs);
    }
    inline const BlendComponent* FromAPI(const WGPUBlendComponent* rhs) {
        return reinterpret_cast<const BlendComponent*>(rhs);
    }
    inline BlendComponent* FromAPI(WGPUBlendComponent* rhs) {
        return reinterpret_cast<BlendComponent*>(rhs);
    }
    inline const BlendComponent& FromAPI(const WGPUBlendComponent& rhs) {
        return *reinterpret_cast<const BlendComponent*>(&rhs);
    }
    inline BlendComponent& FromAPI(WGPUBlendComponent& rhs) {
        return *reinterpret_cast<BlendComponent*>(&rhs);
    }

    inline const WGPUBufferBindingLayout* ToAPI(const BufferBindingLayout* rhs) {
        return reinterpret_cast<const WGPUBufferBindingLayout*>(rhs);
    }
    inline WGPUBufferBindingLayout* ToAPI(BufferBindingLayout* rhs) {
        return reinterpret_cast<WGPUBufferBindingLayout*>(rhs);
    }
    inline const BufferBindingLayout* FromAPI(const WGPUBufferBindingLayout* rhs) {
        return reinterpret_cast<const BufferBindingLayout*>(rhs);
    }
    inline BufferBindingLayout* FromAPI(WGPUBufferBindingLayout* rhs) {
        return reinterpret_cast<BufferBindingLayout*>(rhs);
    }
    inline const BufferBindingLayout& FromAPI(const WGPUBufferBindingLayout& rhs) {
        return *reinterpret_cast<const BufferBindingLayout*>(&rhs);
    }
    inline BufferBindingLayout& FromAPI(WGPUBufferBindingLayout& rhs) {
        return *reinterpret_cast<BufferBindingLayout*>(&rhs);
    }

    inline const WGPUBufferHostMappedPointer* ToAPI(const BufferHostMappedPointer* rhs) {
        return reinterpret_cast<const WGPUBufferHostMappedPointer*>(rhs);
    }
    inline WGPUBufferHostMappedPointer* ToAPI(BufferHostMappedPointer* rhs) {
        return reinterpret_cast<WGPUBufferHostMappedPointer*>(rhs);
    }
    inline const BufferHostMappedPointer* FromAPI(const WGPUBufferHostMappedPointer* rhs) {
        return reinterpret_cast<const BufferHostMappedPointer*>(rhs);
    }
    inline BufferHostMappedPointer* FromAPI(WGPUBufferHostMappedPointer* rhs) {
        return reinterpret_cast<BufferHostMappedPointer*>(rhs);
    }
    inline const BufferHostMappedPointer& FromAPI(const WGPUBufferHostMappedPointer& rhs) {
        return *reinterpret_cast<const BufferHostMappedPointer*>(&rhs);
    }
    inline BufferHostMappedPointer& FromAPI(WGPUBufferHostMappedPointer& rhs) {
        return *reinterpret_cast<BufferHostMappedPointer*>(&rhs);
    }

    inline const WGPUColor* ToAPI(const Color* rhs) {
        return reinterpret_cast<const WGPUColor*>(rhs);
    }
    inline WGPUColor* ToAPI(Color* rhs) {
        return reinterpret_cast<WGPUColor*>(rhs);
    }
    inline const Color* FromAPI(const WGPUColor* rhs) {
        return reinterpret_cast<const Color*>(rhs);
    }
    inline Color* FromAPI(WGPUColor* rhs) {
        return reinterpret_cast<Color*>(rhs);
    }
    inline const Color& FromAPI(const WGPUColor& rhs) {
        return *reinterpret_cast<const Color*>(&rhs);
    }
    inline Color& FromAPI(WGPUColor& rhs) {
        return *reinterpret_cast<Color*>(&rhs);
    }

    inline const WGPUColorSpaceDawn* ToAPI(const ColorSpaceDawn* rhs) {
        return reinterpret_cast<const WGPUColorSpaceDawn*>(rhs);
    }
    inline WGPUColorSpaceDawn* ToAPI(ColorSpaceDawn* rhs) {
        return reinterpret_cast<WGPUColorSpaceDawn*>(rhs);
    }
    inline const ColorSpaceDawn* FromAPI(const WGPUColorSpaceDawn* rhs) {
        return reinterpret_cast<const ColorSpaceDawn*>(rhs);
    }
    inline ColorSpaceDawn* FromAPI(WGPUColorSpaceDawn* rhs) {
        return reinterpret_cast<ColorSpaceDawn*>(rhs);
    }
    inline const ColorSpaceDawn& FromAPI(const WGPUColorSpaceDawn& rhs) {
        return *reinterpret_cast<const ColorSpaceDawn*>(&rhs);
    }
    inline ColorSpaceDawn& FromAPI(WGPUColorSpaceDawn& rhs) {
        return *reinterpret_cast<ColorSpaceDawn*>(&rhs);
    }

    inline const WGPUColorTargetStateExpandResolveTextureDawn* ToAPI(const ColorTargetStateExpandResolveTextureDawn* rhs) {
        return reinterpret_cast<const WGPUColorTargetStateExpandResolveTextureDawn*>(rhs);
    }
    inline WGPUColorTargetStateExpandResolveTextureDawn* ToAPI(ColorTargetStateExpandResolveTextureDawn* rhs) {
        return reinterpret_cast<WGPUColorTargetStateExpandResolveTextureDawn*>(rhs);
    }
    inline const ColorTargetStateExpandResolveTextureDawn* FromAPI(const WGPUColorTargetStateExpandResolveTextureDawn* rhs) {
        return reinterpret_cast<const ColorTargetStateExpandResolveTextureDawn*>(rhs);
    }
    inline ColorTargetStateExpandResolveTextureDawn* FromAPI(WGPUColorTargetStateExpandResolveTextureDawn* rhs) {
        return reinterpret_cast<ColorTargetStateExpandResolveTextureDawn*>(rhs);
    }
    inline const ColorTargetStateExpandResolveTextureDawn& FromAPI(const WGPUColorTargetStateExpandResolveTextureDawn& rhs) {
        return *reinterpret_cast<const ColorTargetStateExpandResolveTextureDawn*>(&rhs);
    }
    inline ColorTargetStateExpandResolveTextureDawn& FromAPI(WGPUColorTargetStateExpandResolveTextureDawn& rhs) {
        return *reinterpret_cast<ColorTargetStateExpandResolveTextureDawn*>(&rhs);
    }

    inline const WGPUCommandBufferDescriptor* ToAPI(const CommandBufferDescriptor* rhs) {
        return reinterpret_cast<const WGPUCommandBufferDescriptor*>(rhs);
    }
    inline WGPUCommandBufferDescriptor* ToAPI(CommandBufferDescriptor* rhs) {
        return reinterpret_cast<WGPUCommandBufferDescriptor*>(rhs);
    }
    inline const CommandBufferDescriptor* FromAPI(const WGPUCommandBufferDescriptor* rhs) {
        return reinterpret_cast<const CommandBufferDescriptor*>(rhs);
    }
    inline CommandBufferDescriptor* FromAPI(WGPUCommandBufferDescriptor* rhs) {
        return reinterpret_cast<CommandBufferDescriptor*>(rhs);
    }
    inline const CommandBufferDescriptor& FromAPI(const WGPUCommandBufferDescriptor& rhs) {
        return *reinterpret_cast<const CommandBufferDescriptor*>(&rhs);
    }
    inline CommandBufferDescriptor& FromAPI(WGPUCommandBufferDescriptor& rhs) {
        return *reinterpret_cast<CommandBufferDescriptor*>(&rhs);
    }

    inline const WGPUCompatibilityModeLimits* ToAPI(const CompatibilityModeLimits* rhs) {
        return reinterpret_cast<const WGPUCompatibilityModeLimits*>(rhs);
    }
    inline WGPUCompatibilityModeLimits* ToAPI(CompatibilityModeLimits* rhs) {
        return reinterpret_cast<WGPUCompatibilityModeLimits*>(rhs);
    }
    inline const CompatibilityModeLimits* FromAPI(const WGPUCompatibilityModeLimits* rhs) {
        return reinterpret_cast<const CompatibilityModeLimits*>(rhs);
    }
    inline CompatibilityModeLimits* FromAPI(WGPUCompatibilityModeLimits* rhs) {
        return reinterpret_cast<CompatibilityModeLimits*>(rhs);
    }
    inline const CompatibilityModeLimits& FromAPI(const WGPUCompatibilityModeLimits& rhs) {
        return *reinterpret_cast<const CompatibilityModeLimits*>(&rhs);
    }
    inline CompatibilityModeLimits& FromAPI(WGPUCompatibilityModeLimits& rhs) {
        return *reinterpret_cast<CompatibilityModeLimits*>(&rhs);
    }

    inline const WGPUConstantEntry* ToAPI(const ConstantEntry* rhs) {
        return reinterpret_cast<const WGPUConstantEntry*>(rhs);
    }
    inline WGPUConstantEntry* ToAPI(ConstantEntry* rhs) {
        return reinterpret_cast<WGPUConstantEntry*>(rhs);
    }
    inline const ConstantEntry* FromAPI(const WGPUConstantEntry* rhs) {
        return reinterpret_cast<const ConstantEntry*>(rhs);
    }
    inline ConstantEntry* FromAPI(WGPUConstantEntry* rhs) {
        return reinterpret_cast<ConstantEntry*>(rhs);
    }
    inline const ConstantEntry& FromAPI(const WGPUConstantEntry& rhs) {
        return *reinterpret_cast<const ConstantEntry*>(&rhs);
    }
    inline ConstantEntry& FromAPI(WGPUConstantEntry& rhs) {
        return *reinterpret_cast<ConstantEntry*>(&rhs);
    }

    inline const WGPUCopyTextureForBrowserOptions* ToAPI(const CopyTextureForBrowserOptions* rhs) {
        return reinterpret_cast<const WGPUCopyTextureForBrowserOptions*>(rhs);
    }
    inline WGPUCopyTextureForBrowserOptions* ToAPI(CopyTextureForBrowserOptions* rhs) {
        return reinterpret_cast<WGPUCopyTextureForBrowserOptions*>(rhs);
    }
    inline const CopyTextureForBrowserOptions* FromAPI(const WGPUCopyTextureForBrowserOptions* rhs) {
        return reinterpret_cast<const CopyTextureForBrowserOptions*>(rhs);
    }
    inline CopyTextureForBrowserOptions* FromAPI(WGPUCopyTextureForBrowserOptions* rhs) {
        return reinterpret_cast<CopyTextureForBrowserOptions*>(rhs);
    }
    inline const CopyTextureForBrowserOptions& FromAPI(const WGPUCopyTextureForBrowserOptions& rhs) {
        return *reinterpret_cast<const CopyTextureForBrowserOptions*>(&rhs);
    }
    inline CopyTextureForBrowserOptions& FromAPI(WGPUCopyTextureForBrowserOptions& rhs) {
        return *reinterpret_cast<CopyTextureForBrowserOptions*>(&rhs);
    }

    inline const WGPUDawnAdapterPropertiesPowerPreference* ToAPI(const DawnAdapterPropertiesPowerPreference* rhs) {
        return reinterpret_cast<const WGPUDawnAdapterPropertiesPowerPreference*>(rhs);
    }
    inline WGPUDawnAdapterPropertiesPowerPreference* ToAPI(DawnAdapterPropertiesPowerPreference* rhs) {
        return reinterpret_cast<WGPUDawnAdapterPropertiesPowerPreference*>(rhs);
    }
    inline const DawnAdapterPropertiesPowerPreference* FromAPI(const WGPUDawnAdapterPropertiesPowerPreference* rhs) {
        return reinterpret_cast<const DawnAdapterPropertiesPowerPreference*>(rhs);
    }
    inline DawnAdapterPropertiesPowerPreference* FromAPI(WGPUDawnAdapterPropertiesPowerPreference* rhs) {
        return reinterpret_cast<DawnAdapterPropertiesPowerPreference*>(rhs);
    }
    inline const DawnAdapterPropertiesPowerPreference& FromAPI(const WGPUDawnAdapterPropertiesPowerPreference& rhs) {
        return *reinterpret_cast<const DawnAdapterPropertiesPowerPreference*>(&rhs);
    }
    inline DawnAdapterPropertiesPowerPreference& FromAPI(WGPUDawnAdapterPropertiesPowerPreference& rhs) {
        return *reinterpret_cast<DawnAdapterPropertiesPowerPreference*>(&rhs);
    }

    inline const WGPUDawnBufferDescriptorErrorInfoFromWireClient* ToAPI(const DawnBufferDescriptorErrorInfoFromWireClient* rhs) {
        return reinterpret_cast<const WGPUDawnBufferDescriptorErrorInfoFromWireClient*>(rhs);
    }
    inline WGPUDawnBufferDescriptorErrorInfoFromWireClient* ToAPI(DawnBufferDescriptorErrorInfoFromWireClient* rhs) {
        return reinterpret_cast<WGPUDawnBufferDescriptorErrorInfoFromWireClient*>(rhs);
    }
    inline const DawnBufferDescriptorErrorInfoFromWireClient* FromAPI(const WGPUDawnBufferDescriptorErrorInfoFromWireClient* rhs) {
        return reinterpret_cast<const DawnBufferDescriptorErrorInfoFromWireClient*>(rhs);
    }
    inline DawnBufferDescriptorErrorInfoFromWireClient* FromAPI(WGPUDawnBufferDescriptorErrorInfoFromWireClient* rhs) {
        return reinterpret_cast<DawnBufferDescriptorErrorInfoFromWireClient*>(rhs);
    }
    inline const DawnBufferDescriptorErrorInfoFromWireClient& FromAPI(const WGPUDawnBufferDescriptorErrorInfoFromWireClient& rhs) {
        return *reinterpret_cast<const DawnBufferDescriptorErrorInfoFromWireClient*>(&rhs);
    }
    inline DawnBufferDescriptorErrorInfoFromWireClient& FromAPI(WGPUDawnBufferDescriptorErrorInfoFromWireClient& rhs) {
        return *reinterpret_cast<DawnBufferDescriptorErrorInfoFromWireClient*>(&rhs);
    }

    inline const WGPUDawnCompilationMessageUtf16* ToAPI(const DawnCompilationMessageUtf16* rhs) {
        return reinterpret_cast<const WGPUDawnCompilationMessageUtf16*>(rhs);
    }
    inline WGPUDawnCompilationMessageUtf16* ToAPI(DawnCompilationMessageUtf16* rhs) {
        return reinterpret_cast<WGPUDawnCompilationMessageUtf16*>(rhs);
    }
    inline const DawnCompilationMessageUtf16* FromAPI(const WGPUDawnCompilationMessageUtf16* rhs) {
        return reinterpret_cast<const DawnCompilationMessageUtf16*>(rhs);
    }
    inline DawnCompilationMessageUtf16* FromAPI(WGPUDawnCompilationMessageUtf16* rhs) {
        return reinterpret_cast<DawnCompilationMessageUtf16*>(rhs);
    }
    inline const DawnCompilationMessageUtf16& FromAPI(const WGPUDawnCompilationMessageUtf16& rhs) {
        return *reinterpret_cast<const DawnCompilationMessageUtf16*>(&rhs);
    }
    inline DawnCompilationMessageUtf16& FromAPI(WGPUDawnCompilationMessageUtf16& rhs) {
        return *reinterpret_cast<DawnCompilationMessageUtf16*>(&rhs);
    }

    inline const WGPUDawnConsumeAdapterDescriptor* ToAPI(const DawnConsumeAdapterDescriptor* rhs) {
        return reinterpret_cast<const WGPUDawnConsumeAdapterDescriptor*>(rhs);
    }
    inline WGPUDawnConsumeAdapterDescriptor* ToAPI(DawnConsumeAdapterDescriptor* rhs) {
        return reinterpret_cast<WGPUDawnConsumeAdapterDescriptor*>(rhs);
    }
    inline const DawnConsumeAdapterDescriptor* FromAPI(const WGPUDawnConsumeAdapterDescriptor* rhs) {
        return reinterpret_cast<const DawnConsumeAdapterDescriptor*>(rhs);
    }
    inline DawnConsumeAdapterDescriptor* FromAPI(WGPUDawnConsumeAdapterDescriptor* rhs) {
        return reinterpret_cast<DawnConsumeAdapterDescriptor*>(rhs);
    }
    inline const DawnConsumeAdapterDescriptor& FromAPI(const WGPUDawnConsumeAdapterDescriptor& rhs) {
        return *reinterpret_cast<const DawnConsumeAdapterDescriptor*>(&rhs);
    }
    inline DawnConsumeAdapterDescriptor& FromAPI(WGPUDawnConsumeAdapterDescriptor& rhs) {
        return *reinterpret_cast<DawnConsumeAdapterDescriptor*>(&rhs);
    }

    inline const WGPUDawnDeviceAllocatorControl* ToAPI(const DawnDeviceAllocatorControl* rhs) {
        return reinterpret_cast<const WGPUDawnDeviceAllocatorControl*>(rhs);
    }
    inline WGPUDawnDeviceAllocatorControl* ToAPI(DawnDeviceAllocatorControl* rhs) {
        return reinterpret_cast<WGPUDawnDeviceAllocatorControl*>(rhs);
    }
    inline const DawnDeviceAllocatorControl* FromAPI(const WGPUDawnDeviceAllocatorControl* rhs) {
        return reinterpret_cast<const DawnDeviceAllocatorControl*>(rhs);
    }
    inline DawnDeviceAllocatorControl* FromAPI(WGPUDawnDeviceAllocatorControl* rhs) {
        return reinterpret_cast<DawnDeviceAllocatorControl*>(rhs);
    }
    inline const DawnDeviceAllocatorControl& FromAPI(const WGPUDawnDeviceAllocatorControl& rhs) {
        return *reinterpret_cast<const DawnDeviceAllocatorControl*>(&rhs);
    }
    inline DawnDeviceAllocatorControl& FromAPI(WGPUDawnDeviceAllocatorControl& rhs) {
        return *reinterpret_cast<DawnDeviceAllocatorControl*>(&rhs);
    }

    inline const WGPUDawnDrmFormatProperties* ToAPI(const DawnDrmFormatProperties* rhs) {
        return reinterpret_cast<const WGPUDawnDrmFormatProperties*>(rhs);
    }
    inline WGPUDawnDrmFormatProperties* ToAPI(DawnDrmFormatProperties* rhs) {
        return reinterpret_cast<WGPUDawnDrmFormatProperties*>(rhs);
    }
    inline const DawnDrmFormatProperties* FromAPI(const WGPUDawnDrmFormatProperties* rhs) {
        return reinterpret_cast<const DawnDrmFormatProperties*>(rhs);
    }
    inline DawnDrmFormatProperties* FromAPI(WGPUDawnDrmFormatProperties* rhs) {
        return reinterpret_cast<DawnDrmFormatProperties*>(rhs);
    }
    inline const DawnDrmFormatProperties& FromAPI(const WGPUDawnDrmFormatProperties& rhs) {
        return *reinterpret_cast<const DawnDrmFormatProperties*>(&rhs);
    }
    inline DawnDrmFormatProperties& FromAPI(WGPUDawnDrmFormatProperties& rhs) {
        return *reinterpret_cast<DawnDrmFormatProperties*>(&rhs);
    }

    inline const WGPUDawnEncoderInternalUsageDescriptor* ToAPI(const DawnEncoderInternalUsageDescriptor* rhs) {
        return reinterpret_cast<const WGPUDawnEncoderInternalUsageDescriptor*>(rhs);
    }
    inline WGPUDawnEncoderInternalUsageDescriptor* ToAPI(DawnEncoderInternalUsageDescriptor* rhs) {
        return reinterpret_cast<WGPUDawnEncoderInternalUsageDescriptor*>(rhs);
    }
    inline const DawnEncoderInternalUsageDescriptor* FromAPI(const WGPUDawnEncoderInternalUsageDescriptor* rhs) {
        return reinterpret_cast<const DawnEncoderInternalUsageDescriptor*>(rhs);
    }
    inline DawnEncoderInternalUsageDescriptor* FromAPI(WGPUDawnEncoderInternalUsageDescriptor* rhs) {
        return reinterpret_cast<DawnEncoderInternalUsageDescriptor*>(rhs);
    }
    inline const DawnEncoderInternalUsageDescriptor& FromAPI(const WGPUDawnEncoderInternalUsageDescriptor& rhs) {
        return *reinterpret_cast<const DawnEncoderInternalUsageDescriptor*>(&rhs);
    }
    inline DawnEncoderInternalUsageDescriptor& FromAPI(WGPUDawnEncoderInternalUsageDescriptor& rhs) {
        return *reinterpret_cast<DawnEncoderInternalUsageDescriptor*>(&rhs);
    }

    inline const WGPUDawnFakeBufferOOMForTesting* ToAPI(const DawnFakeBufferOOMForTesting* rhs) {
        return reinterpret_cast<const WGPUDawnFakeBufferOOMForTesting*>(rhs);
    }
    inline WGPUDawnFakeBufferOOMForTesting* ToAPI(DawnFakeBufferOOMForTesting* rhs) {
        return reinterpret_cast<WGPUDawnFakeBufferOOMForTesting*>(rhs);
    }
    inline const DawnFakeBufferOOMForTesting* FromAPI(const WGPUDawnFakeBufferOOMForTesting* rhs) {
        return reinterpret_cast<const DawnFakeBufferOOMForTesting*>(rhs);
    }
    inline DawnFakeBufferOOMForTesting* FromAPI(WGPUDawnFakeBufferOOMForTesting* rhs) {
        return reinterpret_cast<DawnFakeBufferOOMForTesting*>(rhs);
    }
    inline const DawnFakeBufferOOMForTesting& FromAPI(const WGPUDawnFakeBufferOOMForTesting& rhs) {
        return *reinterpret_cast<const DawnFakeBufferOOMForTesting*>(&rhs);
    }
    inline DawnFakeBufferOOMForTesting& FromAPI(WGPUDawnFakeBufferOOMForTesting& rhs) {
        return *reinterpret_cast<DawnFakeBufferOOMForTesting*>(&rhs);
    }

    inline const WGPUDawnFakeDeviceInitializeErrorForTesting* ToAPI(const DawnFakeDeviceInitializeErrorForTesting* rhs) {
        return reinterpret_cast<const WGPUDawnFakeDeviceInitializeErrorForTesting*>(rhs);
    }
    inline WGPUDawnFakeDeviceInitializeErrorForTesting* ToAPI(DawnFakeDeviceInitializeErrorForTesting* rhs) {
        return reinterpret_cast<WGPUDawnFakeDeviceInitializeErrorForTesting*>(rhs);
    }
    inline const DawnFakeDeviceInitializeErrorForTesting* FromAPI(const WGPUDawnFakeDeviceInitializeErrorForTesting* rhs) {
        return reinterpret_cast<const DawnFakeDeviceInitializeErrorForTesting*>(rhs);
    }
    inline DawnFakeDeviceInitializeErrorForTesting* FromAPI(WGPUDawnFakeDeviceInitializeErrorForTesting* rhs) {
        return reinterpret_cast<DawnFakeDeviceInitializeErrorForTesting*>(rhs);
    }
    inline const DawnFakeDeviceInitializeErrorForTesting& FromAPI(const WGPUDawnFakeDeviceInitializeErrorForTesting& rhs) {
        return *reinterpret_cast<const DawnFakeDeviceInitializeErrorForTesting*>(&rhs);
    }
    inline DawnFakeDeviceInitializeErrorForTesting& FromAPI(WGPUDawnFakeDeviceInitializeErrorForTesting& rhs) {
        return *reinterpret_cast<DawnFakeDeviceInitializeErrorForTesting*>(&rhs);
    }

    inline const WGPUDawnHostMappedPointerLimits* ToAPI(const DawnHostMappedPointerLimits* rhs) {
        return reinterpret_cast<const WGPUDawnHostMappedPointerLimits*>(rhs);
    }
    inline WGPUDawnHostMappedPointerLimits* ToAPI(DawnHostMappedPointerLimits* rhs) {
        return reinterpret_cast<WGPUDawnHostMappedPointerLimits*>(rhs);
    }
    inline const DawnHostMappedPointerLimits* FromAPI(const WGPUDawnHostMappedPointerLimits* rhs) {
        return reinterpret_cast<const DawnHostMappedPointerLimits*>(rhs);
    }
    inline DawnHostMappedPointerLimits* FromAPI(WGPUDawnHostMappedPointerLimits* rhs) {
        return reinterpret_cast<DawnHostMappedPointerLimits*>(rhs);
    }
    inline const DawnHostMappedPointerLimits& FromAPI(const WGPUDawnHostMappedPointerLimits& rhs) {
        return *reinterpret_cast<const DawnHostMappedPointerLimits*>(&rhs);
    }
    inline DawnHostMappedPointerLimits& FromAPI(WGPUDawnHostMappedPointerLimits& rhs) {
        return *reinterpret_cast<DawnHostMappedPointerLimits*>(&rhs);
    }

    inline const WGPUDawnInjectedInvalidSType* ToAPI(const DawnInjectedInvalidSType* rhs) {
        return reinterpret_cast<const WGPUDawnInjectedInvalidSType*>(rhs);
    }
    inline WGPUDawnInjectedInvalidSType* ToAPI(DawnInjectedInvalidSType* rhs) {
        return reinterpret_cast<WGPUDawnInjectedInvalidSType*>(rhs);
    }
    inline const DawnInjectedInvalidSType* FromAPI(const WGPUDawnInjectedInvalidSType* rhs) {
        return reinterpret_cast<const DawnInjectedInvalidSType*>(rhs);
    }
    inline DawnInjectedInvalidSType* FromAPI(WGPUDawnInjectedInvalidSType* rhs) {
        return reinterpret_cast<DawnInjectedInvalidSType*>(rhs);
    }
    inline const DawnInjectedInvalidSType& FromAPI(const WGPUDawnInjectedInvalidSType& rhs) {
        return *reinterpret_cast<const DawnInjectedInvalidSType*>(&rhs);
    }
    inline DawnInjectedInvalidSType& FromAPI(WGPUDawnInjectedInvalidSType& rhs) {
        return *reinterpret_cast<DawnInjectedInvalidSType*>(&rhs);
    }

    inline const WGPUDawnRenderPassSampleCount* ToAPI(const DawnRenderPassSampleCount* rhs) {
        return reinterpret_cast<const WGPUDawnRenderPassSampleCount*>(rhs);
    }
    inline WGPUDawnRenderPassSampleCount* ToAPI(DawnRenderPassSampleCount* rhs) {
        return reinterpret_cast<WGPUDawnRenderPassSampleCount*>(rhs);
    }
    inline const DawnRenderPassSampleCount* FromAPI(const WGPUDawnRenderPassSampleCount* rhs) {
        return reinterpret_cast<const DawnRenderPassSampleCount*>(rhs);
    }
    inline DawnRenderPassSampleCount* FromAPI(WGPUDawnRenderPassSampleCount* rhs) {
        return reinterpret_cast<DawnRenderPassSampleCount*>(rhs);
    }
    inline const DawnRenderPassSampleCount& FromAPI(const WGPUDawnRenderPassSampleCount& rhs) {
        return *reinterpret_cast<const DawnRenderPassSampleCount*>(&rhs);
    }
    inline DawnRenderPassSampleCount& FromAPI(WGPUDawnRenderPassSampleCount& rhs) {
        return *reinterpret_cast<DawnRenderPassSampleCount*>(&rhs);
    }

    inline const WGPUDawnShaderModuleSPIRVOptionsDescriptor* ToAPI(const DawnShaderModuleSPIRVOptionsDescriptor* rhs) {
        return reinterpret_cast<const WGPUDawnShaderModuleSPIRVOptionsDescriptor*>(rhs);
    }
    inline WGPUDawnShaderModuleSPIRVOptionsDescriptor* ToAPI(DawnShaderModuleSPIRVOptionsDescriptor* rhs) {
        return reinterpret_cast<WGPUDawnShaderModuleSPIRVOptionsDescriptor*>(rhs);
    }
    inline const DawnShaderModuleSPIRVOptionsDescriptor* FromAPI(const WGPUDawnShaderModuleSPIRVOptionsDescriptor* rhs) {
        return reinterpret_cast<const DawnShaderModuleSPIRVOptionsDescriptor*>(rhs);
    }
    inline DawnShaderModuleSPIRVOptionsDescriptor* FromAPI(WGPUDawnShaderModuleSPIRVOptionsDescriptor* rhs) {
        return reinterpret_cast<DawnShaderModuleSPIRVOptionsDescriptor*>(rhs);
    }
    inline const DawnShaderModuleSPIRVOptionsDescriptor& FromAPI(const WGPUDawnShaderModuleSPIRVOptionsDescriptor& rhs) {
        return *reinterpret_cast<const DawnShaderModuleSPIRVOptionsDescriptor*>(&rhs);
    }
    inline DawnShaderModuleSPIRVOptionsDescriptor& FromAPI(WGPUDawnShaderModuleSPIRVOptionsDescriptor& rhs) {
        return *reinterpret_cast<DawnShaderModuleSPIRVOptionsDescriptor*>(&rhs);
    }

    inline const WGPUDawnTexelCopyBufferRowAlignmentLimits* ToAPI(const DawnTexelCopyBufferRowAlignmentLimits* rhs) {
        return reinterpret_cast<const WGPUDawnTexelCopyBufferRowAlignmentLimits*>(rhs);
    }
    inline WGPUDawnTexelCopyBufferRowAlignmentLimits* ToAPI(DawnTexelCopyBufferRowAlignmentLimits* rhs) {
        return reinterpret_cast<WGPUDawnTexelCopyBufferRowAlignmentLimits*>(rhs);
    }
    inline const DawnTexelCopyBufferRowAlignmentLimits* FromAPI(const WGPUDawnTexelCopyBufferRowAlignmentLimits* rhs) {
        return reinterpret_cast<const DawnTexelCopyBufferRowAlignmentLimits*>(rhs);
    }
    inline DawnTexelCopyBufferRowAlignmentLimits* FromAPI(WGPUDawnTexelCopyBufferRowAlignmentLimits* rhs) {
        return reinterpret_cast<DawnTexelCopyBufferRowAlignmentLimits*>(rhs);
    }
    inline const DawnTexelCopyBufferRowAlignmentLimits& FromAPI(const WGPUDawnTexelCopyBufferRowAlignmentLimits& rhs) {
        return *reinterpret_cast<const DawnTexelCopyBufferRowAlignmentLimits*>(&rhs);
    }
    inline DawnTexelCopyBufferRowAlignmentLimits& FromAPI(WGPUDawnTexelCopyBufferRowAlignmentLimits& rhs) {
        return *reinterpret_cast<DawnTexelCopyBufferRowAlignmentLimits*>(&rhs);
    }

    inline const WGPUDawnTextureInternalUsageDescriptor* ToAPI(const DawnTextureInternalUsageDescriptor* rhs) {
        return reinterpret_cast<const WGPUDawnTextureInternalUsageDescriptor*>(rhs);
    }
    inline WGPUDawnTextureInternalUsageDescriptor* ToAPI(DawnTextureInternalUsageDescriptor* rhs) {
        return reinterpret_cast<WGPUDawnTextureInternalUsageDescriptor*>(rhs);
    }
    inline const DawnTextureInternalUsageDescriptor* FromAPI(const WGPUDawnTextureInternalUsageDescriptor* rhs) {
        return reinterpret_cast<const DawnTextureInternalUsageDescriptor*>(rhs);
    }
    inline DawnTextureInternalUsageDescriptor* FromAPI(WGPUDawnTextureInternalUsageDescriptor* rhs) {
        return reinterpret_cast<DawnTextureInternalUsageDescriptor*>(rhs);
    }
    inline const DawnTextureInternalUsageDescriptor& FromAPI(const WGPUDawnTextureInternalUsageDescriptor& rhs) {
        return *reinterpret_cast<const DawnTextureInternalUsageDescriptor*>(&rhs);
    }
    inline DawnTextureInternalUsageDescriptor& FromAPI(WGPUDawnTextureInternalUsageDescriptor& rhs) {
        return *reinterpret_cast<DawnTextureInternalUsageDescriptor*>(&rhs);
    }

    inline const WGPUDawnWireWGSLControl* ToAPI(const DawnWireWGSLControl* rhs) {
        return reinterpret_cast<const WGPUDawnWireWGSLControl*>(rhs);
    }
    inline WGPUDawnWireWGSLControl* ToAPI(DawnWireWGSLControl* rhs) {
        return reinterpret_cast<WGPUDawnWireWGSLControl*>(rhs);
    }
    inline const DawnWireWGSLControl* FromAPI(const WGPUDawnWireWGSLControl* rhs) {
        return reinterpret_cast<const DawnWireWGSLControl*>(rhs);
    }
    inline DawnWireWGSLControl* FromAPI(WGPUDawnWireWGSLControl* rhs) {
        return reinterpret_cast<DawnWireWGSLControl*>(rhs);
    }
    inline const DawnWireWGSLControl& FromAPI(const WGPUDawnWireWGSLControl& rhs) {
        return *reinterpret_cast<const DawnWireWGSLControl*>(&rhs);
    }
    inline DawnWireWGSLControl& FromAPI(WGPUDawnWireWGSLControl& rhs) {
        return *reinterpret_cast<DawnWireWGSLControl*>(&rhs);
    }

    inline const WGPUExtent2D* ToAPI(const Extent2D* rhs) {
        return reinterpret_cast<const WGPUExtent2D*>(rhs);
    }
    inline WGPUExtent2D* ToAPI(Extent2D* rhs) {
        return reinterpret_cast<WGPUExtent2D*>(rhs);
    }
    inline const Extent2D* FromAPI(const WGPUExtent2D* rhs) {
        return reinterpret_cast<const Extent2D*>(rhs);
    }
    inline Extent2D* FromAPI(WGPUExtent2D* rhs) {
        return reinterpret_cast<Extent2D*>(rhs);
    }
    inline const Extent2D& FromAPI(const WGPUExtent2D& rhs) {
        return *reinterpret_cast<const Extent2D*>(&rhs);
    }
    inline Extent2D& FromAPI(WGPUExtent2D& rhs) {
        return *reinterpret_cast<Extent2D*>(&rhs);
    }

    inline const WGPUExtent3D* ToAPI(const Extent3D* rhs) {
        return reinterpret_cast<const WGPUExtent3D*>(rhs);
    }
    inline WGPUExtent3D* ToAPI(Extent3D* rhs) {
        return reinterpret_cast<WGPUExtent3D*>(rhs);
    }
    inline const Extent3D* FromAPI(const WGPUExtent3D* rhs) {
        return reinterpret_cast<const Extent3D*>(rhs);
    }
    inline Extent3D* FromAPI(WGPUExtent3D* rhs) {
        return reinterpret_cast<Extent3D*>(rhs);
    }
    inline const Extent3D& FromAPI(const WGPUExtent3D& rhs) {
        return *reinterpret_cast<const Extent3D*>(&rhs);
    }
    inline Extent3D& FromAPI(WGPUExtent3D& rhs) {
        return *reinterpret_cast<Extent3D*>(&rhs);
    }

    inline const WGPUExternalTextureBindingEntry* ToAPI(const ExternalTextureBindingEntry* rhs) {
        return reinterpret_cast<const WGPUExternalTextureBindingEntry*>(rhs);
    }
    inline WGPUExternalTextureBindingEntry* ToAPI(ExternalTextureBindingEntry* rhs) {
        return reinterpret_cast<WGPUExternalTextureBindingEntry*>(rhs);
    }
    inline const ExternalTextureBindingEntry* FromAPI(const WGPUExternalTextureBindingEntry* rhs) {
        return reinterpret_cast<const ExternalTextureBindingEntry*>(rhs);
    }
    inline ExternalTextureBindingEntry* FromAPI(WGPUExternalTextureBindingEntry* rhs) {
        return reinterpret_cast<ExternalTextureBindingEntry*>(rhs);
    }
    inline const ExternalTextureBindingEntry& FromAPI(const WGPUExternalTextureBindingEntry& rhs) {
        return *reinterpret_cast<const ExternalTextureBindingEntry*>(&rhs);
    }
    inline ExternalTextureBindingEntry& FromAPI(WGPUExternalTextureBindingEntry& rhs) {
        return *reinterpret_cast<ExternalTextureBindingEntry*>(&rhs);
    }

    inline const WGPUExternalTextureBindingLayout* ToAPI(const ExternalTextureBindingLayout* rhs) {
        return reinterpret_cast<const WGPUExternalTextureBindingLayout*>(rhs);
    }
    inline WGPUExternalTextureBindingLayout* ToAPI(ExternalTextureBindingLayout* rhs) {
        return reinterpret_cast<WGPUExternalTextureBindingLayout*>(rhs);
    }
    inline const ExternalTextureBindingLayout* FromAPI(const WGPUExternalTextureBindingLayout* rhs) {
        return reinterpret_cast<const ExternalTextureBindingLayout*>(rhs);
    }
    inline ExternalTextureBindingLayout* FromAPI(WGPUExternalTextureBindingLayout* rhs) {
        return reinterpret_cast<ExternalTextureBindingLayout*>(rhs);
    }
    inline const ExternalTextureBindingLayout& FromAPI(const WGPUExternalTextureBindingLayout& rhs) {
        return *reinterpret_cast<const ExternalTextureBindingLayout*>(&rhs);
    }
    inline ExternalTextureBindingLayout& FromAPI(WGPUExternalTextureBindingLayout& rhs) {
        return *reinterpret_cast<ExternalTextureBindingLayout*>(&rhs);
    }

    inline const WGPUFuture* ToAPI(const Future* rhs) {
        return reinterpret_cast<const WGPUFuture*>(rhs);
    }
    inline WGPUFuture* ToAPI(Future* rhs) {
        return reinterpret_cast<WGPUFuture*>(rhs);
    }
    inline const Future* FromAPI(const WGPUFuture* rhs) {
        return reinterpret_cast<const Future*>(rhs);
    }
    inline Future* FromAPI(WGPUFuture* rhs) {
        return reinterpret_cast<Future*>(rhs);
    }
    inline const Future& FromAPI(const WGPUFuture& rhs) {
        return *reinterpret_cast<const Future*>(&rhs);
    }
    inline Future& FromAPI(WGPUFuture& rhs) {
        return *reinterpret_cast<Future*>(&rhs);
    }

    inline const WGPUInstanceLimits* ToAPI(const InstanceLimits* rhs) {
        return reinterpret_cast<const WGPUInstanceLimits*>(rhs);
    }
    inline WGPUInstanceLimits* ToAPI(InstanceLimits* rhs) {
        return reinterpret_cast<WGPUInstanceLimits*>(rhs);
    }
    inline const InstanceLimits* FromAPI(const WGPUInstanceLimits* rhs) {
        return reinterpret_cast<const InstanceLimits*>(rhs);
    }
    inline InstanceLimits* FromAPI(WGPUInstanceLimits* rhs) {
        return reinterpret_cast<InstanceLimits*>(rhs);
    }
    inline const InstanceLimits& FromAPI(const WGPUInstanceLimits& rhs) {
        return *reinterpret_cast<const InstanceLimits*>(&rhs);
    }
    inline InstanceLimits& FromAPI(WGPUInstanceLimits& rhs) {
        return *reinterpret_cast<InstanceLimits*>(&rhs);
    }

    inline const WGPUMemoryHeapInfo* ToAPI(const MemoryHeapInfo* rhs) {
        return reinterpret_cast<const WGPUMemoryHeapInfo*>(rhs);
    }
    inline WGPUMemoryHeapInfo* ToAPI(MemoryHeapInfo* rhs) {
        return reinterpret_cast<WGPUMemoryHeapInfo*>(rhs);
    }
    inline const MemoryHeapInfo* FromAPI(const WGPUMemoryHeapInfo* rhs) {
        return reinterpret_cast<const MemoryHeapInfo*>(rhs);
    }
    inline MemoryHeapInfo* FromAPI(WGPUMemoryHeapInfo* rhs) {
        return reinterpret_cast<MemoryHeapInfo*>(rhs);
    }
    inline const MemoryHeapInfo& FromAPI(const WGPUMemoryHeapInfo& rhs) {
        return *reinterpret_cast<const MemoryHeapInfo*>(&rhs);
    }
    inline MemoryHeapInfo& FromAPI(WGPUMemoryHeapInfo& rhs) {
        return *reinterpret_cast<MemoryHeapInfo*>(&rhs);
    }

    inline const WGPUMultisampleState* ToAPI(const MultisampleState* rhs) {
        return reinterpret_cast<const WGPUMultisampleState*>(rhs);
    }
    inline WGPUMultisampleState* ToAPI(MultisampleState* rhs) {
        return reinterpret_cast<WGPUMultisampleState*>(rhs);
    }
    inline const MultisampleState* FromAPI(const WGPUMultisampleState* rhs) {
        return reinterpret_cast<const MultisampleState*>(rhs);
    }
    inline MultisampleState* FromAPI(WGPUMultisampleState* rhs) {
        return reinterpret_cast<MultisampleState*>(rhs);
    }
    inline const MultisampleState& FromAPI(const WGPUMultisampleState& rhs) {
        return *reinterpret_cast<const MultisampleState*>(&rhs);
    }
    inline MultisampleState& FromAPI(WGPUMultisampleState& rhs) {
        return *reinterpret_cast<MultisampleState*>(&rhs);
    }

    inline const WGPUOrigin2D* ToAPI(const Origin2D* rhs) {
        return reinterpret_cast<const WGPUOrigin2D*>(rhs);
    }
    inline WGPUOrigin2D* ToAPI(Origin2D* rhs) {
        return reinterpret_cast<WGPUOrigin2D*>(rhs);
    }
    inline const Origin2D* FromAPI(const WGPUOrigin2D* rhs) {
        return reinterpret_cast<const Origin2D*>(rhs);
    }
    inline Origin2D* FromAPI(WGPUOrigin2D* rhs) {
        return reinterpret_cast<Origin2D*>(rhs);
    }
    inline const Origin2D& FromAPI(const WGPUOrigin2D& rhs) {
        return *reinterpret_cast<const Origin2D*>(&rhs);
    }
    inline Origin2D& FromAPI(WGPUOrigin2D& rhs) {
        return *reinterpret_cast<Origin2D*>(&rhs);
    }

    inline const WGPUOrigin3D* ToAPI(const Origin3D* rhs) {
        return reinterpret_cast<const WGPUOrigin3D*>(rhs);
    }
    inline WGPUOrigin3D* ToAPI(Origin3D* rhs) {
        return reinterpret_cast<WGPUOrigin3D*>(rhs);
    }
    inline const Origin3D* FromAPI(const WGPUOrigin3D* rhs) {
        return reinterpret_cast<const Origin3D*>(rhs);
    }
    inline Origin3D* FromAPI(WGPUOrigin3D* rhs) {
        return reinterpret_cast<Origin3D*>(rhs);
    }
    inline const Origin3D& FromAPI(const WGPUOrigin3D& rhs) {
        return *reinterpret_cast<const Origin3D*>(&rhs);
    }
    inline Origin3D& FromAPI(WGPUOrigin3D& rhs) {
        return *reinterpret_cast<Origin3D*>(&rhs);
    }

    inline const WGPUPassTimestampWrites* ToAPI(const PassTimestampWrites* rhs) {
        return reinterpret_cast<const WGPUPassTimestampWrites*>(rhs);
    }
    inline WGPUPassTimestampWrites* ToAPI(PassTimestampWrites* rhs) {
        return reinterpret_cast<WGPUPassTimestampWrites*>(rhs);
    }
    inline const PassTimestampWrites* FromAPI(const WGPUPassTimestampWrites* rhs) {
        return reinterpret_cast<const PassTimestampWrites*>(rhs);
    }
    inline PassTimestampWrites* FromAPI(WGPUPassTimestampWrites* rhs) {
        return reinterpret_cast<PassTimestampWrites*>(rhs);
    }
    inline const PassTimestampWrites& FromAPI(const WGPUPassTimestampWrites& rhs) {
        return *reinterpret_cast<const PassTimestampWrites*>(&rhs);
    }
    inline PassTimestampWrites& FromAPI(WGPUPassTimestampWrites& rhs) {
        return *reinterpret_cast<PassTimestampWrites*>(&rhs);
    }

    inline const WGPUPipelineLayoutResourceTable* ToAPI(const PipelineLayoutResourceTable* rhs) {
        return reinterpret_cast<const WGPUPipelineLayoutResourceTable*>(rhs);
    }
    inline WGPUPipelineLayoutResourceTable* ToAPI(PipelineLayoutResourceTable* rhs) {
        return reinterpret_cast<WGPUPipelineLayoutResourceTable*>(rhs);
    }
    inline const PipelineLayoutResourceTable* FromAPI(const WGPUPipelineLayoutResourceTable* rhs) {
        return reinterpret_cast<const PipelineLayoutResourceTable*>(rhs);
    }
    inline PipelineLayoutResourceTable* FromAPI(WGPUPipelineLayoutResourceTable* rhs) {
        return reinterpret_cast<PipelineLayoutResourceTable*>(rhs);
    }
    inline const PipelineLayoutResourceTable& FromAPI(const WGPUPipelineLayoutResourceTable& rhs) {
        return *reinterpret_cast<const PipelineLayoutResourceTable*>(&rhs);
    }
    inline PipelineLayoutResourceTable& FromAPI(WGPUPipelineLayoutResourceTable& rhs) {
        return *reinterpret_cast<PipelineLayoutResourceTable*>(&rhs);
    }

    inline const WGPUPipelineLayoutStorageAttachment* ToAPI(const PipelineLayoutStorageAttachment* rhs) {
        return reinterpret_cast<const WGPUPipelineLayoutStorageAttachment*>(rhs);
    }
    inline WGPUPipelineLayoutStorageAttachment* ToAPI(PipelineLayoutStorageAttachment* rhs) {
        return reinterpret_cast<WGPUPipelineLayoutStorageAttachment*>(rhs);
    }
    inline const PipelineLayoutStorageAttachment* FromAPI(const WGPUPipelineLayoutStorageAttachment* rhs) {
        return reinterpret_cast<const PipelineLayoutStorageAttachment*>(rhs);
    }
    inline PipelineLayoutStorageAttachment* FromAPI(WGPUPipelineLayoutStorageAttachment* rhs) {
        return reinterpret_cast<PipelineLayoutStorageAttachment*>(rhs);
    }
    inline const PipelineLayoutStorageAttachment& FromAPI(const WGPUPipelineLayoutStorageAttachment& rhs) {
        return *reinterpret_cast<const PipelineLayoutStorageAttachment*>(&rhs);
    }
    inline PipelineLayoutStorageAttachment& FromAPI(WGPUPipelineLayoutStorageAttachment& rhs) {
        return *reinterpret_cast<PipelineLayoutStorageAttachment*>(&rhs);
    }

    inline const WGPUPrimitiveState* ToAPI(const PrimitiveState* rhs) {
        return reinterpret_cast<const WGPUPrimitiveState*>(rhs);
    }
    inline WGPUPrimitiveState* ToAPI(PrimitiveState* rhs) {
        return reinterpret_cast<WGPUPrimitiveState*>(rhs);
    }
    inline const PrimitiveState* FromAPI(const WGPUPrimitiveState* rhs) {
        return reinterpret_cast<const PrimitiveState*>(rhs);
    }
    inline PrimitiveState* FromAPI(WGPUPrimitiveState* rhs) {
        return reinterpret_cast<PrimitiveState*>(rhs);
    }
    inline const PrimitiveState& FromAPI(const WGPUPrimitiveState& rhs) {
        return *reinterpret_cast<const PrimitiveState*>(&rhs);
    }
    inline PrimitiveState& FromAPI(WGPUPrimitiveState& rhs) {
        return *reinterpret_cast<PrimitiveState*>(&rhs);
    }

    inline const WGPUQuerySetDescriptor* ToAPI(const QuerySetDescriptor* rhs) {
        return reinterpret_cast<const WGPUQuerySetDescriptor*>(rhs);
    }
    inline WGPUQuerySetDescriptor* ToAPI(QuerySetDescriptor* rhs) {
        return reinterpret_cast<WGPUQuerySetDescriptor*>(rhs);
    }
    inline const QuerySetDescriptor* FromAPI(const WGPUQuerySetDescriptor* rhs) {
        return reinterpret_cast<const QuerySetDescriptor*>(rhs);
    }
    inline QuerySetDescriptor* FromAPI(WGPUQuerySetDescriptor* rhs) {
        return reinterpret_cast<QuerySetDescriptor*>(rhs);
    }
    inline const QuerySetDescriptor& FromAPI(const WGPUQuerySetDescriptor& rhs) {
        return *reinterpret_cast<const QuerySetDescriptor*>(&rhs);
    }
    inline QuerySetDescriptor& FromAPI(WGPUQuerySetDescriptor& rhs) {
        return *reinterpret_cast<QuerySetDescriptor*>(&rhs);
    }

    inline const WGPUQueueDescriptor* ToAPI(const QueueDescriptor* rhs) {
        return reinterpret_cast<const WGPUQueueDescriptor*>(rhs);
    }
    inline WGPUQueueDescriptor* ToAPI(QueueDescriptor* rhs) {
        return reinterpret_cast<WGPUQueueDescriptor*>(rhs);
    }
    inline const QueueDescriptor* FromAPI(const WGPUQueueDescriptor* rhs) {
        return reinterpret_cast<const QueueDescriptor*>(rhs);
    }
    inline QueueDescriptor* FromAPI(WGPUQueueDescriptor* rhs) {
        return reinterpret_cast<QueueDescriptor*>(rhs);
    }
    inline const QueueDescriptor& FromAPI(const WGPUQueueDescriptor& rhs) {
        return *reinterpret_cast<const QueueDescriptor*>(&rhs);
    }
    inline QueueDescriptor& FromAPI(WGPUQueueDescriptor& rhs) {
        return *reinterpret_cast<QueueDescriptor*>(&rhs);
    }

    inline const WGPURenderBundleDescriptor* ToAPI(const RenderBundleDescriptor* rhs) {
        return reinterpret_cast<const WGPURenderBundleDescriptor*>(rhs);
    }
    inline WGPURenderBundleDescriptor* ToAPI(RenderBundleDescriptor* rhs) {
        return reinterpret_cast<WGPURenderBundleDescriptor*>(rhs);
    }
    inline const RenderBundleDescriptor* FromAPI(const WGPURenderBundleDescriptor* rhs) {
        return reinterpret_cast<const RenderBundleDescriptor*>(rhs);
    }
    inline RenderBundleDescriptor* FromAPI(WGPURenderBundleDescriptor* rhs) {
        return reinterpret_cast<RenderBundleDescriptor*>(rhs);
    }
    inline const RenderBundleDescriptor& FromAPI(const WGPURenderBundleDescriptor& rhs) {
        return *reinterpret_cast<const RenderBundleDescriptor*>(&rhs);
    }
    inline RenderBundleDescriptor& FromAPI(WGPURenderBundleDescriptor& rhs) {
        return *reinterpret_cast<RenderBundleDescriptor*>(&rhs);
    }

    inline const WGPURenderBundleEncoderDescriptor* ToAPI(const RenderBundleEncoderDescriptor* rhs) {
        return reinterpret_cast<const WGPURenderBundleEncoderDescriptor*>(rhs);
    }
    inline WGPURenderBundleEncoderDescriptor* ToAPI(RenderBundleEncoderDescriptor* rhs) {
        return reinterpret_cast<WGPURenderBundleEncoderDescriptor*>(rhs);
    }
    inline const RenderBundleEncoderDescriptor* FromAPI(const WGPURenderBundleEncoderDescriptor* rhs) {
        return reinterpret_cast<const RenderBundleEncoderDescriptor*>(rhs);
    }
    inline RenderBundleEncoderDescriptor* FromAPI(WGPURenderBundleEncoderDescriptor* rhs) {
        return reinterpret_cast<RenderBundleEncoderDescriptor*>(rhs);
    }
    inline const RenderBundleEncoderDescriptor& FromAPI(const WGPURenderBundleEncoderDescriptor& rhs) {
        return *reinterpret_cast<const RenderBundleEncoderDescriptor*>(&rhs);
    }
    inline RenderBundleEncoderDescriptor& FromAPI(WGPURenderBundleEncoderDescriptor& rhs) {
        return *reinterpret_cast<RenderBundleEncoderDescriptor*>(&rhs);
    }

    inline const WGPURenderPassDepthStencilAttachment* ToAPI(const RenderPassDepthStencilAttachment* rhs) {
        return reinterpret_cast<const WGPURenderPassDepthStencilAttachment*>(rhs);
    }
    inline WGPURenderPassDepthStencilAttachment* ToAPI(RenderPassDepthStencilAttachment* rhs) {
        return reinterpret_cast<WGPURenderPassDepthStencilAttachment*>(rhs);
    }
    inline const RenderPassDepthStencilAttachment* FromAPI(const WGPURenderPassDepthStencilAttachment* rhs) {
        return reinterpret_cast<const RenderPassDepthStencilAttachment*>(rhs);
    }
    inline RenderPassDepthStencilAttachment* FromAPI(WGPURenderPassDepthStencilAttachment* rhs) {
        return reinterpret_cast<RenderPassDepthStencilAttachment*>(rhs);
    }
    inline const RenderPassDepthStencilAttachment& FromAPI(const WGPURenderPassDepthStencilAttachment& rhs) {
        return *reinterpret_cast<const RenderPassDepthStencilAttachment*>(&rhs);
    }
    inline RenderPassDepthStencilAttachment& FromAPI(WGPURenderPassDepthStencilAttachment& rhs) {
        return *reinterpret_cast<RenderPassDepthStencilAttachment*>(&rhs);
    }

    inline const WGPURenderPassDescriptorResolveRect* ToAPI(const RenderPassDescriptorResolveRect* rhs) {
        return reinterpret_cast<const WGPURenderPassDescriptorResolveRect*>(rhs);
    }
    inline WGPURenderPassDescriptorResolveRect* ToAPI(RenderPassDescriptorResolveRect* rhs) {
        return reinterpret_cast<WGPURenderPassDescriptorResolveRect*>(rhs);
    }
    inline const RenderPassDescriptorResolveRect* FromAPI(const WGPURenderPassDescriptorResolveRect* rhs) {
        return reinterpret_cast<const RenderPassDescriptorResolveRect*>(rhs);
    }
    inline RenderPassDescriptorResolveRect* FromAPI(WGPURenderPassDescriptorResolveRect* rhs) {
        return reinterpret_cast<RenderPassDescriptorResolveRect*>(rhs);
    }
    inline const RenderPassDescriptorResolveRect& FromAPI(const WGPURenderPassDescriptorResolveRect& rhs) {
        return *reinterpret_cast<const RenderPassDescriptorResolveRect*>(&rhs);
    }
    inline RenderPassDescriptorResolveRect& FromAPI(WGPURenderPassDescriptorResolveRect& rhs) {
        return *reinterpret_cast<RenderPassDescriptorResolveRect*>(&rhs);
    }

    inline const WGPURenderPassMaxDrawCount* ToAPI(const RenderPassMaxDrawCount* rhs) {
        return reinterpret_cast<const WGPURenderPassMaxDrawCount*>(rhs);
    }
    inline WGPURenderPassMaxDrawCount* ToAPI(RenderPassMaxDrawCount* rhs) {
        return reinterpret_cast<WGPURenderPassMaxDrawCount*>(rhs);
    }
    inline const RenderPassMaxDrawCount* FromAPI(const WGPURenderPassMaxDrawCount* rhs) {
        return reinterpret_cast<const RenderPassMaxDrawCount*>(rhs);
    }
    inline RenderPassMaxDrawCount* FromAPI(WGPURenderPassMaxDrawCount* rhs) {
        return reinterpret_cast<RenderPassMaxDrawCount*>(rhs);
    }
    inline const RenderPassMaxDrawCount& FromAPI(const WGPURenderPassMaxDrawCount& rhs) {
        return *reinterpret_cast<const RenderPassMaxDrawCount*>(&rhs);
    }
    inline RenderPassMaxDrawCount& FromAPI(WGPURenderPassMaxDrawCount& rhs) {
        return *reinterpret_cast<RenderPassMaxDrawCount*>(&rhs);
    }

    inline const WGPURequestAdapterWebXROptions* ToAPI(const RequestAdapterWebXROptions* rhs) {
        return reinterpret_cast<const WGPURequestAdapterWebXROptions*>(rhs);
    }
    inline WGPURequestAdapterWebXROptions* ToAPI(RequestAdapterWebXROptions* rhs) {
        return reinterpret_cast<WGPURequestAdapterWebXROptions*>(rhs);
    }
    inline const RequestAdapterWebXROptions* FromAPI(const WGPURequestAdapterWebXROptions* rhs) {
        return reinterpret_cast<const RequestAdapterWebXROptions*>(rhs);
    }
    inline RequestAdapterWebXROptions* FromAPI(WGPURequestAdapterWebXROptions* rhs) {
        return reinterpret_cast<RequestAdapterWebXROptions*>(rhs);
    }
    inline const RequestAdapterWebXROptions& FromAPI(const WGPURequestAdapterWebXROptions& rhs) {
        return *reinterpret_cast<const RequestAdapterWebXROptions*>(&rhs);
    }
    inline RequestAdapterWebXROptions& FromAPI(WGPURequestAdapterWebXROptions& rhs) {
        return *reinterpret_cast<RequestAdapterWebXROptions*>(&rhs);
    }

    inline const WGPUResourceTableDescriptor* ToAPI(const ResourceTableDescriptor* rhs) {
        return reinterpret_cast<const WGPUResourceTableDescriptor*>(rhs);
    }
    inline WGPUResourceTableDescriptor* ToAPI(ResourceTableDescriptor* rhs) {
        return reinterpret_cast<WGPUResourceTableDescriptor*>(rhs);
    }
    inline const ResourceTableDescriptor* FromAPI(const WGPUResourceTableDescriptor* rhs) {
        return reinterpret_cast<const ResourceTableDescriptor*>(rhs);
    }
    inline ResourceTableDescriptor* FromAPI(WGPUResourceTableDescriptor* rhs) {
        return reinterpret_cast<ResourceTableDescriptor*>(rhs);
    }
    inline const ResourceTableDescriptor& FromAPI(const WGPUResourceTableDescriptor& rhs) {
        return *reinterpret_cast<const ResourceTableDescriptor*>(&rhs);
    }
    inline ResourceTableDescriptor& FromAPI(WGPUResourceTableDescriptor& rhs) {
        return *reinterpret_cast<ResourceTableDescriptor*>(&rhs);
    }

    inline const WGPUSamplerBindingLayout* ToAPI(const SamplerBindingLayout* rhs) {
        return reinterpret_cast<const WGPUSamplerBindingLayout*>(rhs);
    }
    inline WGPUSamplerBindingLayout* ToAPI(SamplerBindingLayout* rhs) {
        return reinterpret_cast<WGPUSamplerBindingLayout*>(rhs);
    }
    inline const SamplerBindingLayout* FromAPI(const WGPUSamplerBindingLayout* rhs) {
        return reinterpret_cast<const SamplerBindingLayout*>(rhs);
    }
    inline SamplerBindingLayout* FromAPI(WGPUSamplerBindingLayout* rhs) {
        return reinterpret_cast<SamplerBindingLayout*>(rhs);
    }
    inline const SamplerBindingLayout& FromAPI(const WGPUSamplerBindingLayout& rhs) {
        return *reinterpret_cast<const SamplerBindingLayout*>(&rhs);
    }
    inline SamplerBindingLayout& FromAPI(WGPUSamplerBindingLayout& rhs) {
        return *reinterpret_cast<SamplerBindingLayout*>(&rhs);
    }

    inline const WGPUShaderModuleCompilationOptions* ToAPI(const ShaderModuleCompilationOptions* rhs) {
        return reinterpret_cast<const WGPUShaderModuleCompilationOptions*>(rhs);
    }
    inline WGPUShaderModuleCompilationOptions* ToAPI(ShaderModuleCompilationOptions* rhs) {
        return reinterpret_cast<WGPUShaderModuleCompilationOptions*>(rhs);
    }
    inline const ShaderModuleCompilationOptions* FromAPI(const WGPUShaderModuleCompilationOptions* rhs) {
        return reinterpret_cast<const ShaderModuleCompilationOptions*>(rhs);
    }
    inline ShaderModuleCompilationOptions* FromAPI(WGPUShaderModuleCompilationOptions* rhs) {
        return reinterpret_cast<ShaderModuleCompilationOptions*>(rhs);
    }
    inline const ShaderModuleCompilationOptions& FromAPI(const WGPUShaderModuleCompilationOptions& rhs) {
        return *reinterpret_cast<const ShaderModuleCompilationOptions*>(&rhs);
    }
    inline ShaderModuleCompilationOptions& FromAPI(WGPUShaderModuleCompilationOptions& rhs) {
        return *reinterpret_cast<ShaderModuleCompilationOptions*>(&rhs);
    }

    inline const WGPUShaderSourceSPIRV* ToAPI(const ShaderSourceSPIRV* rhs) {
        return reinterpret_cast<const WGPUShaderSourceSPIRV*>(rhs);
    }
    inline WGPUShaderSourceSPIRV* ToAPI(ShaderSourceSPIRV* rhs) {
        return reinterpret_cast<WGPUShaderSourceSPIRV*>(rhs);
    }
    inline const ShaderSourceSPIRV* FromAPI(const WGPUShaderSourceSPIRV* rhs) {
        return reinterpret_cast<const ShaderSourceSPIRV*>(rhs);
    }
    inline ShaderSourceSPIRV* FromAPI(WGPUShaderSourceSPIRV* rhs) {
        return reinterpret_cast<ShaderSourceSPIRV*>(rhs);
    }
    inline const ShaderSourceSPIRV& FromAPI(const WGPUShaderSourceSPIRV& rhs) {
        return *reinterpret_cast<const ShaderSourceSPIRV*>(&rhs);
    }
    inline ShaderSourceSPIRV& FromAPI(WGPUShaderSourceSPIRV& rhs) {
        return *reinterpret_cast<ShaderSourceSPIRV*>(&rhs);
    }

    inline const WGPUShaderSourceWGSL* ToAPI(const ShaderSourceWGSL* rhs) {
        return reinterpret_cast<const WGPUShaderSourceWGSL*>(rhs);
    }
    inline WGPUShaderSourceWGSL* ToAPI(ShaderSourceWGSL* rhs) {
        return reinterpret_cast<WGPUShaderSourceWGSL*>(rhs);
    }
    inline const ShaderSourceWGSL* FromAPI(const WGPUShaderSourceWGSL* rhs) {
        return reinterpret_cast<const ShaderSourceWGSL*>(rhs);
    }
    inline ShaderSourceWGSL* FromAPI(WGPUShaderSourceWGSL* rhs) {
        return reinterpret_cast<ShaderSourceWGSL*>(rhs);
    }
    inline const ShaderSourceWGSL& FromAPI(const WGPUShaderSourceWGSL& rhs) {
        return *reinterpret_cast<const ShaderSourceWGSL*>(&rhs);
    }
    inline ShaderSourceWGSL& FromAPI(WGPUShaderSourceWGSL& rhs) {
        return *reinterpret_cast<ShaderSourceWGSL*>(&rhs);
    }

    inline const WGPUStaticSamplerBindingLayout* ToAPI(const StaticSamplerBindingLayout* rhs) {
        return reinterpret_cast<const WGPUStaticSamplerBindingLayout*>(rhs);
    }
    inline WGPUStaticSamplerBindingLayout* ToAPI(StaticSamplerBindingLayout* rhs) {
        return reinterpret_cast<WGPUStaticSamplerBindingLayout*>(rhs);
    }
    inline const StaticSamplerBindingLayout* FromAPI(const WGPUStaticSamplerBindingLayout* rhs) {
        return reinterpret_cast<const StaticSamplerBindingLayout*>(rhs);
    }
    inline StaticSamplerBindingLayout* FromAPI(WGPUStaticSamplerBindingLayout* rhs) {
        return reinterpret_cast<StaticSamplerBindingLayout*>(rhs);
    }
    inline const StaticSamplerBindingLayout& FromAPI(const WGPUStaticSamplerBindingLayout& rhs) {
        return *reinterpret_cast<const StaticSamplerBindingLayout*>(&rhs);
    }
    inline StaticSamplerBindingLayout& FromAPI(WGPUStaticSamplerBindingLayout& rhs) {
        return *reinterpret_cast<StaticSamplerBindingLayout*>(&rhs);
    }

    inline const WGPUStencilFaceState* ToAPI(const StencilFaceState* rhs) {
        return reinterpret_cast<const WGPUStencilFaceState*>(rhs);
    }
    inline WGPUStencilFaceState* ToAPI(StencilFaceState* rhs) {
        return reinterpret_cast<WGPUStencilFaceState*>(rhs);
    }
    inline const StencilFaceState* FromAPI(const WGPUStencilFaceState* rhs) {
        return reinterpret_cast<const StencilFaceState*>(rhs);
    }
    inline StencilFaceState* FromAPI(WGPUStencilFaceState* rhs) {
        return reinterpret_cast<StencilFaceState*>(rhs);
    }
    inline const StencilFaceState& FromAPI(const WGPUStencilFaceState& rhs) {
        return *reinterpret_cast<const StencilFaceState*>(&rhs);
    }
    inline StencilFaceState& FromAPI(WGPUStencilFaceState& rhs) {
        return *reinterpret_cast<StencilFaceState*>(&rhs);
    }

    inline const WGPUStorageTextureBindingLayout* ToAPI(const StorageTextureBindingLayout* rhs) {
        return reinterpret_cast<const WGPUStorageTextureBindingLayout*>(rhs);
    }
    inline WGPUStorageTextureBindingLayout* ToAPI(StorageTextureBindingLayout* rhs) {
        return reinterpret_cast<WGPUStorageTextureBindingLayout*>(rhs);
    }
    inline const StorageTextureBindingLayout* FromAPI(const WGPUStorageTextureBindingLayout* rhs) {
        return reinterpret_cast<const StorageTextureBindingLayout*>(rhs);
    }
    inline StorageTextureBindingLayout* FromAPI(WGPUStorageTextureBindingLayout* rhs) {
        return reinterpret_cast<StorageTextureBindingLayout*>(rhs);
    }
    inline const StorageTextureBindingLayout& FromAPI(const WGPUStorageTextureBindingLayout& rhs) {
        return *reinterpret_cast<const StorageTextureBindingLayout*>(&rhs);
    }
    inline StorageTextureBindingLayout& FromAPI(WGPUStorageTextureBindingLayout& rhs) {
        return *reinterpret_cast<StorageTextureBindingLayout*>(&rhs);
    }

    inline const WGPUSubgroupMatrixConfig* ToAPI(const SubgroupMatrixConfig* rhs) {
        return reinterpret_cast<const WGPUSubgroupMatrixConfig*>(rhs);
    }
    inline WGPUSubgroupMatrixConfig* ToAPI(SubgroupMatrixConfig* rhs) {
        return reinterpret_cast<WGPUSubgroupMatrixConfig*>(rhs);
    }
    inline const SubgroupMatrixConfig* FromAPI(const WGPUSubgroupMatrixConfig* rhs) {
        return reinterpret_cast<const SubgroupMatrixConfig*>(rhs);
    }
    inline SubgroupMatrixConfig* FromAPI(WGPUSubgroupMatrixConfig* rhs) {
        return reinterpret_cast<SubgroupMatrixConfig*>(rhs);
    }
    inline const SubgroupMatrixConfig& FromAPI(const WGPUSubgroupMatrixConfig& rhs) {
        return *reinterpret_cast<const SubgroupMatrixConfig*>(&rhs);
    }
    inline SubgroupMatrixConfig& FromAPI(WGPUSubgroupMatrixConfig& rhs) {
        return *reinterpret_cast<SubgroupMatrixConfig*>(&rhs);
    }

    inline const WGPUSupportedFeatures* ToAPI(const SupportedFeatures* rhs) {
        return reinterpret_cast<const WGPUSupportedFeatures*>(rhs);
    }
    inline WGPUSupportedFeatures* ToAPI(SupportedFeatures* rhs) {
        return reinterpret_cast<WGPUSupportedFeatures*>(rhs);
    }
    inline const SupportedFeatures* FromAPI(const WGPUSupportedFeatures* rhs) {
        return reinterpret_cast<const SupportedFeatures*>(rhs);
    }
    inline SupportedFeatures* FromAPI(WGPUSupportedFeatures* rhs) {
        return reinterpret_cast<SupportedFeatures*>(rhs);
    }
    inline const SupportedFeatures& FromAPI(const WGPUSupportedFeatures& rhs) {
        return *reinterpret_cast<const SupportedFeatures*>(&rhs);
    }
    inline SupportedFeatures& FromAPI(WGPUSupportedFeatures& rhs) {
        return *reinterpret_cast<SupportedFeatures*>(&rhs);
    }

    inline const WGPUSupportedInstanceFeatures* ToAPI(const SupportedInstanceFeatures* rhs) {
        return reinterpret_cast<const WGPUSupportedInstanceFeatures*>(rhs);
    }
    inline WGPUSupportedInstanceFeatures* ToAPI(SupportedInstanceFeatures* rhs) {
        return reinterpret_cast<WGPUSupportedInstanceFeatures*>(rhs);
    }
    inline const SupportedInstanceFeatures* FromAPI(const WGPUSupportedInstanceFeatures* rhs) {
        return reinterpret_cast<const SupportedInstanceFeatures*>(rhs);
    }
    inline SupportedInstanceFeatures* FromAPI(WGPUSupportedInstanceFeatures* rhs) {
        return reinterpret_cast<SupportedInstanceFeatures*>(rhs);
    }
    inline const SupportedInstanceFeatures& FromAPI(const WGPUSupportedInstanceFeatures& rhs) {
        return *reinterpret_cast<const SupportedInstanceFeatures*>(&rhs);
    }
    inline SupportedInstanceFeatures& FromAPI(WGPUSupportedInstanceFeatures& rhs) {
        return *reinterpret_cast<SupportedInstanceFeatures*>(&rhs);
    }

    inline const WGPUSupportedWGSLLanguageFeatures* ToAPI(const SupportedWGSLLanguageFeatures* rhs) {
        return reinterpret_cast<const WGPUSupportedWGSLLanguageFeatures*>(rhs);
    }
    inline WGPUSupportedWGSLLanguageFeatures* ToAPI(SupportedWGSLLanguageFeatures* rhs) {
        return reinterpret_cast<WGPUSupportedWGSLLanguageFeatures*>(rhs);
    }
    inline const SupportedWGSLLanguageFeatures* FromAPI(const WGPUSupportedWGSLLanguageFeatures* rhs) {
        return reinterpret_cast<const SupportedWGSLLanguageFeatures*>(rhs);
    }
    inline SupportedWGSLLanguageFeatures* FromAPI(WGPUSupportedWGSLLanguageFeatures* rhs) {
        return reinterpret_cast<SupportedWGSLLanguageFeatures*>(rhs);
    }
    inline const SupportedWGSLLanguageFeatures& FromAPI(const WGPUSupportedWGSLLanguageFeatures& rhs) {
        return *reinterpret_cast<const SupportedWGSLLanguageFeatures*>(&rhs);
    }
    inline SupportedWGSLLanguageFeatures& FromAPI(WGPUSupportedWGSLLanguageFeatures& rhs) {
        return *reinterpret_cast<SupportedWGSLLanguageFeatures*>(&rhs);
    }

    inline const WGPUSurfaceCapabilities* ToAPI(const SurfaceCapabilities* rhs) {
        return reinterpret_cast<const WGPUSurfaceCapabilities*>(rhs);
    }
    inline WGPUSurfaceCapabilities* ToAPI(SurfaceCapabilities* rhs) {
        return reinterpret_cast<WGPUSurfaceCapabilities*>(rhs);
    }
    inline const SurfaceCapabilities* FromAPI(const WGPUSurfaceCapabilities* rhs) {
        return reinterpret_cast<const SurfaceCapabilities*>(rhs);
    }
    inline SurfaceCapabilities* FromAPI(WGPUSurfaceCapabilities* rhs) {
        return reinterpret_cast<SurfaceCapabilities*>(rhs);
    }
    inline const SurfaceCapabilities& FromAPI(const WGPUSurfaceCapabilities& rhs) {
        return *reinterpret_cast<const SurfaceCapabilities*>(&rhs);
    }
    inline SurfaceCapabilities& FromAPI(WGPUSurfaceCapabilities& rhs) {
        return *reinterpret_cast<SurfaceCapabilities*>(&rhs);
    }

    inline const WGPUSurfaceColorManagement* ToAPI(const SurfaceColorManagement* rhs) {
        return reinterpret_cast<const WGPUSurfaceColorManagement*>(rhs);
    }
    inline WGPUSurfaceColorManagement* ToAPI(SurfaceColorManagement* rhs) {
        return reinterpret_cast<WGPUSurfaceColorManagement*>(rhs);
    }
    inline const SurfaceColorManagement* FromAPI(const WGPUSurfaceColorManagement* rhs) {
        return reinterpret_cast<const SurfaceColorManagement*>(rhs);
    }
    inline SurfaceColorManagement* FromAPI(WGPUSurfaceColorManagement* rhs) {
        return reinterpret_cast<SurfaceColorManagement*>(rhs);
    }
    inline const SurfaceColorManagement& FromAPI(const WGPUSurfaceColorManagement& rhs) {
        return *reinterpret_cast<const SurfaceColorManagement*>(&rhs);
    }
    inline SurfaceColorManagement& FromAPI(WGPUSurfaceColorManagement& rhs) {
        return *reinterpret_cast<SurfaceColorManagement*>(&rhs);
    }

    inline const WGPUSurfaceConfiguration* ToAPI(const SurfaceConfiguration* rhs) {
        return reinterpret_cast<const WGPUSurfaceConfiguration*>(rhs);
    }
    inline WGPUSurfaceConfiguration* ToAPI(SurfaceConfiguration* rhs) {
        return reinterpret_cast<WGPUSurfaceConfiguration*>(rhs);
    }
    inline const SurfaceConfiguration* FromAPI(const WGPUSurfaceConfiguration* rhs) {
        return reinterpret_cast<const SurfaceConfiguration*>(rhs);
    }
    inline SurfaceConfiguration* FromAPI(WGPUSurfaceConfiguration* rhs) {
        return reinterpret_cast<SurfaceConfiguration*>(rhs);
    }
    inline const SurfaceConfiguration& FromAPI(const WGPUSurfaceConfiguration& rhs) {
        return *reinterpret_cast<const SurfaceConfiguration*>(&rhs);
    }
    inline SurfaceConfiguration& FromAPI(WGPUSurfaceConfiguration& rhs) {
        return *reinterpret_cast<SurfaceConfiguration*>(&rhs);
    }

    inline const WGPUSurfaceDescriptorFromWindowsCoreWindow* ToAPI(const SurfaceDescriptorFromWindowsCoreWindow* rhs) {
        return reinterpret_cast<const WGPUSurfaceDescriptorFromWindowsCoreWindow*>(rhs);
    }
    inline WGPUSurfaceDescriptorFromWindowsCoreWindow* ToAPI(SurfaceDescriptorFromWindowsCoreWindow* rhs) {
        return reinterpret_cast<WGPUSurfaceDescriptorFromWindowsCoreWindow*>(rhs);
    }
    inline const SurfaceDescriptorFromWindowsCoreWindow* FromAPI(const WGPUSurfaceDescriptorFromWindowsCoreWindow* rhs) {
        return reinterpret_cast<const SurfaceDescriptorFromWindowsCoreWindow*>(rhs);
    }
    inline SurfaceDescriptorFromWindowsCoreWindow* FromAPI(WGPUSurfaceDescriptorFromWindowsCoreWindow* rhs) {
        return reinterpret_cast<SurfaceDescriptorFromWindowsCoreWindow*>(rhs);
    }
    inline const SurfaceDescriptorFromWindowsCoreWindow& FromAPI(const WGPUSurfaceDescriptorFromWindowsCoreWindow& rhs) {
        return *reinterpret_cast<const SurfaceDescriptorFromWindowsCoreWindow*>(&rhs);
    }
    inline SurfaceDescriptorFromWindowsCoreWindow& FromAPI(WGPUSurfaceDescriptorFromWindowsCoreWindow& rhs) {
        return *reinterpret_cast<SurfaceDescriptorFromWindowsCoreWindow*>(&rhs);
    }

    inline const WGPUSurfaceDescriptorFromWindowsUWPSwapChainPanel* ToAPI(const SurfaceDescriptorFromWindowsUWPSwapChainPanel* rhs) {
        return reinterpret_cast<const WGPUSurfaceDescriptorFromWindowsUWPSwapChainPanel*>(rhs);
    }
    inline WGPUSurfaceDescriptorFromWindowsUWPSwapChainPanel* ToAPI(SurfaceDescriptorFromWindowsUWPSwapChainPanel* rhs) {
        return reinterpret_cast<WGPUSurfaceDescriptorFromWindowsUWPSwapChainPanel*>(rhs);
    }
    inline const SurfaceDescriptorFromWindowsUWPSwapChainPanel* FromAPI(const WGPUSurfaceDescriptorFromWindowsUWPSwapChainPanel* rhs) {
        return reinterpret_cast<const SurfaceDescriptorFromWindowsUWPSwapChainPanel*>(rhs);
    }
    inline SurfaceDescriptorFromWindowsUWPSwapChainPanel* FromAPI(WGPUSurfaceDescriptorFromWindowsUWPSwapChainPanel* rhs) {
        return reinterpret_cast<SurfaceDescriptorFromWindowsUWPSwapChainPanel*>(rhs);
    }
    inline const SurfaceDescriptorFromWindowsUWPSwapChainPanel& FromAPI(const WGPUSurfaceDescriptorFromWindowsUWPSwapChainPanel& rhs) {
        return *reinterpret_cast<const SurfaceDescriptorFromWindowsUWPSwapChainPanel*>(&rhs);
    }
    inline SurfaceDescriptorFromWindowsUWPSwapChainPanel& FromAPI(WGPUSurfaceDescriptorFromWindowsUWPSwapChainPanel& rhs) {
        return *reinterpret_cast<SurfaceDescriptorFromWindowsUWPSwapChainPanel*>(&rhs);
    }

    inline const WGPUSurfaceDescriptorFromWindowsWinUISwapChainPanel* ToAPI(const SurfaceDescriptorFromWindowsWinUISwapChainPanel* rhs) {
        return reinterpret_cast<const WGPUSurfaceDescriptorFromWindowsWinUISwapChainPanel*>(rhs);
    }
    inline WGPUSurfaceDescriptorFromWindowsWinUISwapChainPanel* ToAPI(SurfaceDescriptorFromWindowsWinUISwapChainPanel* rhs) {
        return reinterpret_cast<WGPUSurfaceDescriptorFromWindowsWinUISwapChainPanel*>(rhs);
    }
    inline const SurfaceDescriptorFromWindowsWinUISwapChainPanel* FromAPI(const WGPUSurfaceDescriptorFromWindowsWinUISwapChainPanel* rhs) {
        return reinterpret_cast<const SurfaceDescriptorFromWindowsWinUISwapChainPanel*>(rhs);
    }
    inline SurfaceDescriptorFromWindowsWinUISwapChainPanel* FromAPI(WGPUSurfaceDescriptorFromWindowsWinUISwapChainPanel* rhs) {
        return reinterpret_cast<SurfaceDescriptorFromWindowsWinUISwapChainPanel*>(rhs);
    }
    inline const SurfaceDescriptorFromWindowsWinUISwapChainPanel& FromAPI(const WGPUSurfaceDescriptorFromWindowsWinUISwapChainPanel& rhs) {
        return *reinterpret_cast<const SurfaceDescriptorFromWindowsWinUISwapChainPanel*>(&rhs);
    }
    inline SurfaceDescriptorFromWindowsWinUISwapChainPanel& FromAPI(WGPUSurfaceDescriptorFromWindowsWinUISwapChainPanel& rhs) {
        return *reinterpret_cast<SurfaceDescriptorFromWindowsWinUISwapChainPanel*>(&rhs);
    }

    inline const WGPUSurfaceTexture* ToAPI(const SurfaceTexture* rhs) {
        return reinterpret_cast<const WGPUSurfaceTexture*>(rhs);
    }
    inline WGPUSurfaceTexture* ToAPI(SurfaceTexture* rhs) {
        return reinterpret_cast<WGPUSurfaceTexture*>(rhs);
    }
    inline const SurfaceTexture* FromAPI(const WGPUSurfaceTexture* rhs) {
        return reinterpret_cast<const SurfaceTexture*>(rhs);
    }
    inline SurfaceTexture* FromAPI(WGPUSurfaceTexture* rhs) {
        return reinterpret_cast<SurfaceTexture*>(rhs);
    }
    inline const SurfaceTexture& FromAPI(const WGPUSurfaceTexture& rhs) {
        return *reinterpret_cast<const SurfaceTexture*>(&rhs);
    }
    inline SurfaceTexture& FromAPI(WGPUSurfaceTexture& rhs) {
        return *reinterpret_cast<SurfaceTexture*>(&rhs);
    }

    inline const WGPUTexelBufferBindingEntry* ToAPI(const TexelBufferBindingEntry* rhs) {
        return reinterpret_cast<const WGPUTexelBufferBindingEntry*>(rhs);
    }
    inline WGPUTexelBufferBindingEntry* ToAPI(TexelBufferBindingEntry* rhs) {
        return reinterpret_cast<WGPUTexelBufferBindingEntry*>(rhs);
    }
    inline const TexelBufferBindingEntry* FromAPI(const WGPUTexelBufferBindingEntry* rhs) {
        return reinterpret_cast<const TexelBufferBindingEntry*>(rhs);
    }
    inline TexelBufferBindingEntry* FromAPI(WGPUTexelBufferBindingEntry* rhs) {
        return reinterpret_cast<TexelBufferBindingEntry*>(rhs);
    }
    inline const TexelBufferBindingEntry& FromAPI(const WGPUTexelBufferBindingEntry& rhs) {
        return *reinterpret_cast<const TexelBufferBindingEntry*>(&rhs);
    }
    inline TexelBufferBindingEntry& FromAPI(WGPUTexelBufferBindingEntry& rhs) {
        return *reinterpret_cast<TexelBufferBindingEntry*>(&rhs);
    }

    inline const WGPUTexelBufferBindingLayout* ToAPI(const TexelBufferBindingLayout* rhs) {
        return reinterpret_cast<const WGPUTexelBufferBindingLayout*>(rhs);
    }
    inline WGPUTexelBufferBindingLayout* ToAPI(TexelBufferBindingLayout* rhs) {
        return reinterpret_cast<WGPUTexelBufferBindingLayout*>(rhs);
    }
    inline const TexelBufferBindingLayout* FromAPI(const WGPUTexelBufferBindingLayout* rhs) {
        return reinterpret_cast<const TexelBufferBindingLayout*>(rhs);
    }
    inline TexelBufferBindingLayout* FromAPI(WGPUTexelBufferBindingLayout* rhs) {
        return reinterpret_cast<TexelBufferBindingLayout*>(rhs);
    }
    inline const TexelBufferBindingLayout& FromAPI(const WGPUTexelBufferBindingLayout& rhs) {
        return *reinterpret_cast<const TexelBufferBindingLayout*>(&rhs);
    }
    inline TexelBufferBindingLayout& FromAPI(WGPUTexelBufferBindingLayout& rhs) {
        return *reinterpret_cast<TexelBufferBindingLayout*>(&rhs);
    }

    inline const WGPUTexelBufferViewDescriptor* ToAPI(const TexelBufferViewDescriptor* rhs) {
        return reinterpret_cast<const WGPUTexelBufferViewDescriptor*>(rhs);
    }
    inline WGPUTexelBufferViewDescriptor* ToAPI(TexelBufferViewDescriptor* rhs) {
        return reinterpret_cast<WGPUTexelBufferViewDescriptor*>(rhs);
    }
    inline const TexelBufferViewDescriptor* FromAPI(const WGPUTexelBufferViewDescriptor* rhs) {
        return reinterpret_cast<const TexelBufferViewDescriptor*>(rhs);
    }
    inline TexelBufferViewDescriptor* FromAPI(WGPUTexelBufferViewDescriptor* rhs) {
        return reinterpret_cast<TexelBufferViewDescriptor*>(rhs);
    }
    inline const TexelBufferViewDescriptor& FromAPI(const WGPUTexelBufferViewDescriptor& rhs) {
        return *reinterpret_cast<const TexelBufferViewDescriptor*>(&rhs);
    }
    inline TexelBufferViewDescriptor& FromAPI(WGPUTexelBufferViewDescriptor& rhs) {
        return *reinterpret_cast<TexelBufferViewDescriptor*>(&rhs);
    }

    inline const WGPUTexelCopyBufferLayout* ToAPI(const TexelCopyBufferLayout* rhs) {
        return reinterpret_cast<const WGPUTexelCopyBufferLayout*>(rhs);
    }
    inline WGPUTexelCopyBufferLayout* ToAPI(TexelCopyBufferLayout* rhs) {
        return reinterpret_cast<WGPUTexelCopyBufferLayout*>(rhs);
    }
    inline const TexelCopyBufferLayout* FromAPI(const WGPUTexelCopyBufferLayout* rhs) {
        return reinterpret_cast<const TexelCopyBufferLayout*>(rhs);
    }
    inline TexelCopyBufferLayout* FromAPI(WGPUTexelCopyBufferLayout* rhs) {
        return reinterpret_cast<TexelCopyBufferLayout*>(rhs);
    }
    inline const TexelCopyBufferLayout& FromAPI(const WGPUTexelCopyBufferLayout& rhs) {
        return *reinterpret_cast<const TexelCopyBufferLayout*>(&rhs);
    }
    inline TexelCopyBufferLayout& FromAPI(WGPUTexelCopyBufferLayout& rhs) {
        return *reinterpret_cast<TexelCopyBufferLayout*>(&rhs);
    }

    inline const WGPUTextureBindingLayout* ToAPI(const TextureBindingLayout* rhs) {
        return reinterpret_cast<const WGPUTextureBindingLayout*>(rhs);
    }
    inline WGPUTextureBindingLayout* ToAPI(TextureBindingLayout* rhs) {
        return reinterpret_cast<WGPUTextureBindingLayout*>(rhs);
    }
    inline const TextureBindingLayout* FromAPI(const WGPUTextureBindingLayout* rhs) {
        return reinterpret_cast<const TextureBindingLayout*>(rhs);
    }
    inline TextureBindingLayout* FromAPI(WGPUTextureBindingLayout* rhs) {
        return reinterpret_cast<TextureBindingLayout*>(rhs);
    }
    inline const TextureBindingLayout& FromAPI(const WGPUTextureBindingLayout& rhs) {
        return *reinterpret_cast<const TextureBindingLayout*>(&rhs);
    }
    inline TextureBindingLayout& FromAPI(WGPUTextureBindingLayout& rhs) {
        return *reinterpret_cast<TextureBindingLayout*>(&rhs);
    }

    inline const WGPUTextureBindingViewDimension* ToAPI(const TextureBindingViewDimension* rhs) {
        return reinterpret_cast<const WGPUTextureBindingViewDimension*>(rhs);
    }
    inline WGPUTextureBindingViewDimension* ToAPI(TextureBindingViewDimension* rhs) {
        return reinterpret_cast<WGPUTextureBindingViewDimension*>(rhs);
    }
    inline const TextureBindingViewDimension* FromAPI(const WGPUTextureBindingViewDimension* rhs) {
        return reinterpret_cast<const TextureBindingViewDimension*>(rhs);
    }
    inline TextureBindingViewDimension* FromAPI(WGPUTextureBindingViewDimension* rhs) {
        return reinterpret_cast<TextureBindingViewDimension*>(rhs);
    }
    inline const TextureBindingViewDimension& FromAPI(const WGPUTextureBindingViewDimension& rhs) {
        return *reinterpret_cast<const TextureBindingViewDimension*>(&rhs);
    }
    inline TextureBindingViewDimension& FromAPI(WGPUTextureBindingViewDimension& rhs) {
        return *reinterpret_cast<TextureBindingViewDimension*>(&rhs);
    }

    inline const WGPUTextureComponentSwizzle* ToAPI(const TextureComponentSwizzle* rhs) {
        return reinterpret_cast<const WGPUTextureComponentSwizzle*>(rhs);
    }
    inline WGPUTextureComponentSwizzle* ToAPI(TextureComponentSwizzle* rhs) {
        return reinterpret_cast<WGPUTextureComponentSwizzle*>(rhs);
    }
    inline const TextureComponentSwizzle* FromAPI(const WGPUTextureComponentSwizzle* rhs) {
        return reinterpret_cast<const TextureComponentSwizzle*>(rhs);
    }
    inline TextureComponentSwizzle* FromAPI(WGPUTextureComponentSwizzle* rhs) {
        return reinterpret_cast<TextureComponentSwizzle*>(rhs);
    }
    inline const TextureComponentSwizzle& FromAPI(const WGPUTextureComponentSwizzle& rhs) {
        return *reinterpret_cast<const TextureComponentSwizzle*>(&rhs);
    }
    inline TextureComponentSwizzle& FromAPI(WGPUTextureComponentSwizzle& rhs) {
        return *reinterpret_cast<TextureComponentSwizzle*>(&rhs);
    }

    inline const WGPUVertexAttribute* ToAPI(const VertexAttribute* rhs) {
        return reinterpret_cast<const WGPUVertexAttribute*>(rhs);
    }
    inline WGPUVertexAttribute* ToAPI(VertexAttribute* rhs) {
        return reinterpret_cast<WGPUVertexAttribute*>(rhs);
    }
    inline const VertexAttribute* FromAPI(const WGPUVertexAttribute* rhs) {
        return reinterpret_cast<const VertexAttribute*>(rhs);
    }
    inline VertexAttribute* FromAPI(WGPUVertexAttribute* rhs) {
        return reinterpret_cast<VertexAttribute*>(rhs);
    }
    inline const VertexAttribute& FromAPI(const WGPUVertexAttribute& rhs) {
        return *reinterpret_cast<const VertexAttribute*>(&rhs);
    }
    inline VertexAttribute& FromAPI(WGPUVertexAttribute& rhs) {
        return *reinterpret_cast<VertexAttribute*>(&rhs);
    }

    inline const WGPUYCbCrVkDescriptor* ToAPI(const YCbCrVkDescriptor* rhs) {
        return reinterpret_cast<const WGPUYCbCrVkDescriptor*>(rhs);
    }
    inline WGPUYCbCrVkDescriptor* ToAPI(YCbCrVkDescriptor* rhs) {
        return reinterpret_cast<WGPUYCbCrVkDescriptor*>(rhs);
    }
    inline const YCbCrVkDescriptor* FromAPI(const WGPUYCbCrVkDescriptor* rhs) {
        return reinterpret_cast<const YCbCrVkDescriptor*>(rhs);
    }
    inline YCbCrVkDescriptor* FromAPI(WGPUYCbCrVkDescriptor* rhs) {
        return reinterpret_cast<YCbCrVkDescriptor*>(rhs);
    }
    inline const YCbCrVkDescriptor& FromAPI(const WGPUYCbCrVkDescriptor& rhs) {
        return *reinterpret_cast<const YCbCrVkDescriptor*>(&rhs);
    }
    inline YCbCrVkDescriptor& FromAPI(WGPUYCbCrVkDescriptor& rhs) {
        return *reinterpret_cast<YCbCrVkDescriptor*>(&rhs);
    }

    inline const WGPUAdapterPropertiesMemoryHeaps* ToAPI(const AdapterPropertiesMemoryHeaps* rhs) {
        return reinterpret_cast<const WGPUAdapterPropertiesMemoryHeaps*>(rhs);
    }
    inline WGPUAdapterPropertiesMemoryHeaps* ToAPI(AdapterPropertiesMemoryHeaps* rhs) {
        return reinterpret_cast<WGPUAdapterPropertiesMemoryHeaps*>(rhs);
    }
    inline const AdapterPropertiesMemoryHeaps* FromAPI(const WGPUAdapterPropertiesMemoryHeaps* rhs) {
        return reinterpret_cast<const AdapterPropertiesMemoryHeaps*>(rhs);
    }
    inline AdapterPropertiesMemoryHeaps* FromAPI(WGPUAdapterPropertiesMemoryHeaps* rhs) {
        return reinterpret_cast<AdapterPropertiesMemoryHeaps*>(rhs);
    }
    inline const AdapterPropertiesMemoryHeaps& FromAPI(const WGPUAdapterPropertiesMemoryHeaps& rhs) {
        return *reinterpret_cast<const AdapterPropertiesMemoryHeaps*>(&rhs);
    }
    inline AdapterPropertiesMemoryHeaps& FromAPI(WGPUAdapterPropertiesMemoryHeaps& rhs) {
        return *reinterpret_cast<AdapterPropertiesMemoryHeaps*>(&rhs);
    }

    inline const WGPUAdapterPropertiesSubgroupMatrixConfigs* ToAPI(const AdapterPropertiesSubgroupMatrixConfigs* rhs) {
        return reinterpret_cast<const WGPUAdapterPropertiesSubgroupMatrixConfigs*>(rhs);
    }
    inline WGPUAdapterPropertiesSubgroupMatrixConfigs* ToAPI(AdapterPropertiesSubgroupMatrixConfigs* rhs) {
        return reinterpret_cast<WGPUAdapterPropertiesSubgroupMatrixConfigs*>(rhs);
    }
    inline const AdapterPropertiesSubgroupMatrixConfigs* FromAPI(const WGPUAdapterPropertiesSubgroupMatrixConfigs* rhs) {
        return reinterpret_cast<const AdapterPropertiesSubgroupMatrixConfigs*>(rhs);
    }
    inline AdapterPropertiesSubgroupMatrixConfigs* FromAPI(WGPUAdapterPropertiesSubgroupMatrixConfigs* rhs) {
        return reinterpret_cast<AdapterPropertiesSubgroupMatrixConfigs*>(rhs);
    }
    inline const AdapterPropertiesSubgroupMatrixConfigs& FromAPI(const WGPUAdapterPropertiesSubgroupMatrixConfigs& rhs) {
        return *reinterpret_cast<const AdapterPropertiesSubgroupMatrixConfigs*>(&rhs);
    }
    inline AdapterPropertiesSubgroupMatrixConfigs& FromAPI(WGPUAdapterPropertiesSubgroupMatrixConfigs& rhs) {
        return *reinterpret_cast<AdapterPropertiesSubgroupMatrixConfigs*>(&rhs);
    }

    inline const WGPUBindGroupEntry* ToAPI(const BindGroupEntry* rhs) {
        return reinterpret_cast<const WGPUBindGroupEntry*>(rhs);
    }
    inline WGPUBindGroupEntry* ToAPI(BindGroupEntry* rhs) {
        return reinterpret_cast<WGPUBindGroupEntry*>(rhs);
    }
    inline const BindGroupEntry* FromAPI(const WGPUBindGroupEntry* rhs) {
        return reinterpret_cast<const BindGroupEntry*>(rhs);
    }
    inline BindGroupEntry* FromAPI(WGPUBindGroupEntry* rhs) {
        return reinterpret_cast<BindGroupEntry*>(rhs);
    }
    inline const BindGroupEntry& FromAPI(const WGPUBindGroupEntry& rhs) {
        return *reinterpret_cast<const BindGroupEntry*>(&rhs);
    }
    inline BindGroupEntry& FromAPI(WGPUBindGroupEntry& rhs) {
        return *reinterpret_cast<BindGroupEntry*>(&rhs);
    }

    inline const WGPUBindGroupLayoutEntry* ToAPI(const BindGroupLayoutEntry* rhs) {
        return reinterpret_cast<const WGPUBindGroupLayoutEntry*>(rhs);
    }
    inline WGPUBindGroupLayoutEntry* ToAPI(BindGroupLayoutEntry* rhs) {
        return reinterpret_cast<WGPUBindGroupLayoutEntry*>(rhs);
    }
    inline const BindGroupLayoutEntry* FromAPI(const WGPUBindGroupLayoutEntry* rhs) {
        return reinterpret_cast<const BindGroupLayoutEntry*>(rhs);
    }
    inline BindGroupLayoutEntry* FromAPI(WGPUBindGroupLayoutEntry* rhs) {
        return reinterpret_cast<BindGroupLayoutEntry*>(rhs);
    }
    inline const BindGroupLayoutEntry& FromAPI(const WGPUBindGroupLayoutEntry& rhs) {
        return *reinterpret_cast<const BindGroupLayoutEntry*>(&rhs);
    }
    inline BindGroupLayoutEntry& FromAPI(WGPUBindGroupLayoutEntry& rhs) {
        return *reinterpret_cast<BindGroupLayoutEntry*>(&rhs);
    }

    inline const WGPUBlendState* ToAPI(const BlendState* rhs) {
        return reinterpret_cast<const WGPUBlendState*>(rhs);
    }
    inline WGPUBlendState* ToAPI(BlendState* rhs) {
        return reinterpret_cast<WGPUBlendState*>(rhs);
    }
    inline const BlendState* FromAPI(const WGPUBlendState* rhs) {
        return reinterpret_cast<const BlendState*>(rhs);
    }
    inline BlendState* FromAPI(WGPUBlendState* rhs) {
        return reinterpret_cast<BlendState*>(rhs);
    }
    inline const BlendState& FromAPI(const WGPUBlendState& rhs) {
        return *reinterpret_cast<const BlendState*>(&rhs);
    }
    inline BlendState& FromAPI(WGPUBlendState& rhs) {
        return *reinterpret_cast<BlendState*>(&rhs);
    }

    inline const WGPUBufferDescriptor* ToAPI(const BufferDescriptor* rhs) {
        return reinterpret_cast<const WGPUBufferDescriptor*>(rhs);
    }
    inline WGPUBufferDescriptor* ToAPI(BufferDescriptor* rhs) {
        return reinterpret_cast<WGPUBufferDescriptor*>(rhs);
    }
    inline const BufferDescriptor* FromAPI(const WGPUBufferDescriptor* rhs) {
        return reinterpret_cast<const BufferDescriptor*>(rhs);
    }
    inline BufferDescriptor* FromAPI(WGPUBufferDescriptor* rhs) {
        return reinterpret_cast<BufferDescriptor*>(rhs);
    }
    inline const BufferDescriptor& FromAPI(const WGPUBufferDescriptor& rhs) {
        return *reinterpret_cast<const BufferDescriptor*>(&rhs);
    }
    inline BufferDescriptor& FromAPI(WGPUBufferDescriptor& rhs) {
        return *reinterpret_cast<BufferDescriptor*>(&rhs);
    }

    inline const WGPUCommandEncoderDescriptor* ToAPI(const CommandEncoderDescriptor* rhs) {
        return reinterpret_cast<const WGPUCommandEncoderDescriptor*>(rhs);
    }
    inline WGPUCommandEncoderDescriptor* ToAPI(CommandEncoderDescriptor* rhs) {
        return reinterpret_cast<WGPUCommandEncoderDescriptor*>(rhs);
    }
    inline const CommandEncoderDescriptor* FromAPI(const WGPUCommandEncoderDescriptor* rhs) {
        return reinterpret_cast<const CommandEncoderDescriptor*>(rhs);
    }
    inline CommandEncoderDescriptor* FromAPI(WGPUCommandEncoderDescriptor* rhs) {
        return reinterpret_cast<CommandEncoderDescriptor*>(rhs);
    }
    inline const CommandEncoderDescriptor& FromAPI(const WGPUCommandEncoderDescriptor& rhs) {
        return *reinterpret_cast<const CommandEncoderDescriptor*>(&rhs);
    }
    inline CommandEncoderDescriptor& FromAPI(WGPUCommandEncoderDescriptor& rhs) {
        return *reinterpret_cast<CommandEncoderDescriptor*>(&rhs);
    }

    inline const WGPUCompilationMessage* ToAPI(const CompilationMessage* rhs) {
        return reinterpret_cast<const WGPUCompilationMessage*>(rhs);
    }
    inline WGPUCompilationMessage* ToAPI(CompilationMessage* rhs) {
        return reinterpret_cast<WGPUCompilationMessage*>(rhs);
    }
    inline const CompilationMessage* FromAPI(const WGPUCompilationMessage* rhs) {
        return reinterpret_cast<const CompilationMessage*>(rhs);
    }
    inline CompilationMessage* FromAPI(WGPUCompilationMessage* rhs) {
        return reinterpret_cast<CompilationMessage*>(rhs);
    }
    inline const CompilationMessage& FromAPI(const WGPUCompilationMessage& rhs) {
        return *reinterpret_cast<const CompilationMessage*>(&rhs);
    }
    inline CompilationMessage& FromAPI(WGPUCompilationMessage& rhs) {
        return *reinterpret_cast<CompilationMessage*>(&rhs);
    }

    inline const WGPUComputePassDescriptor* ToAPI(const ComputePassDescriptor* rhs) {
        return reinterpret_cast<const WGPUComputePassDescriptor*>(rhs);
    }
    inline WGPUComputePassDescriptor* ToAPI(ComputePassDescriptor* rhs) {
        return reinterpret_cast<WGPUComputePassDescriptor*>(rhs);
    }
    inline const ComputePassDescriptor* FromAPI(const WGPUComputePassDescriptor* rhs) {
        return reinterpret_cast<const ComputePassDescriptor*>(rhs);
    }
    inline ComputePassDescriptor* FromAPI(WGPUComputePassDescriptor* rhs) {
        return reinterpret_cast<ComputePassDescriptor*>(rhs);
    }
    inline const ComputePassDescriptor& FromAPI(const WGPUComputePassDescriptor& rhs) {
        return *reinterpret_cast<const ComputePassDescriptor*>(&rhs);
    }
    inline ComputePassDescriptor& FromAPI(WGPUComputePassDescriptor& rhs) {
        return *reinterpret_cast<ComputePassDescriptor*>(&rhs);
    }

    inline const WGPUComputeState* ToAPI(const ComputeState* rhs) {
        return reinterpret_cast<const WGPUComputeState*>(rhs);
    }
    inline WGPUComputeState* ToAPI(ComputeState* rhs) {
        return reinterpret_cast<WGPUComputeState*>(rhs);
    }
    inline const ComputeState* FromAPI(const WGPUComputeState* rhs) {
        return reinterpret_cast<const ComputeState*>(rhs);
    }
    inline ComputeState* FromAPI(WGPUComputeState* rhs) {
        return reinterpret_cast<ComputeState*>(rhs);
    }
    inline const ComputeState& FromAPI(const WGPUComputeState& rhs) {
        return *reinterpret_cast<const ComputeState*>(&rhs);
    }
    inline ComputeState& FromAPI(WGPUComputeState& rhs) {
        return *reinterpret_cast<ComputeState*>(&rhs);
    }

    inline const WGPUDawnDrmFormatCapabilities* ToAPI(const DawnDrmFormatCapabilities* rhs) {
        return reinterpret_cast<const WGPUDawnDrmFormatCapabilities*>(rhs);
    }
    inline WGPUDawnDrmFormatCapabilities* ToAPI(DawnDrmFormatCapabilities* rhs) {
        return reinterpret_cast<WGPUDawnDrmFormatCapabilities*>(rhs);
    }
    inline const DawnDrmFormatCapabilities* FromAPI(const WGPUDawnDrmFormatCapabilities* rhs) {
        return reinterpret_cast<const DawnDrmFormatCapabilities*>(rhs);
    }
    inline DawnDrmFormatCapabilities* FromAPI(WGPUDawnDrmFormatCapabilities* rhs) {
        return reinterpret_cast<DawnDrmFormatCapabilities*>(rhs);
    }
    inline const DawnDrmFormatCapabilities& FromAPI(const WGPUDawnDrmFormatCapabilities& rhs) {
        return *reinterpret_cast<const DawnDrmFormatCapabilities*>(&rhs);
    }
    inline DawnDrmFormatCapabilities& FromAPI(WGPUDawnDrmFormatCapabilities& rhs) {
        return *reinterpret_cast<DawnDrmFormatCapabilities*>(&rhs);
    }

    inline const WGPUDepthStencilState* ToAPI(const DepthStencilState* rhs) {
        return reinterpret_cast<const WGPUDepthStencilState*>(rhs);
    }
    inline WGPUDepthStencilState* ToAPI(DepthStencilState* rhs) {
        return reinterpret_cast<WGPUDepthStencilState*>(rhs);
    }
    inline const DepthStencilState* FromAPI(const WGPUDepthStencilState* rhs) {
        return reinterpret_cast<const DepthStencilState*>(rhs);
    }
    inline DepthStencilState* FromAPI(WGPUDepthStencilState* rhs) {
        return reinterpret_cast<DepthStencilState*>(rhs);
    }
    inline const DepthStencilState& FromAPI(const WGPUDepthStencilState& rhs) {
        return *reinterpret_cast<const DepthStencilState*>(&rhs);
    }
    inline DepthStencilState& FromAPI(WGPUDepthStencilState& rhs) {
        return *reinterpret_cast<DepthStencilState*>(&rhs);
    }

    inline const WGPUExternalTextureDescriptor* ToAPI(const ExternalTextureDescriptor* rhs) {
        return reinterpret_cast<const WGPUExternalTextureDescriptor*>(rhs);
    }
    inline WGPUExternalTextureDescriptor* ToAPI(ExternalTextureDescriptor* rhs) {
        return reinterpret_cast<WGPUExternalTextureDescriptor*>(rhs);
    }
    inline const ExternalTextureDescriptor* FromAPI(const WGPUExternalTextureDescriptor* rhs) {
        return reinterpret_cast<const ExternalTextureDescriptor*>(rhs);
    }
    inline ExternalTextureDescriptor* FromAPI(WGPUExternalTextureDescriptor* rhs) {
        return reinterpret_cast<ExternalTextureDescriptor*>(rhs);
    }
    inline const ExternalTextureDescriptor& FromAPI(const WGPUExternalTextureDescriptor& rhs) {
        return *reinterpret_cast<const ExternalTextureDescriptor*>(&rhs);
    }
    inline ExternalTextureDescriptor& FromAPI(WGPUExternalTextureDescriptor& rhs) {
        return *reinterpret_cast<ExternalTextureDescriptor*>(&rhs);
    }

    inline const WGPUFutureWaitInfo* ToAPI(const FutureWaitInfo* rhs) {
        return reinterpret_cast<const WGPUFutureWaitInfo*>(rhs);
    }
    inline WGPUFutureWaitInfo* ToAPI(FutureWaitInfo* rhs) {
        return reinterpret_cast<WGPUFutureWaitInfo*>(rhs);
    }
    inline const FutureWaitInfo* FromAPI(const WGPUFutureWaitInfo* rhs) {
        return reinterpret_cast<const FutureWaitInfo*>(rhs);
    }
    inline FutureWaitInfo* FromAPI(WGPUFutureWaitInfo* rhs) {
        return reinterpret_cast<FutureWaitInfo*>(rhs);
    }
    inline const FutureWaitInfo& FromAPI(const WGPUFutureWaitInfo& rhs) {
        return *reinterpret_cast<const FutureWaitInfo*>(&rhs);
    }
    inline FutureWaitInfo& FromAPI(WGPUFutureWaitInfo& rhs) {
        return *reinterpret_cast<FutureWaitInfo*>(&rhs);
    }

    inline const WGPUImageCopyExternalTexture* ToAPI(const ImageCopyExternalTexture* rhs) {
        return reinterpret_cast<const WGPUImageCopyExternalTexture*>(rhs);
    }
    inline WGPUImageCopyExternalTexture* ToAPI(ImageCopyExternalTexture* rhs) {
        return reinterpret_cast<WGPUImageCopyExternalTexture*>(rhs);
    }
    inline const ImageCopyExternalTexture* FromAPI(const WGPUImageCopyExternalTexture* rhs) {
        return reinterpret_cast<const ImageCopyExternalTexture*>(rhs);
    }
    inline ImageCopyExternalTexture* FromAPI(WGPUImageCopyExternalTexture* rhs) {
        return reinterpret_cast<ImageCopyExternalTexture*>(rhs);
    }
    inline const ImageCopyExternalTexture& FromAPI(const WGPUImageCopyExternalTexture& rhs) {
        return *reinterpret_cast<const ImageCopyExternalTexture*>(&rhs);
    }
    inline ImageCopyExternalTexture& FromAPI(WGPUImageCopyExternalTexture& rhs) {
        return *reinterpret_cast<ImageCopyExternalTexture*>(&rhs);
    }

    inline const WGPUInstanceDescriptor* ToAPI(const InstanceDescriptor* rhs) {
        return reinterpret_cast<const WGPUInstanceDescriptor*>(rhs);
    }
    inline WGPUInstanceDescriptor* ToAPI(InstanceDescriptor* rhs) {
        return reinterpret_cast<WGPUInstanceDescriptor*>(rhs);
    }
    inline const InstanceDescriptor* FromAPI(const WGPUInstanceDescriptor* rhs) {
        return reinterpret_cast<const InstanceDescriptor*>(rhs);
    }
    inline InstanceDescriptor* FromAPI(WGPUInstanceDescriptor* rhs) {
        return reinterpret_cast<InstanceDescriptor*>(rhs);
    }
    inline const InstanceDescriptor& FromAPI(const WGPUInstanceDescriptor& rhs) {
        return *reinterpret_cast<const InstanceDescriptor*>(&rhs);
    }
    inline InstanceDescriptor& FromAPI(WGPUInstanceDescriptor& rhs) {
        return *reinterpret_cast<InstanceDescriptor*>(&rhs);
    }

    inline const WGPULimits* ToAPI(const Limits* rhs) {
        return reinterpret_cast<const WGPULimits*>(rhs);
    }
    inline WGPULimits* ToAPI(Limits* rhs) {
        return reinterpret_cast<WGPULimits*>(rhs);
    }
    inline const Limits* FromAPI(const WGPULimits* rhs) {
        return reinterpret_cast<const Limits*>(rhs);
    }
    inline Limits* FromAPI(WGPULimits* rhs) {
        return reinterpret_cast<Limits*>(rhs);
    }
    inline const Limits& FromAPI(const WGPULimits& rhs) {
        return *reinterpret_cast<const Limits*>(&rhs);
    }
    inline Limits& FromAPI(WGPULimits& rhs) {
        return *reinterpret_cast<Limits*>(&rhs);
    }

    inline const WGPUPipelineLayoutPixelLocalStorage* ToAPI(const PipelineLayoutPixelLocalStorage* rhs) {
        return reinterpret_cast<const WGPUPipelineLayoutPixelLocalStorage*>(rhs);
    }
    inline WGPUPipelineLayoutPixelLocalStorage* ToAPI(PipelineLayoutPixelLocalStorage* rhs) {
        return reinterpret_cast<WGPUPipelineLayoutPixelLocalStorage*>(rhs);
    }
    inline const PipelineLayoutPixelLocalStorage* FromAPI(const WGPUPipelineLayoutPixelLocalStorage* rhs) {
        return reinterpret_cast<const PipelineLayoutPixelLocalStorage*>(rhs);
    }
    inline PipelineLayoutPixelLocalStorage* FromAPI(WGPUPipelineLayoutPixelLocalStorage* rhs) {
        return reinterpret_cast<PipelineLayoutPixelLocalStorage*>(rhs);
    }
    inline const PipelineLayoutPixelLocalStorage& FromAPI(const WGPUPipelineLayoutPixelLocalStorage& rhs) {
        return *reinterpret_cast<const PipelineLayoutPixelLocalStorage*>(&rhs);
    }
    inline PipelineLayoutPixelLocalStorage& FromAPI(WGPUPipelineLayoutPixelLocalStorage& rhs) {
        return *reinterpret_cast<PipelineLayoutPixelLocalStorage*>(&rhs);
    }

    inline const WGPURenderPassColorAttachment* ToAPI(const RenderPassColorAttachment* rhs) {
        return reinterpret_cast<const WGPURenderPassColorAttachment*>(rhs);
    }
    inline WGPURenderPassColorAttachment* ToAPI(RenderPassColorAttachment* rhs) {
        return reinterpret_cast<WGPURenderPassColorAttachment*>(rhs);
    }
    inline const RenderPassColorAttachment* FromAPI(const WGPURenderPassColorAttachment* rhs) {
        return reinterpret_cast<const RenderPassColorAttachment*>(rhs);
    }
    inline RenderPassColorAttachment* FromAPI(WGPURenderPassColorAttachment* rhs) {
        return reinterpret_cast<RenderPassColorAttachment*>(rhs);
    }
    inline const RenderPassColorAttachment& FromAPI(const WGPURenderPassColorAttachment& rhs) {
        return *reinterpret_cast<const RenderPassColorAttachment*>(&rhs);
    }
    inline RenderPassColorAttachment& FromAPI(WGPURenderPassColorAttachment& rhs) {
        return *reinterpret_cast<RenderPassColorAttachment*>(&rhs);
    }

    inline const WGPURenderPassRenderAreaRect* ToAPI(const RenderPassRenderAreaRect* rhs) {
        return reinterpret_cast<const WGPURenderPassRenderAreaRect*>(rhs);
    }
    inline WGPURenderPassRenderAreaRect* ToAPI(RenderPassRenderAreaRect* rhs) {
        return reinterpret_cast<WGPURenderPassRenderAreaRect*>(rhs);
    }
    inline const RenderPassRenderAreaRect* FromAPI(const WGPURenderPassRenderAreaRect* rhs) {
        return reinterpret_cast<const RenderPassRenderAreaRect*>(rhs);
    }
    inline RenderPassRenderAreaRect* FromAPI(WGPURenderPassRenderAreaRect* rhs) {
        return reinterpret_cast<RenderPassRenderAreaRect*>(rhs);
    }
    inline const RenderPassRenderAreaRect& FromAPI(const WGPURenderPassRenderAreaRect& rhs) {
        return *reinterpret_cast<const RenderPassRenderAreaRect*>(&rhs);
    }
    inline RenderPassRenderAreaRect& FromAPI(WGPURenderPassRenderAreaRect& rhs) {
        return *reinterpret_cast<RenderPassRenderAreaRect*>(&rhs);
    }

    inline const WGPURenderPassStorageAttachment* ToAPI(const RenderPassStorageAttachment* rhs) {
        return reinterpret_cast<const WGPURenderPassStorageAttachment*>(rhs);
    }
    inline WGPURenderPassStorageAttachment* ToAPI(RenderPassStorageAttachment* rhs) {
        return reinterpret_cast<WGPURenderPassStorageAttachment*>(rhs);
    }
    inline const RenderPassStorageAttachment* FromAPI(const WGPURenderPassStorageAttachment* rhs) {
        return reinterpret_cast<const RenderPassStorageAttachment*>(rhs);
    }
    inline RenderPassStorageAttachment* FromAPI(WGPURenderPassStorageAttachment* rhs) {
        return reinterpret_cast<RenderPassStorageAttachment*>(rhs);
    }
    inline const RenderPassStorageAttachment& FromAPI(const WGPURenderPassStorageAttachment& rhs) {
        return *reinterpret_cast<const RenderPassStorageAttachment*>(&rhs);
    }
    inline RenderPassStorageAttachment& FromAPI(WGPURenderPassStorageAttachment& rhs) {
        return *reinterpret_cast<RenderPassStorageAttachment*>(&rhs);
    }

    inline const WGPURequestAdapterOptions* ToAPI(const RequestAdapterOptions* rhs) {
        return reinterpret_cast<const WGPURequestAdapterOptions*>(rhs);
    }
    inline WGPURequestAdapterOptions* ToAPI(RequestAdapterOptions* rhs) {
        return reinterpret_cast<WGPURequestAdapterOptions*>(rhs);
    }
    inline const RequestAdapterOptions* FromAPI(const WGPURequestAdapterOptions* rhs) {
        return reinterpret_cast<const RequestAdapterOptions*>(rhs);
    }
    inline RequestAdapterOptions* FromAPI(WGPURequestAdapterOptions* rhs) {
        return reinterpret_cast<RequestAdapterOptions*>(rhs);
    }
    inline const RequestAdapterOptions& FromAPI(const WGPURequestAdapterOptions& rhs) {
        return *reinterpret_cast<const RequestAdapterOptions*>(&rhs);
    }
    inline RequestAdapterOptions& FromAPI(WGPURequestAdapterOptions& rhs) {
        return *reinterpret_cast<RequestAdapterOptions*>(&rhs);
    }

    inline const WGPUSamplerDescriptor* ToAPI(const SamplerDescriptor* rhs) {
        return reinterpret_cast<const WGPUSamplerDescriptor*>(rhs);
    }
    inline WGPUSamplerDescriptor* ToAPI(SamplerDescriptor* rhs) {
        return reinterpret_cast<WGPUSamplerDescriptor*>(rhs);
    }
    inline const SamplerDescriptor* FromAPI(const WGPUSamplerDescriptor* rhs) {
        return reinterpret_cast<const SamplerDescriptor*>(rhs);
    }
    inline SamplerDescriptor* FromAPI(WGPUSamplerDescriptor* rhs) {
        return reinterpret_cast<SamplerDescriptor*>(rhs);
    }
    inline const SamplerDescriptor& FromAPI(const WGPUSamplerDescriptor& rhs) {
        return *reinterpret_cast<const SamplerDescriptor*>(&rhs);
    }
    inline SamplerDescriptor& FromAPI(WGPUSamplerDescriptor& rhs) {
        return *reinterpret_cast<SamplerDescriptor*>(&rhs);
    }

    inline const WGPUShaderModuleDescriptor* ToAPI(const ShaderModuleDescriptor* rhs) {
        return reinterpret_cast<const WGPUShaderModuleDescriptor*>(rhs);
    }
    inline WGPUShaderModuleDescriptor* ToAPI(ShaderModuleDescriptor* rhs) {
        return reinterpret_cast<WGPUShaderModuleDescriptor*>(rhs);
    }
    inline const ShaderModuleDescriptor* FromAPI(const WGPUShaderModuleDescriptor* rhs) {
        return reinterpret_cast<const ShaderModuleDescriptor*>(rhs);
    }
    inline ShaderModuleDescriptor* FromAPI(WGPUShaderModuleDescriptor* rhs) {
        return reinterpret_cast<ShaderModuleDescriptor*>(rhs);
    }
    inline const ShaderModuleDescriptor& FromAPI(const WGPUShaderModuleDescriptor& rhs) {
        return *reinterpret_cast<const ShaderModuleDescriptor*>(&rhs);
    }
    inline ShaderModuleDescriptor& FromAPI(WGPUShaderModuleDescriptor& rhs) {
        return *reinterpret_cast<ShaderModuleDescriptor*>(&rhs);
    }

    inline const WGPUSurfaceDescriptor* ToAPI(const SurfaceDescriptor* rhs) {
        return reinterpret_cast<const WGPUSurfaceDescriptor*>(rhs);
    }
    inline WGPUSurfaceDescriptor* ToAPI(SurfaceDescriptor* rhs) {
        return reinterpret_cast<WGPUSurfaceDescriptor*>(rhs);
    }
    inline const SurfaceDescriptor* FromAPI(const WGPUSurfaceDescriptor* rhs) {
        return reinterpret_cast<const SurfaceDescriptor*>(rhs);
    }
    inline SurfaceDescriptor* FromAPI(WGPUSurfaceDescriptor* rhs) {
        return reinterpret_cast<SurfaceDescriptor*>(rhs);
    }
    inline const SurfaceDescriptor& FromAPI(const WGPUSurfaceDescriptor& rhs) {
        return *reinterpret_cast<const SurfaceDescriptor*>(&rhs);
    }
    inline SurfaceDescriptor& FromAPI(WGPUSurfaceDescriptor& rhs) {
        return *reinterpret_cast<SurfaceDescriptor*>(&rhs);
    }

    inline const WGPUTexelCopyBufferInfo* ToAPI(const TexelCopyBufferInfo* rhs) {
        return reinterpret_cast<const WGPUTexelCopyBufferInfo*>(rhs);
    }
    inline WGPUTexelCopyBufferInfo* ToAPI(TexelCopyBufferInfo* rhs) {
        return reinterpret_cast<WGPUTexelCopyBufferInfo*>(rhs);
    }
    inline const TexelCopyBufferInfo* FromAPI(const WGPUTexelCopyBufferInfo* rhs) {
        return reinterpret_cast<const TexelCopyBufferInfo*>(rhs);
    }
    inline TexelCopyBufferInfo* FromAPI(WGPUTexelCopyBufferInfo* rhs) {
        return reinterpret_cast<TexelCopyBufferInfo*>(rhs);
    }
    inline const TexelCopyBufferInfo& FromAPI(const WGPUTexelCopyBufferInfo& rhs) {
        return *reinterpret_cast<const TexelCopyBufferInfo*>(&rhs);
    }
    inline TexelCopyBufferInfo& FromAPI(WGPUTexelCopyBufferInfo& rhs) {
        return *reinterpret_cast<TexelCopyBufferInfo*>(&rhs);
    }

    inline const WGPUTexelCopyTextureInfo* ToAPI(const TexelCopyTextureInfo* rhs) {
        return reinterpret_cast<const WGPUTexelCopyTextureInfo*>(rhs);
    }
    inline WGPUTexelCopyTextureInfo* ToAPI(TexelCopyTextureInfo* rhs) {
        return reinterpret_cast<WGPUTexelCopyTextureInfo*>(rhs);
    }
    inline const TexelCopyTextureInfo* FromAPI(const WGPUTexelCopyTextureInfo* rhs) {
        return reinterpret_cast<const TexelCopyTextureInfo*>(rhs);
    }
    inline TexelCopyTextureInfo* FromAPI(WGPUTexelCopyTextureInfo* rhs) {
        return reinterpret_cast<TexelCopyTextureInfo*>(rhs);
    }
    inline const TexelCopyTextureInfo& FromAPI(const WGPUTexelCopyTextureInfo& rhs) {
        return *reinterpret_cast<const TexelCopyTextureInfo*>(&rhs);
    }
    inline TexelCopyTextureInfo& FromAPI(WGPUTexelCopyTextureInfo& rhs) {
        return *reinterpret_cast<TexelCopyTextureInfo*>(&rhs);
    }

    inline const WGPUTextureComponentSwizzleDescriptor* ToAPI(const TextureComponentSwizzleDescriptor* rhs) {
        return reinterpret_cast<const WGPUTextureComponentSwizzleDescriptor*>(rhs);
    }
    inline WGPUTextureComponentSwizzleDescriptor* ToAPI(TextureComponentSwizzleDescriptor* rhs) {
        return reinterpret_cast<WGPUTextureComponentSwizzleDescriptor*>(rhs);
    }
    inline const TextureComponentSwizzleDescriptor* FromAPI(const WGPUTextureComponentSwizzleDescriptor* rhs) {
        return reinterpret_cast<const TextureComponentSwizzleDescriptor*>(rhs);
    }
    inline TextureComponentSwizzleDescriptor* FromAPI(WGPUTextureComponentSwizzleDescriptor* rhs) {
        return reinterpret_cast<TextureComponentSwizzleDescriptor*>(rhs);
    }
    inline const TextureComponentSwizzleDescriptor& FromAPI(const WGPUTextureComponentSwizzleDescriptor& rhs) {
        return *reinterpret_cast<const TextureComponentSwizzleDescriptor*>(&rhs);
    }
    inline TextureComponentSwizzleDescriptor& FromAPI(WGPUTextureComponentSwizzleDescriptor& rhs) {
        return *reinterpret_cast<TextureComponentSwizzleDescriptor*>(&rhs);
    }

    inline const WGPUTextureDescriptor* ToAPI(const TextureDescriptor* rhs) {
        return reinterpret_cast<const WGPUTextureDescriptor*>(rhs);
    }
    inline WGPUTextureDescriptor* ToAPI(TextureDescriptor* rhs) {
        return reinterpret_cast<WGPUTextureDescriptor*>(rhs);
    }
    inline const TextureDescriptor* FromAPI(const WGPUTextureDescriptor* rhs) {
        return reinterpret_cast<const TextureDescriptor*>(rhs);
    }
    inline TextureDescriptor* FromAPI(WGPUTextureDescriptor* rhs) {
        return reinterpret_cast<TextureDescriptor*>(rhs);
    }
    inline const TextureDescriptor& FromAPI(const WGPUTextureDescriptor& rhs) {
        return *reinterpret_cast<const TextureDescriptor*>(&rhs);
    }
    inline TextureDescriptor& FromAPI(WGPUTextureDescriptor& rhs) {
        return *reinterpret_cast<TextureDescriptor*>(&rhs);
    }

    inline const WGPUVertexBufferLayout* ToAPI(const VertexBufferLayout* rhs) {
        return reinterpret_cast<const WGPUVertexBufferLayout*>(rhs);
    }
    inline WGPUVertexBufferLayout* ToAPI(VertexBufferLayout* rhs) {
        return reinterpret_cast<WGPUVertexBufferLayout*>(rhs);
    }
    inline const VertexBufferLayout* FromAPI(const WGPUVertexBufferLayout* rhs) {
        return reinterpret_cast<const VertexBufferLayout*>(rhs);
    }
    inline VertexBufferLayout* FromAPI(WGPUVertexBufferLayout* rhs) {
        return reinterpret_cast<VertexBufferLayout*>(rhs);
    }
    inline const VertexBufferLayout& FromAPI(const WGPUVertexBufferLayout& rhs) {
        return *reinterpret_cast<const VertexBufferLayout*>(&rhs);
    }
    inline VertexBufferLayout& FromAPI(WGPUVertexBufferLayout& rhs) {
        return *reinterpret_cast<VertexBufferLayout*>(&rhs);
    }

    inline const WGPUAdapterInfo* ToAPI(const AdapterInfo* rhs) {
        return reinterpret_cast<const WGPUAdapterInfo*>(rhs);
    }
    inline WGPUAdapterInfo* ToAPI(AdapterInfo* rhs) {
        return reinterpret_cast<WGPUAdapterInfo*>(rhs);
    }
    inline const AdapterInfo* FromAPI(const WGPUAdapterInfo* rhs) {
        return reinterpret_cast<const AdapterInfo*>(rhs);
    }
    inline AdapterInfo* FromAPI(WGPUAdapterInfo* rhs) {
        return reinterpret_cast<AdapterInfo*>(rhs);
    }
    inline const AdapterInfo& FromAPI(const WGPUAdapterInfo& rhs) {
        return *reinterpret_cast<const AdapterInfo*>(&rhs);
    }
    inline AdapterInfo& FromAPI(WGPUAdapterInfo& rhs) {
        return *reinterpret_cast<AdapterInfo*>(&rhs);
    }

    inline const WGPUBindGroupDescriptor* ToAPI(const BindGroupDescriptor* rhs) {
        return reinterpret_cast<const WGPUBindGroupDescriptor*>(rhs);
    }
    inline WGPUBindGroupDescriptor* ToAPI(BindGroupDescriptor* rhs) {
        return reinterpret_cast<WGPUBindGroupDescriptor*>(rhs);
    }
    inline const BindGroupDescriptor* FromAPI(const WGPUBindGroupDescriptor* rhs) {
        return reinterpret_cast<const BindGroupDescriptor*>(rhs);
    }
    inline BindGroupDescriptor* FromAPI(WGPUBindGroupDescriptor* rhs) {
        return reinterpret_cast<BindGroupDescriptor*>(rhs);
    }
    inline const BindGroupDescriptor& FromAPI(const WGPUBindGroupDescriptor& rhs) {
        return *reinterpret_cast<const BindGroupDescriptor*>(&rhs);
    }
    inline BindGroupDescriptor& FromAPI(WGPUBindGroupDescriptor& rhs) {
        return *reinterpret_cast<BindGroupDescriptor*>(&rhs);
    }

    inline const WGPUBindGroupLayoutDescriptor* ToAPI(const BindGroupLayoutDescriptor* rhs) {
        return reinterpret_cast<const WGPUBindGroupLayoutDescriptor*>(rhs);
    }
    inline WGPUBindGroupLayoutDescriptor* ToAPI(BindGroupLayoutDescriptor* rhs) {
        return reinterpret_cast<WGPUBindGroupLayoutDescriptor*>(rhs);
    }
    inline const BindGroupLayoutDescriptor* FromAPI(const WGPUBindGroupLayoutDescriptor* rhs) {
        return reinterpret_cast<const BindGroupLayoutDescriptor*>(rhs);
    }
    inline BindGroupLayoutDescriptor* FromAPI(WGPUBindGroupLayoutDescriptor* rhs) {
        return reinterpret_cast<BindGroupLayoutDescriptor*>(rhs);
    }
    inline const BindGroupLayoutDescriptor& FromAPI(const WGPUBindGroupLayoutDescriptor& rhs) {
        return *reinterpret_cast<const BindGroupLayoutDescriptor*>(&rhs);
    }
    inline BindGroupLayoutDescriptor& FromAPI(WGPUBindGroupLayoutDescriptor& rhs) {
        return *reinterpret_cast<BindGroupLayoutDescriptor*>(&rhs);
    }

    inline const WGPUColorTargetState* ToAPI(const ColorTargetState* rhs) {
        return reinterpret_cast<const WGPUColorTargetState*>(rhs);
    }
    inline WGPUColorTargetState* ToAPI(ColorTargetState* rhs) {
        return reinterpret_cast<WGPUColorTargetState*>(rhs);
    }
    inline const ColorTargetState* FromAPI(const WGPUColorTargetState* rhs) {
        return reinterpret_cast<const ColorTargetState*>(rhs);
    }
    inline ColorTargetState* FromAPI(WGPUColorTargetState* rhs) {
        return reinterpret_cast<ColorTargetState*>(rhs);
    }
    inline const ColorTargetState& FromAPI(const WGPUColorTargetState& rhs) {
        return *reinterpret_cast<const ColorTargetState*>(&rhs);
    }
    inline ColorTargetState& FromAPI(WGPUColorTargetState& rhs) {
        return *reinterpret_cast<ColorTargetState*>(&rhs);
    }

    inline const WGPUCompilationInfo* ToAPI(const CompilationInfo* rhs) {
        return reinterpret_cast<const WGPUCompilationInfo*>(rhs);
    }
    inline WGPUCompilationInfo* ToAPI(CompilationInfo* rhs) {
        return reinterpret_cast<WGPUCompilationInfo*>(rhs);
    }
    inline const CompilationInfo* FromAPI(const WGPUCompilationInfo* rhs) {
        return reinterpret_cast<const CompilationInfo*>(rhs);
    }
    inline CompilationInfo* FromAPI(WGPUCompilationInfo* rhs) {
        return reinterpret_cast<CompilationInfo*>(rhs);
    }
    inline const CompilationInfo& FromAPI(const WGPUCompilationInfo& rhs) {
        return *reinterpret_cast<const CompilationInfo*>(&rhs);
    }
    inline CompilationInfo& FromAPI(WGPUCompilationInfo& rhs) {
        return *reinterpret_cast<CompilationInfo*>(&rhs);
    }

    inline const WGPUComputePipelineDescriptor* ToAPI(const ComputePipelineDescriptor* rhs) {
        return reinterpret_cast<const WGPUComputePipelineDescriptor*>(rhs);
    }
    inline WGPUComputePipelineDescriptor* ToAPI(ComputePipelineDescriptor* rhs) {
        return reinterpret_cast<WGPUComputePipelineDescriptor*>(rhs);
    }
    inline const ComputePipelineDescriptor* FromAPI(const WGPUComputePipelineDescriptor* rhs) {
        return reinterpret_cast<const ComputePipelineDescriptor*>(rhs);
    }
    inline ComputePipelineDescriptor* FromAPI(WGPUComputePipelineDescriptor* rhs) {
        return reinterpret_cast<ComputePipelineDescriptor*>(rhs);
    }
    inline const ComputePipelineDescriptor& FromAPI(const WGPUComputePipelineDescriptor& rhs) {
        return *reinterpret_cast<const ComputePipelineDescriptor*>(&rhs);
    }
    inline ComputePipelineDescriptor& FromAPI(WGPUComputePipelineDescriptor& rhs) {
        return *reinterpret_cast<ComputePipelineDescriptor*>(&rhs);
    }

    inline const WGPUDawnFormatCapabilities* ToAPI(const DawnFormatCapabilities* rhs) {
        return reinterpret_cast<const WGPUDawnFormatCapabilities*>(rhs);
    }
    inline WGPUDawnFormatCapabilities* ToAPI(DawnFormatCapabilities* rhs) {
        return reinterpret_cast<WGPUDawnFormatCapabilities*>(rhs);
    }
    inline const DawnFormatCapabilities* FromAPI(const WGPUDawnFormatCapabilities* rhs) {
        return reinterpret_cast<const DawnFormatCapabilities*>(rhs);
    }
    inline DawnFormatCapabilities* FromAPI(WGPUDawnFormatCapabilities* rhs) {
        return reinterpret_cast<DawnFormatCapabilities*>(rhs);
    }
    inline const DawnFormatCapabilities& FromAPI(const WGPUDawnFormatCapabilities& rhs) {
        return *reinterpret_cast<const DawnFormatCapabilities*>(&rhs);
    }
    inline DawnFormatCapabilities& FromAPI(WGPUDawnFormatCapabilities& rhs) {
        return *reinterpret_cast<DawnFormatCapabilities*>(&rhs);
    }

    inline const WGPUDeviceDescriptor* ToAPI(const DeviceDescriptor* rhs) {
        return reinterpret_cast<const WGPUDeviceDescriptor*>(rhs);
    }
    inline WGPUDeviceDescriptor* ToAPI(DeviceDescriptor* rhs) {
        return reinterpret_cast<WGPUDeviceDescriptor*>(rhs);
    }
    inline const DeviceDescriptor* FromAPI(const WGPUDeviceDescriptor* rhs) {
        return reinterpret_cast<const DeviceDescriptor*>(rhs);
    }
    inline DeviceDescriptor* FromAPI(WGPUDeviceDescriptor* rhs) {
        return reinterpret_cast<DeviceDescriptor*>(rhs);
    }
    inline const DeviceDescriptor& FromAPI(const WGPUDeviceDescriptor& rhs) {
        return *reinterpret_cast<const DeviceDescriptor*>(&rhs);
    }
    inline DeviceDescriptor& FromAPI(WGPUDeviceDescriptor& rhs) {
        return *reinterpret_cast<DeviceDescriptor*>(&rhs);
    }

    inline const WGPUPipelineLayoutDescriptor* ToAPI(const PipelineLayoutDescriptor* rhs) {
        return reinterpret_cast<const WGPUPipelineLayoutDescriptor*>(rhs);
    }
    inline WGPUPipelineLayoutDescriptor* ToAPI(PipelineLayoutDescriptor* rhs) {
        return reinterpret_cast<WGPUPipelineLayoutDescriptor*>(rhs);
    }
    inline const PipelineLayoutDescriptor* FromAPI(const WGPUPipelineLayoutDescriptor* rhs) {
        return reinterpret_cast<const PipelineLayoutDescriptor*>(rhs);
    }
    inline PipelineLayoutDescriptor* FromAPI(WGPUPipelineLayoutDescriptor* rhs) {
        return reinterpret_cast<PipelineLayoutDescriptor*>(rhs);
    }
    inline const PipelineLayoutDescriptor& FromAPI(const WGPUPipelineLayoutDescriptor& rhs) {
        return *reinterpret_cast<const PipelineLayoutDescriptor*>(&rhs);
    }
    inline PipelineLayoutDescriptor& FromAPI(WGPUPipelineLayoutDescriptor& rhs) {
        return *reinterpret_cast<PipelineLayoutDescriptor*>(&rhs);
    }

    inline const WGPURenderPassPixelLocalStorage* ToAPI(const RenderPassPixelLocalStorage* rhs) {
        return reinterpret_cast<const WGPURenderPassPixelLocalStorage*>(rhs);
    }
    inline WGPURenderPassPixelLocalStorage* ToAPI(RenderPassPixelLocalStorage* rhs) {
        return reinterpret_cast<WGPURenderPassPixelLocalStorage*>(rhs);
    }
    inline const RenderPassPixelLocalStorage* FromAPI(const WGPURenderPassPixelLocalStorage* rhs) {
        return reinterpret_cast<const RenderPassPixelLocalStorage*>(rhs);
    }
    inline RenderPassPixelLocalStorage* FromAPI(WGPURenderPassPixelLocalStorage* rhs) {
        return reinterpret_cast<RenderPassPixelLocalStorage*>(rhs);
    }
    inline const RenderPassPixelLocalStorage& FromAPI(const WGPURenderPassPixelLocalStorage& rhs) {
        return *reinterpret_cast<const RenderPassPixelLocalStorage*>(&rhs);
    }
    inline RenderPassPixelLocalStorage& FromAPI(WGPURenderPassPixelLocalStorage& rhs) {
        return *reinterpret_cast<RenderPassPixelLocalStorage*>(&rhs);
    }

    inline const WGPUTextureViewDescriptor* ToAPI(const TextureViewDescriptor* rhs) {
        return reinterpret_cast<const WGPUTextureViewDescriptor*>(rhs);
    }
    inline WGPUTextureViewDescriptor* ToAPI(TextureViewDescriptor* rhs) {
        return reinterpret_cast<WGPUTextureViewDescriptor*>(rhs);
    }
    inline const TextureViewDescriptor* FromAPI(const WGPUTextureViewDescriptor* rhs) {
        return reinterpret_cast<const TextureViewDescriptor*>(rhs);
    }
    inline TextureViewDescriptor* FromAPI(WGPUTextureViewDescriptor* rhs) {
        return reinterpret_cast<TextureViewDescriptor*>(rhs);
    }
    inline const TextureViewDescriptor& FromAPI(const WGPUTextureViewDescriptor& rhs) {
        return *reinterpret_cast<const TextureViewDescriptor*>(&rhs);
    }
    inline TextureViewDescriptor& FromAPI(WGPUTextureViewDescriptor& rhs) {
        return *reinterpret_cast<TextureViewDescriptor*>(&rhs);
    }

    inline const WGPUVertexState* ToAPI(const VertexState* rhs) {
        return reinterpret_cast<const WGPUVertexState*>(rhs);
    }
    inline WGPUVertexState* ToAPI(VertexState* rhs) {
        return reinterpret_cast<WGPUVertexState*>(rhs);
    }
    inline const VertexState* FromAPI(const WGPUVertexState* rhs) {
        return reinterpret_cast<const VertexState*>(rhs);
    }
    inline VertexState* FromAPI(WGPUVertexState* rhs) {
        return reinterpret_cast<VertexState*>(rhs);
    }
    inline const VertexState& FromAPI(const WGPUVertexState& rhs) {
        return *reinterpret_cast<const VertexState*>(&rhs);
    }
    inline VertexState& FromAPI(WGPUVertexState& rhs) {
        return *reinterpret_cast<VertexState*>(&rhs);
    }

    inline const WGPUFragmentState* ToAPI(const FragmentState* rhs) {
        return reinterpret_cast<const WGPUFragmentState*>(rhs);
    }
    inline WGPUFragmentState* ToAPI(FragmentState* rhs) {
        return reinterpret_cast<WGPUFragmentState*>(rhs);
    }
    inline const FragmentState* FromAPI(const WGPUFragmentState* rhs) {
        return reinterpret_cast<const FragmentState*>(rhs);
    }
    inline FragmentState* FromAPI(WGPUFragmentState* rhs) {
        return reinterpret_cast<FragmentState*>(rhs);
    }
    inline const FragmentState& FromAPI(const WGPUFragmentState& rhs) {
        return *reinterpret_cast<const FragmentState*>(&rhs);
    }
    inline FragmentState& FromAPI(WGPUFragmentState& rhs) {
        return *reinterpret_cast<FragmentState*>(&rhs);
    }

    inline const WGPURenderPassDescriptor* ToAPI(const RenderPassDescriptor* rhs) {
        return reinterpret_cast<const WGPURenderPassDescriptor*>(rhs);
    }
    inline WGPURenderPassDescriptor* ToAPI(RenderPassDescriptor* rhs) {
        return reinterpret_cast<WGPURenderPassDescriptor*>(rhs);
    }
    inline const RenderPassDescriptor* FromAPI(const WGPURenderPassDescriptor* rhs) {
        return reinterpret_cast<const RenderPassDescriptor*>(rhs);
    }
    inline RenderPassDescriptor* FromAPI(WGPURenderPassDescriptor* rhs) {
        return reinterpret_cast<RenderPassDescriptor*>(rhs);
    }
    inline const RenderPassDescriptor& FromAPI(const WGPURenderPassDescriptor& rhs) {
        return *reinterpret_cast<const RenderPassDescriptor*>(&rhs);
    }
    inline RenderPassDescriptor& FromAPI(WGPURenderPassDescriptor& rhs) {
        return *reinterpret_cast<RenderPassDescriptor*>(&rhs);
    }

    inline const WGPURenderPipelineDescriptor* ToAPI(const RenderPipelineDescriptor* rhs) {
        return reinterpret_cast<const WGPURenderPipelineDescriptor*>(rhs);
    }
    inline WGPURenderPipelineDescriptor* ToAPI(RenderPipelineDescriptor* rhs) {
        return reinterpret_cast<WGPURenderPipelineDescriptor*>(rhs);
    }
    inline const RenderPipelineDescriptor* FromAPI(const WGPURenderPipelineDescriptor* rhs) {
        return reinterpret_cast<const RenderPipelineDescriptor*>(rhs);
    }
    inline RenderPipelineDescriptor* FromAPI(WGPURenderPipelineDescriptor* rhs) {
        return reinterpret_cast<RenderPipelineDescriptor*>(rhs);
    }
    inline const RenderPipelineDescriptor& FromAPI(const WGPURenderPipelineDescriptor& rhs) {
        return *reinterpret_cast<const RenderPipelineDescriptor*>(&rhs);
    }
    inline RenderPipelineDescriptor& FromAPI(WGPURenderPipelineDescriptor& rhs) {
        return *reinterpret_cast<RenderPipelineDescriptor*>(&rhs);
    }


    inline WGPUAdapter ToAPI(Adapter* rhs) {
        return reinterpret_cast<WGPUAdapter>(rhs);
    }

    inline Adapter* FromAPI(WGPUAdapter rhs) {
        return reinterpret_cast<Adapter*>(rhs);
    }
    inline const Adapter* const* FromAPI(const WGPUAdapter* rhs) {
        return reinterpret_cast<const Adapter* const*>(rhs);
    }
    inline Adapter* const* FromAPI(WGPUAdapter* rhs) {
        return reinterpret_cast<Adapter* const*>(rhs);
    }
    inline WGPUBindGroup ToAPI(BindGroup* rhs) {
        return reinterpret_cast<WGPUBindGroup>(rhs);
    }

    inline BindGroup* FromAPI(WGPUBindGroup rhs) {
        return reinterpret_cast<BindGroup*>(rhs);
    }
    inline const BindGroup* const* FromAPI(const WGPUBindGroup* rhs) {
        return reinterpret_cast<const BindGroup* const*>(rhs);
    }
    inline BindGroup* const* FromAPI(WGPUBindGroup* rhs) {
        return reinterpret_cast<BindGroup* const*>(rhs);
    }
    inline WGPUBindGroupLayout ToAPI(BindGroupLayout* rhs) {
        return reinterpret_cast<WGPUBindGroupLayout>(rhs);
    }

    inline BindGroupLayout* FromAPI(WGPUBindGroupLayout rhs) {
        return reinterpret_cast<BindGroupLayout*>(rhs);
    }
    inline const BindGroupLayout* const* FromAPI(const WGPUBindGroupLayout* rhs) {
        return reinterpret_cast<const BindGroupLayout* const*>(rhs);
    }
    inline BindGroupLayout* const* FromAPI(WGPUBindGroupLayout* rhs) {
        return reinterpret_cast<BindGroupLayout* const*>(rhs);
    }
    inline WGPUBuffer ToAPI(Buffer* rhs) {
        return reinterpret_cast<WGPUBuffer>(rhs);
    }

    inline Buffer* FromAPI(WGPUBuffer rhs) {
        return reinterpret_cast<Buffer*>(rhs);
    }
    inline const Buffer* const* FromAPI(const WGPUBuffer* rhs) {
        return reinterpret_cast<const Buffer* const*>(rhs);
    }
    inline Buffer* const* FromAPI(WGPUBuffer* rhs) {
        return reinterpret_cast<Buffer* const*>(rhs);
    }
    inline WGPUCommandBuffer ToAPI(CommandBuffer* rhs) {
        return reinterpret_cast<WGPUCommandBuffer>(rhs);
    }

    inline CommandBuffer* FromAPI(WGPUCommandBuffer rhs) {
        return reinterpret_cast<CommandBuffer*>(rhs);
    }
    inline const CommandBuffer* const* FromAPI(const WGPUCommandBuffer* rhs) {
        return reinterpret_cast<const CommandBuffer* const*>(rhs);
    }
    inline CommandBuffer* const* FromAPI(WGPUCommandBuffer* rhs) {
        return reinterpret_cast<CommandBuffer* const*>(rhs);
    }
    inline WGPUCommandEncoder ToAPI(CommandEncoder* rhs) {
        return reinterpret_cast<WGPUCommandEncoder>(rhs);
    }

    inline CommandEncoder* FromAPI(WGPUCommandEncoder rhs) {
        return reinterpret_cast<CommandEncoder*>(rhs);
    }
    inline const CommandEncoder* const* FromAPI(const WGPUCommandEncoder* rhs) {
        return reinterpret_cast<const CommandEncoder* const*>(rhs);
    }
    inline CommandEncoder* const* FromAPI(WGPUCommandEncoder* rhs) {
        return reinterpret_cast<CommandEncoder* const*>(rhs);
    }
    inline WGPUComputePipeline ToAPI(ComputePipeline* rhs) {
        return reinterpret_cast<WGPUComputePipeline>(rhs);
    }

    inline ComputePipeline* FromAPI(WGPUComputePipeline rhs) {
        return reinterpret_cast<ComputePipeline*>(rhs);
    }
    inline const ComputePipeline* const* FromAPI(const WGPUComputePipeline* rhs) {
        return reinterpret_cast<const ComputePipeline* const*>(rhs);
    }
    inline ComputePipeline* const* FromAPI(WGPUComputePipeline* rhs) {
        return reinterpret_cast<ComputePipeline* const*>(rhs);
    }
    inline WGPUDevice ToAPI(Device* rhs) {
        return reinterpret_cast<WGPUDevice>(rhs);
    }

    inline Device* FromAPI(WGPUDevice rhs) {
        return reinterpret_cast<Device*>(rhs);
    }
    inline const Device* const* FromAPI(const WGPUDevice* rhs) {
        return reinterpret_cast<const Device* const*>(rhs);
    }
    inline Device* const* FromAPI(WGPUDevice* rhs) {
        return reinterpret_cast<Device* const*>(rhs);
    }
    inline WGPUExternalTexture ToAPI(ExternalTexture* rhs) {
        return reinterpret_cast<WGPUExternalTexture>(rhs);
    }

    inline ExternalTexture* FromAPI(WGPUExternalTexture rhs) {
        return reinterpret_cast<ExternalTexture*>(rhs);
    }
    inline const ExternalTexture* const* FromAPI(const WGPUExternalTexture* rhs) {
        return reinterpret_cast<const ExternalTexture* const*>(rhs);
    }
    inline ExternalTexture* const* FromAPI(WGPUExternalTexture* rhs) {
        return reinterpret_cast<ExternalTexture* const*>(rhs);
    }
    inline WGPUInstance ToAPI(Instance* rhs) {
        return reinterpret_cast<WGPUInstance>(rhs);
    }

    inline Instance* FromAPI(WGPUInstance rhs) {
        return reinterpret_cast<Instance*>(rhs);
    }
    inline const Instance* const* FromAPI(const WGPUInstance* rhs) {
        return reinterpret_cast<const Instance* const*>(rhs);
    }
    inline Instance* const* FromAPI(WGPUInstance* rhs) {
        return reinterpret_cast<Instance* const*>(rhs);
    }
    inline WGPUPipelineLayout ToAPI(PipelineLayout* rhs) {
        return reinterpret_cast<WGPUPipelineLayout>(rhs);
    }

    inline PipelineLayout* FromAPI(WGPUPipelineLayout rhs) {
        return reinterpret_cast<PipelineLayout*>(rhs);
    }
    inline const PipelineLayout* const* FromAPI(const WGPUPipelineLayout* rhs) {
        return reinterpret_cast<const PipelineLayout* const*>(rhs);
    }
    inline PipelineLayout* const* FromAPI(WGPUPipelineLayout* rhs) {
        return reinterpret_cast<PipelineLayout* const*>(rhs);
    }
    inline WGPUQuerySet ToAPI(QuerySet* rhs) {
        return reinterpret_cast<WGPUQuerySet>(rhs);
    }

    inline QuerySet* FromAPI(WGPUQuerySet rhs) {
        return reinterpret_cast<QuerySet*>(rhs);
    }
    inline const QuerySet* const* FromAPI(const WGPUQuerySet* rhs) {
        return reinterpret_cast<const QuerySet* const*>(rhs);
    }
    inline QuerySet* const* FromAPI(WGPUQuerySet* rhs) {
        return reinterpret_cast<QuerySet* const*>(rhs);
    }
    inline WGPUQueue ToAPI(Queue* rhs) {
        return reinterpret_cast<WGPUQueue>(rhs);
    }

    inline Queue* FromAPI(WGPUQueue rhs) {
        return reinterpret_cast<Queue*>(rhs);
    }
    inline const Queue* const* FromAPI(const WGPUQueue* rhs) {
        return reinterpret_cast<const Queue* const*>(rhs);
    }
    inline Queue* const* FromAPI(WGPUQueue* rhs) {
        return reinterpret_cast<Queue* const*>(rhs);
    }
    inline WGPURenderBundle ToAPI(RenderBundle* rhs) {
        return reinterpret_cast<WGPURenderBundle>(rhs);
    }

    inline RenderBundle* FromAPI(WGPURenderBundle rhs) {
        return reinterpret_cast<RenderBundle*>(rhs);
    }
    inline const RenderBundle* const* FromAPI(const WGPURenderBundle* rhs) {
        return reinterpret_cast<const RenderBundle* const*>(rhs);
    }
    inline RenderBundle* const* FromAPI(WGPURenderBundle* rhs) {
        return reinterpret_cast<RenderBundle* const*>(rhs);
    }
    inline WGPURenderPipeline ToAPI(RenderPipeline* rhs) {
        return reinterpret_cast<WGPURenderPipeline>(rhs);
    }

    inline RenderPipeline* FromAPI(WGPURenderPipeline rhs) {
        return reinterpret_cast<RenderPipeline*>(rhs);
    }
    inline const RenderPipeline* const* FromAPI(const WGPURenderPipeline* rhs) {
        return reinterpret_cast<const RenderPipeline* const*>(rhs);
    }
    inline RenderPipeline* const* FromAPI(WGPURenderPipeline* rhs) {
        return reinterpret_cast<RenderPipeline* const*>(rhs);
    }
    inline WGPUResourceTable ToAPI(ResourceTable* rhs) {
        return reinterpret_cast<WGPUResourceTable>(rhs);
    }

    inline ResourceTable* FromAPI(WGPUResourceTable rhs) {
        return reinterpret_cast<ResourceTable*>(rhs);
    }
    inline const ResourceTable* const* FromAPI(const WGPUResourceTable* rhs) {
        return reinterpret_cast<const ResourceTable* const*>(rhs);
    }
    inline ResourceTable* const* FromAPI(WGPUResourceTable* rhs) {
        return reinterpret_cast<ResourceTable* const*>(rhs);
    }
    inline WGPUSampler ToAPI(Sampler* rhs) {
        return reinterpret_cast<WGPUSampler>(rhs);
    }

    inline Sampler* FromAPI(WGPUSampler rhs) {
        return reinterpret_cast<Sampler*>(rhs);
    }
    inline const Sampler* const* FromAPI(const WGPUSampler* rhs) {
        return reinterpret_cast<const Sampler* const*>(rhs);
    }
    inline Sampler* const* FromAPI(WGPUSampler* rhs) {
        return reinterpret_cast<Sampler* const*>(rhs);
    }
    inline WGPUShaderModule ToAPI(ShaderModule* rhs) {
        return reinterpret_cast<WGPUShaderModule>(rhs);
    }

    inline ShaderModule* FromAPI(WGPUShaderModule rhs) {
        return reinterpret_cast<ShaderModule*>(rhs);
    }
    inline const ShaderModule* const* FromAPI(const WGPUShaderModule* rhs) {
        return reinterpret_cast<const ShaderModule* const*>(rhs);
    }
    inline ShaderModule* const* FromAPI(WGPUShaderModule* rhs) {
        return reinterpret_cast<ShaderModule* const*>(rhs);
    }
    inline WGPUSurface ToAPI(Surface* rhs) {
        return reinterpret_cast<WGPUSurface>(rhs);
    }

    inline Surface* FromAPI(WGPUSurface rhs) {
        return reinterpret_cast<Surface*>(rhs);
    }
    inline const Surface* const* FromAPI(const WGPUSurface* rhs) {
        return reinterpret_cast<const Surface* const*>(rhs);
    }
    inline Surface* const* FromAPI(WGPUSurface* rhs) {
        return reinterpret_cast<Surface* const*>(rhs);
    }
    inline WGPUTexelBufferView ToAPI(TexelBufferView* rhs) {
        return reinterpret_cast<WGPUTexelBufferView>(rhs);
    }

    inline TexelBufferView* FromAPI(WGPUTexelBufferView rhs) {
        return reinterpret_cast<TexelBufferView*>(rhs);
    }
    inline const TexelBufferView* const* FromAPI(const WGPUTexelBufferView* rhs) {
        return reinterpret_cast<const TexelBufferView* const*>(rhs);
    }
    inline TexelBufferView* const* FromAPI(WGPUTexelBufferView* rhs) {
        return reinterpret_cast<TexelBufferView* const*>(rhs);
    }
    inline WGPUTexture ToAPI(Texture* rhs) {
        return reinterpret_cast<WGPUTexture>(rhs);
    }

    inline Texture* FromAPI(WGPUTexture rhs) {
        return reinterpret_cast<Texture*>(rhs);
    }
    inline const Texture* const* FromAPI(const WGPUTexture* rhs) {
        return reinterpret_cast<const Texture* const*>(rhs);
    }
    inline Texture* const* FromAPI(WGPUTexture* rhs) {
        return reinterpret_cast<Texture* const*>(rhs);
    }
    inline WGPUTextureView ToAPI(TextureView* rhs) {
        return reinterpret_cast<WGPUTextureView>(rhs);
    }

    inline TextureView* FromAPI(WGPUTextureView rhs) {
        return reinterpret_cast<TextureView*>(rhs);
    }
    inline const TextureView* const* FromAPI(const WGPUTextureView* rhs) {
        return reinterpret_cast<const TextureView* const*>(rhs);
    }
    inline TextureView* const* FromAPI(WGPUTextureView* rhs) {
        return reinterpret_cast<TextureView* const*>(rhs);
    }
    inline WGPUComputePassEncoder ToAPI(ComputePassEncoder* rhs) {
        return reinterpret_cast<WGPUComputePassEncoder>(rhs);
    }

    inline ComputePassEncoder* FromAPI(WGPUComputePassEncoder rhs) {
        return reinterpret_cast<ComputePassEncoder*>(rhs);
    }
    inline const ComputePassEncoder* const* FromAPI(const WGPUComputePassEncoder* rhs) {
        return reinterpret_cast<const ComputePassEncoder* const*>(rhs);
    }
    inline ComputePassEncoder* const* FromAPI(WGPUComputePassEncoder* rhs) {
        return reinterpret_cast<ComputePassEncoder* const*>(rhs);
    }
    inline WGPURenderBundleEncoder ToAPI(RenderBundleEncoder* rhs) {
        return reinterpret_cast<WGPURenderBundleEncoder>(rhs);
    }

    inline RenderBundleEncoder* FromAPI(WGPURenderBundleEncoder rhs) {
        return reinterpret_cast<RenderBundleEncoder*>(rhs);
    }
    inline const RenderBundleEncoder* const* FromAPI(const WGPURenderBundleEncoder* rhs) {
        return reinterpret_cast<const RenderBundleEncoder* const*>(rhs);
    }
    inline RenderBundleEncoder* const* FromAPI(WGPURenderBundleEncoder* rhs) {
        return reinterpret_cast<RenderBundleEncoder* const*>(rhs);
    }
    inline WGPURenderPassEncoder ToAPI(RenderPassEncoder* rhs) {
        return reinterpret_cast<WGPURenderPassEncoder>(rhs);
    }

    inline RenderPassEncoder* FromAPI(WGPURenderPassEncoder rhs) {
        return reinterpret_cast<RenderPassEncoder*>(rhs);
    }
    inline const RenderPassEncoder* const* FromAPI(const WGPURenderPassEncoder* rhs) {
        return reinterpret_cast<const RenderPassEncoder* const*>(rhs);
    }
    inline RenderPassEncoder* const* FromAPI(WGPURenderPassEncoder* rhs) {
        return reinterpret_cast<RenderPassEncoder* const*>(rhs);
    }

    inline WGPUFuture ToAPI(Future rhs) {
        return {rhs.id};
    }
    inline Future FromAPI(WGPUFuture rhs) {
        return Future{rhs.id};
    }
    inline WGPUStringView ToAPI(StringView rhs) {
        return {rhs.data, rhs.length};
    }
    inline StringView FromAPI(WGPUStringView rhs) {
        return StringView(rhs);
    }

    inline WGPUAdapterType ToAPI(wgpu::AdapterType rhs) {
        return static_cast<WGPUAdapterType>(rhs);
    }
    inline WGPUAddressMode ToAPI(wgpu::AddressMode rhs) {
        return static_cast<WGPUAddressMode>(rhs);
    }
    inline WGPUAlphaMode ToAPI(wgpu::AlphaMode rhs) {
        return static_cast<WGPUAlphaMode>(rhs);
    }
    inline WGPUBackendType ToAPI(wgpu::BackendType rhs) {
        return static_cast<WGPUBackendType>(rhs);
    }
    inline WGPUBlendFactor ToAPI(wgpu::BlendFactor rhs) {
        return static_cast<WGPUBlendFactor>(rhs);
    }
    inline WGPUBlendOperation ToAPI(wgpu::BlendOperation rhs) {
        return static_cast<WGPUBlendOperation>(rhs);
    }
    inline WGPUBufferBindingType ToAPI(wgpu::BufferBindingType rhs) {
        return static_cast<WGPUBufferBindingType>(rhs);
    }
    inline WGPUBufferMapState ToAPI(wgpu::BufferMapState rhs) {
        return static_cast<WGPUBufferMapState>(rhs);
    }
    inline WGPUCallbackMode ToAPI(wgpu::CallbackMode rhs) {
        return static_cast<WGPUCallbackMode>(rhs);
    }
    inline WGPUColorSpacePrimariesDawn ToAPI(wgpu::ColorSpacePrimariesDawn rhs) {
        return static_cast<WGPUColorSpacePrimariesDawn>(rhs);
    }
    inline WGPUColorSpaceTransferDawn ToAPI(wgpu::ColorSpaceTransferDawn rhs) {
        return static_cast<WGPUColorSpaceTransferDawn>(rhs);
    }
    inline WGPUColorSpaceYCbCrMatrixDawn ToAPI(wgpu::ColorSpaceYCbCrMatrixDawn rhs) {
        return static_cast<WGPUColorSpaceYCbCrMatrixDawn>(rhs);
    }
    inline WGPUColorSpaceYCbCrRangeDawn ToAPI(wgpu::ColorSpaceYCbCrRangeDawn rhs) {
        return static_cast<WGPUColorSpaceYCbCrRangeDawn>(rhs);
    }
    inline WGPUCompareFunction ToAPI(wgpu::CompareFunction rhs) {
        return static_cast<WGPUCompareFunction>(rhs);
    }
    inline WGPUCompilationInfoRequestStatus ToAPI(wgpu::CompilationInfoRequestStatus rhs) {
        return static_cast<WGPUCompilationInfoRequestStatus>(rhs);
    }
    inline WGPUCompilationMessageType ToAPI(wgpu::CompilationMessageType rhs) {
        return static_cast<WGPUCompilationMessageType>(rhs);
    }
    inline WGPUComponentSwizzle ToAPI(wgpu::ComponentSwizzle rhs) {
        return static_cast<WGPUComponentSwizzle>(rhs);
    }
    inline WGPUCompositeAlphaMode ToAPI(wgpu::CompositeAlphaMode rhs) {
        return static_cast<WGPUCompositeAlphaMode>(rhs);
    }
    inline WGPUCreatePipelineAsyncStatus ToAPI(wgpu::CreatePipelineAsyncStatus rhs) {
        return static_cast<WGPUCreatePipelineAsyncStatus>(rhs);
    }
    inline WGPUCullMode ToAPI(wgpu::CullMode rhs) {
        return static_cast<WGPUCullMode>(rhs);
    }
    inline WGPUDeviceLostReason ToAPI(wgpu::DeviceLostReason rhs) {
        return static_cast<WGPUDeviceLostReason>(rhs);
    }
    inline WGPUErrorFilter ToAPI(wgpu::ErrorFilter rhs) {
        return static_cast<WGPUErrorFilter>(rhs);
    }
    inline WGPUErrorType ToAPI(wgpu::ErrorType rhs) {
        return static_cast<WGPUErrorType>(rhs);
    }
    inline WGPUExternalTextureRotation ToAPI(wgpu::ExternalTextureRotation rhs) {
        return static_cast<WGPUExternalTextureRotation>(rhs);
    }
    inline WGPUFeatureLevel ToAPI(wgpu::FeatureLevel rhs) {
        return static_cast<WGPUFeatureLevel>(rhs);
    }
    inline WGPUFeatureName ToAPI(wgpu::FeatureName rhs) {
        return static_cast<WGPUFeatureName>(rhs);
    }
    inline WGPUFilterMode ToAPI(wgpu::FilterMode rhs) {
        return static_cast<WGPUFilterMode>(rhs);
    }
    inline WGPUFrontFace ToAPI(wgpu::FrontFace rhs) {
        return static_cast<WGPUFrontFace>(rhs);
    }
    inline WGPUIndexFormat ToAPI(wgpu::IndexFormat rhs) {
        return static_cast<WGPUIndexFormat>(rhs);
    }
    inline WGPUInstanceFeatureName ToAPI(wgpu::InstanceFeatureName rhs) {
        return static_cast<WGPUInstanceFeatureName>(rhs);
    }
    inline WGPULoadOp ToAPI(wgpu::LoadOp rhs) {
        return static_cast<WGPULoadOp>(rhs);
    }
    inline WGPULoggingType ToAPI(wgpu::LoggingType rhs) {
        return static_cast<WGPULoggingType>(rhs);
    }
    inline WGPUMapAsyncStatus ToAPI(wgpu::MapAsyncStatus rhs) {
        return static_cast<WGPUMapAsyncStatus>(rhs);
    }
    inline WGPUMipmapFilterMode ToAPI(wgpu::MipmapFilterMode rhs) {
        return static_cast<WGPUMipmapFilterMode>(rhs);
    }
    inline WGPUOptionalBool ToAPI(wgpu::OptionalBool rhs) {
        return static_cast<WGPUOptionalBool>(rhs);
    }
    inline WGPUPopErrorScopeStatus ToAPI(wgpu::PopErrorScopeStatus rhs) {
        return static_cast<WGPUPopErrorScopeStatus>(rhs);
    }
    inline WGPUPowerPreference ToAPI(wgpu::PowerPreference rhs) {
        return static_cast<WGPUPowerPreference>(rhs);
    }
    inline WGPUPredefinedColorSpace ToAPI(wgpu::PredefinedColorSpace rhs) {
        return static_cast<WGPUPredefinedColorSpace>(rhs);
    }
    inline WGPUPresentMode ToAPI(wgpu::PresentMode rhs) {
        return static_cast<WGPUPresentMode>(rhs);
    }
    inline WGPUPrimitiveTopology ToAPI(wgpu::PrimitiveTopology rhs) {
        return static_cast<WGPUPrimitiveTopology>(rhs);
    }
    inline WGPUQueryType ToAPI(wgpu::QueryType rhs) {
        return static_cast<WGPUQueryType>(rhs);
    }
    inline WGPUQueueWorkDoneStatus ToAPI(wgpu::QueueWorkDoneStatus rhs) {
        return static_cast<WGPUQueueWorkDoneStatus>(rhs);
    }
    inline WGPURequestAdapterStatus ToAPI(wgpu::RequestAdapterStatus rhs) {
        return static_cast<WGPURequestAdapterStatus>(rhs);
    }
    inline WGPURequestDeviceStatus ToAPI(wgpu::RequestDeviceStatus rhs) {
        return static_cast<WGPURequestDeviceStatus>(rhs);
    }
    inline WGPUSamplerBindingType ToAPI(wgpu::SamplerBindingType rhs) {
        return static_cast<WGPUSamplerBindingType>(rhs);
    }
    inline WGPUStatus ToAPI(wgpu::Status rhs) {
        return static_cast<WGPUStatus>(rhs);
    }
    inline WGPUStencilOperation ToAPI(wgpu::StencilOperation rhs) {
        return static_cast<WGPUStencilOperation>(rhs);
    }
    inline WGPUStorageTextureAccess ToAPI(wgpu::StorageTextureAccess rhs) {
        return static_cast<WGPUStorageTextureAccess>(rhs);
    }
    inline WGPUStoreOp ToAPI(wgpu::StoreOp rhs) {
        return static_cast<WGPUStoreOp>(rhs);
    }
    inline WGPUSType ToAPI(wgpu::SType rhs) {
        return static_cast<WGPUSType>(rhs);
    }
    inline WGPUSubgroupMatrixComponentType ToAPI(wgpu::SubgroupMatrixComponentType rhs) {
        return static_cast<WGPUSubgroupMatrixComponentType>(rhs);
    }
    inline WGPUSurfaceGetCurrentTextureStatus ToAPI(wgpu::SurfaceGetCurrentTextureStatus rhs) {
        return static_cast<WGPUSurfaceGetCurrentTextureStatus>(rhs);
    }
    inline WGPUTexelBufferAccess ToAPI(wgpu::TexelBufferAccess rhs) {
        return static_cast<WGPUTexelBufferAccess>(rhs);
    }
    inline WGPUTextureAspect ToAPI(wgpu::TextureAspect rhs) {
        return static_cast<WGPUTextureAspect>(rhs);
    }
    inline WGPUTextureDimension ToAPI(wgpu::TextureDimension rhs) {
        return static_cast<WGPUTextureDimension>(rhs);
    }
    inline WGPUTextureFormat ToAPI(wgpu::TextureFormat rhs) {
        return static_cast<WGPUTextureFormat>(rhs);
    }
    inline WGPUTextureSampleType ToAPI(wgpu::TextureSampleType rhs) {
        return static_cast<WGPUTextureSampleType>(rhs);
    }
    inline WGPUTextureViewDimension ToAPI(wgpu::TextureViewDimension rhs) {
        return static_cast<WGPUTextureViewDimension>(rhs);
    }
    inline WGPUToneMappingMode ToAPI(wgpu::ToneMappingMode rhs) {
        return static_cast<WGPUToneMappingMode>(rhs);
    }
    inline WGPUVertexFormat ToAPI(wgpu::VertexFormat rhs) {
        return static_cast<WGPUVertexFormat>(rhs);
    }
    inline WGPUVertexStepMode ToAPI(wgpu::VertexStepMode rhs) {
        return static_cast<WGPUVertexStepMode>(rhs);
    }
    inline WGPUWaitStatus ToAPI(wgpu::WaitStatus rhs) {
        return static_cast<WGPUWaitStatus>(rhs);
    }
    inline WGPUWGSLLanguageFeatureName ToAPI(wgpu::WGSLLanguageFeatureName rhs) {
        return static_cast<WGPUWGSLLanguageFeatureName>(rhs);
    }
    inline WGPUBufferUsage ToAPI(wgpu::BufferUsage rhs) {
        return static_cast<WGPUBufferUsage>(rhs);
    }
    inline WGPUColorWriteMask ToAPI(wgpu::ColorWriteMask rhs) {
        return static_cast<WGPUColorWriteMask>(rhs);
    }
    inline WGPUHeapProperty ToAPI(wgpu::HeapProperty rhs) {
        return static_cast<WGPUHeapProperty>(rhs);
    }
    inline WGPUMapMode ToAPI(wgpu::MapMode rhs) {
        return static_cast<WGPUMapMode>(rhs);
    }
    inline WGPUShaderStage ToAPI(wgpu::ShaderStage rhs) {
        return static_cast<WGPUShaderStage>(rhs);
    }
    inline WGPUTextureUsage ToAPI(wgpu::TextureUsage rhs) {
        return static_cast<WGPUTextureUsage>(rhs);
    }

    inline wgpu::AdapterType FromAPI(WGPUAdapterType rhs) {
        return static_cast<wgpu::AdapterType>(rhs);
    }
    inline wgpu::AddressMode FromAPI(WGPUAddressMode rhs) {
        return static_cast<wgpu::AddressMode>(rhs);
    }
    inline wgpu::AlphaMode FromAPI(WGPUAlphaMode rhs) {
        return static_cast<wgpu::AlphaMode>(rhs);
    }
    inline wgpu::BackendType FromAPI(WGPUBackendType rhs) {
        return static_cast<wgpu::BackendType>(rhs);
    }
    inline wgpu::BlendFactor FromAPI(WGPUBlendFactor rhs) {
        return static_cast<wgpu::BlendFactor>(rhs);
    }
    inline wgpu::BlendOperation FromAPI(WGPUBlendOperation rhs) {
        return static_cast<wgpu::BlendOperation>(rhs);
    }
    inline wgpu::BufferBindingType FromAPI(WGPUBufferBindingType rhs) {
        return static_cast<wgpu::BufferBindingType>(rhs);
    }
    inline wgpu::BufferMapState FromAPI(WGPUBufferMapState rhs) {
        return static_cast<wgpu::BufferMapState>(rhs);
    }
    inline wgpu::CallbackMode FromAPI(WGPUCallbackMode rhs) {
        return static_cast<wgpu::CallbackMode>(rhs);
    }
    inline wgpu::ColorSpacePrimariesDawn FromAPI(WGPUColorSpacePrimariesDawn rhs) {
        return static_cast<wgpu::ColorSpacePrimariesDawn>(rhs);
    }
    inline wgpu::ColorSpaceTransferDawn FromAPI(WGPUColorSpaceTransferDawn rhs) {
        return static_cast<wgpu::ColorSpaceTransferDawn>(rhs);
    }
    inline wgpu::ColorSpaceYCbCrMatrixDawn FromAPI(WGPUColorSpaceYCbCrMatrixDawn rhs) {
        return static_cast<wgpu::ColorSpaceYCbCrMatrixDawn>(rhs);
    }
    inline wgpu::ColorSpaceYCbCrRangeDawn FromAPI(WGPUColorSpaceYCbCrRangeDawn rhs) {
        return static_cast<wgpu::ColorSpaceYCbCrRangeDawn>(rhs);
    }
    inline wgpu::CompareFunction FromAPI(WGPUCompareFunction rhs) {
        return static_cast<wgpu::CompareFunction>(rhs);
    }
    inline wgpu::CompilationInfoRequestStatus FromAPI(WGPUCompilationInfoRequestStatus rhs) {
        return static_cast<wgpu::CompilationInfoRequestStatus>(rhs);
    }
    inline wgpu::CompilationMessageType FromAPI(WGPUCompilationMessageType rhs) {
        return static_cast<wgpu::CompilationMessageType>(rhs);
    }
    inline wgpu::ComponentSwizzle FromAPI(WGPUComponentSwizzle rhs) {
        return static_cast<wgpu::ComponentSwizzle>(rhs);
    }
    inline wgpu::CompositeAlphaMode FromAPI(WGPUCompositeAlphaMode rhs) {
        return static_cast<wgpu::CompositeAlphaMode>(rhs);
    }
    inline wgpu::CreatePipelineAsyncStatus FromAPI(WGPUCreatePipelineAsyncStatus rhs) {
        return static_cast<wgpu::CreatePipelineAsyncStatus>(rhs);
    }
    inline wgpu::CullMode FromAPI(WGPUCullMode rhs) {
        return static_cast<wgpu::CullMode>(rhs);
    }
    inline wgpu::DeviceLostReason FromAPI(WGPUDeviceLostReason rhs) {
        return static_cast<wgpu::DeviceLostReason>(rhs);
    }
    inline wgpu::ErrorFilter FromAPI(WGPUErrorFilter rhs) {
        return static_cast<wgpu::ErrorFilter>(rhs);
    }
    inline wgpu::ErrorType FromAPI(WGPUErrorType rhs) {
        return static_cast<wgpu::ErrorType>(rhs);
    }
    inline wgpu::ExternalTextureRotation FromAPI(WGPUExternalTextureRotation rhs) {
        return static_cast<wgpu::ExternalTextureRotation>(rhs);
    }
    inline wgpu::FeatureLevel FromAPI(WGPUFeatureLevel rhs) {
        return static_cast<wgpu::FeatureLevel>(rhs);
    }
    inline wgpu::FeatureName FromAPI(WGPUFeatureName rhs) {
        return static_cast<wgpu::FeatureName>(rhs);
    }
    inline wgpu::FilterMode FromAPI(WGPUFilterMode rhs) {
        return static_cast<wgpu::FilterMode>(rhs);
    }
    inline wgpu::FrontFace FromAPI(WGPUFrontFace rhs) {
        return static_cast<wgpu::FrontFace>(rhs);
    }
    inline wgpu::IndexFormat FromAPI(WGPUIndexFormat rhs) {
        return static_cast<wgpu::IndexFormat>(rhs);
    }
    inline wgpu::InstanceFeatureName FromAPI(WGPUInstanceFeatureName rhs) {
        return static_cast<wgpu::InstanceFeatureName>(rhs);
    }
    inline wgpu::LoadOp FromAPI(WGPULoadOp rhs) {
        return static_cast<wgpu::LoadOp>(rhs);
    }
    inline wgpu::LoggingType FromAPI(WGPULoggingType rhs) {
        return static_cast<wgpu::LoggingType>(rhs);
    }
    inline wgpu::MapAsyncStatus FromAPI(WGPUMapAsyncStatus rhs) {
        return static_cast<wgpu::MapAsyncStatus>(rhs);
    }
    inline wgpu::MipmapFilterMode FromAPI(WGPUMipmapFilterMode rhs) {
        return static_cast<wgpu::MipmapFilterMode>(rhs);
    }
    inline wgpu::OptionalBool FromAPI(WGPUOptionalBool rhs) {
        return static_cast<wgpu::OptionalBool>(rhs);
    }
    inline wgpu::PopErrorScopeStatus FromAPI(WGPUPopErrorScopeStatus rhs) {
        return static_cast<wgpu::PopErrorScopeStatus>(rhs);
    }
    inline wgpu::PowerPreference FromAPI(WGPUPowerPreference rhs) {
        return static_cast<wgpu::PowerPreference>(rhs);
    }
    inline wgpu::PredefinedColorSpace FromAPI(WGPUPredefinedColorSpace rhs) {
        return static_cast<wgpu::PredefinedColorSpace>(rhs);
    }
    inline wgpu::PresentMode FromAPI(WGPUPresentMode rhs) {
        return static_cast<wgpu::PresentMode>(rhs);
    }
    inline wgpu::PrimitiveTopology FromAPI(WGPUPrimitiveTopology rhs) {
        return static_cast<wgpu::PrimitiveTopology>(rhs);
    }
    inline wgpu::QueryType FromAPI(WGPUQueryType rhs) {
        return static_cast<wgpu::QueryType>(rhs);
    }
    inline wgpu::QueueWorkDoneStatus FromAPI(WGPUQueueWorkDoneStatus rhs) {
        return static_cast<wgpu::QueueWorkDoneStatus>(rhs);
    }
    inline wgpu::RequestAdapterStatus FromAPI(WGPURequestAdapterStatus rhs) {
        return static_cast<wgpu::RequestAdapterStatus>(rhs);
    }
    inline wgpu::RequestDeviceStatus FromAPI(WGPURequestDeviceStatus rhs) {
        return static_cast<wgpu::RequestDeviceStatus>(rhs);
    }
    inline wgpu::SamplerBindingType FromAPI(WGPUSamplerBindingType rhs) {
        return static_cast<wgpu::SamplerBindingType>(rhs);
    }
    inline wgpu::Status FromAPI(WGPUStatus rhs) {
        return static_cast<wgpu::Status>(rhs);
    }
    inline wgpu::StencilOperation FromAPI(WGPUStencilOperation rhs) {
        return static_cast<wgpu::StencilOperation>(rhs);
    }
    inline wgpu::StorageTextureAccess FromAPI(WGPUStorageTextureAccess rhs) {
        return static_cast<wgpu::StorageTextureAccess>(rhs);
    }
    inline wgpu::StoreOp FromAPI(WGPUStoreOp rhs) {
        return static_cast<wgpu::StoreOp>(rhs);
    }
    inline wgpu::SType FromAPI(WGPUSType rhs) {
        return static_cast<wgpu::SType>(rhs);
    }
    inline wgpu::SubgroupMatrixComponentType FromAPI(WGPUSubgroupMatrixComponentType rhs) {
        return static_cast<wgpu::SubgroupMatrixComponentType>(rhs);
    }
    inline wgpu::SurfaceGetCurrentTextureStatus FromAPI(WGPUSurfaceGetCurrentTextureStatus rhs) {
        return static_cast<wgpu::SurfaceGetCurrentTextureStatus>(rhs);
    }
    inline wgpu::TexelBufferAccess FromAPI(WGPUTexelBufferAccess rhs) {
        return static_cast<wgpu::TexelBufferAccess>(rhs);
    }
    inline wgpu::TextureAspect FromAPI(WGPUTextureAspect rhs) {
        return static_cast<wgpu::TextureAspect>(rhs);
    }
    inline wgpu::TextureDimension FromAPI(WGPUTextureDimension rhs) {
        return static_cast<wgpu::TextureDimension>(rhs);
    }
    inline wgpu::TextureFormat FromAPI(WGPUTextureFormat rhs) {
        return static_cast<wgpu::TextureFormat>(rhs);
    }
    inline wgpu::TextureSampleType FromAPI(WGPUTextureSampleType rhs) {
        return static_cast<wgpu::TextureSampleType>(rhs);
    }
    inline wgpu::TextureViewDimension FromAPI(WGPUTextureViewDimension rhs) {
        return static_cast<wgpu::TextureViewDimension>(rhs);
    }
    inline wgpu::ToneMappingMode FromAPI(WGPUToneMappingMode rhs) {
        return static_cast<wgpu::ToneMappingMode>(rhs);
    }
    inline wgpu::VertexFormat FromAPI(WGPUVertexFormat rhs) {
        return static_cast<wgpu::VertexFormat>(rhs);
    }
    inline wgpu::VertexStepMode FromAPI(WGPUVertexStepMode rhs) {
        return static_cast<wgpu::VertexStepMode>(rhs);
    }
    inline wgpu::WaitStatus FromAPI(WGPUWaitStatus rhs) {
        return static_cast<wgpu::WaitStatus>(rhs);
    }
    inline wgpu::WGSLLanguageFeatureName FromAPI(WGPUWGSLLanguageFeatureName rhs) {
        return static_cast<wgpu::WGSLLanguageFeatureName>(rhs);
    }

}  // namespace dawn::wire::client

#endif  // DAWNWIRE_CLIENT_DAWN_PLATFORM_AUTOGEN_H_
