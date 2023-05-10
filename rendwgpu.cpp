// rendwgpu.cpp : Defines the entry point for the application.
//

#include <iostream>
#include "GLFW\glfw3.h"

#include "webgpu\webgpu.h"
#include "webgpu\wgpu.h"

#define WEBGPU_CPP_IMPLEMENTATION
#include "webgpu\webgpu.hpp"

#include "glfw3webgpu\glfw3webgpu.h"
#include "glm\glm.hpp"
#include "glm\ext.hpp"
using glm::vec3;
using glm::mat4;

#include <cassert>
#include <fstream>

using namespace std;
using namespace wgpu;

struct Uniforms {
	mat4 proj;
	mat4 view;
	mat4 model;
	float time;
	float padding[3];
};

WGPUAdapter requestAdapter(WGPUInstance instance, WGPURequestAdapterOptions const* options) {
	// A simple structure holding the local information shared with the
	// onAdapterRequestEnded callback.
	struct UserData {
		WGPUAdapter adapter = nullptr;
		bool requestEnded = false;
	};
	UserData userData;

	// Callback called by wgpuInstanceRequestAdapter when the request returns
	// This is a C++ lambda function, but could be any function defined in the
	// global scope. It must be non-capturing (the brackets [] are empty) so
	// that it behaves like a regular C function pointer, which is what
	// wgpuInstanceRequestAdapter expects (WebGPU being a C API). The workaround
	// is to convey what we want to capture through the pUserData pointer,
	// provided as the last argument of wgpuInstanceRequestAdapter and received
	// by the callback as its last argument.
	auto onAdapterRequestEnded = [](WGPURequestAdapterStatus status, WGPUAdapter adapter, char const* message, void* pUserData) {
		UserData& userData = *reinterpret_cast<UserData*>(pUserData);
		if (status == WGPURequestAdapterStatus_Success) {
			userData.adapter = adapter;
		}
		else {
			std::cout << "Could not get WebGPU adapter: " << message << std::endl;
		}
		userData.requestEnded = true;
	};

	// Call to the WebGPU request adapter procedure
	wgpuInstanceRequestAdapter(
		instance /* equivalent of navigator.gpu */,
		options,
		onAdapterRequestEnded,
		(void*)&userData
	);

	// In theory we should wait until onAdapterReady has been called, which
	// could take some time (what the 'await' keyword does in the JavaScript
	// code). In practice, we know that when the wgpuInstanceRequestAdapter()
	// function returns its callback has been called.
	assert(userData.requestEnded);

	return userData.adapter;
}

WGPUDevice requestDevice(WGPUAdapter adapter, WGPUDeviceDescriptor const* descriptor) {
	struct UserData {
		WGPUDevice device = nullptr;
		bool requestEnded = false;
	};
	UserData userData;

	auto onDeviceRequestEnded = [](WGPURequestDeviceStatus status, WGPUDevice device, char const* message, void* pUserData) {
		UserData& userData = *reinterpret_cast<UserData*>(pUserData);
		if (status == WGPURequestDeviceStatus_Success) {
			userData.device = device;
		}
		else {
			std::cout << "Could not get WebGPU adapter: " << message << std::endl;
		}
		userData.requestEnded = true;
	};

	wgpuAdapterRequestDevice(
		adapter,
		descriptor,
		onDeviceRequestEnded,
		(void*)&userData
	);

	assert(userData.requestEnded);

	return userData.device;
}

