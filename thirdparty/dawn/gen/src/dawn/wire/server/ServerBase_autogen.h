
#ifndef DAWNWIRE_SERVER_SERVERBASE_AUTOGEN_H_
#define DAWNWIRE_SERVER_SERVERBASE_AUTOGEN_H_

#include <memory>
#include <tuple>

#include "dawn/dawn_proc_table.h"
#include "dawn/wire/Wire.h"
#include "dawn/wire/WireCmd_autogen.h"
#include "dawn/wire/server/WGPUTraits_autogen.h"
#include "src/dawn/common/Mutex.h"
#include "src/dawn/wire/ChunkedCommandHandler.h"
#include "src/dawn/wire/WireDeserializeAllocator.h"
#include "src/dawn/wire/server/ObjectStorage.h"

namespace dawn::wire::server {

    class ServerBase : public ChunkedCommandHandler, public ObjectIdResolver {
      public:
        explicit ServerBase(const DawnProcTable& procs) : mProcs(std::make_shared<DawnProcTable>(procs)) {}
        ~ServerBase() override = default;

        Mutex::AutoLock GetGuard() { return Mutex::AutoLock(&mMutex); }

        // Implementation of the ObjectIdResolver interface
        WireResult GetFromId(ObjectId id, WGPUAdapter* out) const final {
            return std::get<KnownObjects<WGPUAdapter>>(mKnown).GetNativeHandle(id, out);
        }

        WireResult GetOptionalFromId(ObjectId id, WGPUAdapter* out) const final {
            if (id == 0) {
                *out = nullptr;
                return WireResult::Success;
            }
            return GetFromId(id, out);
        }
        WireResult GetFromId(ObjectId id, WGPUBindGroup* out) const final {
            return std::get<KnownObjects<WGPUBindGroup>>(mKnown).GetNativeHandle(id, out);
        }

        WireResult GetOptionalFromId(ObjectId id, WGPUBindGroup* out) const final {
            if (id == 0) {
                *out = nullptr;
                return WireResult::Success;
            }
            return GetFromId(id, out);
        }
        WireResult GetFromId(ObjectId id, WGPUBindGroupLayout* out) const final {
            return std::get<KnownObjects<WGPUBindGroupLayout>>(mKnown).GetNativeHandle(id, out);
        }

        WireResult GetOptionalFromId(ObjectId id, WGPUBindGroupLayout* out) const final {
            if (id == 0) {
                *out = nullptr;
                return WireResult::Success;
            }
            return GetFromId(id, out);
        }
        WireResult GetFromId(ObjectId id, WGPUBuffer* out) const final {
            return std::get<KnownObjects<WGPUBuffer>>(mKnown).GetNativeHandle(id, out);
        }

        WireResult GetOptionalFromId(ObjectId id, WGPUBuffer* out) const final {
            if (id == 0) {
                *out = nullptr;
                return WireResult::Success;
            }
            return GetFromId(id, out);
        }
        WireResult GetFromId(ObjectId id, WGPUCommandBuffer* out) const final {
            return std::get<KnownObjects<WGPUCommandBuffer>>(mKnown).GetNativeHandle(id, out);
        }

        WireResult GetOptionalFromId(ObjectId id, WGPUCommandBuffer* out) const final {
            if (id == 0) {
                *out = nullptr;
                return WireResult::Success;
            }
            return GetFromId(id, out);
        }
        WireResult GetFromId(ObjectId id, WGPUCommandEncoder* out) const final {
            return std::get<KnownObjects<WGPUCommandEncoder>>(mKnown).GetNativeHandle(id, out);
        }

        WireResult GetOptionalFromId(ObjectId id, WGPUCommandEncoder* out) const final {
            if (id == 0) {
                *out = nullptr;
                return WireResult::Success;
            }
            return GetFromId(id, out);
        }
        WireResult GetFromId(ObjectId id, WGPUComputePipeline* out) const final {
            return std::get<KnownObjects<WGPUComputePipeline>>(mKnown).GetNativeHandle(id, out);
        }

