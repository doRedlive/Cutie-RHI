// do@Redlive

#include "opengl-backend.h"

namespace cutie::opengl
{

// Format conversion table: Cutie Format -> GL base format (for glTexImage2D format param)
int convertFormat(cutie::Format format)
{
    switch (format)
    {
    case Format::R8_UINT:           return GL_RED;
    case Format::R8_SINT:           return GL_RED;
    case Format::R8_UNORM:          return GL_RED;
    case Format::R8_SNORM:          return GL_RED;
    case Format::RG8_UINT:          return GL_RG;
    case Format::RG8_SINT:          return GL_RG;
    case Format::RG8_UNORM:         return GL_RG;
    case Format::RG8_SNORM:         return GL_RG;
    case Format::R16_UINT:          return GL_RED;
    case Format::R16_SINT:          return GL_RED;
    case Format::R16_UNORM:         return GL_RED;
    case Format::R16_SNORM:         return GL_RED;
    case Format::R16_FLOAT:         return GL_RED;
    case Format::BGRA4_UNORM:       return GL_RGBA;
    case Format::B5G6R5_UNORM:      return GL_RGB;
    case Format::B5G5R5A1_UNORM:    return GL_RGBA;
    case Format::RGBA8_UINT:        return GL_RGBA;
    case Format::RGBA8_SINT:        return GL_RGBA;
    case Format::RGBA8_UNORM:       return GL_RGBA;
    case Format::RGBA8_SNORM:       return GL_RGBA;
    case Format::BGRA8_UNORM:       return GL_BGRA;
    case Format::BGRX8_UNORM:       return GL_BGRA;
    case Format::SRGBA8_UNORM:      return GL_RGBA;
    case Format::SBGRA8_UNORM:      return GL_BGRA;
    case Format::SBGRX8_UNORM:      return GL_BGRA;
    case Format::R10G10B10A2_UNORM: return GL_RGBA;
    case Format::R11G11B10_FLOAT:   return GL_RGB;
    case Format::RG16_UINT:         return GL_RG;
    case Format::RG16_SINT:         return GL_RG;
    case Format::RG16_UNORM:        return GL_RG;
    case Format::RG16_SNORM:        return GL_RG;
    case Format::RG16_FLOAT:        return GL_RG;
    case Format::R32_UINT:          return GL_RED;
    case Format::R32_SINT:          return GL_RED;
    case Format::R32_FLOAT:         return GL_RED;
    case Format::RGBA16_UINT:       return GL_RGBA;
    case Format::RGBA16_SINT:       return GL_RGBA;
    case Format::RGBA16_UNORM:      return GL_RGBA;
    case Format::RGBA16_SNORM:      return GL_RGBA;
    case Format::RGBA16_FLOAT:      return GL_RGBA;
    case Format::RG32_UINT:         return GL_RG;
    case Format::RG32_SINT:         return GL_RG;
    case Format::RG32_FLOAT:        return GL_RG;
    case Format::RGB32_UINT:        return GL_RGB;
    case Format::RGB32_SINT:        return GL_RGB;
    case Format::RGB32_FLOAT:       return GL_RGB;
    case Format::RGBA32_UINT:       return GL_RGBA;
    case Format::RGBA32_SINT:       return GL_RGBA;
    case Format::RGBA32_FLOAT:      return GL_RGBA;
    case Format::D16:               return GL_DEPTH_COMPONENT;
    case Format::D24S8:             return GL_DEPTH_STENCIL;
    case Format::X24G8_UINT:        return GL_DEPTH_STENCIL;
    case Format::D32:               return GL_DEPTH_COMPONENT;
    case Format::D32S8:             return GL_DEPTH_STENCIL;
    case Format::X32G8_UINT:        return GL_DEPTH_STENCIL;
    case Format::BC1_UNORM:         return GL_COMPRESSED_RGBA_S3TC_DXT1_EXT;
    case Format::BC1_UNORM_SRGB:    return GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT1_EXT;
    case Format::BC2_UNORM:         return GL_COMPRESSED_RGBA_S3TC_DXT3_EXT;
    case Format::BC2_UNORM_SRGB:    return GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT3_EXT;
    case Format::BC3_UNORM:         return GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
    case Format::BC3_UNORM_SRGB:    return GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT5_EXT;
    case Format::BC4_UNORM:         return GL_COMPRESSED_RED_RGTC1;
    case Format::BC4_SNORM:         return GL_COMPRESSED_SIGNED_RED_RGTC1;
    case Format::BC5_UNORM:         return GL_COMPRESSED_RG_RGTC2;
    case Format::BC5_SNORM:         return GL_COMPRESSED_SIGNED_RG_RGTC2;
    case Format::BC6H_UFLOAT:       return GL_COMPRESSED_RGB_BPTC_UNSIGNED_FLOAT;
    case Format::BC6H_SFLOAT:       return GL_COMPRESSED_RGB_BPTC_SIGNED_FLOAT;
    case Format::BC7_UNORM:         return GL_COMPRESSED_RGBA_BPTC_UNORM;
    case Format::BC7_UNORM_SRGB:    return GL_COMPRESSED_SRGB_ALPHA_BPTC_UNORM;
    default:                        return 0;
    }
}

// Format conversion table: Cutie Format -> GL internal format
int convertFormatInternal(cutie::Format format)
{
    switch (format)
    {
    case Format::R8_UINT:           return GL_R8UI;
    case Format::R8_SINT:           return GL_R8I;
    case Format::R8_UNORM:          return GL_R8;
    case Format::R8_SNORM:          return GL_R8_SNORM;
    case Format::RG8_UINT:          return GL_RG8UI;
    case Format::RG8_SINT:          return GL_RG8I;
    case Format::RG8_UNORM:         return GL_RG8;
    case Format::RG8_SNORM:         return GL_RG8_SNORM;
    case Format::R16_UINT:          return GL_R16UI;
    case Format::R16_SINT:          return GL_R16I;
    case Format::R16_UNORM:         return GL_R16;
    case Format::R16_SNORM:         return GL_R16_SNORM;
    case Format::R16_FLOAT:         return GL_R16F;
    case Format::BGRA4_UNORM:       return GL_RGBA4;
    case Format::B5G6R5_UNORM:      return GL_RGB565;
    case Format::B5G5R5A1_UNORM:    return GL_RGB5_A1;
    case Format::RGBA8_UINT:        return GL_RGBA8UI;
    case Format::RGBA8_SINT:        return GL_RGBA8I;
    case Format::RGBA8_UNORM:       return GL_RGBA8;
    case Format::RGBA8_SNORM:       return GL_RGBA8_SNORM;
    case Format::BGRA8_UNORM:       return GL_RGBA8;
    case Format::BGRX8_UNORM:       return GL_RGBA8;
    case Format::SRGBA8_UNORM:      return GL_SRGB8_ALPHA8;
    case Format::SBGRA8_UNORM:      return GL_SRGB8_ALPHA8;
    case Format::SBGRX8_UNORM:      return GL_SRGB8_ALPHA8;
    case Format::R10G10B10A2_UNORM: return GL_RGB10_A2;
    case Format::R11G11B10_FLOAT:   return GL_R11F_G11F_B10F;
    case Format::RG16_UINT:         return GL_RG16UI;
    case Format::RG16_SINT:         return GL_RG16I;
    case Format::RG16_UNORM:        return GL_RG16;
    case Format::RG16_SNORM:        return GL_RG16_SNORM;
    case Format::RG16_FLOAT:        return GL_RG16F;
    case Format::R32_UINT:          return GL_R32UI;
    case Format::R32_SINT:          return GL_R32I;
    case Format::R32_FLOAT:         return GL_R32F;
    case Format::RGBA16_UINT:       return GL_RGBA16UI;
    case Format::RGBA16_SINT:       return GL_RGBA16I;
    case Format::RGBA16_UNORM:      return GL_RGBA16;
    case Format::RGBA16_SNORM:      return GL_RGBA16_SNORM;
    case Format::RGBA16_FLOAT:      return GL_RGBA16F;
    case Format::RG32_UINT:         return GL_RG32UI;
    case Format::RG32_SINT:         return GL_RG32I;
    case Format::RG32_FLOAT:        return GL_RG32F;
    case Format::RGB32_UINT:        return GL_RGB32UI;
    case Format::RGB32_SINT:        return GL_RGB32I;
    case Format::RGB32_FLOAT:       return GL_RGB32F;
    case Format::RGBA32_UINT:       return GL_RGBA32UI;
    case Format::RGBA32_SINT:       return GL_RGBA32I;
    case Format::RGBA32_FLOAT:      return GL_RGBA32F;
    case Format::D16:               return GL_DEPTH_COMPONENT16;
    case Format::D24S8:             return GL_DEPTH24_STENCIL8;
    case Format::X24G8_UINT:        return GL_DEPTH24_STENCIL8;
    case Format::D32:               return GL_DEPTH_COMPONENT32F;
    case Format::D32S8:             return GL_DEPTH32F_STENCIL8;
    case Format::X32G8_UINT:        return GL_DEPTH32F_STENCIL8;
    case Format::BC1_UNORM:         return GL_COMPRESSED_RGBA_S3TC_DXT1_EXT;
    case Format::BC1_UNORM_SRGB:    return GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT1_EXT;
    case Format::BC2_UNORM:         return GL_COMPRESSED_RGBA_S3TC_DXT3_EXT;
    case Format::BC2_UNORM_SRGB:    return GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT3_EXT;
    case Format::BC3_UNORM:         return GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
    case Format::BC3_UNORM_SRGB:    return GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT5_EXT;
    case Format::BC4_UNORM:         return GL_COMPRESSED_RED_RGTC1;
    case Format::BC4_SNORM:         return GL_COMPRESSED_SIGNED_RED_RGTC1;
    case Format::BC5_UNORM:         return GL_COMPRESSED_RG_RGTC2;
    case Format::BC5_SNORM:         return GL_COMPRESSED_SIGNED_RG_RGTC2;
    case Format::BC6H_UFLOAT:       return GL_COMPRESSED_RGB_BPTC_UNSIGNED_FLOAT;
    case Format::BC6H_SFLOAT:       return GL_COMPRESSED_RGB_BPTC_SIGNED_FLOAT;
    case Format::BC7_UNORM:         return GL_COMPRESSED_RGBA_BPTC_UNORM;
    case Format::BC7_UNORM_SRGB:    return GL_COMPRESSED_SRGB_ALPHA_BPTC_UNORM;
    default:                        return 0;
    }
}

// Format conversion: Cutie Format -> GL data type
int convertFormatType(cutie::Format format)
{
    switch (format)
    {
    case Format::R8_UINT:
    case Format::RG8_UINT:
    case Format::RGBA8_UINT:
        return GL_UNSIGNED_BYTE;

    case Format::R8_SINT:
    case Format::RG8_SINT:
    case Format::RGBA8_SINT:
    case Format::R8_SNORM:
    case Format::RG8_SNORM:
    case Format::RGBA8_SNORM:
        return GL_BYTE;

    case Format::R8_UNORM:
    case Format::RG8_UNORM:
    case Format::RGBA8_UNORM:
    case Format::BGRA8_UNORM:
    case Format::BGRX8_UNORM:
    case Format::SRGBA8_UNORM:
    case Format::SBGRA8_UNORM:
    case Format::SBGRX8_UNORM:
        return GL_UNSIGNED_BYTE;

    case Format::R16_UINT:
    case Format::RG16_UINT:
    case Format::RGBA16_UINT:
        return GL_UNSIGNED_SHORT;

    case Format::R16_SINT:
    case Format::RG16_SINT:
    case Format::RGBA16_SINT:
        return GL_SHORT;

    case Format::R16_UNORM:
    case Format::RG16_UNORM:
    case Format::RGBA16_UNORM:
        return GL_UNSIGNED_SHORT;

    case Format::R16_SNORM:
    case Format::RG16_SNORM:
    case Format::RGBA16_SNORM:
        return GL_SHORT;

    case Format::R16_FLOAT:
    case Format::RG16_FLOAT:
    case Format::RGBA16_FLOAT:
        return GL_HALF_FLOAT;

    case Format::R32_UINT:
    case Format::RG32_UINT:
    case Format::RGB32_UINT:
    case Format::RGBA32_UINT:
        return GL_UNSIGNED_INT;

    case Format::R32_SINT:
    case Format::RG32_SINT:
    case Format::RGB32_SINT:
    case Format::RGBA32_SINT:
        return GL_INT;

    case Format::R32_FLOAT:
    case Format::RG32_FLOAT:
    case Format::RGB32_FLOAT:
    case Format::RGBA32_FLOAT:
        return GL_FLOAT;

    case Format::R10G10B10A2_UNORM:
        return GL_UNSIGNED_INT_10_10_10_2;

    case Format::R11G11B10_FLOAT:
        return GL_UNSIGNED_INT_10F_11F_11F_REV;

    case Format::BGRA4_UNORM:
        return GL_UNSIGNED_SHORT_4_4_4_4;

    case Format::B5G6R5_UNORM:
        return GL_UNSIGNED_SHORT_5_6_5;

    case Format::B5G5R5A1_UNORM:
        return GL_UNSIGNED_SHORT_5_5_5_1;

    case Format::D16:
        return GL_UNSIGNED_SHORT;

    case Format::D24S8:
    case Format::X24G8_UINT:
        return GL_UNSIGNED_INT_24_8;

    case Format::D32:
    case Format::D32S8:
    case Format::X32G8_UINT:
        return GL_FLOAT;

    default:
        return GL_UNSIGNED_BYTE;
    }
}

} // namespace cutie::opengl
