// do@Redlive

#include "opengl-backend.h"

namespace cutie::opengl
{

Sampler::Sampler(const GLContext& context) : m_Context(context) {}
Sampler::~Sampler()
{
    if (sampler) glDeleteSamplers(1, &sampler);
}

} // namespace cutie::opengl