        WireResult GetOptionalFromId(ObjectId id, WGPUComputePipeline* out) const final {
            if (id == 0) {
                *out = nullptr;
                return WireResult::Success;
            }
            return GetFromId(id, out);
        }
        WireResult GetFromId(ObjectId id, WGPUDevice* out) const final {
            return std::get<KnownObjects<WGPUDevice>>(mKnown).GetNativeHandle(id, out);
        }

        WireResult GetOptionalFromId(ObjectId id, WGPUDevice* out) const final {
            if (id == 0) {
                *out = nullptr;
                return WireResult::Success;
            }
            return GetFromId(id, out);
        }
        WireResult GetFromId(ObjectId id, WGPUExternalTexture* out) const final {
            return std::get<KnownObjects<WGPUExternalTexture>>(mKnown).GetNativeHandle(id, out);
        }

        WireResult GetOptionalFromId(ObjectId id, WGPUExternalTexture* out) const final {
            if (id == 0) {
                *out = nullptr;
                return WireResult::Success;
            }
            return GetFromId(id, out);
        }
        WireResult GetFromId(ObjectId id, WGPUInstance* out) const final {
            return std::get<KnownObjects<WGPUInstance>>(mKnown).GetNativeHandle(id, out);
        }

        WireResult GetOptionalFromId(ObjectId id, WGPUInstance* out) const final {
            if (id == 0) {
                *out = nullptr;
                return WireResult::Success;
            }
            return GetFromId(id, out);
        }
        WireResult GetFromId(ObjectId id, WGPUPipelineLayout* out) const final {
            return std::get<KnownObjects<WGPUPipelineLayout>>(mKnown).GetNativeHandle(id, out);
        }

        WireResult GetOptionalFromId(ObjectId id, WGPUPipelineLayout* out) const final {
            if (id == 0) {
                *out = nullptr;
                return WireResult::Success;
            }
            return GetFromId(id, out);
        }
        WireResult GetFromId(ObjectId id, WGPUQuerySet* out) const final {
            return std::get<KnownObjects<WGPUQuerySet>>(mKnown).GetNativeHandle(id, out);
        }

        WireResult GetOptionalFromId(ObjectId id, WGPUQuerySet* out) const final {
            if (id == 0) {
                *out = nullptr;
                return WireResult::Success;
            }
            return GetFromId(id, out);
        }
        WireResult GetFromId(ObjectId id, WGPUQueue* out) const final {
            return std::get<KnownObjects<WGPUQueue>>(mKnown).GetNativeHandle(id, out);
        }

        WireResult GetOptionalFromId(ObjectId id, WGPUQueue* out) const final {
            if (id == 0) {
                *out = nullptr;
                return WireResult::Success;
            }
            return GetFromId(id, out);
        }
        WireResult GetFromId(ObjectId id, WGPURenderBundle* out) const final {
            return std::get<KnownObjects<WGPURenderBundle>>(mKnown).GetNativeHandle(id, out);
        }

        WireResult GetOptionalFromId(ObjectId id, WGPURenderBundle* out) const final {
            if (id == 0) {
                *out = nullptr;
                return WireResult::Success;
            }
            return GetFromId(id, out);
        }
        WireResult GetFromId(ObjectId id, WGPURenderPipeline* out) const final {
            return std::get<KnownObjects<WGPURenderPipeline>>(mKnown).GetNativeHandle(id, out);
        }

        WireResult GetOptionalFromId(ObjectId id, WGPURenderPipeline* out) const final {
            if (id == 0) {
                *out = nullptr;
                return WireResult::Success;
            }
            return GetFromId(id, out);
        }
        WireResult GetFromId(ObjectId id, WGPUResourceTable* out) const final {
            return std::get<KnownObjects<WGPUResourceTable>>(mKnown).GetNativeHandle(id, out);
        }

        WireResult GetOptionalFromId(ObjectId id, WGPUResourceTable* out) const final {
            if (id == 0) {
                *out = nullptr;
                return WireResult::Success;
            }
            return GetFromId(id, out);
        }
        WireResult GetFromId(ObjectId id, WGPUSampler* out) const final {
            return std::get<KnownObjects<WGPUSampler>>(mKnown).GetNativeHandle(id, out);
        }

