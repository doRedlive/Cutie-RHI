// do@Redlive

#include "opengl-backend.h"
#include <cutie/utils.h>

namespace cutie::opengl
{

CommandList::CommandList(const GLContext& context, IDevice* device, const CommandListParameters& params)
    : m_Context(context), m_Device(device), m_Desc(params)
{
}

CommandList::~CommandList() {}

Object CommandList::getNativeObject(ObjectType objectType) { return nullptr; }

void CommandList::open() { m_Open = true; clearState(); }
void CommandList::close() { m_Open = false; }

void CommandList::clearState()
{
    m_CurrentGraphicsState = {};
    m_CurrentComputeState = {};
    m_CurrentFramebuffer = nullptr;
    m_CurrentGraphicsPipeline = nullptr;
    m_CurrentComputePipeline = nullptr;
}

void CommandList::clearTextureFloat(ITexture* texture, const TextureSubresourceSet& subresources, const Color& clearColor)
{
    if (!texture) return;
    auto* tex = checked_cast<Texture*>(texture);

    GLuint tempFBO = 0;
    glGenFramebuffers(1, &tempFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, tempFBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, tex->target, tex->texture, 0);
    glClearColor(clearColor.r, clearColor.g, clearColor.b, clearColor.a);
    glClear(GL_COLOR_BUFFER_BIT);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glDeleteFramebuffers(1, &tempFBO);
}

void CommandList::clearDepthStencilTexture(ITexture* texture, const TextureSubresourceSet& subresources, bool clearDepth, float depth, bool clearStencil, uint8_t stencil)
{
    if (!texture) return;
    auto* tex = checked_cast<Texture*>(texture);

    GLuint tempFBO = 0;
    glGenFramebuffers(1, &tempFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, tempFBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, tex->target, tex->texture, 0);

    GLbitfield mask = 0;
    if (clearDepth) { glClearDepthf(depth); mask |= GL_DEPTH_BUFFER_BIT; }
    if (clearStencil) { glClearStencil(stencil); mask |= GL_STENCIL_BUFFER_BIT; }
    glClear(mask);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glDeleteFramebuffers(1, &tempFBO);
}

void CommandList::clearTextureUInt(ITexture* texture, const TextureSubresourceSet& subresources, uint32_t clearValue) {}

void CommandList::copyTexture(ITexture* dst, const TextureSlice& dstSlice, ITexture* src, const TextureSlice& srcSlice)
{
    if (!dst || !src) return;
    auto* dstTex = checked_cast<Texture*>(dst);
    auto* srcTex = checked_cast<Texture*>(src);

    if (m_Context.supportsCopyImageSubData)
    {
        glCopyImageSubData(srcTex->texture, srcTex->target, srcSlice.mipLevel,
            srcSlice.x, srcSlice.y, srcSlice.z,
            dstTex->texture, dstTex->target, dstSlice.mipLevel,
            dstSlice.x, dstSlice.y, dstSlice.z,
            srcSlice.width, srcSlice.height, srcSlice.depth);
    }
}

void CommandList::copyTexture(ITexture* dst, const TextureSlice& dstSlice, IStagingTexture* src, const TextureSlice& srcSlice) {}
void CommandList::copyTexture(IStagingTexture* dst, const TextureSlice& dstSlice, ITexture* src, const TextureSlice& srcSlice) {}

void CommandList::writeTexture(ITexture* texture, uint32_t arraySlice, uint32_t mipLevel, const void* data, size_t rowPitch, size_t depthPitch)
{
    if (!texture || !data) return;
    auto* tex = checked_cast<Texture*>(texture);

    GLint prevBinding = 0;
    glGetIntegerv(GL_TEXTURE_BINDING_2D, &prevBinding);
    glBindTexture(tex->target, tex->texture);

    switch (tex->desc.dimension)
    {
    case TextureDimension::Texture2D:
        glTexSubImage2D(tex->target, mipLevel, 0, 0, tex->desc.width >> mipLevel,
            tex->desc.height >> mipLevel, tex->format, tex->type, data);
        break;
    case TextureDimension::Texture3D:
        glTexSubImage3D(tex->target, mipLevel, 0, 0, 0, tex->desc.width >> mipLevel,
            tex->desc.height >> mipLevel, tex->desc.depth >> mipLevel, tex->format, tex->type, data);
        break;
    default: break;
    }

    glBindTexture(tex->target, prevBinding);
}

void CommandList::resolveTexture(ITexture* dst, const TextureSubresourceSet& dstSubresources, ITexture* src, const TextureSubresourceSet& srcSubresources) {}

void CommandList::writeBuffer(IBuffer* buffer, const void* data, size_t dataSize, uint64_t destOffsetBytes)
{
    if (!buffer || !data) return;
    auto* buf = checked_cast<Buffer*>(buffer);
    glBindBuffer(buf->target, buf->buffer);
    glBufferSubData(buf->target, (GLintptr)destOffsetBytes, (GLsizeiptr)dataSize, data);
    glBindBuffer(buf->target, 0);
}

void CommandList::copyBuffer(IBuffer* dst, uint64_t dstOffset, IBuffer* src, uint64_t srcOffset, uint64_t numBytes)
{
    if (!dst || !src) return;
    auto* dstBuf = checked_cast<Buffer*>(dst);
    auto* srcBuf = checked_cast<Buffer*>(src);

    glBindBuffer(GL_COPY_READ_BUFFER, srcBuf->buffer);
    glBindBuffer(GL_COPY_WRITE_BUFFER, dstBuf->buffer);
    glCopyBufferSubData(GL_COPY_READ_BUFFER, GL_COPY_WRITE_BUFFER, (GLintptr)srcOffset, (GLintptr)dstOffset, (GLsizeiptr)numBytes);
    glBindBuffer(GL_COPY_READ_BUFFER, 0);
    glBindBuffer(GL_COPY_WRITE_BUFFER, 0);
}

void CommandList::clearBufferUInt(IBuffer* buffer, uint32_t clearValue)
{
    if (!buffer) return;
    auto* buf = checked_cast<Buffer*>(buffer);
    glBindBuffer(buf->target, buf->buffer);

    std::vector<uint32_t> data(buf->desc.byteSize / sizeof(uint32_t), clearValue);
    glBufferSubData(buf->target, 0, buf->desc.byteSize, data.data());
    glBindBuffer(buf->target, 0);
}

void CommandList::setPushConstants(const void* data, size_t byteSize) {}
void CommandList::setGraphicsState(const GraphicsState& state) { m_CurrentGraphicsState = state; }
void CommandList::setComputeState(const ComputeState& state) { m_CurrentComputeState = state; }

GLenum CommandList::convertPrimitiveType(PrimitiveType primType)
{
    switch (primType)
    {
    case PrimitiveType::PointList:     return GL_POINTS;
    case PrimitiveType::LineList:      return GL_LINES;
    case PrimitiveType::LineStrip:     return GL_LINE_STRIP;
    case PrimitiveType::TriangleList:  return GL_TRIANGLES;
    case PrimitiveType::TriangleStrip: return GL_TRIANGLE_STRIP;
    case PrimitiveType::TriangleFan:   return GL_TRIANGLE_FAN;
    case PrimitiveType::PatchList:     return GL_PATCHES;
    default: return GL_TRIANGLES;
    }
}

void CommandList::draw(const DrawArguments& args)
{
    auto* pipeline = checked_cast<GraphicsPipeline*>(m_CurrentGraphicsState.pipeline);
    if (!pipeline) return;
    GLenum mode = convertPrimitiveType(pipeline->primitiveType);

    if (args.instanceCount > 1 && m_Context.supportsBaseInstance)
        glDrawArraysInstancedBaseInstance(mode, args.startVertexLocation, args.vertexCount, args.instanceCount, args.startInstanceLocation);
    else
        glDrawArrays(mode, args.startVertexLocation, args.vertexCount);
}

void CommandList::drawIndexed(const DrawArguments& args)
{
    auto* pipeline = checked_cast<GraphicsPipeline*>(m_CurrentGraphicsState.pipeline);
    if (!pipeline) return;
    GLenum mode = convertPrimitiveType(pipeline->primitiveType);

    if (args.instanceCount > 1 && m_Context.supportsBaseInstance)
    {
        glDrawElementsInstancedBaseVertexBaseInstance(mode, args.vertexCount,
            GL_UNSIGNED_INT, (const void*)(uintptr_t)(args.startIndexLocation * sizeof(uint32_t)),
            args.instanceCount, args.startVertexLocation, args.startInstanceLocation);
    }
    else if (args.instanceCount > 1)
    {
        glDrawElementsInstanced(mode, args.vertexCount, GL_UNSIGNED_INT,
            (const void*)(uintptr_t)(args.startIndexLocation * sizeof(uint32_t)), args.instanceCount);
    }
    else
    {
        glDrawElements(mode, args.vertexCount, GL_UNSIGNED_INT,
            (const void*)(uintptr_t)(args.startIndexLocation * sizeof(uint32_t)));
    }
}

void CommandList::drawIndirect(uint32_t offsetBytes, uint32_t drawCount)
{
    auto* pipeline = checked_cast<GraphicsPipeline*>(m_CurrentGraphicsState.pipeline);
    if (!pipeline) return;
    GLenum mode = convertPrimitiveType(pipeline->primitiveType);

    if (m_Context.supportsMultiDrawIndirect)
        glMultiDrawArraysIndirect(mode, (const void*)(uintptr_t)offsetBytes, drawCount, 0);
}

void CommandList::drawIndexedIndirect(uint32_t offsetBytes, uint32_t drawCount)
{
    auto* pipeline = checked_cast<GraphicsPipeline*>(m_CurrentGraphicsState.pipeline);
    if (!pipeline) return;
    GLenum mode = convertPrimitiveType(pipeline->primitiveType);

    if (m_Context.supportsMultiDrawIndirect)
        glMultiDrawElementsIndirect(mode, GL_UNSIGNED_INT, (const void*)(uintptr_t)offsetBytes, drawCount, 0);
}

void CommandList::drawIndexedIndirectCount(uint64_t argBufferOffset, uint64_t countBufferOffset, uint32_t maxDrawCount)
{
    auto* pipeline = checked_cast<GraphicsPipeline*>(m_CurrentGraphicsState.pipeline);
    if (!pipeline) return;
    GLenum mode = convertPrimitiveType(pipeline->primitiveType);

    if (m_Context.supportsDrawIndirectCount)
        glMultiDrawElementsIndirectCount(mode, GL_UNSIGNED_INT, (const void*)(uintptr_t)argBufferOffset,
            (GLintptr)countBufferOffset, maxDrawCount, 0);
}

void CommandList::dispatch(uint32_t groupsX, uint32_t groupsY, uint32_t groupsZ)
{
    if (m_Context.supportsComputeShaders)
        glDispatchCompute(groupsX, groupsY, groupsZ);
}

void CommandList::dispatchIndirect(uint32_t offsetBytes)
{
    if (m_Context.supportsComputeShaders)
        glDispatchComputeIndirect((GLintptr)offsetBytes);
}

void CommandList::beginTimerQuery(ITimerQuery* query)
{
    if (!query) return;
    auto* tq = checked_cast<TimerQuery*>(query);
    if (!tq->queryBegin) glGenQueries(1, &tq->queryBegin);
    if (!tq->queryEnd) glGenQueries(1, &tq->queryEnd);
    glQueryCounter(tq->queryBegin, GL_TIMESTAMP);
    tq->started = true;
    tq->resolved = false;
}

void CommandList::endTimerQuery(ITimerQuery* query)
{
    if (!query) return;
    auto* tq = checked_cast<TimerQuery*>(query);
    glQueryCounter(tq->queryEnd, GL_TIMESTAMP);
}

void CommandList::beginMarker(const char* name)
{
    if (m_Context.supportsDebugMarkers)
        glPushDebugGroup(GL_DEBUG_SOURCE_APPLICATION, 0, -1, name);
}

void CommandList::endMarker()
{
    if (m_Context.supportsDebugMarkers)
        glPopDebugGroup();
}

void CommandList::setEnableAutomaticBarriers(bool enable) {}
void CommandList::setResourceStatesForBindingSet(IBindingSet* bindingSet) {}
void CommandList::setEnableUavBarriersForTexture(ITexture* texture, bool enableBarriers) {}
void CommandList::setEnableUavBarriersForBuffer(IBuffer* buffer, bool enableBarriers) {}
void CommandList::beginTrackingTextureState(ITexture* texture, const TextureSubresourceSet& subresources, ResourceStates stateBits) {}
void CommandList::beginTrackingBufferState(IBuffer* buffer, ResourceStates stateBits) {}
void CommandList::setTextureState(ITexture* texture, const TextureSubresourceSet& subresources, ResourceStates stateBits) {}
void CommandList::setBufferState(IBuffer* buffer, ResourceStates stateBits) {}
void CommandList::setAccelStructState(rt::IAccelStruct* as, ResourceStates stateBits) {}
void CommandList::setPermanentTextureState(ITexture* texture, ResourceStates stateBits) {}
void CommandList::setPermanentBufferState(IBuffer* buffer, ResourceStates stateBits) {}
void CommandList::commitBarriers() {}
ResourceStates CommandList::getTextureSubresourceState(ITexture* texture, uint32_t arraySlice, uint32_t mipLevel) { return ResourceStates::Common; }
ResourceStates CommandList::getBufferState(IBuffer* buffer) { return ResourceStates::Common; }

void CommandList::applyGraphicsBindings(const GraphicsState& state) {}
void CommandList::applyComputeBindings(const ComputeState& state) {}
void CommandList::applyBlendState(const BlendState& state) {}
void CommandList::applyDepthStencilState(const DepthStencilState& state) {}
void CommandList::applyRasterState(const RasterState& state) {}

GLenum CommandList::convertIndexFormat(Format format)
{
    switch (format)
    {
    case Format::R16_UINT: return GL_UNSIGNED_SHORT;
    default: return GL_UNSIGNED_INT;
    }
}

} // namespace cutie::opengl
