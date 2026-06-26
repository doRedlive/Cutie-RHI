// do@Redlive

#pragma once

#include <cutie/opengl.h>
#include <cutie/common/misc.h>
#include <cutie/common/containers.h>

#include <cstring>
#include <string>
#include <vector>
#include <unordered_map>
#include <atomic>

namespace cutie::opengl
{
    class Device;
    class CommandList;
    class Buffer;
    class Texture;
    class StagingTexture;
    class Shader;
    class ShaderLibrary;
    class Sampler;
    class InputLayout;
    class Framebuffer;
    class GraphicsPipeline;
    class ComputePipeline;
    class BindingLayout;
    class BindingSet;
    class DescriptorTable;
    class Heap;
    class EventQuery;
    class TimerQuery;
    class CommandListLifetimeTracker;

    struct GLContext
    {
        DeviceDesc desc;
        IMessageCallback* messageCallback = nullptr;

        int glMajorVersion = 0;
        int glMinorVersion = 0;

        bool supportsComputeShaders = false;
        bool supportsSSBO = false;
        bool supportsTextureStorage = false;
        bool supportsMultiDrawIndirect = false;
        bool supportsDrawIndirectCount = false;
        bool supportsBaseInstance = false;
        bool supportsCopyImageSubData = false;
        bool supportsImageLoadStore = false;
        bool supportsDebugMarkers = false;
        bool supportsTimerQueries = false;

        uint32_t maxTextureSize = 0;
        uint32_t maxArrayTextureLayers = 0;
        uint32_t maxUniformBlockSize = 0;
        uint32_t maxShaderStorageBlockSize = 0;
        uint32_t maxComputeWorkGroupInvocations = 0;
        uint32_t maxComputeWorkGroupCount[3] = {};
        uint32_t maxComputeWorkGroupSize[3] = {};

        void error(const std::string& message) const
        {
            if (messageCallback)
                messageCallback->message(MessageSeverity::Error, message.c_str());
        }

        void warning(const std::string& message) const
        {
            if (messageCallback)
                messageCallback->message(MessageSeverity::Warning, message.c_str());
        }

        void info(const std::string& message) const
        {
            if (messageCallback)
                messageCallback->message(MessageSeverity::Info, message.c_str());
        }
    };

    class Buffer : public RefCounter<IBuffer>
    {
    public:
        BufferDesc desc;
        GLuint buffer = 0;
        GLenum target = GL_ARRAY_BUFFER;
        void* mappedMemory = nullptr;

        explicit Buffer(const GLContext& context);
        ~Buffer() override;

        const BufferDesc& getDesc() const override { return desc; }
        GpuVirtualAddress getGpuVirtualAddress() const override { return 0; }
        Object getNativeObject(ObjectType objectType) override;

    private:
        const GLContext& m_Context;
    };

    class Texture : public RefCounter<ITexture>
    {
    public:
        TextureDesc desc;
        GLuint texture = 0;
        GLenum target = GL_TEXTURE_2D;
        GLenum internalFormat = GL_RGBA8;
        GLenum format = GL_RGBA;
        GLenum type = GL_UNSIGNED_BYTE;

        explicit Texture(const GLContext& context);
        ~Texture() override;

        const TextureDesc& getDesc() const override { return desc; }
        Object getNativeObject(ObjectType objectType) override;
        Object getNativeView(ObjectType objectType, Format format = Format::UNKNOWN,
            TextureSubresourceSet subresources = TextureSubresourceSet::AllSubresources,
            TextureDimension dimension = TextureDimension::Unknown, bool isReadOnlyDSV = false) override;

    private:
        const GLContext& m_Context;
    };

    class StagingTexture : public RefCounter<IStagingTexture>
    {
    public:
        TextureDesc desc;
        BufferHandle stagingBuffer;

        explicit StagingTexture(const GLContext& context);
        ~StagingTexture() override;

        const TextureDesc& getDesc() const override { return desc; }

    private:
        const GLContext& m_Context;
    };

    class Shader : public RefCounter<IShader>
    {
    public:
        ShaderDesc desc;
        std::vector<uint8_t> bytecode;
        std::string glslSource;
        GLuint shaderObject = 0;
        bool compiled = false;