        WireResult GetOptionalFromId(ObjectId id, WGPUSampler* out) const final {
            if (id == 0) {
                *out = nullptr;
                return WireResult::Success;
            }
            return GetFromId(id, out);
        }
        WireResult GetFromId(ObjectId id, WGPUShaderModule* out) const final {
            return std::get<KnownObjects<WGPUShaderModule>>(mKnown).GetNativeHandle(id, out);
        }

        WireResult GetOptionalFromId(ObjectId id, WGPUShaderModule* out) const final {
            if (id == 0) {
                *out = nullptr;
                return WireResult::Success;
            }
            return GetFromId(id, out);
        }
        WireResult GetFromId(ObjectId id, WGPUSurface* out) const final {
            return std::get<KnownObjects<WGPUSurface>>(mKnown).GetNativeHandle(id, out);
        }

        WireResult GetOptionalFromId(ObjectId id, WGPUSurface* out) const final {
            if (id == 0) {
                *out = nullptr;
                return WireResult::Success;
            }
            return GetFromId(id, out);
        }
        WireResult GetFromId(ObjectId id, WGPUTexelBufferView* out) const final {
            return std::get<KnownObjects<WGPUTexelBufferView>>(mKnown).GetNativeHandle(id, out);
        }

        WireResult GetOptionalFromId(ObjectId id, WGPUTexelBufferView* out) const final {
            if (id == 0) {
                *out = nullptr;
                return WireResult::Success;
            }
            return GetFromId(id, out);
        }
        WireResult GetFromId(ObjectId id, WGPUTexture* out) const final {
            return std::get<KnownObjects<WGPUTexture>>(mKnown).GetNativeHandle(id, out);
        }

        WireResult GetOptionalFromId(ObjectId id, WGPUTexture* out) const final {
            if (id == 0) {
                *out = nullptr;
                return WireResult::Success;
            }
            return GetFromId(id, out);
        }
        WireResult GetFromId(ObjectId id, WGPUTextureView* out) const final {
            return std::get<KnownObjects<WGPUTextureView>>(mKnown).GetNativeHandle(id, out);
        }

        WireResult GetOptionalFromId(ObjectId id, WGPUTextureView* out) const final {
            if (id == 0) {
                *out = nullptr;
                return WireResult::Success;
            }
            return GetFromId(id, out);
        }
        WireResult GetFromId(ObjectId id, WGPUComputePassEncoder* out) const final {
            return std::get<KnownObjects<WGPUComputePassEncoder>>(mKnown).GetNativeHandle(id, out);
        }

        WireResult GetOptionalFromId(ObjectId id, WGPUComputePassEncoder* out) const final {
            if (id == 0) {
                *out = nullptr;
                return WireResult::Success;
            }
            return GetFromId(id, out);
        }
        WireResult GetFromId(ObjectId id, WGPURenderBundleEncoder* out) const final {
            return std::get<KnownObjects<WGPURenderBundleEncoder>>(mKnown).GetNativeHandle(id, out);
        }

        WireResult GetOptionalFromId(ObjectId id, WGPURenderBundleEncoder* out) const final {
            if (id == 0) {
                *out = nullptr;
                return WireResult::Success;
            }
            return GetFromId(id, out);
        }
        WireResult GetFromId(ObjectId id, WGPURenderPassEncoder* out) const final {
            return std::get<KnownObjects<WGPURenderPassEncoder>>(mKnown).GetNativeHandle(id, out);
        }

        WireResult GetOptionalFromId(ObjectId id, WGPURenderPassEncoder* out) const final {
            if (id == 0) {
                *out = nullptr;
                return WireResult::Success;
            }
            return GetFromId(id, out);
        }

      protected:
        // Proc table may be used by children as well.
        std::shared_ptr<const DawnProcTable> mProcs;

