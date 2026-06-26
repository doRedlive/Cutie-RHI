// do@Redlive

#include "opengl-backend.h"

namespace cutie::opengl
{

Buffer::Buffer(const GLContext& context) : m_Context(context) {}

Buffer::~Buffer()
{
    if (buffer) glDeleteBuffers(1, &buffer);
}

Object Buffer::getNativeObject(ObjectType objectType)
{
    if (objectType == ObjectTypes::Cutie_GL_Device)
        return Object(buffer);
    return nullptr;
}

} // namespace cutie::opengl
