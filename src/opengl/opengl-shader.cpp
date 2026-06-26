// do@Redlive

#include "opengl-backend.h"

namespace cutie::opengl
{

Shader::Shader(const GLContext& context) : m_Context(context) {}

Shader::~Shader()
{
    if (shaderObject) glDeleteShader(shaderObject);
}

void Shader::getBytecode(const void** ppBytecode, size_t* pSize) const
{
    if (ppBytecode) *ppBytecode = bytecode.data();
    if (pSize) *pSize = bytecode.size();
}

Object Shader::getNativeObject(ObjectType objectType) { return nullptr; }

bool Shader::compileGLSL() { return false; }

GLenum Shader::getGLShaderType() const
{
    switch (desc.shaderType)
    {
    case ShaderType::Vertex:   return GL_VERTEX_SHADER;
    case ShaderType::Hull:     return GL_TESS_CONTROL_SHADER;
    case ShaderType::Domain:   return GL_TESS_EVALUATION_SHADER;
    case ShaderType::Geometry: return GL_GEOMETRY_SHADER;
    case ShaderType::Pixel:    return GL_FRAGMENT_SHADER;
    case ShaderType::Compute:  return GL_COMPUTE_SHADER;
    default: return 0;
    }
}

ShaderLibrary::ShaderLibrary(const GLContext& context) : m_Context(context) {}

ShaderLibrary::~ShaderLibrary() {}

void ShaderLibrary::getBytecode(const void** ppBytecode, size_t* pSize) const
{
    if (ppBytecode) *ppBytecode = bytecode.data();
    if (pSize) *pSize = bytecode.size();
}

ShaderHandle ShaderLibrary::getShader(const char* entryName, ShaderType shaderType)
{
    auto it = entryMap.find(entryName);
    return (it != entryMap.end()) ? shaders[it->second] : nullptr;
}

} // namespace cutie::opengl