        // Template functions that implement helpers on KnownObjects.
        template <typename T>
        WireResult Get(ObjectId id, Reserved<T>* result) {
            return std::get<KnownObjects<T>>(mKnown).Get(id, result);
        }
        template <typename T>
        WireResult Get(ObjectId id, Known<T>* result) {
            return std::get<KnownObjects<T>>(mKnown).Get(id, result);
        }
        template <typename T>
        WireResult FillReservation(ObjectHandle handle, T nativeHandle, Known<T>* known = nullptr) {
            auto result = std::get<KnownObjects<T>>(mKnown).FillReservation(handle, nativeHandle, known);
            if (result == WireResult::FatalError) {
                Release(nativeHandle);
            }
            return result;
        }
        template <typename T>
        WireResult Allocate(Reserved<T>* result,
                            ObjectHandle handle,
                            AllocationState state = AllocationState::Allocated) {
            // Allocations always take the lock because |vector::push_back| may be called which
            // can invalidate pointers.
            auto serverGuard = GetGuard();
            return std::get<KnownObjects<T>>(mKnown).Allocate(result, handle, state);
        }
        template <typename T>
        WireResult Free(ObjectId id, ObjectData<T>* data) {
            // Free always take the lock to ensure that any callback handlers (which always take
            // the lock), i.e. On*Callback functions, cannot race with object deletion.
            auto serverGuard = GetGuard();
            Reserved<T> obj;
            WIRE_TRY(Get(id, &obj));
            *data = std::move(*obj.data);
            std::get<KnownObjects<T>>(mKnown).Free(id);
            return WireResult::Success;
        }

        // Special functions that are currently needed.
        bool IsDeviceKnown(WGPUDevice device) const {
            return std::get<KnownObjects<WGPUDevice>>(mKnown).IsKnown(device);
        }
        std::vector<WGPUDevice> GetAllDeviceHandles() const {
            return std::get<KnownObjects<WGPUDevice>>(mKnown).GetAllHandles();
        }
        std::vector<WGPUInstance> GetAllInstanceHandles() const {
            return std::get<KnownObjects<WGPUInstance>>(mKnown).GetAllHandles();
        }

