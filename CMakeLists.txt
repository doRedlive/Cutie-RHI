#
# Copyright (c) 2014-2021, NVIDIA CORPORATION. All rights reserved.
#
# Permission is hereby granted, free of charge, to any person obtaining a
# copy of this software and associated documentation files (the "Software"),
# to deal in the Software without restriction, including without limitation
# the rights to use, copy, modify, merge, publish, distribute, sublicense,
# and/or sell copies of the Software, and to permit persons to whom the
# Software is furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
# THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
# FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
# DEALINGS IN THE SOFTWARE.


cmake_minimum_required(VERSION 3.11)

project(cutie VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(CMakeDependentOption)

option(CUTIE_BUILD_SHARED "Build CUTIE as a shared library (DLL or .so)" OFF)
option(CUTIE_INSTALL "Generate install rules for CUTIE" ON)
cmake_dependent_option(CUTIE_INSTALL_EXPORTS "Install CMake exports" OFF "CUTIE_INSTALL" OFF)

option(CUTIE_WITH_VALIDATION "Build CUTIE the validation layer" ON)
option(CUTIE_WITH_VULKAN "Build the CUTIE Vulkan backend" ON)
option(CUTIE_WITH_RTXMU "Use RTXMU for acceleration structure management" OFF)
option(CUTIE_WITH_AFTERMATH "Include Aftermath support (requires NSight Aftermath SDK)" OFF)
option(CUTIE_WITH_OPENGL "Build the CUTIE OpenGL backend" ON)

cmake_dependent_option(CUTIE_WITH_NVAPI "Include NVAPI support (requires NVAPI SDK)" OFF "WIN32" OFF)
cmake_dependent_option(CUTIE_WITH_DX11 "Build the CUTIE D3D11 backend" ON "WIN32" OFF)
cmake_dependent_option(CUTIE_WITH_DX12 "Build the CUTIE D3D12 backend" ON "WIN32" OFF)

if(CUTIE_WITH_VULKAN)
    option(CUTIE_FETCH_VULKAN_HEADERS "Fetch Vulkan-Headers when building the Vulkan backend and no external targets are provided" ON)
    set(CUTIE_VULKAN_HEADERS_GIT_REPOSITORY "https://github.com/KhronosGroup/Vulkan-Headers.git" CACHE STRING "Vulkan-Headers git repository used by CUTIE_FETCH_VULKAN_HEADERS")
    set(CUTIE_VULKAN_HEADERS_GIT_TAG "v1.4.352" CACHE STRING "Vulkan-Headers git tag or commit used by CUTIE_FETCH_VULKAN_HEADERS")
    set(CUTIE_VULKAN_HEADERS_FETCH_DIR "" CACHE STRING "Source directory for fetched Vulkan-Headers; empty uses CMake FetchContent default")
endif()

if(CUTIE_WITH_DX12)
    option(CUTIE_D3D12_WITH_DXR12_OPACITY_MICROMAP "Use D3D12 native Opacity Micromaps from DXR 1.2" OFF)
    option(CUTIE_FETCH_DIRECTX_HEADERS "Fetch DirectX-Headers when building the D3D12 backend and no external targets are provided" ON)
    set(CUTIE_DIRECTX_HEADERS_GIT_REPOSITORY "https://github.com/microsoft/DirectX-Headers.git" CACHE STRING "DirectX-Headers git repository used by CUTIE_FETCH_DIRECTX_HEADERS")
    set(CUTIE_DIRECTX_HEADERS_GIT_TAG "v1.717.0-preview" CACHE STRING "DirectX-Headers git tag or commit used by CUTIE_FETCH_DIRECTX_HEADERS")
    set(CUTIE_DIRECTX_HEADERS_FETCH_DIR "" CACHE STRING "Source directory for fetched DirectX-Headers; empty uses CMake FetchContent default")
endif()

if(CUTIE_WITH_RTXMU)
    set(CUTIE_RTXMU_GIT_REPOSITORY "https://github.com/NVIDIA-RTX/RTXMU.git" CACHE STRING "RTXMU git repository")
    set(CUTIE_RTXMU_GIT_TAG "v1.4" CACHE STRING "RTXMU git tag or commit")
    set(CUTIE_RTXMU_FETCH_DIR "" CACHE STRING "Source directory for fetched RTXMU; empty uses CMake FetchContent default")
endif()

if (CUTIE_BUILD_SHARED)
    set(CMAKE_POSITION_INDEPENDENT_CODE ON)
endif()

if (MSVC)
    string(REGEX REPLACE "/W[0-4]\\s*" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
    string(APPEND CMAKE_CXX_FLAGS " /W4")
    if (NOT (CMAKE_CXX_COMPILER_ID MATCHES "Clang"))
        string(APPEND CMAKE_CXX_FLAGS " /MP")
    endif()
endif()


set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

if(CUTIE_WITH_VULKAN)
    set(cutie_vulkan_headers_target "")

    if(TARGET Vulkan::Headers)
        set(cutie_vulkan_headers_target Vulkan::Headers)
    elseif(TARGET Vulkan-Headers)
        set(cutie_vulkan_headers_target Vulkan-Headers)
    endif()

    if((NOT cutie_vulkan_headers_target) AND CUTIE_FETCH_VULKAN_HEADERS)
        include(FetchContent)

        message(STATUS "Fetching Vulkan-Headers from ${CUTIE_VULKAN_HEADERS_GIT_REPOSITORY}, tag ${CUTIE_VULKAN_HEADERS_GIT_TAG}")

        if(CUTIE_VULKAN_HEADERS_FETCH_DIR)
            FetchContent_Declare(
                vulkan_headers
                GIT_REPOSITORY ${CUTIE_VULKAN_HEADERS_GIT_REPOSITORY}
                GIT_TAG ${CUTIE_VULKAN_HEADERS_GIT_TAG}
                SOURCE_DIR ${CUTIE_VULKAN_HEADERS_FETCH_DIR})
        else()
            FetchContent_Declare(
                vulkan_headers
                GIT_REPOSITORY ${CUTIE_VULKAN_HEADERS_GIT_REPOSITORY}
                GIT_TAG ${CUTIE_VULKAN_HEADERS_GIT_TAG})
        endif()

        FetchContent_MakeAvailable(vulkan_headers)

        if(TARGET Vulkan::Headers)
            set(cutie_vulkan_headers_target Vulkan::Headers)
        elseif(TARGET Vulkan-Headers)
            set(cutie_vulkan_headers_target Vulkan-Headers)
        endif()
    endif()

    if(NOT cutie_vulkan_headers_target)
        message(FATAL_ERROR "CUTIE Vulkan backend requires Vulkan-Headers CMake target (Vulkan::Headers or Vulkan-Headers), or enable CUTIE_FETCH_VULKAN_HEADERS.")
    endif()
endif()

if(CUTIE_WITH_DX12)
    set(cutie_directx_headers_target "")
    set(cutie_directx_guids_target "")

    if(TARGET Microsoft::DirectX-Headers)
        set(cutie_directx_headers_target Microsoft::DirectX-Headers)
    elseif(TARGET DirectX-Headers)
        set(cutie_directx_headers_target DirectX-Headers)
    endif()

    if(TARGET Microsoft::DirectX-Guids)
        set(cutie_directx_guids_target Microsoft::DirectX-Guids)
    elseif(TARGET DirectX-Guids)
        set(cutie_directx_guids_target DirectX-Guids)
    endif()

    if((NOT cutie_directx_headers_target OR NOT cutie_directx_guids_target) AND CUTIE_FETCH_DIRECTX_HEADERS)
        include(FetchContent)

        message(STATUS "Fetching DirectX-Headers from ${CUTIE_DIRECTX_HEADERS_GIT_REPOSITORY}, tag ${CUTIE_DIRECTX_HEADERS_GIT_TAG}")

        if(CUTIE_DIRECTX_HEADERS_FETCH_DIR)
            FetchContent_Declare(
                directx_headers
                GIT_REPOSITORY ${CUTIE_DIRECTX_HEADERS_GIT_REPOSITORY}
                GIT_TAG ${CUTIE_DIRECTX_HEADERS_GIT_TAG}
                SOURCE_DIR ${CUTIE_DIRECTX_HEADERS_FETCH_DIR})
        else()
            FetchContent_Declare(
                directx_headers
                GIT_REPOSITORY ${CUTIE_DIRECTX_HEADERS_GIT_REPOSITORY}
                GIT_TAG ${CUTIE_DIRECTX_HEADERS_GIT_TAG})
        endif()

        FetchContent_MakeAvailable(directx_headers)

        if(TARGET Microsoft::DirectX-Headers)
            set(cutie_directx_headers_target Microsoft::DirectX-Headers)
        endif()

        if(TARGET Microsoft::DirectX-Guids)
            set(cutie_directx_guids_target Microsoft::DirectX-Guids)
        endif()
    endif()

    if(NOT cutie_directx_headers_target OR NOT cutie_directx_guids_target)
        message(FATAL_ERROR "CUTIE D3D12 requires DirectX-Headers and DirectX-Guids CMake targets. Provide Microsoft::DirectX-Headers and Microsoft::DirectX-Guids, or enable CUTIE_FETCH_DIRECTX_HEADERS.")
    endif()
endif()

if (CUTIE_WITH_RTXMU)
    get_target_property(RTXMU_VULKAN_INCLUDE_DIR ${cutie_vulkan_headers_target} INTERFACE_INCLUDE_DIRECTORIES)
    
    option(RTXMU_WITH_VULKAN "" ${CUTIE_WITH_VULKAN})
    option(RTXMU_WITH_D3D12 "" ${CUTIE_WITH_DX12})

    message(STATUS "Fetching RTXMU from ${CUTIE_RTXMU_GIT_REPOSITORY}, tag ${CUTIE_RTXMU_GIT_TAG}")

    if(CUTIE_RTXMU_FETCH_DIR)
        FetchContent_Declare(
            rtxmu
            GIT_REPOSITORY ${CUTIE_RTXMU_GIT_REPOSITORY}
            GIT_TAG ${CUTIE_RTXMU_GIT_TAG}
            SOURCE_DIR ${CUTIE_RTXMU_FETCH_DIR})
    else()
        FetchContent_Declare(
            rtxmu
            GIT_REPOSITORY ${CUTIE_RTXMU_GIT_REPOSITORY}
            GIT_TAG ${CUTIE_RTXMU_GIT_TAG})
    endif()

    FetchContent_MakeAvailable(rtxmu)
endif()

if (CUTIE_WITH_NVAPI AND NOT TARGET nvapi)
    find_package(NVAPI REQUIRED)

    if (NVAPI_FOUND)
        add_library(nvapi STATIC IMPORTED GLOBAL)
        target_include_directories(nvapi INTERFACE "${NVAPI_INCLUDE_DIR}")
        set_property(TARGET nvapi PROPERTY IMPORTED_LOCATION "${NVAPI_LIBRARY}")
    endif()
endif()

if (CUTIE_WITH_AFTERMATH AND NOT TARGET aftermath)
    include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/FetchAftermath.cmake")
endif()

if(CUTIE_WITH_OPENGL)
    set(cutie_glad_target "")

    if(TARGET glad::glad)
        set(cutie_glad_target glad::glad)
    elseif(TARGET glad)
        set(cutie_glad_target glad)
    endif()

    option(CUTIE_FETCH_GLAD "Fetch GLAD when no external glad target is provided" ON)
    set(CUTIE_GLAD_GIT_REPOSITORY "https://github.com/Dav1dde/glad.git" CACHE STRING "GLAD git repository")
    set(CUTIE_GLAD_GIT_TAG "v2.0.8" CACHE STRING "GLAD git tag")

    if((NOT cutie_glad_target) AND CUTIE_FETCH_GLAD)
        include(FetchContent)
        message(STATUS "Fetching GLAD from ${CUTIE_GLAD_GIT_REPOSITORY}, tag ${CUTIE_GLAD_GIT_TAG}")
        FetchContent_Declare(glad_ext
            GIT_REPOSITORY ${CUTIE_GLAD_GIT_REPOSITORY}
            GIT_TAG ${CUTIE_GLAD_GIT_TAG})
        FetchContent_MakeAvailable(glad_ext)

        if(TARGET glad)
            set(cutie_glad_target glad)
        endif()
    endif()

    if(NOT cutie_glad_target)
        message(FATAL_ERROR "CUTIE OpenGL backend requires GLAD (target 'glad' or 'glad::glad'). "
            "Generate at https://glad.dav1d.de/ and install, or enable CUTIE_FETCH_GLAD.")
    endif()
endif()

set(include_common
    include/cutie/cutie.h
    include/cutie/cutieHLSL.h
    include/cutie/utils.h
    include/cutie/common/containers.h
    include/cutie/common/misc.h
    include/cutie/common/resource.h
    include/cutie/common/aftermath.h)
set(src_common
    src/common/format-info.cpp
    src/common/misc.cpp
    src/common/state-tracking.cpp
    src/common/state-tracking.h
    src/common/utils.cpp
    src/common/aftermath.cpp)

if(MSVC)
    set(misc_common 
        tools/cutie.natvis)
else()
    set(misc_common "")
endif()

set(include_validation
    include/cutie/validation.h)
set(src_validation
    src/validation/validation-commandlist.cpp
    src/validation/validation-device.cpp
    src/validation/validation-backend.h)

set(include_d3d11
    include/cutie/d3d11.h)
set(src_d3d11
    src/common/dxgi-format.h
    src/common/dxgi-format.cpp
    src/d3d11/d3d11-buffer.cpp
    src/d3d11/d3d11-commandlist.cpp
    src/d3d11/d3d11-compute.cpp
    src/d3d11/d3d11-constants.cpp
    src/d3d11/d3d11-backend.h
    src/d3d11/d3d11-device.cpp
    src/d3d11/d3d11-graphics.cpp
    src/d3d11/d3d11-queries.cpp
    src/d3d11/d3d11-resource-bindings.cpp
    src/d3d11/d3d11-shader.cpp
    src/d3d11/d3d11-texture.cpp)

set(include_d3d12
    include/cutie/d3d12.h)
set(src_d3d12
    src/common/dxgi-format.h
    src/common/dxgi-format.cpp
    src/common/versioning.h
    src/d3d12/d3d12-buffer.cpp
    src/d3d12/d3d12-commandlist.cpp
    src/d3d12/d3d12-compute.cpp
    src/d3d12/d3d12-constants.cpp
    src/d3d12/d3d12-backend.h
    src/d3d12/d3d12-descriptor-heap.cpp
    src/d3d12/d3d12-device.cpp
    src/d3d12/d3d12-graphics.cpp
    src/d3d12/d3d12-meshlets.cpp
    src/d3d12/d3d12-queries.cpp
    src/d3d12/d3d12-raytracing.cpp
    src/d3d12/d3d12-resource-bindings.cpp
    src/d3d12/d3d12-shader.cpp
    src/d3d12/d3d12-state-tracking.cpp
    src/d3d12/d3d12-texture.cpp
    src/d3d12/d3d12-upload.cpp)

set(include_vk
    include/cutie/vulkan.h)
set(src_vk
    src/common/versioning.h
    src/vulkan/vulkan-allocator.cpp
    src/vulkan/vulkan-buffer.cpp
    src/vulkan/vulkan-commandlist.cpp
    src/vulkan/vulkan-compute.cpp
    src/vulkan/vulkan-constants.cpp
    src/vulkan/vulkan-device.cpp
    src/vulkan/vulkan-graphics.cpp
    src/vulkan/vulkan-meshlets.cpp
    src/vulkan/vulkan-queries.cpp
    src/vulkan/vulkan-queue.cpp
    src/vulkan/vulkan-raytracing.cpp
    src/vulkan/vulkan-resource-bindings.cpp
    src/vulkan/vulkan-shader.cpp
    src/vulkan/vulkan-staging-texture.cpp
    src/vulkan/vulkan-state-tracking.cpp
    src/vulkan/vulkan-texture.cpp
    src/vulkan/vulkan-upload.cpp
    src/vulkan/vulkan-backend.h)

set(include_gl
    include/cutie/opengl.h)
set(src_gl
    src/opengl/opengl-backend.h
    src/opengl/opengl-device.cpp
    src/opengl/opengl-buffer.cpp
    src/opengl/opengl-commandlist.cpp
    src/opengl/opengl-texture.cpp
    src/opengl/opengl-shader.cpp
    src/opengl/opengl-pipeline.cpp
    src/opengl/opengl-framebuffer.cpp
    src/opengl/opengl-sampler.cpp
    src/opengl/opengl-inputlayout.cpp
    src/opengl/opengl-bindings.cpp
    src/opengl/opengl-query.cpp
    src/opengl/opengl-heap.cpp
    src/opengl/opengl-format.cpp)

# CUTIE interface and common implementation functions

if (CUTIE_BUILD_SHARED)
    add_library(cutie SHARED
        ${include_common}
        ${src_common}
        ${misc_common})
else()
    add_library(cutie STATIC
        ${include_common}
        ${src_common}
        ${misc_common})
endif()


if (CUTIE_BUILD_SHARED)
    target_compile_definitions(cutie PRIVATE CUTIE_SHARED_LIBRARY_BUILD=1)
    if(CUTIE_WITH_VULKAN)
        target_compile_definitions(cutie PRIVATE VULKAN_HPP_STORAGE_SHARED VULKAN_HPP_STORAGE_SHARED_EXPORT)
    endif()
    set_property(TARGET cutie PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

if (CUTIE_WITH_VALIDATION)
    target_sources(cutie PRIVATE
        ${include_validation}
        ${src_validation})
endif()

target_include_directories(cutie PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:$<INSTALL_PREFIX>/include>)

set_target_properties(cutie PROPERTIES FOLDER "CUTIE")

target_compile_definitions(cutie PRIVATE CUTIE_WITH_AFTERMATH=$<BOOL:${CUTIE_WITH_AFTERMATH}>)

# implementations

if (CUTIE_WITH_DX11)
    if (CUTIE_BUILD_SHARED)
        set(cutie_d3d11_target cutie)

        target_sources(${cutie_d3d11_target} PRIVATE
            ${include_d3d11}
            ${src_d3d11})
    else()
        set(cutie_d3d11_target cutie_d3d11)

        add_library(${cutie_d3d11_target} STATIC
            ${include_d3d11}
            ${src_d3d11})

        set_target_properties(${cutie_d3d11_target} PROPERTIES FOLDER "CUTIE")
        target_include_directories(${cutie_d3d11_target} PRIVATE include)
    endif()

    target_link_libraries(${cutie_d3d11_target} PUBLIC d3d11 dxguid)

    if (CUTIE_WITH_NVAPI)
        target_link_libraries(${cutie_d3d11_target} PUBLIC nvapi)
    endif()
    target_compile_definitions(${cutie_d3d11_target} PRIVATE CUTIE_D3D11_WITH_NVAPI=$<BOOL:${CUTIE_WITH_NVAPI}>)

    if (CUTIE_WITH_AFTERMATH)
        target_link_libraries(${cutie_d3d11_target} PUBLIC aftermath)
    endif()
    target_compile_definitions(${cutie_d3d11_target} PRIVATE CUTIE_WITH_AFTERMATH=$<BOOL:${CUTIE_WITH_AFTERMATH}>)
endif()

if (CUTIE_WITH_DX12)
    if (CUTIE_BUILD_SHARED)
        set(cutie_d3d12_target cutie)

        target_sources(${cutie_d3d12_target} PRIVATE
            ${include_d3d12}
            ${src_d3d12})
    else()
        set(cutie_d3d12_target cutie_d3d12)

        add_library(${cutie_d3d12_target} STATIC
            ${include_d3d12}
            ${src_d3d12})

        set_target_properties(${cutie_d3d12_target} PROPERTIES FOLDER "CUTIE")
        target_include_directories(${cutie_d3d12_target} PRIVATE include)
    endif()

    if (CUTIE_WITH_RTXMU)
        target_compile_definitions(${cutie_d3d12_target} PUBLIC CUTIE_WITH_RTXMU=1)
        target_link_libraries(${cutie_d3d12_target} PUBLIC rtxmu)
    endif()

    target_compile_definitions(${cutie_d3d12_target} PRIVATE CUTIE_D3D12_WITH_DXR12_OPACITY_MICROMAP=$<BOOL:${CUTIE_D3D12_WITH_DXR12_OPACITY_MICROMAP}>)

    target_link_libraries(${cutie_d3d12_target} PUBLIC ${cutie_directx_headers_target} ${cutie_directx_guids_target} d3d12)

    if (CUTIE_WITH_NVAPI)
        target_link_libraries(${cutie_d3d12_target} PUBLIC nvapi)
    endif()
    target_compile_definitions(${cutie_d3d12_target} PRIVATE CUTIE_D3D12_WITH_NVAPI=$<BOOL:${CUTIE_WITH_NVAPI}>)

    if (CUTIE_WITH_AFTERMATH)
        target_link_libraries(${cutie_d3d12_target} PUBLIC aftermath)
    endif()
    target_compile_definitions(${cutie_d3d12_target} PRIVATE CUTIE_WITH_AFTERMATH=$<BOOL:${CUTIE_WITH_AFTERMATH}>)
endif()

if (CUTIE_WITH_VULKAN)
    if (CUTIE_BUILD_SHARED)
        set(cutie_vulkan_target cutie)

        target_sources(${cutie_vulkan_target} PRIVATE
            ${include_vk}
            ${src_vk})
    else()
        set(cutie_vulkan_target cutie_vk)

        add_library(${cutie_vulkan_target} STATIC
            ${include_vk}
            ${src_vk})

        set_target_properties(${cutie_vulkan_target} PROPERTIES FOLDER "CUTIE")
        target_include_directories(${cutie_vulkan_target} PRIVATE include)
    endif()
    
    if(WIN32)
        target_compile_definitions(${cutie_vulkan_target} PUBLIC VK_USE_PLATFORM_WIN32_KHR)
        target_compile_definitions(${cutie_vulkan_target} PRIVATE NOMINMAX)
    endif()

    if (CUTIE_WITH_RTXMU)
        target_compile_definitions(${cutie_vulkan_target} PUBLIC CUTIE_WITH_RTXMU=1)
        target_link_libraries(${cutie_vulkan_target} PUBLIC rtxmu)
    endif()

    target_link_libraries(${cutie_vulkan_target} PRIVATE ${cutie_vulkan_headers_target})

    if (CUTIE_WITH_AFTERMATH)
        target_link_libraries(${cutie_vulkan_target} PUBLIC aftermath)
    endif()
    target_compile_definitions(${cutie_vulkan_target} PRIVATE CUTIE_WITH_AFTERMATH=$<BOOL:${CUTIE_WITH_AFTERMATH}>)
endif()

if (CUTIE_WITH_OPENGL)
    if (CUTIE_BUILD_SHARED)
        set(cutie_opengl_target cutie)
        target_sources(${cutie_opengl_target} PRIVATE ${include_gl} ${src_gl})
    else()
        set(cutie_opengl_target cutie_gl)
        add_library(${cutie_opengl_target} STATIC ${include_gl} ${src_gl})
        set_target_properties(${cutie_opengl_target} PROPERTIES FOLDER "CUTIE")
        target_include_directories(${cutie_opengl_target} PRIVATE include)
    endif()

    if(WIN32)
        target_link_libraries(${cutie_opengl_target} PUBLIC opengl32)
    elseif(APPLE)
        find_library(OPENGL_LIBRARY OpenGL REQUIRED)
        target_link_libraries(${cutie_opengl_target} PUBLIC ${OPENGL_LIBRARY})
    else()
        find_package(OpenGL REQUIRED)
        target_link_libraries(${cutie_opengl_target} PUBLIC OpenGL::GL)
    endif()

    target_link_libraries(${cutie_opengl_target} PRIVATE ${cutie_glad_target})

    if (CUTIE_WITH_AFTERMATH)
        target_link_libraries(${cutie_opengl_target} PUBLIC aftermath)
    endif()
    target_compile_definitions(${cutie_opengl_target} PRIVATE CUTIE_WITH_AFTERMATH=$<BOOL:${CUTIE_WITH_AFTERMATH}>)
endif()


if (CUTIE_INSTALL)
    install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/cutie
        DESTINATION ${CMAKE_INSTALL_PREFIX}/include)

    install(TARGETS cutie
        EXPORT "cutieTargets"
        RUNTIME DESTINATION bin
        ARCHIVE DESTINATION lib
        LIBRARY DESTINATION lib)

    if (NOT CUTIE_BUILD_SHARED)
        if (CUTIE_WITH_DX11)
            install(TARGETS ${cutie_d3d11_target} DESTINATION "lib" EXPORT "cutieTargets")
        endif()

        if (CUTIE_WITH_DX12)
            install(TARGETS ${cutie_d3d12_target} DESTINATION "lib" EXPORT "cutieTargets")
        endif()

        if (CUTIE_WITH_VULKAN)
            install(TARGETS ${cutie_vulkan_target} DESTINATION "lib" EXPORT "cutieTargets")
        endif()

        if (CUTIE_WITH_OPENGL)
            install(TARGETS ${cutie_opengl_target} DESTINATION "lib" EXPORT "cutieTargets")
        endif()
    endif()

    if (CUTIE_INSTALL_EXPORTS)
        set(cutie_CONFIG_PATH "lib/cmake/cutie")

        include(CMakePackageConfigHelpers)

        configure_package_config_file(
            src/cutieConfig.cmake.in
            src/cutieConfig.cmake
            INSTALL_DESTINATION "${cutie_CONFIG_PATH}"
            NO_CHECK_REQUIRED_COMPONENTS_MACRO)

        write_basic_package_version_file(
            src/cutieConfigVersion.cmake 
            VERSION ${cutie_VERSION} 
            COMPATIBILITY ExactVersion)

        install(FILES
            "${cutie_BINARY_DIR}/src/cutieConfig.cmake"
            "${cutie_BINARY_DIR}/src/cutieConfigVersion.cmake"
            DESTINATION "${cutie_CONFIG_PATH}")

        install(EXPORT "cutieTargets" 
            FILE "cutieTargets.cmake"
            EXPORT_LINK_INTERFACE_LIBRARIES
            DESTINATION "${cutie_CONFIG_PATH}")
    endif()
endif()