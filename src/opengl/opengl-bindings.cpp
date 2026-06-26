// do@Redlive

#include "opengl-backend.h"

namespace cutie::opengl
{

BindingLayout::BindingLayout(const GLContext& context) : m_Context(context) {}
BindingLayout::~BindingLayout() {}

BindingSet::BindingSet(const GLContext& context) : m_Context(context) {}
BindingSet::~BindingSet() {}

DescriptorTable::DescriptorTable(const GLContext& context) : m_Context(context) {}
DescriptorTable::~DescriptorTable() {}

} // namespace cutie::opengl
