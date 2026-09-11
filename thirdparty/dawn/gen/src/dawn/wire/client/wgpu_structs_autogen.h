


#ifndef DAWNWIRE_CLIENT_WGPU_STRUCTS_AUTOGEN_H_
#define DAWNWIRE_CLIENT_WGPU_STRUCTS_AUTOGEN_H_

#include "absl/strings/string_view.h"
#include "dawn/webgpu_cpp.h"
#include "src/utils/span.h"

#include <cmath>
#include <optional>
#include <string_view>

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

    using wgpu::ChainedStruct;
    using wgpu::ChainedStructOut;


    struct StringView {
        char const * data = nullptr;
        size_t length = WGPU_STRLEN;

        inline constexpr StringView() noexcept = default;

explicit(false) inline constexpr StringView(const std::string_view& sv) noexcept {
    this->data = sv.data();
    this->length = sv.length();
}

explicit(false) inline constexpr StringView(const char* s) {
    this->data = s;
    this->length = WGPU_STRLEN;  // use strlen
}

explicit(false) inline constexpr StringView(WGPUStringView s) {
    this->data = s.data;
    this->length = s.length;
}

inline constexpr StringView(const char* data, size_t length) {
    this->data = data;
    this->length = length;
}

explicit(false) inline constexpr StringView(std::nullptr_t) {
    this->data = nullptr;
    this->length = WGPU_STRLEN;
}

explicit(false) inline constexpr StringView(std::nullopt_t) {
    this->data = nullptr;
    this->length = WGPU_STRLEN;
}

bool IsUndefined() const {
    return this->data == nullptr && this->length == wgpu::kStrlen;
}

// NOLINTNEXTLINE(google-explicit-constructor)
operator std::string_view() const {
    if (this->length == wgpu::kStrlen) {
        if (IsUndefined()) {
            return {};
        }
        return {this->data};
    }
    return {this->data, this->length};
}

template <typename View,
          typename = std::enable_if_t<std::is_constructible_v<View, const char*, size_t>>>
explicit operator View() const {
    if (this->length == wgpu::kStrlen) {
        if (IsUndefined()) {
            return {};
        }
        return {this->data};
    }
    return {this->data, this->length};
}


        #ifndef ABSL_USES_STD_STRING_VIEW
        // NOLINTNEXTLINE(google-explicit-constructor)
        operator absl::string_view() const {
            if (this->length == wgpu::kStrlen) {
                if (IsUndefined()) {
                    return {};
                }
                return {this->data};
            }
            return {this->data, this->length};
        }
        #endif
    };

    // NOLINTBEGIN(bugprone-invalid-enum-default-initialization)

    struct AdapterPropertiesD3D : ChainedStructOut {
        AdapterPropertiesD3D() {
            sType = wgpu::SType::AdapterPropertiesD3D;
        }
        alignas(wgpu::AdapterPropertiesD3D::kFirstMemberAlignment)


        uint32_t shaderModel ;

    };

    struct AdapterPropertiesVk : ChainedStructOut {
        AdapterPropertiesVk() {
            sType = wgpu::SType::AdapterPropertiesVk;
        }
        alignas(wgpu::AdapterPropertiesVk::kFirstMemberAlignment)


        uint32_t driverVersion ;

    };

    struct AdapterPropertiesWGPU : ChainedStructOut {
        AdapterPropertiesWGPU() {
            sType = wgpu::SType::AdapterPropertiesWGPU;
        }
        alignas(wgpu::AdapterPropertiesWGPU::kFirstMemberAlignment)


        wgpu::BackendType backendType  = wgpu::BackendType::Undefined;

    };

    struct BindingResource {
        ChainedStruct const * nextInChain = nullptr;


        Buffer* buffer  = nullptr;


        uint64_t offset  = 0;


        uint64_t size  = wgpu::kWholeSize;


        Sampler* sampler  = nullptr;


        TextureView* textureView  = nullptr;

    };

    struct BlendComponent {


        wgpu::BlendOperation operation  = wgpu::BlendOperation::Add;


        wgpu::BlendFactor srcFactor  = wgpu::BlendFactor::One;


        wgpu::BlendFactor dstFactor  = wgpu::BlendFactor::Zero;

    };

    struct BufferBindingLayout {
        ChainedStruct const * nextInChain = nullptr;


        wgpu::BufferBindingType type  = wgpu::BufferBindingType::Uniform;


        wgpu::Bool hasDynamicOffset  = false;


        uint64_t minBindingSize  = 0;

    };

    struct BufferHostMappedPointer : ChainedStruct {
        BufferHostMappedPointer() {
            sType = wgpu::SType::BufferHostMappedPointer;
        }
        alignas(wgpu::BufferHostMappedPointer::kFirstMemberAlignment)


        void * pointer ;


        WGPUCallback disposeCallback ;


        void * userdata ;

    };

    struct Color {


        double r ;


        double g ;


        double b ;


        double a ;

    };

    struct ColorSpaceDawn {
        ChainedStruct const * nextInChain = nullptr;


        wgpu::ColorSpacePrimariesDawn primaries  = {};


        wgpu::ColorSpaceTransferDawn transfer  = wgpu::ColorSpaceTransferDawn::Identity;


        wgpu::ColorSpaceYCbCrRangeDawn yCbCrRange  = wgpu::ColorSpaceYCbCrRangeDawn::Identity;


        wgpu::ColorSpaceYCbCrMatrixDawn yCbCrMatrix  = wgpu::ColorSpaceYCbCrMatrixDawn::Identity;


        float hdrReferenceWhiteLuminance  = 0;

    };

    struct ColorTargetStateExpandResolveTextureDawn : ChainedStruct {
        ColorTargetStateExpandResolveTextureDawn() {
            sType = wgpu::SType::ColorTargetStateExpandResolveTextureDawn;
        }
        alignas(wgpu::ColorTargetStateExpandResolveTextureDawn::kFirstMemberAlignment)


        wgpu::Bool enabled  = false;

    };

    struct CommandBufferDescriptor {
        ChainedStruct const * nextInChain = nullptr;


        StringView label ;

    };

    struct CompatibilityModeLimits : ChainedStructOut {
        CompatibilityModeLimits() {
            sType = wgpu::SType::CompatibilityModeLimits;
        }
        alignas(wgpu::CompatibilityModeLimits::kFirstMemberAlignment)


        uint32_t maxStorageBuffersInVertexStage  = wgpu::kLimitU32Undefined;


        uint32_t maxStorageTexturesInVertexStage  = wgpu::kLimitU32Undefined;


        uint32_t maxStorageBuffersInFragmentStage  = wgpu::kLimitU32Undefined;


        uint32_t maxStorageTexturesInFragmentStage  = wgpu::kLimitU32Undefined;

    };

    struct ConstantEntry {
        ChainedStruct const * nextInChain = nullptr;


        StringView key ;


        double value ;

    };

    struct CopyTextureForBrowserOptions {
        ChainedStruct const * nextInChain = nullptr;


        wgpu::Bool flipY  = false;


        wgpu::Bool needsColorSpaceConversion  = false;


        wgpu::AlphaMode srcAlphaMode  = wgpu::AlphaMode::Unpremultiplied;


        float const * srcTransferFunctionParameters  = nullptr;


        float const * conversionMatrix  = nullptr;


        float const * dstTransferFunctionParameters  = nullptr;


        wgpu::AlphaMode dstAlphaMode  = wgpu::AlphaMode::Unpremultiplied;


        wgpu::Bool internalUsage  = false;

    };

    struct DawnAdapterPropertiesPowerPreference : ChainedStructOut {
        DawnAdapterPropertiesPowerPreference() {
            sType = wgpu::SType::DawnAdapterPropertiesPowerPreference;
        }
        alignas(wgpu::DawnAdapterPropertiesPowerPreference::kFirstMemberAlignment)


        wgpu::PowerPreference powerPreference  = wgpu::PowerPreference::Undefined;

    };

    struct DawnBufferDescriptorErrorInfoFromWireClient : ChainedStruct {
        DawnBufferDescriptorErrorInfoFromWireClient() {
            sType = wgpu::SType::DawnBufferDescriptorErrorInfoFromWireClient;
        }
        alignas(wgpu::DawnBufferDescriptorErrorInfoFromWireClient::kFirstMemberAlignment)


        wgpu::Bool outOfMemory  = false;

    };

    struct DawnCompilationMessageUtf16 : ChainedStruct {
        DawnCompilationMessageUtf16() {
            sType = wgpu::SType::DawnCompilationMessageUtf16;
        }
        alignas(wgpu::DawnCompilationMessageUtf16::kFirstMemberAlignment)


        uint64_t linePos ;


        uint64_t offset ;


        uint64_t length ;

    };

    struct DawnConsumeAdapterDescriptor : ChainedStruct {
        DawnConsumeAdapterDescriptor() {
            sType = wgpu::SType::DawnConsumeAdapterDescriptor;
        }
        alignas(wgpu::DawnConsumeAdapterDescriptor::kFirstMemberAlignment)


        wgpu::Bool consumeAdapter  = false;

    };

    struct DawnDeviceAllocatorControl : ChainedStruct {
        DawnDeviceAllocatorControl() {
            sType = wgpu::SType::DawnDeviceAllocatorControl;
        }
        alignas(wgpu::DawnDeviceAllocatorControl::kFirstMemberAlignment)


        size_t allocatorHeapBlockSize  = 0;

    };

    struct DawnDrmFormatProperties {


        uint64_t modifier ;


        uint32_t modifierPlaneCount ;

    };

    struct DawnEncoderInternalUsageDescriptor : ChainedStruct {
        DawnEncoderInternalUsageDescriptor() {
            sType = wgpu::SType::DawnEncoderInternalUsageDescriptor;
        }
        alignas(wgpu::DawnEncoderInternalUsageDescriptor::kFirstMemberAlignment)


        wgpu::Bool useInternalUsages  = false;

    };

    struct DawnFakeBufferOOMForTesting : ChainedStruct {
        DawnFakeBufferOOMForTesting() {
            sType = wgpu::SType::DawnFakeBufferOOMForTesting;
        }
        alignas(wgpu::DawnFakeBufferOOMForTesting::kFirstMemberAlignment)


        wgpu::Bool fakeOOMAtWireClientMap ;


        wgpu::Bool fakeOOMAtNativeMap ;


        wgpu::Bool fakeOOMAtDevice ;

    };

    struct DawnFakeDeviceInitializeErrorForTesting : ChainedStruct {
        DawnFakeDeviceInitializeErrorForTesting() {
            sType = wgpu::SType::DawnFakeDeviceInitializeErrorForTesting;
        }

    };

    struct DawnHostMappedPointerLimits : ChainedStructOut {
        DawnHostMappedPointerLimits() {
            sType = wgpu::SType::DawnHostMappedPointerLimits;
        }
        alignas(wgpu::DawnHostMappedPointerLimits::kFirstMemberAlignment)


        uint32_t hostMappedPointerAlignment  = wgpu::kLimitU32Undefined;

    };

    struct DawnInjectedInvalidSType : ChainedStruct {
        DawnInjectedInvalidSType() {
            sType = wgpu::SType::DawnInjectedInvalidSType;
        }
        alignas(wgpu::DawnInjectedInvalidSType::kFirstMemberAlignment)


        wgpu::SType invalidSType  = {};

    };

    struct DawnRenderPassSampleCount : ChainedStruct {
        DawnRenderPassSampleCount() {
            sType = wgpu::SType::DawnRenderPassSampleCount;
        }
        alignas(wgpu::DawnRenderPassSampleCount::kFirstMemberAlignment)


        uint32_t sampleCount  = 1;

    };

    struct DawnShaderModuleSPIRVOptionsDescriptor : ChainedStruct {
        DawnShaderModuleSPIRVOptionsDescriptor() {
            sType = wgpu::SType::DawnShaderModuleSPIRVOptionsDescriptor;
        }
        alignas(wgpu::DawnShaderModuleSPIRVOptionsDescriptor::kFirstMemberAlignment)


        wgpu::Bool allowNonUniformDerivatives  = false;

    };

    struct DawnTexelCopyBufferRowAlignmentLimits : ChainedStructOut {
        DawnTexelCopyBufferRowAlignmentLimits() {
            sType = wgpu::SType::DawnTexelCopyBufferRowAlignmentLimits;
        }
        alignas(wgpu::DawnTexelCopyBufferRowAlignmentLimits::kFirstMemberAlignment)


        uint32_t minTexelCopyBufferRowAlignment  = wgpu::kLimitU32Undefined;

    };

    struct DawnTextureInternalUsageDescriptor : ChainedStruct {
        DawnTextureInternalUsageDescriptor() {
            sType = wgpu::SType::DawnTextureInternalUsageDescriptor;
        }
        alignas(wgpu::DawnTextureInternalUsageDescriptor::kFirstMemberAlignment)


        wgpu::TextureUsage internalUsage  = wgpu::TextureUsage::None;

    };

    struct DawnWireWGSLControl : ChainedStruct {
        DawnWireWGSLControl() {
            sType = wgpu::SType::DawnWireWGSLControl;
        }
        alignas(wgpu::DawnWireWGSLControl::kFirstMemberAlignment)


        wgpu::Bool enableExperimental  = false;


        wgpu::Bool enableUnsafe  = false;


        wgpu::Bool enableTesting  = false;

    };

    struct Extent2D {


        uint32_t width ;


        uint32_t height ;

    };

    struct Extent3D {


        uint32_t width ;


        uint32_t height  = 1;


        uint32_t depthOrArrayLayers  = 1;

    };

    struct ExternalTextureBindingEntry : ChainedStruct {
        ExternalTextureBindingEntry() {
            sType = wgpu::SType::ExternalTextureBindingEntry;
        }
        alignas(wgpu::ExternalTextureBindingEntry::kFirstMemberAlignment)


        ExternalTexture* externalTexture ;

    };

    struct ExternalTextureBindingLayout : ChainedStruct {
        ExternalTextureBindingLayout() {
            sType = wgpu::SType::ExternalTextureBindingLayout;
        }

    };

    struct Future {


        uint64_t id ;

    };

    struct InstanceLimits {
        ChainedStructOut * nextInChain = nullptr;


        size_t timedWaitAnyMaxCount  = 0;

    };

    struct MemoryHeapInfo {


        wgpu::HeapProperty properties  = wgpu::HeapProperty::None;


        uint64_t size ;

    };

    struct MultisampleState {
        ChainedStruct const * nextInChain = nullptr;


        uint32_t count  = 1;


        uint32_t mask  = 0xFFFFFFFF;


        wgpu::Bool alphaToCoverageEnabled  = false;

    };

    struct Origin2D {


        uint32_t x  = 0;


        uint32_t y  = 0;

    };

    struct Origin3D {


        uint32_t x  = 0;


        uint32_t y  = 0;


        uint32_t z  = 0;

    };

    struct PassTimestampWrites {
        ChainedStruct const * nextInChain = nullptr;


        QuerySet* querySet ;


        uint32_t beginningOfPassWriteIndex  = wgpu::kQuerySetIndexUndefined;


        uint32_t endOfPassWriteIndex  = wgpu::kQuerySetIndexUndefined;

    };

    struct PipelineLayoutResourceTable : ChainedStruct {
        PipelineLayoutResourceTable() {
            sType = wgpu::SType::PipelineLayoutResourceTable;
        }
        alignas(wgpu::PipelineLayoutResourceTable::kFirstMemberAlignment)


        wgpu::Bool usesResourceTable  = false;

    };

    struct PipelineLayoutStorageAttachment {
        ChainedStruct const * nextInChain = nullptr;


        uint64_t offset  = 0;


        wgpu::TextureFormat format  = wgpu::TextureFormat::Undefined;

    };

    struct PrimitiveState {
        ChainedStruct const * nextInChain = nullptr;


        wgpu::PrimitiveTopology topology  = wgpu::PrimitiveTopology::TriangleList;


        wgpu::IndexFormat stripIndexFormat  = wgpu::IndexFormat::Undefined;


        wgpu::FrontFace frontFace  = wgpu::FrontFace::CCW;


        wgpu::CullMode cullMode  = wgpu::CullMode::None;


        wgpu::Bool unclippedDepth  = false;

    };

    struct QuerySetDescriptor {
        ChainedStruct const * nextInChain = nullptr;


        StringView label ;


        wgpu::QueryType type  = {};


        uint32_t count ;

    };

    struct QueueDescriptor {
        ChainedStruct const * nextInChain = nullptr;


        StringView label ;

    };

    struct RenderBundleDescriptor {
        ChainedStruct const * nextInChain = nullptr;


        StringView label ;

    };

    struct RenderBundleEncoderDescriptor {
        ChainedStruct const * nextInChain = nullptr;


        StringView label ;




        // TODO(https://crbug.com/524405497): Support fixed-length spans.
        ityp::span<size_t, std::remove_pointer_t<wgpu::TextureFormat const *>> colorFormats;


        wgpu::TextureFormat depthStencilFormat  = wgpu::TextureFormat::Undefined;


        uint32_t sampleCount  = 1;


        wgpu::Bool depthReadOnly  = false;


        wgpu::Bool stencilReadOnly  = false;

    };

    struct RenderPassDepthStencilAttachment {
        ChainedStruct const * nextInChain = nullptr;


        TextureView* view ;


        wgpu::LoadOp depthLoadOp  = wgpu::LoadOp::Undefined;


        wgpu::StoreOp depthStoreOp  = wgpu::StoreOp::Undefined;


        float depthClearValue  = wgpu::kDepthClearValueUndefined;


        wgpu::Bool depthReadOnly  = false;


        wgpu::LoadOp stencilLoadOp  = wgpu::LoadOp::Undefined;


        wgpu::StoreOp stencilStoreOp  = wgpu::StoreOp::Undefined;


        uint32_t stencilClearValue  = 0;


        wgpu::Bool stencilReadOnly  = false;

    };

    struct RenderPassDescriptorResolveRect : ChainedStruct {
        RenderPassDescriptorResolveRect() {
            sType = wgpu::SType::RenderPassDescriptorResolveRect;
        }
        alignas(wgpu::RenderPassDescriptorResolveRect::kFirstMemberAlignment)


        uint32_t colorOffsetX ;


        uint32_t colorOffsetY ;


        uint32_t resolveOffsetX ;


        uint32_t resolveOffsetY ;


        uint32_t width ;


        uint32_t height ;

    };

    struct RenderPassMaxDrawCount : ChainedStruct {
        RenderPassMaxDrawCount() {
            sType = wgpu::SType::RenderPassMaxDrawCount;
        }
        alignas(wgpu::RenderPassMaxDrawCount::kFirstMemberAlignment)


        uint64_t maxDrawCount  = 50000000;

    };

    struct RequestAdapterWebXROptions : ChainedStruct {
        RequestAdapterWebXROptions() {
            sType = wgpu::SType::RequestAdapterWebXROptions;
        }
        alignas(wgpu::RequestAdapterWebXROptions::kFirstMemberAlignment)


        wgpu::Bool xrCompatible ;

    };

    struct ResourceTableDescriptor {
        ChainedStruct const * nextInChain = nullptr;


        StringView label ;


        uint32_t size ;

    };

    struct SamplerBindingLayout {
        ChainedStruct const * nextInChain = nullptr;


        wgpu::SamplerBindingType type  = wgpu::SamplerBindingType::Filtering;

    };

    struct ShaderModuleCompilationOptions : ChainedStruct {
        ShaderModuleCompilationOptions() {
            sType = wgpu::SType::ShaderModuleCompilationOptions;
        }
        alignas(wgpu::ShaderModuleCompilationOptions::kFirstMemberAlignment)


        wgpu::Bool strictMath ;

    };

    struct ShaderSourceSPIRV : ChainedStruct {
        ShaderSourceSPIRV() {
            sType = wgpu::SType::ShaderSourceSPIRV;
        }
        alignas(wgpu::ShaderSourceSPIRV::kFirstMemberAlignment)




        // TODO(https://crbug.com/524405497): Support fixed-length spans.
        ityp::span<uint32_t, std::remove_pointer_t<uint32_t const *>> code;

    };

    struct ShaderSourceWGSL : ChainedStruct {
        ShaderSourceWGSL() {
            sType = wgpu::SType::ShaderSourceWGSL;
        }
        alignas(wgpu::ShaderSourceWGSL::kFirstMemberAlignment)


        StringView code ;

    };

    struct StaticSamplerBindingLayout : ChainedStruct {
        StaticSamplerBindingLayout() {
            sType = wgpu::SType::StaticSamplerBindingLayout;
        }
        alignas(wgpu::StaticSamplerBindingLayout::kFirstMemberAlignment)


        Sampler* sampler ;


        uint32_t sampledTextureBinding  = wgpu::kLimitU32Undefined;

    };

    struct StencilFaceState {


        wgpu::CompareFunction compare  = wgpu::CompareFunction::Always;


        wgpu::StencilOperation failOp  = wgpu::StencilOperation::Keep;


        wgpu::StencilOperation depthFailOp  = wgpu::StencilOperation::Keep;


        wgpu::StencilOperation passOp  = wgpu::StencilOperation::Keep;

    };

    struct StorageTextureBindingLayout {
        ChainedStruct const * nextInChain = nullptr;


        wgpu::StorageTextureAccess access  = wgpu::StorageTextureAccess::WriteOnly;


        wgpu::TextureFormat format  = wgpu::TextureFormat::Undefined;


        wgpu::TextureViewDimension viewDimension  = wgpu::TextureViewDimension::e2D;

    };

    struct SubgroupMatrixConfig {


        wgpu::SubgroupMatrixComponentType componentType  = {};


        wgpu::SubgroupMatrixComponentType resultComponentType  = {};


        uint32_t M ;


        uint32_t N ;


        uint32_t K ;

    };

    struct SupportedFeatures {
        SupportedFeatures() = default;
        ~SupportedFeatures();
        SupportedFeatures(const SupportedFeatures&) = delete;
        SupportedFeatures& operator=(const SupportedFeatures&) = delete;
        SupportedFeatures(SupportedFeatures&&);
        SupportedFeatures& operator=(SupportedFeatures&&);





        // TODO(https://crbug.com/524405497): Support fixed-length spans.
        ityp::span<size_t, std::remove_pointer_t<wgpu::FeatureName const *>> features;

      private:
        inline void FreeMembers();
    };

    struct SupportedInstanceFeatures {
        SupportedInstanceFeatures() = default;
        ~SupportedInstanceFeatures();
        SupportedInstanceFeatures(const SupportedInstanceFeatures&) = delete;
        SupportedInstanceFeatures& operator=(const SupportedInstanceFeatures&) = delete;
        SupportedInstanceFeatures(SupportedInstanceFeatures&&);
        SupportedInstanceFeatures& operator=(SupportedInstanceFeatures&&);





        // TODO(https://crbug.com/524405497): Support fixed-length spans.
        ityp::span<size_t, std::remove_pointer_t<wgpu::InstanceFeatureName const *>> features;

      private:
        inline void FreeMembers();
    };

    struct SupportedWGSLLanguageFeatures {
        SupportedWGSLLanguageFeatures() = default;
        ~SupportedWGSLLanguageFeatures();
        SupportedWGSLLanguageFeatures(const SupportedWGSLLanguageFeatures&) = delete;
        SupportedWGSLLanguageFeatures& operator=(const SupportedWGSLLanguageFeatures&) = delete;
        SupportedWGSLLanguageFeatures(SupportedWGSLLanguageFeatures&&);
        SupportedWGSLLanguageFeatures& operator=(SupportedWGSLLanguageFeatures&&);





        // TODO(https://crbug.com/524405497): Support fixed-length spans.
        ityp::span<size_t, std::remove_pointer_t<wgpu::WGSLLanguageFeatureName const *>> features;

      private:
        inline void FreeMembers();
    };

    struct SurfaceCapabilities {
        SurfaceCapabilities() = default;
        ~SurfaceCapabilities();
        SurfaceCapabilities(const SurfaceCapabilities&) = delete;
        SurfaceCapabilities& operator=(const SurfaceCapabilities&) = delete;
        SurfaceCapabilities(SurfaceCapabilities&&);
        SurfaceCapabilities& operator=(SurfaceCapabilities&&);

        ChainedStructOut * nextInChain = nullptr;


        wgpu::TextureUsage usages  = wgpu::TextureUsage::None;




        // TODO(https://crbug.com/524405497): Support fixed-length spans.
        ityp::span<size_t, std::remove_pointer_t<wgpu::TextureFormat const *>> formats;




        // TODO(https://crbug.com/524405497): Support fixed-length spans.
        ityp::span<size_t, std::remove_pointer_t<wgpu::PresentMode const *>> presentModes;




        // TODO(https://crbug.com/524405497): Support fixed-length spans.
        ityp::span<size_t, std::remove_pointer_t<wgpu::CompositeAlphaMode const *>> alphaModes;

      private:
        inline void FreeMembers();
    };

    struct SurfaceColorManagement : ChainedStruct {
        SurfaceColorManagement() {
            sType = wgpu::SType::SurfaceColorManagement;
        }
        alignas(wgpu::SurfaceColorManagement::kFirstMemberAlignment)


        wgpu::PredefinedColorSpace colorSpace  = {};


        wgpu::ToneMappingMode toneMappingMode  = {};

    };

    struct SurfaceConfiguration {
        ChainedStruct const * nextInChain = nullptr;


        Device* device ;


        wgpu::TextureFormat format  = wgpu::TextureFormat::Undefined;


        wgpu::TextureUsage usage  = wgpu::TextureUsage::RenderAttachment;


        uint32_t width ;


        uint32_t height ;




        // TODO(https://crbug.com/524405497): Support fixed-length spans.
        ityp::span<size_t, std::remove_pointer_t<wgpu::TextureFormat const *>> viewFormats;


        wgpu::CompositeAlphaMode alphaMode  = wgpu::CompositeAlphaMode::Auto;


        wgpu::PresentMode presentMode  = wgpu::PresentMode::Fifo;

    };

    struct SurfaceDescriptorFromWindowsCoreWindow : ChainedStruct {
        SurfaceDescriptorFromWindowsCoreWindow() {
            sType = wgpu::SType::SurfaceDescriptorFromWindowsCoreWindow;
        }
        alignas(wgpu::SurfaceDescriptorFromWindowsCoreWindow::kFirstMemberAlignment)


        void * coreWindow  = nullptr;

    };

    struct SurfaceDescriptorFromWindowsUWPSwapChainPanel : ChainedStruct {
        SurfaceDescriptorFromWindowsUWPSwapChainPanel() {
            sType = wgpu::SType::SurfaceDescriptorFromWindowsUWPSwapChainPanel;
        }
        alignas(wgpu::SurfaceDescriptorFromWindowsUWPSwapChainPanel::kFirstMemberAlignment)


        void * swapChainPanel  = nullptr;

    };

    struct SurfaceDescriptorFromWindowsWinUISwapChainPanel : ChainedStruct {
        SurfaceDescriptorFromWindowsWinUISwapChainPanel() {
            sType = wgpu::SType::SurfaceDescriptorFromWindowsWinUISwapChainPanel;
        }
        alignas(wgpu::SurfaceDescriptorFromWindowsWinUISwapChainPanel::kFirstMemberAlignment)


        void * swapChainPanel  = nullptr;

    };

    struct SurfaceTexture {
        ChainedStructOut * nextInChain = nullptr;


        Texture* texture ;


        wgpu::SurfaceGetCurrentTextureStatus status  = {};

    };

    struct TexelBufferBindingEntry : ChainedStruct {
        TexelBufferBindingEntry() {
            sType = wgpu::SType::TexelBufferBindingEntry;
        }
        alignas(wgpu::TexelBufferBindingEntry::kFirstMemberAlignment)


        TexelBufferView* texelBufferView ;

    };

    struct TexelBufferBindingLayout : ChainedStruct {
        TexelBufferBindingLayout() {
            sType = wgpu::SType::TexelBufferBindingLayout;
        }
        alignas(wgpu::TexelBufferBindingLayout::kFirstMemberAlignment)


        wgpu::TexelBufferAccess access  = wgpu::TexelBufferAccess::ReadWrite;


        wgpu::TextureFormat format  = wgpu::TextureFormat::Undefined;

    };

    struct TexelBufferViewDescriptor {
        ChainedStruct const * nextInChain = nullptr;


        StringView label ;


        wgpu::TextureFormat format  = wgpu::TextureFormat::Undefined;


        uint64_t offset  = 0;


        uint64_t size  = wgpu::kWholeSize;

    };

    struct TexelCopyBufferLayout {


        uint64_t offset  = 0;


        uint32_t bytesPerRow  = wgpu::kCopyStrideUndefined;


        uint32_t rowsPerImage  = wgpu::kCopyStrideUndefined;

    };

    struct TextureBindingLayout {
        ChainedStruct const * nextInChain = nullptr;


        wgpu::TextureSampleType sampleType  = wgpu::TextureSampleType::Float;


        wgpu::TextureViewDimension viewDimension  = wgpu::TextureViewDimension::e2D;


        wgpu::Bool multisampled  = false;

    };

    struct TextureBindingViewDimension : ChainedStruct {
        TextureBindingViewDimension() {
            sType = wgpu::SType::TextureBindingViewDimension;
        }
        alignas(wgpu::TextureBindingViewDimension::kFirstMemberAlignment)


        wgpu::TextureViewDimension textureBindingViewDimension  = wgpu::TextureViewDimension::Undefined;

    };

    struct TextureComponentSwizzle {


        wgpu::ComponentSwizzle r  = wgpu::ComponentSwizzle::R;


        wgpu::ComponentSwizzle g  = wgpu::ComponentSwizzle::G;


        wgpu::ComponentSwizzle b  = wgpu::ComponentSwizzle::B;


        wgpu::ComponentSwizzle a  = wgpu::ComponentSwizzle::A;

    };

    struct VertexAttribute {
        ChainedStruct const * nextInChain = nullptr;


        wgpu::VertexFormat format  = {};


        uint64_t offset ;


        uint32_t shaderLocation ;

    };

    struct YCbCrVkDescriptor : ChainedStruct {
        YCbCrVkDescriptor() {
            sType = wgpu::SType::YCbCrVkDescriptor;
        }
        alignas(wgpu::YCbCrVkDescriptor::kFirstMemberAlignment)


        uint32_t vkFormat  = 0;


        uint32_t vkYCbCrModel  = 0;


        uint32_t vkYCbCrRange  = 0;


        uint32_t vkComponentSwizzleRed  = 0;


        uint32_t vkComponentSwizzleGreen  = 0;


        uint32_t vkComponentSwizzleBlue  = 0;


        uint32_t vkComponentSwizzleAlpha  = 0;


        uint32_t vkXChromaOffset  = 0;


        uint32_t vkYChromaOffset  = 0;


        wgpu::FilterMode vkChromaFilter  = wgpu::FilterMode::Nearest;


        wgpu::Bool forceExplicitReconstruction  = false;


        uint64_t externalFormat  = 0;

    };

    struct AdapterPropertiesMemoryHeaps : ChainedStructOut {
        AdapterPropertiesMemoryHeaps() {
            sType = wgpu::SType::AdapterPropertiesMemoryHeaps;
        }
        ~AdapterPropertiesMemoryHeaps();
        AdapterPropertiesMemoryHeaps(const AdapterPropertiesMemoryHeaps&) = delete;
        AdapterPropertiesMemoryHeaps& operator=(const AdapterPropertiesMemoryHeaps&) = delete;
        AdapterPropertiesMemoryHeaps(AdapterPropertiesMemoryHeaps&&);
        AdapterPropertiesMemoryHeaps& operator=(AdapterPropertiesMemoryHeaps&&);

        alignas(wgpu::AdapterPropertiesMemoryHeaps::kFirstMemberAlignment)




        // TODO(https://crbug.com/524405497): Support fixed-length spans.
        ityp::span<size_t, std::remove_pointer_t<MemoryHeapInfo const *>> heapInfo;

      private:
        inline void FreeMembers();
    };

    struct AdapterPropertiesSubgroupMatrixConfigs : ChainedStructOut {
        AdapterPropertiesSubgroupMatrixConfigs() {
            sType = wgpu::SType::AdapterPropertiesSubgroupMatrixConfigs;
        }
        ~AdapterPropertiesSubgroupMatrixConfigs();
        AdapterPropertiesSubgroupMatrixConfigs(const AdapterPropertiesSubgroupMatrixConfigs&) = delete;
        AdapterPropertiesSubgroupMatrixConfigs& operator=(const AdapterPropertiesSubgroupMatrixConfigs&) = delete;
        AdapterPropertiesSubgroupMatrixConfigs(AdapterPropertiesSubgroupMatrixConfigs&&);
        AdapterPropertiesSubgroupMatrixConfigs& operator=(AdapterPropertiesSubgroupMatrixConfigs&&);

        alignas(wgpu::AdapterPropertiesSubgroupMatrixConfigs::kFirstMemberAlignment)




        // TODO(https://crbug.com/524405497): Support fixed-length spans.
        ityp::span<size_t, std::remove_pointer_t<SubgroupMatrixConfig const *>> configs;

      private:
        inline void FreeMembers();
    };

    struct BindGroupEntry {
        ChainedStruct const * nextInChain = nullptr;


        uint32_t binding ;


        Buffer* buffer  = nullptr;


        uint64_t offset  = 0;


        uint64_t size  = wgpu::kWholeSize;


        Sampler* sampler  = nullptr;


        TextureView* textureView  = nullptr;

    };

    struct BindGroupLayoutEntry {
        ChainedStruct const * nextInChain = nullptr;


        uint32_t binding ;


        wgpu::ShaderStage visibility  = wgpu::ShaderStage::None;


        uint32_t bindingArraySize  = 0;


        BufferBindingLayout buffer  = { nullptr, wgpu::BufferBindingType::BindingNotUsed, false, 0 };


        SamplerBindingLayout sampler  = { nullptr, wgpu::SamplerBindingType::BindingNotUsed };


        TextureBindingLayout texture  = { nullptr, wgpu::TextureSampleType::BindingNotUsed, wgpu::TextureViewDimension::e2D, false };


        StorageTextureBindingLayout storageTexture  = { nullptr, wgpu::StorageTextureAccess::BindingNotUsed, wgpu::TextureFormat::Undefined, wgpu::TextureViewDimension::e2D };

    };

    struct BlendState {


        BlendComponent color ;


        BlendComponent alpha ;

    };

    struct BufferDescriptor {
        ChainedStruct const * nextInChain = nullptr;


        StringView label ;


        wgpu::BufferUsage usage  = wgpu::BufferUsage::None;


        uint64_t size ;


        wgpu::Bool mappedAtCreation  = false;

    };

    struct CommandEncoderDescriptor {
        ChainedStruct const * nextInChain = nullptr;


        StringView label ;

    };

    struct CompilationMessage {
        ChainedStruct const * nextInChain = nullptr;


        StringView message ;


        wgpu::CompilationMessageType type  = {};


        uint64_t lineNum ;


        uint64_t linePos ;


        uint64_t offset ;


        uint64_t length ;

    };

    struct ComputePassDescriptor {
        ChainedStruct const * nextInChain = nullptr;


        StringView label ;


        PassTimestampWrites const * timestampWrites  = nullptr;

    };

    struct ComputeState {
        ChainedStruct const * nextInChain = nullptr;


        ShaderModule* module ;


        StringView entryPoint ;




        // TODO(https://crbug.com/524405497): Support fixed-length spans.
        ityp::span<size_t, std::remove_pointer_t<ConstantEntry const *>> constants;

    };

    struct DawnDrmFormatCapabilities : ChainedStructOut {
        DawnDrmFormatCapabilities() {
            sType = wgpu::SType::DawnDrmFormatCapabilities;
        }
        ~DawnDrmFormatCapabilities();
        DawnDrmFormatCapabilities(const DawnDrmFormatCapabilities&) = delete;
        DawnDrmFormatCapabilities& operator=(const DawnDrmFormatCapabilities&) = delete;
        DawnDrmFormatCapabilities(DawnDrmFormatCapabilities&&);
        DawnDrmFormatCapabilities& operator=(DawnDrmFormatCapabilities&&);

        alignas(wgpu::DawnDrmFormatCapabilities::kFirstMemberAlignment)




        // TODO(https://crbug.com/524405497): Support fixed-length spans.
        ityp::span<size_t, std::remove_pointer_t<DawnDrmFormatProperties const *>> properties;

      private:
        inline void FreeMembers();
    };

    struct DepthStencilState {
        ChainedStruct const * nextInChain = nullptr;


        wgpu::TextureFormat format  = wgpu::TextureFormat::Undefined;


        wgpu::OptionalBool depthWriteEnabled  = wgpu::OptionalBool::Undefined;


        wgpu::CompareFunction depthCompare  = wgpu::CompareFunction::Undefined;


        StencilFaceState stencilFront ;


        StencilFaceState stencilBack ;


        uint32_t stencilReadMask  = 0xFFFFFFFF;


        uint32_t stencilWriteMask  = 0xFFFFFFFF;


        int32_t depthBias  = 0;


        float depthBiasSlopeScale  = 0.f;


        float depthBiasClamp  = 0.f;

    };

    struct ExternalTextureDescriptor {
        ChainedStruct const * nextInChain = nullptr;


        StringView label ;


        TextureView* plane0 ;


        TextureView* plane1  = nullptr;


        Origin2D cropOrigin ;


        Extent2D cropSize ;


        Extent2D apparentSize ;


        wgpu::Bool doYuvToRgbConversionOnly  = false;


        float const * yuvToRgbConversionMatrix  = nullptr;


        float const * srcTransferFunctionParameters  = nullptr;


        float const * dstTransferFunctionParameters  = nullptr;


        float const * gamutConversionMatrix  = nullptr;


        wgpu::Bool mirrored  = false;


        wgpu::ExternalTextureRotation rotation  = wgpu::ExternalTextureRotation::Rotate0Degrees;

    };

    struct FutureWaitInfo {


        Future future ;


        wgpu::Bool completed  = false;

    };

    struct ImageCopyExternalTexture {
        ChainedStruct const * nextInChain = nullptr;


        ExternalTexture* externalTexture ;


        Origin3D origin ;


        Extent2D naturalSize ;

    };

    struct InstanceDescriptor {
        ChainedStruct const * nextInChain = nullptr;




        // TODO(https://crbug.com/524405497): Support fixed-length spans.
        ityp::span<size_t, std::remove_pointer_t<wgpu::InstanceFeatureName const *>> requiredFeatures;


        InstanceLimits const * requiredLimits  = nullptr;

    };

    struct Limits {
        ChainedStructOut * nextInChain = nullptr;


        uint32_t maxTextureDimension1D  = wgpu::kLimitU32Undefined;


        uint32_t maxTextureDimension2D  = wgpu::kLimitU32Undefined;


        uint32_t maxTextureDimension3D  = wgpu::kLimitU32Undefined;


        uint32_t maxTextureArrayLayers  = wgpu::kLimitU32Undefined;


        uint32_t maxBindGroups  = wgpu::kLimitU32Undefined;


        uint32_t maxBindGroupsPlusVertexBuffers  = wgpu::kLimitU32Undefined;


        uint32_t maxBindingsPerBindGroup  = wgpu::kLimitU32Undefined;


        uint32_t maxDynamicUniformBuffersPerPipelineLayout  = wgpu::kLimitU32Undefined;


        uint32_t maxDynamicStorageBuffersPerPipelineLayout  = wgpu::kLimitU32Undefined;


        uint32_t maxSampledTexturesPerShaderStage  = wgpu::kLimitU32Undefined;


        uint32_t maxSamplersPerShaderStage  = wgpu::kLimitU32Undefined;


        uint32_t maxStorageBuffersPerShaderStage  = wgpu::kLimitU32Undefined;


        uint32_t maxStorageTexturesPerShaderStage  = wgpu::kLimitU32Undefined;


        uint32_t maxUniformBuffersPerShaderStage  = wgpu::kLimitU32Undefined;


        uint64_t maxUniformBufferBindingSize  = wgpu::kLimitU64Undefined;


        uint64_t maxStorageBufferBindingSize  = wgpu::kLimitU64Undefined;


        uint32_t minUniformBufferOffsetAlignment  = wgpu::kLimitU32Undefined;


        uint32_t minStorageBufferOffsetAlignment  = wgpu::kLimitU32Undefined;


        uint32_t maxVertexBuffers  = wgpu::kLimitU32Undefined;


        uint64_t maxBufferSize  = wgpu::kLimitU64Undefined;


        uint32_t maxVertexAttributes  = wgpu::kLimitU32Undefined;


        uint32_t maxVertexBufferArrayStride  = wgpu::kLimitU32Undefined;


        uint32_t maxInterStageShaderVariables  = wgpu::kLimitU32Undefined;


        uint32_t maxColorAttachments  = wgpu::kLimitU32Undefined;


        uint32_t maxColorAttachmentBytesPerSample  = wgpu::kLimitU32Undefined;


        uint32_t maxComputeWorkgroupStorageSize  = wgpu::kLimitU32Undefined;


        uint32_t maxComputeInvocationsPerWorkgroup  = wgpu::kLimitU32Undefined;


        uint32_t maxComputeWorkgroupSizeX  = wgpu::kLimitU32Undefined;


        uint32_t maxComputeWorkgroupSizeY  = wgpu::kLimitU32Undefined;


        uint32_t maxComputeWorkgroupSizeZ  = wgpu::kLimitU32Undefined;


        uint32_t maxComputeWorkgroupsPerDimension  = wgpu::kLimitU32Undefined;


        uint32_t maxImmediateSize  = wgpu::kLimitU32Undefined;

    };

    struct PipelineLayoutPixelLocalStorage : ChainedStruct {
        PipelineLayoutPixelLocalStorage() {
            sType = wgpu::SType::PipelineLayoutPixelLocalStorage;
        }
        alignas(wgpu::PipelineLayoutPixelLocalStorage::kFirstMemberAlignment)


        uint64_t totalPixelLocalStorageSize ;




        // TODO(https://crbug.com/524405497): Support fixed-length spans.
        ityp::span<size_t, std::remove_pointer_t<PipelineLayoutStorageAttachment const *>> storageAttachments;

    };

    struct RenderPassColorAttachment {
        ChainedStruct const * nextInChain = nullptr;


        TextureView* view  = nullptr;


        uint32_t depthSlice  = wgpu::kDepthSliceUndefined;


        TextureView* resolveTarget  = nullptr;


        wgpu::LoadOp loadOp  = wgpu::LoadOp::Undefined;


        wgpu::StoreOp storeOp  = wgpu::StoreOp::Undefined;


        Color clearValue ;

    };

    struct RenderPassRenderAreaRect : ChainedStruct {
        RenderPassRenderAreaRect() {
            sType = wgpu::SType::RenderPassRenderAreaRect;
        }
        alignas(wgpu::RenderPassRenderAreaRect::kFirstMemberAlignment)


        Origin2D origin ;


        Extent2D size ;

    };

    struct RenderPassStorageAttachment {
        ChainedStruct const * nextInChain = nullptr;


        uint64_t offset  = 0;


        TextureView* storage ;


        wgpu::LoadOp loadOp  = wgpu::LoadOp::Undefined;


        wgpu::StoreOp storeOp  = wgpu::StoreOp::Undefined;


        Color clearValue ;

    };

    struct RequestAdapterOptions {
        ChainedStruct const * nextInChain = nullptr;


        wgpu::FeatureLevel featureLevel  = wgpu::FeatureLevel::Core;


        wgpu::PowerPreference powerPreference  = wgpu::PowerPreference::Undefined;


        wgpu::Bool forceFallbackAdapter  = false;


        wgpu::BackendType backendType  = wgpu::BackendType::Undefined;


        Surface* compatibleSurface  = nullptr;

    };

    struct SamplerDescriptor {
        ChainedStruct const * nextInChain = nullptr;


        StringView label ;


        wgpu::AddressMode addressModeU  = wgpu::AddressMode::ClampToEdge;


        wgpu::AddressMode addressModeV  = wgpu::AddressMode::ClampToEdge;


        wgpu::AddressMode addressModeW  = wgpu::AddressMode::ClampToEdge;


        wgpu::FilterMode magFilter  = wgpu::FilterMode::Nearest;


        wgpu::FilterMode minFilter  = wgpu::FilterMode::Nearest;


        wgpu::MipmapFilterMode mipmapFilter  = wgpu::MipmapFilterMode::Nearest;


        float lodMinClamp  = 0.f;


        float lodMaxClamp  = 32.f;


        wgpu::CompareFunction compare  = wgpu::CompareFunction::Undefined;


        uint16_t maxAnisotropy  = 1;

    };

    struct ShaderModuleDescriptor {
        ChainedStruct const * nextInChain = nullptr;


        StringView label ;

    };

    struct SurfaceDescriptor {
        ChainedStruct const * nextInChain = nullptr;


        StringView label ;

    };

    struct TexelCopyBufferInfo {


        TexelCopyBufferLayout layout ;


        Buffer* buffer ;

    };

    struct TexelCopyTextureInfo {


        Texture* texture ;


        uint32_t mipLevel  = 0;


        Origin3D origin ;


        wgpu::TextureAspect aspect  = wgpu::TextureAspect::All;

    };

    struct TextureComponentSwizzleDescriptor : ChainedStruct {
        TextureComponentSwizzleDescriptor() {
            sType = wgpu::SType::TextureComponentSwizzleDescriptor;
        }
        alignas(wgpu::TextureComponentSwizzleDescriptor::kFirstMemberAlignment)


        TextureComponentSwizzle swizzle ;

    };

    struct TextureDescriptor {
        ChainedStruct const * nextInChain = nullptr;


        StringView label ;


        wgpu::TextureUsage usage  = wgpu::TextureUsage::None;


        wgpu::TextureDimension dimension  = wgpu::TextureDimension::e2D;


        Extent3D size ;


        wgpu::TextureFormat format  = wgpu::TextureFormat::Undefined;


        uint32_t mipLevelCount  = 1;


        uint32_t sampleCount  = 1;




        // TODO(https://crbug.com/524405497): Support fixed-length spans.
        ityp::span<size_t, std::remove_pointer_t<wgpu::TextureFormat const *>> viewFormats;

    };

    struct VertexBufferLayout {
        ChainedStruct const * nextInChain = nullptr;


        wgpu::VertexStepMode stepMode  = wgpu::VertexStepMode::Undefined;


        uint64_t arrayStride ;




        // TODO(https://crbug.com/524405497): Support fixed-length spans.
        ityp::span<size_t, std::remove_pointer_t<VertexAttribute const *>> attributes;

    };

    struct AdapterInfo {
        AdapterInfo() = default;
        ~AdapterInfo();
        AdapterInfo(const AdapterInfo&) = delete;
        AdapterInfo& operator=(const AdapterInfo&) = delete;
        AdapterInfo(AdapterInfo&&);
        AdapterInfo& operator=(AdapterInfo&&);

        ChainedStructOut * nextInChain = nullptr;


        StringView vendor ;


        StringView architecture ;


        StringView device ;


        StringView description ;


        wgpu::BackendType backendType  = wgpu::BackendType::Undefined;


        wgpu::AdapterType adapterType  = {};


        uint32_t vendorID ;


        uint32_t deviceID ;


        uint32_t subgroupMinSize ;


        uint32_t subgroupMaxSize ;

      private:
        inline void FreeMembers();
    };

    struct BindGroupDescriptor {
        ChainedStruct const * nextInChain = nullptr;


        StringView label ;


        BindGroupLayout* layout ;




        // TODO(https://crbug.com/524405497): Support fixed-length spans.
        ityp::span<size_t, std::remove_pointer_t<BindGroupEntry const *>> entries;

    };

    struct BindGroupLayoutDescriptor {
        ChainedStruct const * nextInChain = nullptr;


        StringView label ;




        // TODO(https://crbug.com/524405497): Support fixed-length spans.
        ityp::span<size_t, std::remove_pointer_t<BindGroupLayoutEntry const *>> entries;

    };

    struct ColorTargetState {
        ChainedStruct const * nextInChain = nullptr;


        wgpu::TextureFormat format  = wgpu::TextureFormat::Undefined;


        BlendState const * blend  = nullptr;


        wgpu::ColorWriteMask writeMask  = wgpu::ColorWriteMask::All;

    };

    struct CompilationInfo {
        ChainedStruct const * nextInChain = nullptr;




        // TODO(https://crbug.com/524405497): Support fixed-length spans.
        ityp::span<size_t, std::remove_pointer_t<CompilationMessage const *>> messages;

    };

    struct ComputePipelineDescriptor {
        ChainedStruct const * nextInChain = nullptr;


        StringView label ;


        PipelineLayout* layout  = nullptr;


        ComputeState compute ;

    };

    struct DawnFormatCapabilities {
        ChainedStructOut * nextInChain = nullptr;

    };

    struct DeviceDescriptor {
        ChainedStruct const * nextInChain = nullptr;


        StringView label ;




        // TODO(https://crbug.com/524405497): Support fixed-length spans.
        ityp::span<size_t, std::remove_pointer_t<wgpu::FeatureName const *>> requiredFeatures;


        Limits const * requiredLimits  = nullptr;


        QueueDescriptor defaultQueue ;


        WGPUDeviceLostCallbackInfo deviceLostCallbackInfo  = WGPU_DEVICE_LOST_CALLBACK_INFO_INIT;


        WGPUUncapturedErrorCallbackInfo uncapturedErrorCallbackInfo  = WGPU_UNCAPTURED_ERROR_CALLBACK_INFO_INIT;

    };

    struct PipelineLayoutDescriptor {
        ChainedStruct const * nextInChain = nullptr;


        StringView label ;




        // TODO(https://crbug.com/524405497): Support fixed-length spans.
        ityp::span<size_t, std::remove_pointer_t<BindGroupLayout* const *>> bindGroupLayouts;


        uint32_t immediateSize  = 0;

    };

    struct RenderPassPixelLocalStorage : ChainedStruct {
        RenderPassPixelLocalStorage() {
            sType = wgpu::SType::RenderPassPixelLocalStorage;
        }
        alignas(wgpu::RenderPassPixelLocalStorage::kFirstMemberAlignment)


        uint64_t totalPixelLocalStorageSize ;




        // TODO(https://crbug.com/524405497): Support fixed-length spans.
        ityp::span<size_t, std::remove_pointer_t<RenderPassStorageAttachment const *>> storageAttachments;

    };

    struct TextureViewDescriptor {
        ChainedStruct const * nextInChain = nullptr;


        StringView label ;


        wgpu::TextureFormat format  = wgpu::TextureFormat::Undefined;


        wgpu::TextureViewDimension dimension  = wgpu::TextureViewDimension::Undefined;


        uint32_t baseMipLevel  = 0;


        uint32_t mipLevelCount  = wgpu::kMipLevelCountUndefined;


        uint32_t baseArrayLayer  = 0;


        uint32_t arrayLayerCount  = wgpu::kArrayLayerCountUndefined;


        wgpu::TextureAspect aspect  = wgpu::TextureAspect::All;


        wgpu::TextureUsage usage  = wgpu::TextureUsage::None;

    };

    struct VertexState {
        ChainedStruct const * nextInChain = nullptr;


        ShaderModule* module ;


        StringView entryPoint ;




        // TODO(https://crbug.com/524405497): Support fixed-length spans.
        ityp::span<size_t, std::remove_pointer_t<ConstantEntry const *>> constants;




        // TODO(https://crbug.com/524405497): Support fixed-length spans.
        ityp::span<size_t, std::remove_pointer_t<VertexBufferLayout const *>> buffers;

    };

    struct FragmentState {
        ChainedStruct const * nextInChain = nullptr;


        ShaderModule* module ;


        StringView entryPoint ;




        // TODO(https://crbug.com/524405497): Support fixed-length spans.
        ityp::span<size_t, std::remove_pointer_t<ConstantEntry const *>> constants;




        // TODO(https://crbug.com/524405497): Support fixed-length spans.
        ityp::span<size_t, std::remove_pointer_t<ColorTargetState const *>> targets;

    };

    struct RenderPassDescriptor {
        ChainedStruct const * nextInChain = nullptr;


        StringView label ;




        // TODO(https://crbug.com/524405497): Support fixed-length spans.
        ityp::span<size_t, std::remove_pointer_t<RenderPassColorAttachment const *>> colorAttachments;


        RenderPassDepthStencilAttachment const * depthStencilAttachment  = nullptr;


        QuerySet* occlusionQuerySet  = nullptr;


        PassTimestampWrites const * timestampWrites  = nullptr;

    };

    struct RenderPipelineDescriptor {
        ChainedStruct const * nextInChain = nullptr;


        StringView label ;


        PipelineLayout* layout  = nullptr;


        VertexState vertex ;


        PrimitiveState primitive ;


        DepthStencilState const * depthStencil  = nullptr;


        MultisampleState multisample ;


        FragmentState const * fragment  = nullptr;

    };

    // NOLINTEND(bugprone-invalid-enum-default-initialization)


    // SupportedFeatures
    void APIFreeMembers(WGPUSupportedFeatures);
    // SupportedInstanceFeatures
    void APIFreeMembers(WGPUSupportedInstanceFeatures);
    // SupportedWGSLLanguageFeatures
    void APIFreeMembers(WGPUSupportedWGSLLanguageFeatures);
    // SurfaceCapabilities
    void APIFreeMembers(WGPUSurfaceCapabilities);
    // AdapterPropertiesMemoryHeaps
    void APIFreeMembers(WGPUAdapterPropertiesMemoryHeaps);
    // AdapterPropertiesSubgroupMatrixConfigs
    void APIFreeMembers(WGPUAdapterPropertiesSubgroupMatrixConfigs);
    // DawnDrmFormatCapabilities
    void APIFreeMembers(WGPUDawnDrmFormatCapabilities);
    // AdapterInfo
    void APIFreeMembers(WGPUAdapterInfo);

} // namespace dawn::wire::client

#endif  // DAWNWIRE_CLIENT_WGPU_STRUCTS_AUTOGEN_H_