        template <typename T>
        void Release(T handle) {
            ((*mProcs).*WGPUTraits<T>::Release)(handle);
        }
        void DestroyAllObjects() {
            {
                std::vector<WGPUDevice> handles =
                    std::get<KnownObjects<WGPUDevice>>(mKnown).AcquireAllHandles();
                for (WGPUDevice handle : handles) {
                    Release(handle);
                }
            }
            {
                std::vector<WGPUAdapter> handles =
                    std::get<KnownObjects<WGPUAdapter>>(mKnown).AcquireAllHandles();
                for (WGPUAdapter handle : handles) {
                    Release(handle);
                }
            }
            {
                std::vector<WGPUBindGroup> handles =
                    std::get<KnownObjects<WGPUBindGroup>>(mKnown).AcquireAllHandles();
                for (WGPUBindGroup handle : handles) {
                    Release(handle);
                }
            }
            {
                std::vector<WGPUBindGroupLayout> handles =
                    std::get<KnownObjects<WGPUBindGroupLayout>>(mKnown).AcquireAllHandles();
                for (WGPUBindGroupLayout handle : handles) {
                    Release(handle);
                }
            }
            {
                std::vector<WGPUBuffer> handles =
                    std::get<KnownObjects<WGPUBuffer>>(mKnown).AcquireAllHandles();
                for (WGPUBuffer handle : handles) {
                    Release(handle);
                }
            }
            {
                std::vector<WGPUCommandBuffer> handles =
                    std::get<KnownObjects<WGPUCommandBuffer>>(mKnown).AcquireAllHandles();
                for (WGPUCommandBuffer handle : handles) {
                    Release(handle);
                }
            }
            {
                std::vector<WGPUCommandEncoder> handles =
                    std::get<KnownObjects<WGPUCommandEncoder>>(mKnown).AcquireAllHandles();
                for (WGPUCommandEncoder handle : handles) {
                    Release(handle);
                }
            }
            {
                std::vector<WGPUComputePipeline> handles =
                    std::get<KnownObjects<WGPUComputePipeline>>(mKnown).AcquireAllHandles();
                for (WGPUComputePipeline handle : handles) {
                    Release(handle);
                }
            }
            {
                std::vector<WGPUExternalTexture> handles =
                    std::get<KnownObjects<WGPUExternalTexture>>(mKnown).AcquireAllHandles();
                for (WGPUExternalTexture handle : handles) {
                    Release(handle);
                }
            }
            {
                std::vector<WGPUInstance> handles =
                    std::get<KnownObjects<WGPUInstance>>(mKnown).AcquireAllHandles();
                for (WGPUInstance handle : handles) {
                    Release(handle);
                }
            }
            {
                std::vector<WGPUPipelineLayout> handles =
                    std::get<KnownObjects<WGPUPipelineLayout>>(mKnown).AcquireAllHandles();
                for (WGPUPipelineLayout handle : handles) {
                    Release(handle);
                }
            }
            {
                std::vector<WGPUQuerySet> handles =
                    std::get<KnownObjects<WGPUQuerySet>>(mKnown).AcquireAllHandles();
                for (WGPUQuerySet handle : handles) {
                    Release(handle);
                }
            }
            {
                std::vector<WGPUQueue> handles =
                    std::get<KnownObjects<WGPUQueue>>(mKnown).AcquireAllHandles();
                for (WGPUQueue handle : handles) {
                    Release(handle);
                }
            }
            {
                std::vector<WGPURenderBundle> handles =
                    std::get<KnownObjects<WGPURenderBundle>>(mKnown).AcquireAllHandles();
                for (WGPURenderBundle handle : handles) {
                    Release(handle);
                }
            }
            {
                std::vector<WGPURenderPipeline> handles =
                    std::get<KnownObjects<WGPURenderPipeline>>(mKnown).AcquireAllHandles();
                for (WGPURenderPipeline handle : handles) {
                    Release(handle);
                }
            }
            {
                std::vector<WGPUResourceTable> handles =
                    std::get<KnownObjects<WGPUResourceTable>>(mKnown).AcquireAllHandles();
                for (WGPUResourceTable handle : handles) {
                    Release(handle);
                }
            }
            {
                std::vector<WGPUSampler> handles =
                    std::get<KnownObjects<WGPUSampler>>(mKnown).AcquireAllHandles();
                for (WGPUSampler handle : handles) {
                    Release(handle);
                }
            }
            {
                std::vector<WGPUShaderModule> handles =
                    std::get<KnownObjects<WGPUShaderModule>>(mKnown).AcquireAllHandles();
                for (WGPUShaderModule handle : handles) {
                    Release(handle);
                }
            }
            {
                std::vector<WGPUSurface> handles =
                    std::get<KnownObjects<WGPUSurface>>(mKnown).AcquireAllHandles();
                for (WGPUSurface handle : handles) {
                    Release(handle);
                }
            }
            {
                std::vector<WGPUTexelBufferView> handles =
                    std::get<KnownObjects<WGPUTexelBufferView>>(mKnown).AcquireAllHandles();
                for (WGPUTexelBufferView handle : handles) {
                    Release(handle);
                }
            }
            {
                std::vector<WGPUTexture> handles =
                    std::get<KnownObjects<WGPUTexture>>(mKnown).AcquireAllHandles();
                for (WGPUTexture handle : handles) {
                    Release(handle);
                }
            }
            {
                std::vector<WGPUTextureView> handles =
                    std::get<KnownObjects<WGPUTextureView>>(mKnown).AcquireAllHandles();
                for (WGPUTextureView handle : handles) {
                    Release(handle);
                }
            }
            {
                std::vector<WGPUComputePassEncoder> handles =
                    std::get<KnownObjects<WGPUComputePassEncoder>>(mKnown).AcquireAllHandles();
                for (WGPUComputePassEncoder handle : handles) {
                    Release(handle);
                }
            }
            {
                std::vector<WGPURenderBundleEncoder> handles =
                    std::get<KnownObjects<WGPURenderBundleEncoder>>(mKnown).AcquireAllHandles();
                for (WGPURenderBundleEncoder handle : handles) {
                    Release(handle);
                }
            }
            {
                std::vector<WGPURenderPassEncoder> handles =
                    std::get<KnownObjects<WGPURenderPassEncoder>>(mKnown).AcquireAllHandles();
                for (WGPURenderPassEncoder handle : handles) {
                    Release(handle);
                }
            }
        }

