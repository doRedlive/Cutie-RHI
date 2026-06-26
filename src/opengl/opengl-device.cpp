// do@Redlive

#include "opengl-backend.h"
#include <cutie/utils.h>
#include <cstring>
#include <cstdlib>

namespace cutie::opengl
{

Device::Device(const DeviceDesc& desc)
{
    m_Context.desc = desc;
    m_Context.messageCallback = desc.messageCallback;

    if (!desc.glLoaderFunc)
        return;

    int gladVersion = gladLoadGL((GLADloadfunc)desc.glLoaderFunc);
    if (!gladVersion)
        return;

    queryGLCapabilities();

    CommandListParameters clParams;
    clParams.enableImmediateExecution = true;
    clParams.queueType = CommandQueue::Graphics;
    m_ImmediateCommandList = CommandListHandle::Create(new CommandList(m_Context, this, clParams));
}

Device::~Device()
{
    m_ImmediateCommandList = nullptr;
}

void Device::queryGLCapabilities()
{
    auto& ctx = m_Context;

    ctx.glMajorVersion = GLVersion.major;
    ctx.glMinorVersion = GLVersion.minor;

    if (ctx.desc.glMajorVersion > 0)
    {
        ctx.glMajorVersion = ctx.desc.glMajorVersion;
        ctx.glMinorVersion = ctx.desc.glMinorVersion;
    }

    ctx.supportsComputeShaders = GLAD_GL_VERSION_4_3 || GLAD_GL_ARB_compute_shader;
    ctx.supportsSSBO = GLAD_GL_VERSION_4_3 || GLAD_GL_ARB_shader_storage_buffer_object;
    ctx.supportsTextureStorage = GLAD_GL_VERSION_4_2 || GLAD_GL_ARB_texture_storage;
    ctx.supportsMultiDrawIndirect = GLAD_GL_VERSION_4_3 || GLAD_GL_ARB_multi_draw_indirect;
    ctx.supportsDrawIndirectCount = GLAD_GL_VERSION_4_6 || GLAD_GL_ARB_indirect_parameters;
    ctx.supportsBaseInstance = GLAD_GL_VERSION_4_2 || GLAD_GL_ARB_base_instance;
    ctx.supportsCopyImageSubData = GLAD_GL_VERSION_4_3 || GLAD_GL_ARB_copy_image;
    ctx.supportsImageLoadStore = GLAD_GL_VERSION_4_2 || GLAD_GL_ARB_shader_image_load_store;
    ctx.supportsDebugMarkers = GLAD_GL_VERSION_4_3 || GLAD_GL_KHR_debug;
    ctx.supportsTimerQueries = GLAD_GL_VERSION_3_3 || GLAD_GL_ARB_timer_query;

    glGetIntegerv(GL_MAX_TEXTURE_SIZE, (GLint*)&ctx.maxTextureSize);
    glGetIntegerv(GL_MAX_ARRAY_TEXTURE_LAYERS, (GLint*)&ctx.maxArrayTextureLayers);

    if (ctx.supportsSSBO)
    {
        glGetIntegerv(GL_MAX_UNIFORM_BLOCK_SIZE, (GLint*)&ctx.maxUniformBlockSize);
        glGetIntegerv(GL_MAX_SHADER_STORAGE_BLOCK_SIZE, (GLint*)&ctx.maxShaderStorageBlockSize);
    }

    if (ctx.supportsComputeShaders)
    {
        glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 0, (GLint*)&ctx.maxComputeWorkGroupCount[0]);
        glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 1, (GLint*)&ctx.maxComputeWorkGroupCount[1]);
        glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 2, (GLint*)&ctx.maxComputeWorkGroupCount[2]);
        glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 0, (GLint*)&ctx.maxComputeWorkGroupSize[0]);
        glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 1, (GLint*)&ctx.maxComputeWorkGroupSize[1]);
        glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 2, (GLint*)&ctx.maxComputeWorkGroupSize[2]);
        glGetIntegerv(GL_MAX_COMPUTE_WORK_GROUP_INVOCATIONS, (GLint*)&ctx.maxComputeWorkGroupInvocations);
    }
}