        explicit Shader(const GLContext& context);
        ~Shader() override;

        const ShaderDesc& getDesc() const override { return desc; }
        void getBytecode(const void** ppBytecode, size_t* pSize) const override;
        Object getNativeObject(ObjectType objectType) override;

        bool compileGLSL();
        GLenum getGLShaderType() const;

    private:
        const GLContext& m_Context;
    };

    class ShaderLibrary : public RefCounter<IShaderLibrary>
    {
    public:
        std::vector<uint8_t> bytecode;
        std::vector<ShaderHandle> shaders;
        std::unordered_map<std::string, size_t> entryMap;

        explicit ShaderLibrary(const GLContext& context);
        ~ShaderLibrary() override;

        void getBytecode(const void** ppBytecode, size_t* pSize) const override;
        ShaderHandle getShader(const char* entryName, ShaderType shaderType) override;

    private:
        const GLContext& m_Context;
    };

    class Sampler : public RefCounter<ISampler>
    {
    public:
        SamplerDesc desc;
        GLuint sampler = 0;

        explicit Sampler(const GLContext& context);
        ~Sampler() override;

        const SamplerDesc& getDesc() const override { return desc; }

    private:
        const GLContext& m_Context;
    };

    class InputLayout : public RefCounter<IInputLayout>
    {
    public:
        std::vector<VertexAttributeDesc> attributes;
        GLuint vao = 0;

        explicit InputLayout(const GLContext& context);
        ~InputLayout() override;

        uint32_t getNumAttributes() const override { return uint32_t(attributes.size()); }
        const VertexAttributeDesc* getAttributeDesc(uint32_t index) const override;

    private:
        const GLContext& m_Context;
    };

    class Framebuffer : public RefCounter<IFramebuffer>
    {
    public:
        FramebufferDesc desc;
        FramebufferInfoEx framebufferInfo;
        GLuint fbo = 0;

        explicit Framebuffer(const GLContext& context);
        ~Framebuffer() override;

        const FramebufferDesc& getDesc() const override { return desc; }
        const FramebufferInfoEx& getFramebufferInfo() const override { return framebufferInfo; }
        Object getNativeObject(ObjectType objectType) override;

        void bind();

    private:
        const GLContext& m_Context;
    };

    struct GLBindingMapping
    {
        uint32_t registerSpace;
        uint32_t slot;
        ResourceType resourceType;
        GLint location;
        GLuint textureUnit;
        GLuint imageUnit;
    };

    class GraphicsPipeline : public RefCounter<IGraphicsPipeline>
    {
    public:
        GraphicsPipelineDesc desc;
        FramebufferInfo framebufferInfo;
        GLuint program = 0;
        std::vector<GLBindingMapping> bindingMap;

        BlendState blendState;
        DepthStencilState depthStencilState;
        RasterState rasterState;
        PrimitiveType primitiveType = PrimitiveType::TriangleList;

        explicit GraphicsPipeline(const GLContext& context);
        ~GraphicsPipeline() override;

        const GraphicsPipelineDesc& getDesc() const override { return desc; }
        const FramebufferInfo& getFramebufferInfo() const override { return framebufferInfo; }
        Object getNativeObject(ObjectType objectType) override;

    private:
        const GLContext& m_Context;
    };

    class ComputePipeline : public RefCounter<IComputePipeline>
    {
    public:
        ComputePipelineDesc desc;
        GLuint program = 0;
        std::vector<GLBindingMapping> bindingMap;

        explicit ComputePipeline(const GLContext& context);
        ~ComputePipeline() override;

        const ComputePipelineDesc& getDesc() const override { return desc; }
        Object getNativeObject(ObjectType objectType) override;

    private:
        const GLContext& m_Context;
    };

    class BindingLayout : public RefCounter<IBindingLayout>
    {
    public:
        BindingLayoutDesc desc;

        explicit BindingLayout(const GLContext& context);
        ~BindingLayout() override;

        const BindingLayoutDesc* getDesc() const override { return &desc; }
        const BindlessLayoutDesc* getBindlessDesc() const override { return nullptr; }

    private:
        const GLContext& m_Context;
    };