int main()
{
	unsigned int windowWidth = 1600;
	unsigned int windowHeight = 900;


	glfwInit();
	if (!glfwInit()) {
		std::cerr << "Could not initialize GLFW!" << std::endl;
		return 1;
	}

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
	GLFWwindow* window = glfwCreateWindow(windowWidth, windowHeight, "render", NULL, NULL);
	if (!window) {
		std::cerr << "Could not open window!" << std::endl;
		glfwTerminate();
		return 1;
	}

	//INSTANCE
	Instance instance = wgpu::createInstance(InstanceDescriptor());

	Surface surface = glfwGetWGPUSurface(instance, window);
	 
	//ADAPTER
	RequestAdapterOptions adapterOptions;
	adapterOptions.compatibleSurface = surface;
	Adapter adapter = instance.requestAdapter(adapterOptions);

	

	
	//DEVICE
	SupportedLimits adapterLimits;
	adapter.getLimits(&adapterLimits);

	RequiredLimits deviceReqs;
	deviceReqs.limits.maxVertexAttributes = 2;
	deviceReqs.limits.maxVertexBuffers = 1;
	deviceReqs.limits.maxInterStageShaderComponents = 3;
	deviceReqs.limits.maxBufferSize = sizeof(Uniforms);
	deviceReqs.limits.maxVertexBufferArrayStride = 6 * sizeof(float);
	//alignments should use default values rather than zero
	deviceReqs.limits.minUniformBufferOffsetAlignment = adapterLimits.limits.minUniformBufferOffsetAlignment;
	deviceReqs.limits.minStorageBufferOffsetAlignment = adapterLimits.limits.minStorageBufferOffsetAlignment;

	//for uniform binding
	deviceReqs.limits.maxBindGroups = 1;
	deviceReqs.limits.maxUniformBuffersPerShaderStage = 1;
	deviceReqs.limits.maxUniformBufferBindingSize = sizeof(Uniforms);

	deviceReqs.limits.maxTextureDimension2D = 2048;
	deviceReqs.limits.maxTextureArrayLayers = 1;

	DeviceDescriptor deviceDescriptor;
	deviceDescriptor.label = "Default Device";
	deviceDescriptor.requiredFeaturesCount = 0;
	deviceDescriptor.requiredLimits = &deviceReqs;
	deviceDescriptor.defaultQueue.label = "Default Queue";
	Device device = adapter.requestDevice(deviceDescriptor);

	SupportedLimits limits;
	bool success = device.getLimits(&limits);
	if (success) {
		std::cout << "Device limits:" << std::endl;
		std::cout << " - maxTextureDimension1D: " << limits.limits.maxTextureDimension1D << std::endl;
		std::cout << " - maxTextureDimension2D: " << limits.limits.maxTextureDimension2D << std::endl;
		std::cout << " - maxTextureDimension3D: " << limits.limits.maxTextureDimension3D << std::endl;
		std::cout << " - maxTextureArrayLayers: " << limits.limits.maxTextureArrayLayers << std::endl;
		std::cout << " - maxBindGroups: " << limits.limits.maxBindGroups << std::endl;
		std::cout << " - maxDynamicUniformBuffersPerPipelineLayout: " << limits.limits.maxDynamicUniformBuffersPerPipelineLayout << std::endl;
		std::cout << " - maxDynamicStorageBuffersPerPipelineLayout: " << limits.limits.maxDynamicStorageBuffersPerPipelineLayout << std::endl;
		std::cout << " - maxSampledTexturesPerShaderStage: " << limits.limits.maxSampledTexturesPerShaderStage << std::endl;
		std::cout << " - maxSamplersPerShaderStage: " << limits.limits.maxSamplersPerShaderStage << std::endl;
		std::cout << " - maxStorageBuffersPerShaderStage: " << limits.limits.maxStorageBuffersPerShaderStage << std::endl;
		std::cout << " - maxStorageTexturesPerShaderStage: " << limits.limits.maxStorageTexturesPerShaderStage << std::endl;
		std::cout << " - maxUniformBuffersPerShaderStage: " << limits.limits.maxUniformBuffersPerShaderStage << std::endl;
		std::cout << " - maxUniformBufferBindingSize: " << limits.limits.maxUniformBufferBindingSize << std::endl;
		std::cout << " - maxStorageBufferBindingSize: " << limits.limits.maxStorageBufferBindingSize << std::endl;
		std::cout << " - minUniformBufferOffsetAlignment: " << limits.limits.minUniformBufferOffsetAlignment << std::endl;
		std::cout << " - minStorageBufferOffsetAlignment: " << limits.limits.minStorageBufferOffsetAlignment << std::endl;
		std::cout << " - maxVertexBuffers: " << limits.limits.maxVertexBuffers << std::endl;
		std::cout << " - maxVertexAttributes: " << limits.limits.maxVertexAttributes << std::endl;
		std::cout << " - maxVertexBufferArrayStride: " << limits.limits.maxVertexBufferArrayStride << std::endl;
		std::cout << " - maxInterStageShaderComponents: " << limits.limits.maxInterStageShaderComponents << std::endl;
		std::cout << " - maxComputeWorkgroupStorageSize: " << limits.limits.maxComputeWorkgroupStorageSize << std::endl;
		std::cout << " - maxComputeInvocationsPerWorkgroup: " << limits.limits.maxComputeInvocationsPerWorkgroup << std::endl;
		std::cout << " - maxComputeWorkgroupSizeX: " << limits.limits.maxComputeWorkgroupSizeX << std::endl;
		std::cout << " - maxComputeWorkgroupSizeY: " << limits.limits.maxComputeWorkgroupSizeY << std::endl;
		std::cout << " - maxComputeWorkgroupSizeZ: " << limits.limits.maxComputeWorkgroupSizeZ << std::endl;
		std::cout << " - maxComputeWorkgroupsPerDimension: " << limits.limits.maxComputeWorkgroupsPerDimension << std::endl;
	}

	
	//TODO figure out what this means
	auto onDeviceError = [](WGPUErrorType type, char const* message, void* /* pUserData */) {
		std::cout << "Uncaptured device error: type " << type;
		if (message) std::cout << " (" << message << ")";
		std::cout << std::endl;
	};
	wgpuDeviceSetUncapturedErrorCallback(device, onDeviceError, nullptr /* pUserData */);

	
	//SWAPCHAIN
	SwapChainDescriptor swapChainDescriptor;
	swapChainDescriptor.width = windowWidth;
	swapChainDescriptor.height = windowHeight;
	swapChainDescriptor.usage = WGPUTextureUsage_RenderAttachment;
	TextureFormat swapChainFormat = wgpuSurfaceGetPreferredFormat(surface, adapter);
	swapChainDescriptor.format = swapChainFormat;
	swapChainDescriptor.presentMode = WGPUPresentMode_Mailbox; //TODO - mailbox?
	SwapChain swapChain = device.createSwapChain(surface, swapChainDescriptor);
	
	
	Queue queue = device.getQueue();


	//command buffer descs, use this to create the command buffer each frame
	CommandEncoderDescriptor encoderDescriptor;
	encoderDescriptor.label = "Default Encoder";

	CommandBufferDescriptor bufferDescriptor;
	bufferDescriptor.label = "Default command buffer";

	//shader
	ShaderModuleDescriptor shaderDescriptor;
	shaderDescriptor.hintCount = 0;
	shaderDescriptor.hints = nullptr;
	ShaderModuleWGSLDescriptor shaderCodeDescriptor;
	shaderCodeDescriptor.chain.next = nullptr;
	shaderCodeDescriptor.chain.sType = SType::ShaderModuleWGSLDescriptor;

	ifstream shaderFileStream;
	shaderFileStream.open("E:/Anna/Anna/Visual Studio/rendwgpu/defaultshader.wgsl");
	shaderFileStream.seekg(0, std::ios_base::end);
	int shaderTextLength = (int) shaderFileStream.tellg();
	shaderFileStream.seekg(0);
	std::vector<char> shaderText(shaderTextLength);
	shaderFileStream.read(shaderText.data(), shaderTextLength);
	shaderFileStream.close();
	shaderCodeDescriptor.code = shaderText.data();
	shaderDescriptor.nextInChain = &shaderCodeDescriptor.chain;
	ShaderModule shader = device.createShaderModule(shaderDescriptor);

	//render pipeline object
	RenderPipelineDescriptor pipelineDescriptor;

	pipelineDescriptor.vertex.bufferCount = 0;
	pipelineDescriptor.vertex.buffers = nullptr;
	pipelineDescriptor.vertex.module = shader;//shader module
	pipelineDescriptor.vertex.entryPoint = "vs_main";
	pipelineDescriptor.vertex.constantCount = 0;
	pipelineDescriptor.vertex.constants = nullptr;

	pipelineDescriptor.primitive.topology = PrimitiveTopology::TriangleList;
	pipelineDescriptor.primitive.stripIndexFormat = IndexFormat::Undefined;
	pipelineDescriptor.primitive.frontFace = FrontFace::CCW;
	pipelineDescriptor.primitive.cullMode = CullMode::None;

	BlendState blendState;
	blendState.color.srcFactor = BlendFactor::SrcAlpha;
	blendState.color.dstFactor = BlendFactor::OneMinusSrcAlpha;
	blendState.color.operation = BlendOperation::Add;
	blendState.alpha.srcFactor = BlendFactor::Zero;
	blendState.alpha.dstFactor = BlendFactor::One;
	blendState.alpha.operation = BlendOperation::Add;

	ColorTargetState colorTarget;
	colorTarget.format = swapChainFormat;
	colorTarget.blend = &blendState;
	colorTarget.writeMask = ColorWriteMask::All;

	FragmentState fragmentState;
	fragmentState.module = shader;
	fragmentState.entryPoint = "fs_main";
	fragmentState.constantCount = 0;
	fragmentState.constants = nullptr;
	fragmentState.targetCount = 1;
	fragmentState.targets = &colorTarget;
	pipelineDescriptor.fragment = &fragmentState;

	TextureFormat depthTextureFormat = TextureFormat::Depth24Plus;
	DepthStencilState depthState = Default;
	depthState.depthCompare = CompareFunction::Less;
	depthState.depthWriteEnabled = true;
	depthState.format = depthTextureFormat;
	depthState.stencilReadMask = 0;
	depthState.stencilWriteMask = 0;
	pipelineDescriptor.depthStencil = &depthState;


	// Create the depth texture
	TextureDescriptor depthTextureDesc;
	depthTextureDesc.dimension = TextureDimension::_2D;
	depthTextureDesc.format = depthTextureFormat;
	depthTextureDesc.mipLevelCount = 1;
	depthTextureDesc.sampleCount = 1;
	depthTextureDesc.size = { windowWidth, windowHeight, 1 };
	depthTextureDesc.usage = TextureUsage::RenderAttachment;
	depthTextureDesc.viewFormatCount = 1;
	depthTextureDesc.viewFormats = (WGPUTextureFormat*)&depthTextureFormat;
	Texture depthTexture = device.createTexture(depthTextureDesc);


	// Create the view of the depth texture manipulated by the rasterizer
	TextureViewDescriptor depthTextureViewDesc;
	depthTextureViewDesc.aspect = TextureAspect::DepthOnly;
	depthTextureViewDesc.baseArrayLayer = 0;
	depthTextureViewDesc.arrayLayerCount = 1;
	depthTextureViewDesc.baseMipLevel = 0;
	depthTextureViewDesc.mipLevelCount = 1;
	depthTextureViewDesc.dimension = TextureViewDimension::_2D;
	depthTextureViewDesc.format = depthTextureFormat;
	TextureView depthTextureView = depthTexture.createView(depthTextureViewDesc);

	std::cout << depthTextureViewDesc.format << endl;


	pipelineDescriptor.multisample.count = 1;
	pipelineDescriptor.multisample.mask = ~0u;
	pipelineDescriptor.multisample.alphaToCoverageEnabled = false;

	pipelineDescriptor.layout = nullptr;

	//VERTEX BUFFER
	std::vector<float> vertexData = {
		-0.5f, -0.5f, -0.3f, 0.0f, 0.0f, 0.0f,
		+0.5f, -0.5f, -0.3f, 1.0f, 0.0f, 0.0f,
		+0.5f, +0.5f, -0.3f, 1.0f, 1.0f, 0.0f,
		-0.5f, +0.5f, -0.3f, 0.0f, 1.0f, 0.0f,
		+0.0f, +0.0f, +0.5f, 0.5f, 0.5f, 1.0f,
	};
	int vertexCount = static_cast<int>(vertexData.size() / 6);

	BufferDescriptor vBufferDesc;
	vBufferDesc.size = vertexData.size() * sizeof(float);
	vBufferDesc.usage = BufferUsage::CopyDst | BufferUsage::Vertex;
	vBufferDesc.mappedAtCreation = false;
	vBufferDesc.label = "vertex buffer";
	Buffer vBuffer = device.createBuffer(vBufferDesc);
	queue.writeBuffer(vBuffer, 0, vertexData.data(), vBufferDesc.size);

	//VERTEX BUFFER LAYOUT
	VertexBufferLayout vBufferLayout;

	std::vector<VertexAttribute> vBufferAttribs(2);
	//position
	vBufferAttribs[0].shaderLocation = 0;
	vBufferAttribs[0].offset = 0;
	vBufferAttribs[0].format = VertexFormat::Float32x3;
	//color
	vBufferAttribs[1].shaderLocation = 1;
	vBufferAttribs[1].offset = 3 * sizeof(float);
	vBufferAttribs[1].format = VertexFormat::Float32x3;

	vBufferLayout.attributeCount = static_cast<uint32_t>(vBufferAttribs.size());
	vBufferLayout.attributes = vBufferAttribs.data();
	vBufferLayout.arrayStride = 6 * sizeof(float);
	vBufferLayout.stepMode = VertexStepMode::Vertex;
	pipelineDescriptor.vertex.bufferCount = 1;
	pipelineDescriptor.vertex.buffers = &vBufferLayout;

	//IDX BUFFER
	std::vector<uint16_t> idxData = {
		0, 1, 2,
		0, 2, 3,
		0, 1, 4,
		1, 2, 4,
		2, 3, 4,
		3, 0, 4
	};
	int idxCount = static_cast<int>(idxData.size());
	BufferDescriptor idxBufferDesc;
	//A writeBuffer operation must copy a number of bytes that is a multiple of 4. To ensure so we can switch bufferDesc.size for (bufferDesc.size + 3) & ~3.
	idxBufferDesc.size = (idxCount * sizeof(uint16_t) + 3) & ~3;
	idxBufferDesc.usage = BufferUsage::CopyDst | BufferUsage::Index;
	idxBufferDesc.mappedAtCreation = false;
	idxBufferDesc.label = "idx buffer";
	Buffer idxBuffer = device.createBuffer(idxBufferDesc);
	queue.writeBuffer(idxBuffer, 0, idxData.data(), idxBufferDesc.size);

	//not just Time Uniform buffer
	BufferDescriptor uniformBufferDesc;
	uniformBufferDesc.size = sizeof(Uniforms);
	uniformBufferDesc.usage = BufferUsage::CopyDst | BufferUsage::Uniform;
	uniformBufferDesc.mappedAtCreation = false;
	uniformBufferDesc.label = "uniform buffer";
	Buffer uniformBuffer = device.createBuffer(uniformBufferDesc);

	//Uniform bind group layout
	BindGroupLayoutEntry uniformLayoutEntry;// = Default; //TODO what happens if this isnt here
	uniformLayoutEntry.binding = 0;
	uniformLayoutEntry.visibility = ShaderStage::Vertex;
	uniformLayoutEntry.buffer.type = BufferBindingType::Uniform;
	uniformLayoutEntry.buffer.minBindingSize = sizeof(Uniforms);

	BindGroupLayoutDescriptor uniformLayoutDesc;
	uniformLayoutDesc.entryCount = 1;
	uniformLayoutDesc.entries = &uniformLayoutEntry;
	BindGroupLayout uniformLayout = device.createBindGroupLayout(uniformLayoutDesc);

	//uniform bind group
	BindGroupEntry uniformGroupEntry;
	uniformGroupEntry.binding = 0;
	uniformGroupEntry.buffer = uniformBuffer;
	uniformGroupEntry.offset = 0;
	uniformGroupEntry.size = sizeof(Uniforms);

	BindGroupDescriptor uniformGroupDesc;
	uniformGroupDesc.layout = uniformLayout;
	uniformGroupDesc.entryCount = 1;
	uniformGroupDesc.entries = &uniformGroupEntry;
	BindGroup uniformGroup = device.createBindGroup(uniformGroupDesc);

	RenderPipeline pipeline = device.createRenderPipeline(pipelineDescriptor);

	Uniforms uniformData;


	//where can i find an actual pi
	const float PI = 3.141592654f;

	//proj
	float near = 0.001f;
	float far = 100.0f;
	float ratio = ((float)windowWidth) / ((float)windowHeight);
	float fov = 45 * PI / 180;
	uniformData.proj = glm::perspective(fov, ratio, near, far);

	//view
	uniformData.view = glm::translate(mat4(1), vec3(0, 0, -3));
	uniformData.view = glm::rotate(uniformData.view, -60 * PI / 180, vec3(1, 0, 0));

	//model
	uniformData.model = mat4(1);

	uniformData.time = (float)glfwGetTime();
	queue.writeBuffer(uniformBuffer, 0, &uniformData, sizeof(Uniforms));


	cout << "Hello CMake." << endl;

	while (!glfwWindowShouldClose(window)) {
		glfwPollEvents();

		uniformData.time = (float)glfwGetTime();
		queue.writeBuffer(uniformBuffer, offsetof(Uniforms, time), &uniformData.time, sizeof(float));


		uniformData.model = glm::rotate(mat4(1), uniformData.time, vec3(0.0, 0.0, 1.0));
		uniformData.model = glm::translate(uniformData.model, vec3(1.0, 0.0, 0.0));
		uniformData.model = glm::scale(uniformData.model, vec3(0.5f));
		queue.writeBuffer(uniformBuffer, offsetof(Uniforms, model), &uniformData.model, sizeof(mat4));


		
		TextureView nextFrame = swapChain.getCurrentTextureView();
		if (!nextFrame) {
			std::cerr << "Cannot acquire next swap chain texture" << std::endl;
			break;
		}
		std::cout << "next frame: " << nextFrame << std::endl;




		RenderPassColorAttachment renderPassColorAttachment;
		renderPassColorAttachment.view = nextFrame;
		renderPassColorAttachment.resolveTarget = nullptr;
		renderPassColorAttachment.loadOp = LoadOp::Clear;
		renderPassColorAttachment.storeOp = StoreOp::Store;
		renderPassColorAttachment.clearValue = WGPUColor{ 0.05, 0.1, 0.11, 1.0 };

		RenderPassDepthStencilAttachment renderPassDepthAttatchment;
		renderPassDepthAttatchment.view = depthTextureView;

		renderPassDepthAttatchment.depthClearValue = 1.0f;
		renderPassDepthAttatchment.depthLoadOp = LoadOp::Clear;
		renderPassDepthAttatchment.depthStoreOp = StoreOp::Store;
		renderPassDepthAttatchment.depthReadOnly = false;

		renderPassDepthAttatchment.stencilClearValue = 0;
		renderPassDepthAttatchment.stencilLoadOp = LoadOp::Clear;
		renderPassDepthAttatchment.stencilStoreOp = StoreOp::Store;
		renderPassDepthAttatchment.stencilReadOnly = false;


		RenderPassDescriptor renderPassDescriptor;
		renderPassDescriptor.label = "Default Render Pass";
		renderPassDescriptor.colorAttachmentCount = 1;
		renderPassDescriptor.colorAttachments = &renderPassColorAttachment;
		renderPassDescriptor.depthStencilAttachment = &renderPassDepthAttatchment;
		renderPassDescriptor.timestampWriteCount = 0;
		renderPassDescriptor.timestampWrites = nullptr;


		//swapChain.present();
		CommandEncoder encoder = device.createCommandEncoder(encoderDescriptor);
		RenderPassEncoder renderPass = encoder.beginRenderPass(renderPassDescriptor);
		renderPass.setPipeline(pipeline);
		renderPass.setVertexBuffer(0, vBuffer, 0, vertexData.size() * sizeof(float));
		renderPass.setIndexBuffer(idxBuffer, IndexFormat::Uint16, 0, idxData.size() * sizeof(uint16_t));
		renderPass.setBindGroup(0, uniformGroup, 0, nullptr);
		renderPass.drawIndexed(idxCount, 1, 0, 0, 0);
		renderPass.end();

		wgpuTextureViewDrop(nextFrame);


		CommandBuffer commandBuffer = encoder.finish(bufferDescriptor);
		queue.submit(commandBuffer);

		swapChain.present();

		
		//std::cout << "nextTexture: " << nextFrame << std::endl;
		//std::cout << "A" << std::endl;

	}


	wgpuSwapChainDrop(swapChain);
	wgpuDeviceDrop(device);
	wgpuAdapterDrop(adapter);
	wgpuInstanceDrop(instance);

	glfwDestroyWindow(window);


	glfwTerminate();
	return 0;
}