HeapHandle Device::createHeap(const HeapDesc& desc)
{
    Heap* heap = new Heap(m_Context);
    heap->desc = desc;
    heap->desc.capacity = 0;
    return HeapHandle::Create(heap);
}

TextureHandle Device::createTexture(const TextureDesc& desc)
{
    Texture* texture = new Texture(m_Context);
    texture->desc = desc;

    switch (desc.dimension)
    {
    case TextureDimension::Texture1D:
        texture->target = desc.arraySize > 1 ? GL_TEXTURE_1D_ARRAY : GL_TEXTURE_1D; break;
    case TextureDimension::Texture2D:
        texture->target = desc.arraySize > 1 ? GL_TEXTURE_2D_ARRAY : GL_TEXTURE_2D; break;
    case TextureDimension::Texture2DMS:
        texture->target = desc.arraySize > 1 ? GL_TEXTURE_2D_MULTISAMPLE_ARRAY : GL_TEXTURE_2D_MULTISAMPLE; break;
    case TextureDimension::TextureCube:
        texture->target = desc.arraySize > 1 ? GL_TEXTURE_CUBE_MAP_ARRAY : GL_TEXTURE_CUBE_MAP; break;
    case TextureDimension::Texture3D:
        texture->target = GL_TEXTURE_3D; break;
    default:
        texture->target = GL_TEXTURE_2D; break;
    }

    texture->internalFormat = convertFormatInternal(desc.format);
    texture->format = convertFormat(desc.format);
    texture->type = convertFormatType(desc.format);

    glGenTextures(1, &texture->texture);

    GLint prevBinding = 0;
    glGetIntegerv(GL_TEXTURE_BINDING_2D, &prevBinding);
    glBindTexture(texture->target, texture->texture);

    glTexParameteri(texture->target, GL_TEXTURE_MIN_FILTER, desc.mipLevels > 1 ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR);
    glTexParameteri(texture->target, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(texture->target, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(texture->target, GL_TEXTURE_WRAP_T, GL_REPEAT);
    if (desc.dimension == TextureDimension::Texture3D)
        glTexParameteri(texture->target, GL_TEXTURE_WRAP_R, GL_REPEAT);

    uint32_t w = std::max(1u, desc.width);
    uint32_t h = std::max(1u, desc.height);
    uint32_t d = std::max(1u, desc.depth);
    uint32_t mips = std::max(1u, desc.mipLevels);
    uint32_t layers = std::max(1u, desc.arraySize);

    if (m_Context.supportsTextureStorage)
    {
        if (desc.dimension == TextureDimension::Texture3D)
            glTexStorage3D(GL_TEXTURE_3D, mips, texture->internalFormat, w, h, d);
        else if (desc.dimension == TextureDimension::Texture2DArray || (desc.dimension == TextureDimension::TextureCube && layers > 6))
            glTexStorage3D(texture->target, mips, texture->internalFormat, w, h, layers);
        else if (desc.dimension == TextureDimension::Texture1DArray)
            glTexStorage2D(GL_TEXTURE_1D_ARRAY, mips, texture->internalFormat, w, layers);
        else if (desc.dimension == TextureDimension::Texture2DMS)
            glTexStorage2DMultisample(texture->target, desc.sampleCount, texture->internalFormat, w, h, GL_TRUE);
        else
            glTexStorage2D(texture->target, mips, texture->internalFormat, w, h);
    }

    glBindTexture(texture->target, prevBinding);
    return TextureHandle::Create(texture);
}

MemoryRequirements Device::getTextureMemoryRequirements(ITexture* texture) { return { 0, 0 }; }
bool Device::bindTextureMemory(ITexture* texture, IHeap* heap, uint64_t offset) { return false; }
TextureHandle Device::createHandleForNativeTexture(ObjectType objectType, Object nativeTexture, const TextureDesc& desc) { return nullptr; }

StagingTextureHandle Device::createStagingTexture(const TextureDesc& desc, CpuAccessMode accessMode)
{
    StagingTexture* staging = new StagingTexture(m_Context);
    staging->desc = desc;

    BufferDesc bufferDesc;
    bufferDesc.byteSize = desc.width * std::max(1u, desc.height) * std::max(1u, desc.depth) * 16;
    bufferDesc.debugName = "StagingBuffer";
    bufferDesc.cpuAccess = accessMode;
    staging->stagingBuffer = createBuffer(bufferDesc);

    return StagingTextureHandle::Create(staging);
}

void* Device::mapStagingTexture(IStagingTexture* stagingTexture, const TextureSlice& slice, CpuAccessMode accessMode, size_t* outRowPitch)
{
    if (!stagingTexture) return nullptr;
    auto* staging = checked_cast<StagingTexture*>(stagingTexture);
    if (staging->stagingBuffer)
    {
        if (outRowPitch) *outRowPitch = staging->desc.width * 4;
        return mapBuffer(staging->stagingBuffer, accessMode);
    }
    return nullptr;
}

void Device::unmapStagingTexture(IStagingTexture* stagingTexture)
{
    if (!stagingTexture) return;
    auto* staging = checked_cast<StagingTexture*>(stagingTexture);
    if (staging->stagingBuffer) unmapBuffer(staging->stagingBuffer);
}

void Device::getTextureTiling(ITexture* texture, TextureTiling* pOut) { if (pOut) memset(pOut, 0, sizeof(TextureTiling)); }
void Device::updateTextureTileMappings(ITexture* texture, const TextureTilesMapping* pMappings, size_t numMappings) {}

BufferHandle Device::createBuffer(const BufferDesc& desc)
{
    Buffer* buffer = new Buffer(m_Context);
    buffer->desc = desc;

    if (desc.isVertexBuffer)
        buffer->target = GL_ARRAY_BUFFER;
    else if (desc.isIndexBuffer)
        buffer->target = GL_ELEMENT_ARRAY_BUFFER;
    else if (desc.isConstantBuffer)
        buffer->target = GL_UNIFORM_BUFFER;
    else if (desc.canHaveUAVs || desc.canHaveTypedViews || desc.canHaveRawViews)
        buffer->target = GL_SHADER_STORAGE_BUFFER;
    else if (desc.isDrawIndirectArgs)
        buffer->target = GL_DRAW_INDIRECT_BUFFER;
    else
        buffer->target = GL_ARRAY_BUFFER;

    glGenBuffers(1, &buffer->buffer);
    glBindBuffer(buffer->target, buffer->buffer);

    GLenum usage = GL_STATIC_DRAW;
    if (desc.cpuAccess == CpuAccessMode::Write) usage = GL_STREAM_DRAW;
    else if (desc.cpuAccess == CpuAccessMode::Read) usage = GL_STREAM_READ;

    glBufferData(buffer->target, desc.byteSize, nullptr, usage);
    glBindBuffer(buffer->target, 0);

    return BufferHandle::Create(buffer);
}

MemoryRequirements Device::getBufferMemoryRequirements(IBuffer* buffer) { return { 0, 0 }; }
bool Device::bindBufferMemory(IBuffer* buffer, IHeap* heap, uint64_t offset) { return false; }
BufferHandle Device::createHandleForNativeBuffer(ObjectType objectType, Object nativeBuffer, const BufferDesc& desc) { return nullptr; }

void* Device::mapBuffer(IBuffer* buffer, CpuAccessMode accessMode)
{
    if (!buffer) return nullptr;
    auto* buf = checked_cast<Buffer*>(buffer);

    glBindBuffer(buf->target, buf->buffer);
    buf->mappedMemory = glMapBufferRange(buf->target, 0, buf->desc.byteSize, GL_MAP_READ_BIT | GL_MAP_WRITE_BIT);
    glBindBuffer(buf->target, 0);

    return buf->mappedMemory;
}

void Device::unmapBuffer(IBuffer* buffer)
{
    if (!buffer) return;
    auto* buf = checked_cast<Buffer*>(buffer);

    glBindBuffer(buf->target, buf->buffer);
    glUnmapBuffer(buf->target);
    glBindBuffer(buf->target, 0);

    buf->mappedMemory = nullptr;
}

ShaderHandle Device::createShader(const ShaderDesc& desc, const void* binary, size_t binarySize)
{
    Shader* shader = new Shader(m_Context);
    shader->desc = desc;
    const uint8_t* bytes = static_cast<const uint8_t*>(binary);
    shader->bytecode.assign(bytes, bytes + binarySize);
    return ShaderHandle::Create(shader);
}

ShaderHandle Device::createShaderSpecialization(IShader* baseShader, const ShaderSpecialization* pConstants, uint32_t numConstants)
{
    if (!baseShader) return nullptr;
    baseShader->AddRef();
    return ShaderHandle::Create(checked_cast<Shader*>(baseShader));
}

ShaderLibraryHandle Device::createShaderLibrary(const void* binary, size_t binarySize)
{
    ShaderLibrary* lib = new ShaderLibrary(m_Context);
    const uint8_t* bytes = static_cast<const uint8_t*>(binary);
    lib->bytecode.assign(bytes, bytes + binarySize);
    return ShaderLibraryHandle::Create(lib);
}

SamplerHandle Device::createSampler(const SamplerDesc& desc)
{
    Sampler* sampler = new Sampler(m_Context);
    sampler->desc = desc;

    glGenSamplers(1, &sampler->sampler);

    auto toAddr = [](SamplerAddressMode m) -> GLint {
        switch (m) {
        case SamplerAddressMode::Wrap:   return GL_REPEAT;
        case SamplerAddressMode::Mirror: return GL_MIRRORED_REPEAT;
        case SamplerAddressMode::Clamp:  return GL_CLAMP_TO_EDGE;
        case SamplerAddressMode::Border: return GL_CLAMP_TO_BORDER;
        default: return GL_REPEAT;
        }
    };

    glSamplerParameteri(sampler->sampler, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glSamplerParameteri(sampler->sampler, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glSamplerParameteri(sampler->sampler, GL_TEXTURE_WRAP_S, toAddr(desc.addressU));
    glSamplerParameteri(sampler->sampler, GL_TEXTURE_WRAP_T, toAddr(desc.addressV));
    glSamplerParameteri(sampler->sampler, GL_TEXTURE_WRAP_R, toAddr(desc.addressW));
    glSamplerParameterf(sampler->sampler, GL_TEXTURE_LOD_BIAS, desc.mipBias);
    if (desc.maxAnisotropy > 1.0f)
        glSamplerParameterf(sampler->sampler, GL_TEXTURE_MAX_ANISOTROPY, desc.maxAnisotropy);
    if (desc.reductionType == SamplerReductionType::Comparison)
        glSamplerParameteri(sampler->sampler, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);

    return SamplerHandle::Create(sampler);
}

InputLayoutHandle Device::createInputLayout(const VertexAttributeDesc* pAttributes, uint32_t attributeCount, IShader* vertexShader)
{
    InputLayout* layout = new InputLayout(m_Context);
    for (uint32_t i = 0; i < attributeCount; i++)
        layout->attributes.push_back(pAttributes[i]);

    glGenVertexArrays(1, &layout->vao);
    return InputLayoutHandle::Create(layout);
}

EventQueryHandle Device::createEventQuery() { return EventQueryHandle::Create(new EventQuery(m_Context)); }

void Device::setEventQuery(IEventQuery* query, CommandQueue queue)
{
    if (!query) return;
    auto* eq = checked_cast<EventQuery*>(query);
    if (eq->fence) glDeleteSync(eq->fence);
    eq->fence = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
    eq->started = true;
    eq->resolved = false;
}

bool Device::pollEventQuery(IEventQuery* query)
{
    if (!query) return true;
    auto* eq = checked_cast<EventQuery*>(query);
    if (!eq->started || !eq->fence) return true;
    GLenum r = glClientWaitSync(eq->fence, 0, 0);
    return (r == GL_ALREADY_SIGNALED || r == GL_CONDITION_SATISFIED);
}

void Device::waitEventQuery(IEventQuery* query)
{
    if (!query) return;
    auto* eq = checked_cast<EventQuery*>(query);
    if (!eq->started || !eq->fence) return;
    glClientWaitSync(eq->fence, GL_SYNC_FLUSH_COMMANDS_BIT, GL_TIMEOUT_IGNORED);
}

void Device::resetEventQuery(IEventQuery* query)
{
    if (!query) return;
    auto* eq = checked_cast<EventQuery*>(query);
    if (eq->fence) { glDeleteSync(eq->fence); eq->fence = nullptr; }
    eq->started = false;
    eq->resolved = false;
}

TimerQueryHandle Device::createTimerQuery() { return TimerQueryHandle::Create(new TimerQuery(m_Context)); }

bool Device::pollTimerQuery(ITimerQuery* query)
{
    if (!query) return true;
    auto* tq = checked_cast<TimerQuery*>(query);
    if (!tq->started || tq->resolved) return true;

    GLint available = 0;
    glGetQueryObjectiv(tq->queryEnd, GL_QUERY_RESULT_AVAILABLE, &available);
    if (available)
    {
        GLuint64 elapsed = 0;
        glGetQueryObjectui64v(tq->queryEnd, GL_QUERY_RESULT, &elapsed);
        tq->elapsedTime = float(elapsed) / 1e9f;
        tq->resolved = true;
        return true;
    }
    return false;
}

float Device::getTimerQueryTime(ITimerQuery* query) { return query ? checked_cast<TimerQuery*>(query)->elapsedTime : 0.0f; }

void Device::resetTimerQuery(ITimerQuery* query)
{
    if (!query) return;
    auto* tq = checked_cast<TimerQuery*>(query);
    tq->started = false;
    tq->resolved = false;
    tq->elapsedTime = 0.0f;
}

FramebufferHandle Device::createFramebuffer(const FramebufferDesc& desc)
{
    Framebuffer* fb = new Framebuffer(m_Context);
    fb->desc = desc;
    fb->framebufferInfo.sampleCount = 1;
    fb->framebufferInfo.sampleQuality = 0;

    glGenFramebuffers(1, &fb->fbo);
    return FramebufferHandle::Create(fb);
}

GraphicsPipelineHandle Device::createGraphicsPipeline(const GraphicsPipelineDesc& desc, const FramebufferInfo& framebufferInfo)
{
    auto* pipeline = new GraphicsPipeline(m_Context);
    pipeline->desc = desc;
    pipeline->framebufferInfo = framebufferInfo;
    pipeline->blendState = desc.renderState.blendState;
    pipeline->depthStencilState = desc.renderState.depthStencilState;
    pipeline->rasterState = desc.renderState.rasterState;
    pipeline->primitiveType = desc.primType;
    return GraphicsPipelineHandle::Create(pipeline);
}

GraphicsPipelineHandle Device::createGraphicsPipeline(const GraphicsPipelineDesc& desc, IFramebuffer* framebuffer)
{
    if (framebuffer)
    {
        const auto& fbInfo = framebuffer->getFramebufferInfo();
        FramebufferInfo info;
        for (uint32_t i = 0; i < fbInfo.colorFormats.size(); i++)
            info.colorFormats.push_back(fbInfo.colorFormats[i]);
        info.depthFormat = fbInfo.depthFormat;
        info.sampleCount = fbInfo.sampleCount;
        info.sampleQuality = fbInfo.sampleQuality;
        return createGraphicsPipeline(desc, info);
    }
    return createGraphicsPipeline(desc, FramebufferInfo());
}

ComputePipelineHandle Device::createComputePipeline(const ComputePipelineDesc& desc)
{
    auto* pipeline = new ComputePipeline(m_Context);
    pipeline->desc = desc;
    return ComputePipelineHandle::Create(pipeline);
}

BindingLayoutHandle Device::createBindingLayout(const BindingLayoutDesc& desc)
{
    auto* layout = new BindingLayout(m_Context);
    layout->desc = desc;
    return BindingLayoutHandle::Create(layout);
}

BindingSetHandle Device::createBindingSet(const BindingSetDesc& desc, IBindingLayout* layout)
{
    auto* set = new BindingSet(m_Context);
    set->desc = desc;
    set->layout = layout;
    return BindingSetHandle::Create(set);
}

DescriptorTableHandle Device::createDescriptorTable(IBindingLayout* layout)
{
    return DescriptorTableHandle::Create(new DescriptorTable(m_Context));
}

CommandListHandle Device::createCommandList(const CommandListParameters& params)
{
    CommandListParameters clParams = params;
    clParams.enableImmediateExecution = true;
    return CommandListHandle::Create(new CommandList(m_Context, this, clParams));
}

CommandListLifetimeTrackerHandle Device::createCommandListLifetimeTracker(CommandQueue queue)
{
    return CommandListLifetimeTrackerHandle::Create(new CommandListLifetimeTracker(m_Context));
}

uint64_t Device::executeCommandLists(ICommandList* const* pCommandLists, uint32_t numCommandLists, CommandQueue executionQueue) { return 1; }
bool Device::waitForIdle() { glFinish(); return true; }

bool Device::queryFeatureSupport(Feature feature, void* pInfo, size_t infoSize)
{
    switch (feature)
    {
    case Feature::ConstantBufferRanges: return true;
    case Feature::WaveLaneCountMinMax:
        if (pInfo && infoSize >= sizeof(WaveLaneCountMinMaxFeatureInfo))
        {
            auto* info = static_cast<WaveLaneCountMinMaxFeatureInfo*>(pInfo);
            info->minWaveLaneCount = 1;
            info->maxWaveLaneCount = 1;
        }
        return false;
    default: return false;
    }
}

FormatSupport Device::queryFormatSupport(Format format)
{
    int glFmt = convertFormat(format);
    if (glFmt == 0) return FormatSupport::None;

    FormatSupport support = FormatSupport::Texture | FormatSupport::ShaderLoad |
        FormatSupport::ShaderSample | FormatSupport::Buffer |
        FormatSupport::VertexBuffer | FormatSupport::IndexBuffer |
        FormatSupport::ShaderUavLoad | FormatSupport::ShaderUavStore;

    switch (glFmt)
    {
    case GL_RGBA: case GL_RGBA8: case GL_RGBA16F: case GL_RGBA32F:
    case GL_RGB:  case GL_RGB8:  case GL_RGB16F:  case GL_RGB32F:
    case GL_RG:   case GL_RG8:   case GL_RG16F:   case GL_RG32F:
    case GL_RED:  case GL_R8:    case GL_R16F:    case GL_R32F:
    case GL_R11F_G11F_B10F: case GL_RGB10_A2: case GL_SRGB8_ALPHA8:
        support = support | FormatSupport::RenderTarget | FormatSupport::Blendable;
        break;
    default: break;
    }

    return support;
}

DeviceHandle createDevice(const DeviceDesc& desc)
{
    return DeviceHandle::Create(new Device(desc));
}

} // namespace cutie::opengl
