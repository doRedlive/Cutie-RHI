# CUTIE Tutorial

This tutorial provides a walk-through of the process of integrating CUTIE into a program and using it to render an object in a scene using rasterization and ray tracing pipelines.

## Basic Graphics Example

### Creating the Device

The first step after we include the CUTIE headers and libraries into your project is to create a device wrapper. Assuming the underlying device, context, queues, etc. have been initialized, the CUTIE wrapper can be created using one of the following snippets - they cover different graphics APIs.

#### Direct3D 11
```c++
#include <cutie/d3d11.h>
...
cutie::d3d11::DeviceDesc deviceDesc;
deviceDesc.messageCallback = g_MyMessageCallback;
deviceDesc.context = d3d11DeviceContext;

cutie::DeviceHandle cutieDevice = cutie::d3d11::createDevice(deviceDesc);
```

#### Direct3D 12

```c++
#include <cutie/d3d12.h>
...
cutie::d3d12::DeviceDesc deviceDesc;
deviceDesc.errorCB = g_MyMessageCallback;
deviceDesc.pDevice = d3d12Device;
deviceDesc.pGraphicsCommandQueue = d3d12GraphicsCommandQueue;

cutie::DeviceHandle cutieDevice = cutie::d3d12::createDevice(deviceDesc);
```

#### Vulkan

```c++
#include <cutie/vulkan.h>
...
const char* deviceExtensions[] = {
    "VK_KHR_acceleration_structure",
    "VK_KHR_deferred_host_operations",
    "VK_KHR_ray_tracing_pipeline",
    // list the extensions that were requested when the device was created
};
cutie::vulkan::DeviceDesc deviceDesc;
deviceDesc.errorCB = g_MyMessageCallback;
deviceDesc.physicalDevice = vulkanPhysicalDevice;
deviceDesc.device = vulkanDevice;
deviceDesc.graphicsQueue = vulkanGraphicsQueue;
deviceDesc.graphicsQueueIndex = vulkanGraphicsQueueFamily;
deviceDesc.deviceExtensions = deviceExtensions;
deviceDesc.numDeviceExtensions = std::size(deviceExtensions);

cutie::DeviceHandle cutieDevice = cutie::vulkan::createDevice(deviceDesc);
```

#### Validation
```c++
#include <cutie/validation.h>
...
if (enableValidation) {
    cutie::DeviceHandle cutieValidationLayer = cutie::validation::createValidationLayer(cutieDevice);
    cutieDevice = cutieValidationLayer; // make the rest of the application go through the validation layer
}
```

### Creating the Swap Chain Textures

In order to draw something on the screen, the renderer needs to be able to access the swap chain. CUTIE does not create the swap chain, but it provides the means to create wrappers for native swap chain textures. This is done through the same function on all GAPIs:

```c++
auto textureDesc = cutie::TextureDesc()
    .setDimension(cutie::TextureDimension::Texture2D)
    .setFormat(cutie::Format::RGBA8_UNORM)
    .setWidth(swapChainWidth)
    .setHeight(swapChainHeight)
    .setIsRenderTarget(true)
    .setDebugName("Swap Chain Image");

// In this line, <type> depends on the GAPI and should be one of: D3D11_Resource, D3D12_Resource, VK_Image.
cutie::TextureHandle swapChainTexture = cutieDevice->createHandleForNativeTexture(cutie::ObjectTypes::<type>, nativeTextureOrImage, textureDesc);
```

Now, the `swapChainTexture` variable holds a strong reference to the swap chain texture. It can be used to create a `Framebuffer` object to be rendered into.

```c++
auto framebufferDesc = cutie::FramebufferDesc()
    .addColorAttachment(swapChainTexture); // you can specify a particular subresource if necessary

cutie::FramebufferHandle framebuffer = cutieDevice->createFramebuffer(framebufferDesc);
```

