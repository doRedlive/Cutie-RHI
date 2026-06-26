// do@Redlive

#include "opengl-backend.h"

namespace cutie::opengl
{

Texture::Texture(const GLContext& context) : m_Context(context) {}

Texture::~Texture()
{
    if (texture) glDeleteTextures(1, &texture);
}

Object Texture::getNativeObject(ObjectType objectType)
{
    if (objectType == ObjectTypes::Cutie_GL_Device)
        return Object(texture);
    return nullptr;
}

Object Texture::getNativeView(ObjectType objectType, Format format, TextureSubresourceSet subresources, TextureDimension dimension, bool isReadOnlyDSV)
{
    return Object(texture);
}

StagingTexture::StagingTexture(const GLContext& context) : m_Context(context) {}

StagingTexture::~StagingTexture() {}

} // namespace cutie::opengl
