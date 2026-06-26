// do@Redlive

#include "opengl-backend.h"

namespace cutie::opengl
{

Framebuffer::Framebuffer(const GLContext& context) : m_Context(context) {}

Framebuffer::~Framebuffer()
{
    if (fbo) glDeleteFramebuffers(1, &fbo);
}

Object Framebuffer::getNativeObject(ObjectType objectType) { return Object(fbo); }

void Framebuffer::bind()
{
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
}

} // namespace cutie::opengl