    class BindingSet : public RefCounter<IBindingSet>
    {
    public:
        BindingSetDesc desc;
        RefCountPtr<IBindingLayout> layout;

        explicit BindingSet(const GLContext& context);
        ~BindingSet() override;

        const BindingSetDesc* getDesc() const override { return &desc; }
        IBindingLayout* getLayout() const override { return layout; }

    private:
        const GLContext& m_Context;
    };

    class DescriptorTable : public RefCounter<IDescriptorTable>
    {
    public:
        explicit DescriptorTable(const GLContext& context);
        ~DescriptorTable() override;

        const BindingSetDesc* getDesc() const override { return nullptr; }
        IBindingLayout* getLayout() const override { return nullptr; }
        uint32_t getCapacity() const override { return 0; }
        uint32_t getFirstDescriptorIndexInHeap() const override { return ~0u; }

    private:
        const GLContext& m_Context;
    };

    class Heap : public RefCounter<IHeap>
    {
    public:
        HeapDesc desc;

        explicit Heap(const GLContext& context);
        ~Heap() override;

        const HeapDesc& getDesc() const override { return desc; }

    private:
        const GLContext& m_Context;
    };

    class EventQuery : public RefCounter<IEventQuery>
    {
    public:
        GLsync fence = nullptr;
        bool started = false;
        bool resolved = false;

        explicit EventQuery(const GLContext& context);
        ~EventQuery() override;

    private:
        const GLContext& m_Context;
    };

    class TimerQuery : public RefCounter<ITimerQuery>
    {
    public:
        GLuint queryBegin = 0;
        GLuint queryEnd = 0;
        bool started = false;
        bool resolved = false;
        float elapsedTime = 0.0f;

        explicit TimerQuery(const GLContext& context);
        ~TimerQuery() override;

    private:
        const GLContext& m_Context;
    };

    class CommandListLifetimeTracker : public RefCounter<ICommandListLifetimeTracker>
    {
    public:
        std::vector<CommandListHandle> trackedCommandLists;

        explicit CommandListLifetimeTracker(const GLContext& context);
        ~CommandListLifetimeTracker() override;

        void runGarbageCollection() override;

    private:
        const GLContext& m_Context;
    };

    class CommandList : public RefCounter<ICommandList>
    {
    public:
        explicit CommandList(const GLContext& context, IDevice* device, const CommandListParameters& params);
        ~CommandList() override;

        Object getNativeObject(ObjectType objectType) override;

        void open() override;
        void close() override;
        void clearState() override;

        void clearTextureFloat(ITexture* texture, const TextureSubresourceSet& subresources, const Color& clearColor) override;
        void clearDepthStencilTexture(ITexture* texture, const TextureSubresourceSet& subresources, bool clearDepth, float depth, bool clearStencil, uint8_t stencil) override;
        void clearTextureUInt(ITexture* texture, const TextureSubresourceSet& subresources, uint32_t clearValue) override;

        void copyTexture(ITexture* dst, const TextureSlice& dstSlice, ITexture* src, const TextureSlice& srcSlice) override;
        void copyTexture(ITexture* dst, const TextureSlice& dstSlice, IStagingTexture* src, const TextureSlice& srcSlice) override;
        void copyTexture(IStagingTexture* dst, const TextureSlice& dstSlice, ITexture* src, const TextureSlice& srcSlice) override;
        void writeTexture(ITexture* texture, uint32_t arraySlice, uint32_t mipLevel, const void* data, size_t rowPitch = 0, size_t depthPitch = 0) override;
        void resolveTexture(ITexture* dst, const TextureSubresourceSet& dstSubresources, ITexture* src, const TextureSubresourceSet& srcSubresources) override;

        void writeBuffer(IBuffer* buffer, const void* data, size_t dataSize, uint64_t destOffsetBytes = 0) override;
        void copyBuffer(IBuffer* dst, uint64_t dstOffset, IBuffer* src, uint64_t srcOffset, uint64_t numBytes) override;
        void clearBufferUInt(IBuffer* buffer, uint32_t clearValue) override;

        void setPushConstants(const void* data, size_t byteSize) override;

        void setGraphicsState(const GraphicsState& state) override;
        void setComputeState(const ComputeState& state) override;