On D3D12 and Vulkan, multiple swap chain textures and explicit access synchronization is necessary; this is out of scope for this article, and working implementations can be found in the `DeviceManager` classes in Donut: [D3D11](https://github.com/NVIDIA-RTX/Donut/blob/main/src/app/dx11/DeviceManager_DX11.cpp), [D3D12](https://github.com/NVIDIA-RTX/Donut/blob/main/src/app/dx12/DeviceManager_DX12.cpp), [Vulkan](https://github.com/NVIDIA-RTX/Donut/blob/main/src/app/vulkan/DeviceManager_VK.cpp).

### Creating a Graphics Pipeline

The rest of the rendering code is typically GAPI independent, except for different shader formats.

For this example, let's create a simple graphics pipeline with a vertex shader and a pixel shader that draws some textured geometry. First, we need to create the shader objects and the input layout:

```c++
// Assume the shaders are included as C headers; they could just as well be loaded from files.
const char g_VertexShader[] = ...;
const char g_PixelShader[] = ...;

struct Vertex {
    float position[3];
    float texCoord[2];
};

cutie::ShaderHandle vertexShader = cutieDevice->createShader(
    cutie::ShaderDesc().setShaderType(cutie::ShaderType::Vertex),
    g_VertexShader, sizeof(g_VertexShader));

cutie::VertexAttributeDesc attributes[] = {
    cutie::VertexAttributeDesc()
        .setName("POSITION")
        .setFormat(cutie::Format::RGB32_FLOAT)
        .setOffset(offsetof(Vertex, position))
        .setElementStride(sizeof(Vertex)),
    cutie::VertexAttributeDesc()
        .setName("TEXCOORD")
        .setFormat(cutie::Format::RG32_FLOAT)
        .setOffset(offsetof(Vertex, texCoord))
        .setElementStride(sizeof(Vertex)),
};

cutie::InputLayoutHandle inputLayout = cutieDevice->createInputLayout(
    attributes, uint32_t(std::size(attributes)), vertexShader);

cutie::ShaderHandle pixelShader = cutieDevice->createShader(
    cutie::ShaderDesc().setShaderType(cutie::ShaderType::Pixel),
    g_PixelShader, sizeof(g_PixelShader));
```

In order to create the pipeline, we need to know which render target formats will be used. This information is provided through the `FramebufferInfo` structure:

```c++
auto framebufferInfo = cutie::FramebufferInfo()
    .addColorFormat(cutie::Format::RGBA8_UNORM);
```

Alternatively, `FramebufferInfo` can be obtained from a `Framebuffer` object, if one is available at the time of pipeline creation:

```c++
auto framebufferInfo = framebuffer->getFramebufferInfo();
```

Finally, the pipeline will need to bind some resources, such as constant buffers and textures. We need to declare which resources will be bound to which shader binding slots using a "binding layout" object. For example, let's say our vertex shader will need the view-projection matrix at constant buffer slot b0, and the pixel shader will need the texture at texture slot t0. We'll declare both items in the same layout, visible to all shader stages for simplicity. If necessary, a pipeline can use multiple layouts with different visibility masks to separate the bindings.

```c++
auto layoutDesc = cutie::BindingLayoutDesc()
    .setVisibility(cutie::ShaderType::All)
    .addItem(cutie::BindingLayoutItem::Texture_SRV(0))             // texture at t0
    .addItem(cutie::BindingLayoutItem::VolatileConstantBuffer(0)); // constants at b0

cutie::BindingLayoutHandle bindingLayout = cutieDevice->createBindingLayout(layoutDesc);
```

You may have noticed that the snippet above references a `VolatileConstantBuffer`. That is a special type of constant buffer supported by CUTIE that is more lightweight than a separate buffer object on D3D12 and Vulkan, and has unique semantics, somewhat similar to push constants (which are also supported). For more information on volatile buffers, see the [Programming Guide](ProgrammingGuide.md#buffers).

With all the prerequisites ready, let's create the pipeline:

```c++
auto pipelineDesc = cutie::GraphicsPipelineDesc()
    .setInputLayout(inputLayout)
    .setVertexShader(vertexShader)
    .setPixelShader(pixelShader)
    .addBindingLayout(bindingLayout);

cutie::GraphicsPipelineHandle graphicsPipeline = cutieDevice->createGraphicsPipeline(pipelineDesc, framebufferInfo);
```

Note that the `PipelineDesc`, `BindingLayoutDesc` and other `*Desc` structures are saved in the objects that were created using them, and can be retrieved using the `getDesc()` method of the corresponding object later.

### Creating the Resources

In order to draw something using the pipeline that we just created, we need three more objects: a vertex buffer, a constant buffer, and a texture.

Creating a constant buffer is the easiest part:

```c++
auto constantBufferDesc = cutie::BufferDesc()
    .setByteSize(sizeof(float) * 16) // stores one matrix
    .setIsConstantBuffer(true)
    .setIsVolatile(true)
    .setMaxVersions(16); // number of automatic versions, only necessary on Vulkan

cutie::BufferHandle constantBuffer = cutieDevice->createBuffer(constantBufferDesc);
```

Now let's create a vertex buffer:

```c++
static const Vertex g_Vertices[] = {
    //  position          texCoord
    { { 0.f, 0.f, 0.f }, { 0.f, 0.f } },
    { { 1.f, 0.f, 0.f }, { 1.f, 0.f } },
    // and so on...
};

auto vertexBufferDesc = cutie::BufferDesc()
    .setByteSize(sizeof(g_Vertices))
    .setIsVertexBuffer(true)
    .enableAutomaticStateTracking(cutie::ResourceStates::VertexBuffer)
    .setDebugName("Vertex Buffer");

cutie::BufferHandle vertexBuffer = cutieDevice->createBuffer(vertexBufferDesc);
```

And a texture:

```c++
// Assume the texture pixel data is loaded from and decoded elsewhere.
auto textureDesc = cutie::TextureDesc()
    .setDimension(cutie::TextureDimension::Texture2D)
    .setWidth(textureWidth)
    .setHeight(textureHeight)
    .setFormat(cutie::Format::SRGBA8_UNORM)
    .enableAutomaticStateTracking(cutie::ResourceStates::ShaderResource)
    .setDebugName("Geometry Texture");

cutie::TextureHandle geometryTexture = cutieDevice->createTexture(textureDesc);
```

We'll also need a command list to upload the data and execute the rendering commands:

```c++
cutie::CommandListHandle commandList = cutieDevice->createCommandList();
```

Finally, we'll need a binding set to map our resources to the pipeline at draw time. Binding sets are basically mirror images of the binding layout but they reference actual resources to be bound.

```c++
// Note: the binding set must include all bindings declared in the layout, and nothing else.
// This condition is tested by the validation layer.
// The order of items in the binding set doesn't matter.
auto bindingSetDesc = cutie::BindingSetDesc()
    .addItem(cutie::BindingSetItem::Texture_SRV(0, geometryTexture))
    .addItem(cutie::BindingSetItem::ConstantBuffer(0, constantBuffer));

cutie::BindingSetHandle bindingSet = cutieDevice->createBindingSet(bindingSetDesc, bindingLayout);\
```

### Filling the Resource Data

The following initialization code needs to be executed once on application startup to upload the vertex buffer and texture data:

```c++
commandList->open();

commandList->writeBuffer(vertexBuffer, g_Vertices, sizeof(g_Vertices));

const void* textureData = ...;
const size_t textureRowPitch = ...;
commandList->writeTexture(geometryTexture,
    /* arraySlice = */ 0, /* mipLevel = */ 0,
    textureData, textureRowPitch);

commandList->close();
cutieDevice->executeCommandList(commandList);
```

Note that, unlike D3D12 or Vulkan, there is no need to allocate an upload buffer or issue copy commands. All of that is handled internally by CUTIE inside the `writeTexture` and `writeBuffer` functions. CUTIE contains an upload manager that maintains a pool of upload buffers and tracks their usage, so that chunks can be reused when the GPU has finished executing the command lists that reference them, and allocate new buffers when none are available. Of course, if more control over the upload process is desired, it is also possible to create a separate upload buffer or a staging texture, map them to the CPU, and perform an explicit copy operation.

### Drawing the Geometry

Now that all the objects are created and the resources are filled with data, we can implement the render function that runs on every frame.

```c++
#include <cutie/utils.h> // for ClearColorAttachment
...

// Obtain the current framebuffer from the graphics API
cutie::IFramebuffer* currentFramebuffer = ...;

commandList->open();

// Clear the primary render target
cutie::utils::ClearColorAttachment(commandList, currentFramebuffer, 0, cutie::Color(0.f));

// Fill the constant buffer
float viewProjectionMatrix[16] = {...};
commandList->writeBuffer(constantBuffer, viewProjectionMatrix, sizeof(viewProjectionMatrix));

// Set the graphics state: pipeline, framebuffer, viewport, bindings.
auto graphicsState = cutie::GraphicsState()
    .setPipeline(graphicsPipeline)
    .setFramebuffer(currentFramebuffer)
    .setViewport(cutie::ViewportState().addViewportAndScissorRect(cutie::Viewport(windowWidth, windowHeight)))
    .addBindingSet(bindingSet)
    .addVertexBuffer(vertexBuffer);
commandList->setGraphicsState(graphicsState);

// Draw our geometry
auto drawArguments = cutie::DrawArguments()
    .setVertexCount(std::size(g_Vertices));
commandList->draw(drawArguments);

// Close and execute the command list
commandList->close();
cutieDevice->executeCommandList(commandList);
```

Now we can present the rendered image to the screen. CUTIE does not provide any presentation functions, and that is left up to the application. Working presentation functions for all three GAPI can be found in the same `DeviceManager` classes in Donut, referenced above.

A complete, working example application similar to the code shown above can be found in the [Donut Samples](https://github.com/NVIDIA-RTX/Donut-Samples) repository. Look for the `vertex_buffer` example.

## Ray Tracing Support

CUTIE supports hardware accelerated ray tracing on both Vulkan and D3D12 through two major methods: ray tracing pipelines (`KHR_ray_tracing_pipeline` or DXR 1.0) and ray queries (`KHR_ray_query` or DXR 1.1 TraceRayInline). Both methods use the same acceleration structures (TLAS and BLAS).

Let's go through a basic example of building an acceleration structure and using a ray tracing pipeline to render something.

### Acceleration Structures

First, we need to create the acceleration structure objects. A minimal example includes one BLAS and one TLAS.

```c++
// Need to create the vertex buffer with extra flags
auto vertexBufferDesc = cutie::BufferDesc()
    // ...same parameters as before...
    .setCanHaveRawViews(true)          // we'll need to read the texture UV data in the shader
    .setIsAccelStructBuildInput(true); // we'll need to build the BLAS from the position data

cutie::BufferHandle vertexBuffer = cutieDevice->createBuffer(vertexBufferDesc);

// Geometry descriptor
auto triangles = cutie::rt::GeometryTriangles()
    .setVertexBuffer(vertexBuffer)
    .setVertexFormat(cutie::Format::RGB32_FLOAT)
    .setVertexCount(std::size(g_Vertices))
    .setVertexStride(sizeof(Vertex));

// BLAS descriptor
auto blasDesc = cutie::rt::AccelStructDesc()
    .setDebugName("BLAS")
    .setIsTopLevel(false)
    .addBottomLevelGeometry(cutie::rt::GeometryDesc().setTriangles(triangles));

cutie::rt::AccelStructHandle blas = cutieDevice->createAccelStruct(blasDesc);

auto tlasDesc = cutie::rt::AccelStructDesc()
    .setDebugName("TLAS")
    .setIsTopLevel(true)
    .setTopLevelMaxInstances(1);

cutie::rt::AccelStructHandle tlas = cutieDevice->createAccelStruct(tlasDesc);
```

When the AS objects are created, we can build them. If the geometry is static, they can be built just once, at startup.

```c++
commandList->open();

// Upload the vertex data if that hasn't been done earlier
commandList->writeBuffer(vertexBuffer, g_Vertices, sizeof(g_Vertices));

// Build the BLAS using the geometry array populated earlier.
// It's also possible to obtain the descriptor from the BLAS object using getDesc()
// and write the vertex and index buffer references into that descriptor again
// because CUTIE erases those when it creates the AS object.
commandList->buildBottomLevelAccelStruct(blas,
    blasDesc.geometries.data(), blasDesc.geometries.size());

// Build the TLAS with one instance.
auto instanceDesc = cutie::rt::InstanceDesc()
    .setBLAS(blas)
    .setFlags(1)
    .setTransform(cutie::rt::c_IdentityTransform);

commandList->buildTopLevelAccelStruct(tlas, &instanceDesc, 1);

commandList->close();
cutieDevice->executeCommandList(commandList);
```

### Ray Tracing Pipelines and Shader Tables

Now the acceleration structures are ready for use. Just one tiny bit left: the actual ray tracing pipeline that would use them. To avoid making things complicated, let's assume our ray tracing pipeline is going to be reading one texture and one vertex buffer, so we can bind these things directly - without local root signatures or bindless resources. To see how those approaches can be implemented, please refer to the `rt_reflections` and `rt_bindless` [samples](https://github.com/NVIDIA-RTX/Donut-Samples).

A ray tracing pipeline is based on at least one shader library, and includes multiple shaders: RayGen, ClosestHit, etc. Let's enumerate the shaders and create the pipeline:

```c++
const char g_ShaderLibrary[] = ...;

cutie::ShaderLibraryHandle shaderLibrary = cutieDevice->createShaderLibrary(g_ShaderLibrary, sizeof(g_ShaderLibrary));

auto layoutDesc = cutie::BindingLayoutDesc()
    .setVisibility(cutie::ShaderType::All)
    .addItem(cutie::BindingLayoutItem::Texture_SRV(0))             // texture at t0
    .addItem(cutie::BindingLayoutItem::RawBuffer_SRV(1))           // vertex buffer at t1
    .addItem(cutie::BindingLayoutItem::Texture_UAV(0))             // output texture at u0
    .addItem(cutie::BindingLayoutItem::VolatileConstantBuffer(0)); // constants at b0

cutie::BindingLayoutHandle bindingLayout = cutieDevice->createBindingLayout(layoutDesc);

auto pipelineDesc = cutie::rt::PipelineDesc()
    .addBindingLayout(bindingLayout)
    .setMaxPayloadSize(sizeof(float) * 4)
    .addShader(cutie::rt::PipelineShaderDesc().setShader(
        shaderLibrary->getShader("RayGen", cutie::ShaderType::RayGeneration)))
    .addShader(cutie::rt::PipelineShaderDesc().setShader(
        shaderLibrary->getShader("Miss", cutie::ShaderType::Miss)))
    .addHitGroup(cutie::rt::PipelineHitGroupDesc().setClosestHitShader(
        shaderLibrary->getShader("ClosestHit", cutie::ShaderType::ClosestHit)));

cutie::rt::PipelineHandle rtPipeline = cutieDevice->createRayTracingPipeline(pipelineDesc);
```

Note that each shader or hit group may include its own binding layout. These layouts map to local root signatures on D3D12, but they are not supported on Vulkan due to API constraints.

In order to shoot some rays using this pipeline, we also need to create a shader table. CUTIE provides an easy to use abstraction over the GAPI shader tables that handles the buffer management and shader handle resolutions. Here's how a simple shader table can be created:

```c++
cutie::ShaderTableHandle shaderTable = rtPipeline->createShaderTable();
shaderTable->setRayGenerationShader("RayGen");
shaderTable->addHitGroup("HitGroup", /* localBindingSet = */ nullptr);
shaderTable->addMissShader("Miss");
```

Another thing that makes ray tracing pipelines different from graphics pipelines is that RT pipelines do not render into a framebuffer. Instead, they can only output the ray tracing results into UAVs, or read-write images. You may have noticed the `Texture_UAV` item in the binding layout above. For this tutorial, we'll skip the creation of a temporary texture for this purpose and initialization of the binding set that includes the texture; that's trivial, and similar actions are already covered above.

### Dispatching the Rays

Finally, let's use all these objects and shoot some rays. Here's the render function:

```c++
commandList->open();

// Fill the constant buffer
float viewProjectionMatrix[16] = {...};
commandList->writeBuffer(constantBuffer, viewProjectionMatrix, sizeof(viewProjectionMatrix));

// Set the pipeline, the shader table, and the global bindings
auto rtState = cutie::rt::State()
    .setShaderTable(shaderTable)
    .addBindingSet(bindingSet);
commandList->setRayTracingState(rtState);

// Dispatch the rays
auto dispatchArguments = cutie::DispatchRaysArguments()
    .setWidth(renderWidth)
    .setHeight(renderHeight);
commandList->dispatchRays(dispatchArguments);

// Copy the output texture to the primary framebuffer.
// This is the simplest way to copy the texture contents, but not the most flexible one.
// The copyTexture function cannot do any format conversions, so the input and output
// formats must be copy-compatible. Note that it won't even convert colors from linear
// to sRGB space, for example. It's better to use a full screen quad for blitting.
commandList->copyTexture(
    framebuffer->getDesc().colorAttachments[0].texture,
    cutie::TextureSlice(),
    outputTexture,
    cutie::TextureSlice());

commandList->close();
cutieDevice->executeCommandList(commandList);
```

## Conclusion

In this tutorial, we have shown basic usage of the CUTIE API to create some common rendering resources and pipelines and to draw geometry and trace rays. This does not cover the entire available API, of course, but should give you an idea of what it looks like.

For more information, please refer to the [Programming Guide](ProgrammingGuide.md). To see working applications based on CUTIE, go to the [Donut Samples](https://github.com/NVIDIA-RTX/Donut-Samples) repository.
