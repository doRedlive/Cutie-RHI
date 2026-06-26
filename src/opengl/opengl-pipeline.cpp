// do@Redlive

#include "opengl-backend.h"

namespace cutie::opengl
{

GraphicsPipeline::GraphicsPipeline(const GLContext& context) : m_Context(context) {}

GraphicsPipeline::~GraphicsPipeline()
{
    if (program) glDeleteProgram(program);
}

Object GraphicsPipeline::getNativeObject(ObjectType objectType) { return Object(program); }

ComputePipeline::ComputePipeline(const GLContext& context) : m_Context(context) {}

ComputePipeline::~ComputePipeline()
{
    if (program) glDeleteProgram(program);
}

Object ComputePipeline::getNativeObject(ObjectType objectType) { return Object(program); }

} // namespace cutie::opengl