        void draw(const DrawArguments& args) override;
        void drawIndexed(const DrawArguments& args) override;
        void drawIndirect(uint32_t offsetBytes, uint32_t drawCount) override;
        void drawIndexedIndirect(uint32_t offsetBytes, uint32_t drawCount) override;
        void drawIndexedIndirectCount(uint64_t argBufferOffset, uint64_t countBufferOffset, uint32_t maxDrawCount) override;

        void dispatch(uint32_t groupsX, uint32_t groupsY, uint32_t groupsZ) override;
        void dispatchIndirect(uint32_t offsetBytes) override;

        void beginTimerQuery(ITimerQuery* query) override;
        void endTimerQuery(ITimerQuery* query) override;

        void beginMarker(const char* name) override;
        void endMarker() override;

        void setEnableAutomaticBarriers(bool enable) override;
        void setResourceStatesForBindingSet(IBindingSet* bindingSet) override;
        void setEnableUavBarriersForTexture(ITexture* texture, bool enableBarriers) override;
        void setEnableUavBarriersForBuffer(IBuffer* buffer, bool enableBarriers) override;
        void beginTrackingTextureState(ITexture* texture, const TextureSubresourceSet& subresources, ResourceStates stateBits) override;
        void beginTrackingBufferState(IBuffer* buffer, ResourceStates stateBits) override;
        void setTextureState(ITexture* texture, const TextureSubresourceSet& subresources, ResourceStates stateBits) override;
        void setBufferState(IBuffer* buffer, ResourceStates stateBits) override;
        void setAccelStructState(rt::IAccelStruct* as, ResourceStates stateBits) override;
        void setPermanentTextureState(ITexture* texture, ResourceStates stateBits) override;
        void setPermanentBufferState(IBuffer* buffer, ResourceStates stateBits) override;
        void commitBarriers() override;
        ResourceStates getTextureSubresourceState(ITexture* texture, uint32_t arraySlice, uint32_t mipLevel) override;
        ResourceStates getBufferState(IBuffer* buffer) override;

        IDevice* getDevice() override { return m_Device; }
        const CommandListParameters& getDesc() override { return m_Desc; }

        void clearSamplerFeedbackTexture(ISamplerFeedbackTexture* texture) override {}
        void decodeSamplerFeedbackTexture(IBuffer* buffer, ISamplerFeedbackTexture* texture, Format format) override {}
        void setMeshletState(const MeshletState& state) override {}
        void setRayTracingState(const rt::State& state) override {}
        void dispatchMesh(uint32_t groupsX, uint32_t groupsY, uint32_t groupsZ) override {}
        void dispatchMeshIndirect(uint32_t offsetBytes, uint32_t maxDrawCount) override {}
        void dispatchMeshIndirectCount(uint64_t argBufferOffset, uint64_t countBufferOffset, uint32_t maxDrawCount) override {}
        void dispatchRays(const rt::DispatchRaysArguments& args) override {}
        void buildOpacityMicromap(rt::IOpacityMicromap* omm, const rt::OpacityMicromapDesc& desc) override {}
        void buildBottomLevelAccelStruct(rt::IAccelStruct* as, const rt::GeometryDesc* pGeometries, uint32_t numGeometries, rt::AccelStructBuildFlags buildFlags = rt::AccelStructBuildFlags::PreferFastTrace) override {}
        void compactBottomLevelAccelStructs() override {}
        void copyRaytracingAccelerationStructure(rt::IAccelStruct* dst, rt::IAccelStruct* src) override {}
        void buildTopLevelAccelStruct(rt::IAccelStruct* as, const rt::InstanceDesc* pInstances, uint32_t numInstances, rt::AccelStructBuildFlags buildFlags = rt::AccelStructBuildFlags::PreferFastTrace) override {}
        void buildTopLevelAccelStructFromBuffer(rt::IAccelStruct* as, IBuffer* instanceBuffer, uint64_t offset, uint32_t numInstances, rt::AccelStructBuildFlags buildFlags = rt::AccelStructBuildFlags::PreferFastTrace) override {}
        void executeMultiIndirectClusterOperation(const rt::cluster::OperationDesc& desc) override {}
        void convertCoopVecMatrices(const coopvec::ConvertMatrixLayoutDesc* pDescs, size_t numDescs) override {}
        void setSamplerFeedbackTextureState(ISamplerFeedbackTexture* texture, ResourceStates stateBits) override {}

