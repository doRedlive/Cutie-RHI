// do@Redlive

#include "opengl-backend.h"

namespace cutie::opengl
{

InputLayout::InputLayout(const GLContext& context) : m_Context(context) {}
InputLayout::~InputLayout()
{
    if (vao) glDeleteVertexArrays(1, &vao);
}

const VertexAttributeDesc* InputLayout::getAttributeDesc(uint32_t index) const
{
    return (index < attributes.size()) ? &attributes[index] : nullptr;
}

} // namespace cutie::opengl
