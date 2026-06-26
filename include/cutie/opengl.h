// do@Redlive

#pragma once

#include <glad/gl.h>
#include <cutie/cutie.h>

namespace cutie::ObjectTypes
{
    constexpr ObjectType Cutie_GL_Device = 0x00040101;
};

namespace cutie::opengl
{
    // GLAD loader function type
    typedef void* (*GLloaderFunc)(const char* name);

    struct DeviceDesc
    {
        IMessageCallback* messageCallback = nullptr;
        bool aftermathEnabled = false;

        // Required: GL function loader (e.g. glfwGetProcAddress, SDL_GL_GetProcAddress)
        GLloaderFunc glLoaderFunc = nullptr;

        // GL version override (0 = auto-detect from current context)
        int glMajorVersion = 0;
        int glMinorVersion = 0;
    };

    CUTIE_API DeviceHandle createDevice(const DeviceDesc& desc);

    CUTIE_API int convertFormat(cutie::Format format);
    CUTIE_API int convertFormatInternal(cutie::Format format);
    CUTIE_API int convertFormatType(cutie::Format format);
}