    private:
        const GLContext& m_Context;
        IDevice* m_Device;
        CommandListParameters m_Desc;
        bool m_Open = false;

        GraphicsState m_CurrentGraphicsState;
        ComputeState m_CurrentComputeState;
        FramebufferHandle m_CurrentFramebuffer;
        GraphicsPipelineHandle m_CurrentGraphicsPipeline;
        ComputePipelineHandle m_CurrentComputePipeline;

        void applyGraphicsBindings(const GraphicsState& state);
        void applyComputeBindings(const ComputeState& state);
        void applyBlendState(const BlendState& state);
        void applyDepthStencilState(const DepthStencilState& state);
        void applyRasterState(const RasterState& state);
        GLenum convertPrimitiveType(PrimitiveType primType);
        GLenum convertIndexFormat(Format format);
    };

    class Device : public RefCounter<IDevice>
    {
    public:
        explicit Device(const DeviceDesc& desc);
        ~Device() override;

        GraphicsAPI getGraphicsAPI() const override { return GraphicsAPI::OPENGL; }

        HeapHandle createHeap(const HeapDesc& desc) override;

        TextureHandle createTexture(const TextureDesc& desc) override;
        MemoryRequirements getTextureMemoryRequirements(ITexture* texture) override;
        bool bindTextureMemory(ITexture* texture, IHeap* heap, uint64_t offset) override;
        TextureHandle createHandleForNativeTexture(ObjectType objectType, Object nativeTexture, const TextureDesc& desc) override;
        StagingTextureHandle createStagingTexture(const TextureDesc& desc, CpuAccessMode accessMode) override;
        void* mapStagingTexture(IStagingTexture* stagingTexture, const TextureSlice& slice, CpuAccessMode accessMode, size_t* outRowPitch) override;
        void unmapStagingTexture(IStagingTexture* stagingTexture) override;
        SamplerFeedbackTextureHandle createSamplerFeedbackTexture(ITexture* pairedTexture, const SamplerFeedbackTextureDesc& desc) override { return nullptr; }
        SamplerFeedbackTextureHandle createSamplerFeedbackForNativeTexture(ObjectType objectType, Object nativeResource, const SamplerFeedbackTextureDesc& desc) override { return nullptr; }
        void getTextureTiling(ITexture* texture, TextureTiling* pOut) override;
        void updateTextureTileMappings(ITexture* texture, const TextureTilesMapping* pMappings, size_t numMappings) override;

        BufferHandle createBuffer(const BufferDesc& desc) override;
        MemoryRequirements getBufferMemoryRequirements(IBuffer* buffer) override;
        bool bindBufferMemory(IBuffer* buffer, IHeap* heap, uint64_t offset) override;
        BufferHandle createHandleForNativeBuffer(ObjectType objectType, Object nativeBuffer, const BufferDesc& desc) override;
        void* mapBuffer(IBuffer* buffer, CpuAccessMode accessMode) override;
        void unmapBuffer(IBuffer* buffer) override;

        ShaderHandle createShader(const ShaderDesc& desc, const void* binary, size_t binarySize) override;
        ShaderHandle createShaderSpecialization(IShader* baseShader, const ShaderSpecialization* pConstants, uint32_t numConstants) override;
        ShaderLibraryHandle createShaderLibrary(const void* binary, size_t binarySize) override;
        SamplerHandle createSampler(const SamplerDesc& desc) override;
        InputLayoutHandle createInputLayout(const VertexAttributeDesc* pAttributes, uint32_t attributeCount, IShader* vertexShader) override;

        EventQueryHandle createEventQuery() override;
        void setEventQuery(IEventQuery* query, CommandQueue queue) override;
        bool pollEventQuery(IEventQuery* query) override;
        void waitEventQuery(IEventQuery* query) override;
        void resetEventQuery(IEventQuery* query) override;
        TimerQueryHandle createTimerQuery() override;
        bool pollTimerQuery(ITimerQuery* query) override;
        float getTimerQueryTime(ITimerQuery* query) override;
        void resetTimerQuery(ITimerQuery* query) override;