      private:
        // The list of known IDs for each object type.
        // We use an explicit Mutex to protect these lists instead of MutexProtected because:
        //   1) It allows us to return AutoLock objects to hold the lock across function scopes.
        //   2) It allows us to use |mKnown| when we know we can access it without the lock.
        // To clarify the threading model of the KnownObjects, there is always a "main" thread on
        // the server which is responsible for flushing commands from the client. This thread is
        // the only thread that ever calls |HandleCommandsImpl| which ultimately calls into the
        // command handlers in the generated ServerHandlers.cpp. All other threads that can
        // interact with |mKnown| are async or spontaneous callback threads which always hold the
        // lock for the full duration of their call, see |ForwardToServerHelper| where we force all
        // callbacks to take the lock. On the "main" thread, we only acquire the lock whenever we
        // |Allocate| or |Free|. We need to lock w.r.t other threads for |Allocate| because
        // internally, that can cause a |std::vector::push_back| call which may invalidate all
        // pointers. As a result, we cannot have another thread doing anything during that window.
        // Similarly, for |Free|, we may release the WGPU* backing handle which is also not allowed
        // to race. Reads on the "main" thread are not explicitly synchronized because callbacks
        // are not allowed to allocate or free objects which guarantees that the reads on the
        // "main" thread will be pointer stable. Furthermore, it is actually necessary that we
        // don't hold the lock for reads on the "main" thread because APIs like |Device::Destroy|
        // can cause us to wait on callbacks which as stated above, require the lock to complete.
        // So if we were to hold the lock on the "main" thread during the read and execution of
        // |Device::Destroy|, we could deadlock when we try waiting for callbacks which can't
        // acquire this lock.
        // TODO(https://crbug.com/412761856): Revisit this logic and consider using a rw-lock
        // in conjunction with taking additional refs to "special" objects/function pairs that can
        // cause callbacks to run when called. This would make this logic less specific to the way
        // that the Dawn tests and Chromium uses the wire.
        Mutex mMutex;
        std::tuple<
            KnownObjects<WGPUAdapter>, 
            KnownObjects<WGPUBindGroup>, 
            KnownObjects<WGPUBindGroupLayout>, 
            KnownObjects<WGPUBuffer>, 
            KnownObjects<WGPUCommandBuffer>, 
            KnownObjects<WGPUCommandEncoder>, 
            KnownObjects<WGPUComputePipeline>, 
            KnownObjects<WGPUDevice>, 
            KnownObjects<WGPUExternalTexture>, 
            KnownObjects<WGPUInstance>, 
            KnownObjects<WGPUPipelineLayout>, 
            KnownObjects<WGPUQuerySet>, 
            KnownObjects<WGPUQueue>, 
            KnownObjects<WGPURenderBundle>, 
            KnownObjects<WGPURenderPipeline>, 
            KnownObjects<WGPUResourceTable>, 
            KnownObjects<WGPUSampler>, 
            KnownObjects<WGPUShaderModule>, 
            KnownObjects<WGPUSurface>, 
            KnownObjects<WGPUTexelBufferView>, 
            KnownObjects<WGPUTexture>, 
            KnownObjects<WGPUTextureView>, 
            KnownObjects<WGPUComputePassEncoder>, 
            KnownObjects<WGPURenderBundleEncoder>, 
            KnownObjects<WGPURenderPassEncoder>
        > mKnown;
    };

}  // namespace dawn::wire::server

#endif  // DAWNWIRE_SERVER_SERVERBASE_AUTOGEN_H_