        FramebufferHandle createFramebuffer(const FramebufferDesc& desc) override;
        GraphicsPipelineHandle createGraphicsPipeline(const GraphicsPipelineDesc& desc, const FramebufferInfo& framebufferInfo) override;
        GraphicsPipelineHandle createGraphicsPipeline(const GraphicsPipelineDesc& desc, IFramebuffer* framebuffer) override;
        ComputePipelineHandle createComputePipeline(const ComputePipelineDesc& desc) override;
        MeshletPipelineHandle createMeshletPipeline(const MeshletPipelineDesc& desc, const FramebufferInfo& framebufferInfo) override { return nullptr; }
        MeshletPipelineHandle createMeshletPipeline(const MeshletPipelineDesc& desc, IFramebuffer* framebuffer) override { return nullptr; }
        rt::PipelineHandle createRayTracingPipeline(const rt::PipelineDesc& desc) override { return nullptr; }

        BindingLayoutHandle createBindingLayout(const BindingLayoutDesc& desc) override;
        BindingLayoutHandle createBindlessLayout(const BindlessLayoutDesc& desc) override { return nullptr; }
        BindingSetHandle createBindingSet(const BindingSetDesc& desc, IBindingLayout* layout) override;
        DescriptorTableHandle createDescriptorTable(IBindingLayout* layout) override;
        bool resizeDescriptorTable(IDescriptorTable* descriptorTable, uint32_t newSize, bool keepContents = true) override { return false; }
        bool writeDescriptorTable(IDescriptorTable* descriptorTable, const BindingSetItem& item) override { return false; }

        rt::OpacityMicromapHandle createOpacityMicromap(const rt::OpacityMicromapDesc& desc) override { return nullptr; }
        rt::AccelStructHandle createAccelStruct(const rt::AccelStructDesc& desc) override { return nullptr; }
        MemoryRequirements getAccelStructMemoryRequirements(rt::IAccelStruct* as) override { return { 0, 0 }; }
        rt::cluster::OperationSizeInfo getClusterOperationSizeInfo(const rt::cluster::OperationParams& params) override { return { 0, 0, 0 }; }
        bool bindAccelStructMemory(rt::IAccelStruct* as, IHeap* heap, uint64_t offset) override { return false; }

        CommandListHandle createCommandList(const CommandListParameters& params = CommandListParameters()) override;
        CommandListLifetimeTrackerHandle createCommandListLifetimeTracker(CommandQueue queue = CommandQueue::Graphics) override;
        uint64_t executeCommandLists(ICommandList* const* pCommandLists, uint32_t numCommandLists, CommandQueue executionQueue = CommandQueue::Graphics) override;
        void queueWaitForCommandList(CommandQueue waitQueue, CommandQueue executionQueue, uint64_t instance) override {}
        bool waitForIdle() override;
        void runGarbageCollection() override {}

        bool queryFeatureSupport(Feature feature, void* pInfo = nullptr, size_t infoSize = 0) override;
        FormatSupport queryFormatSupport(Format format) override;
        coopvec::DeviceFeatures queryCoopVecFeatures() override { return {}; }
        coopvec::MatMulFormatSupport queryCoopVecMatMulFormatSupport(const coopvec::MatMulFormatCombo& combo) override { return {}; }
        coopvec::TrainingFormatSupport queryCoopVecTrainingFormatSupport(coopvec::DataType type) override { return {}; }
        size_t getCoopVecMatrixSize(coopvec::DataType type, coopvec::MatrixLayout layout, uint32_t rows, uint32_t columns) override { return 0; }

        Object getNativeQueue(ObjectType objectType, CommandQueue queue) override { return nullptr; }
        IMessageCallback* getMessageCallback() override { return m_Context.messageCallback; }
        bool isAftermathEnabled() const override { return false; }
        AftermathCrashDumpHelper& getAftermathCrashDumpHelper() override { return m_AftermathCrashDumpHelper; }

    private:
        GLContext m_Context;
        CommandListHandle m_ImmediateCommandList;
        AftermathCrashDumpHelper m_AftermathCrashDumpHelper;

        void queryGLCapabilities();
    };

} // namespace cutie::opengl
