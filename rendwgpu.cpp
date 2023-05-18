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

#include "imgui\imgui.h"
#include "imgui\backends\imgui_impl_wgpu.h"
#include "imgui\backends\imgui_impl_glfw.h"


#include <cassert>
#include <fstream>

#include "model.hpp"

using namespace std;
using namespace wgpu;

struct Uniforms {
	mat4 proj;
	mat4 view;
	float time;
	float rotationSpeed;
	float padding[2];
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
	unsigned int windowWidth = 800;
	unsigned int windowHeight = 450;

	cout << "TEST SIZE OF UNIFORMS " << sizeof(Uniforms) << endl;

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


	AdapterProperties adapterProperties;
	adapter.getProperties(&adapterProperties);
	std::cout << " - Backend: " << adapterProperties.backendType << std::endl;

	
	
	//DEVICE
	SupportedLimits adapterLimits;
	adapter.getLimits(&adapterLimits);

	RequiredLimits deviceReqs;
	deviceReqs.limits.maxVertexAttributes = 2;
	deviceReqs.limits.maxVertexBuffers = 1;
	deviceReqs.limits.maxInterStageShaderComponents = 3;
	deviceReqs.limits.maxBufferSize = sizeof(Uniforms);
	deviceReqs.limits.maxVertexBufferArrayStride = 32;
	//alignments should use default values rather than zero
	deviceReqs.limits.minUniformBufferOffsetAlignment = adapterLimits.limits.minUniformBufferOffsetAlignment;
	deviceReqs.limits.minStorageBufferOffsetAlignment = adapterLimits.limits.minStorageBufferOffsetAlignment;

	//for uniform binding
	deviceReqs.limits.maxBindGroups = 1;
	deviceReqs.limits.maxUniformBuffersPerShaderStage = 1;
	deviceReqs.limits.maxUniformBufferBindingSize = sizeof(Uniforms);

	deviceReqs.limits.maxTextureDimension2D = 2048;
	deviceReqs.limits.maxTextureArrayLayers = 1;

	//for imgui
	deviceReqs.limits.maxBufferSize = 268435456; //default
	deviceReqs.limits.maxVertexAttributes = 3;
	deviceReqs.limits.maxInterStageShaderComponents = 6;
	deviceReqs.limits.maxBindGroups = 2;
	deviceReqs.limits.maxSampledTexturesPerShaderStage = 1;
	deviceReqs.limits.maxSamplersPerShaderStage = 1;

	//BACKUP

	deviceReqs.limits.maxTextureDimension1D = adapterLimits.limits.maxTextureDimension1D;
	deviceReqs.limits.maxTextureDimension2D = adapterLimits.limits.maxTextureDimension2D;
	deviceReqs.limits.maxTextureDimension3D = adapterLimits.limits.maxTextureDimension3D;
	deviceReqs.limits.maxTextureArrayLayers = adapterLimits.limits.maxTextureArrayLayers;
	deviceReqs.limits.maxBindGroups = adapterLimits.limits.maxBindGroups;
	deviceReqs.limits.maxDynamicUniformBuffersPerPipelineLayout = adapterLimits.limits.maxDynamicUniformBuffersPerPipelineLayout;
	deviceReqs.limits.maxDynamicStorageBuffersPerPipelineLayout = adapterLimits.limits.maxDynamicStorageBuffersPerPipelineLayout;
	deviceReqs.limits.maxSampledTexturesPerShaderStage = adapterLimits.limits.maxSampledTexturesPerShaderStage;
	deviceReqs.limits.maxSamplersPerShaderStage = adapterLimits.limits.maxSamplersPerShaderStage;
	deviceReqs.limits.maxStorageBuffersPerShaderStage = adapterLimits.limits.maxStorageBuffersPerShaderStage;
	deviceReqs.limits.maxStorageTexturesPerShaderStage = adapterLimits.limits.maxStorageTexturesPerShaderStage;
	deviceReqs.limits.maxUniformBuffersPerShaderStage = adapterLimits.limits.maxUniformBuffersPerShaderStage;
	deviceReqs.limits.maxUniformBufferBindingSize = adapterLimits.limits.maxUniformBufferBindingSize;
	deviceReqs.limits.maxStorageBufferBindingSize = adapterLimits.limits.maxStorageBufferBindingSize;
	deviceReqs.limits.minUniformBufferOffsetAlignment = adapterLimits.limits.minUniformBufferOffsetAlignment;
	deviceReqs.limits.minStorageBufferOffsetAlignment = adapterLimits.limits.minStorageBufferOffsetAlignment;
	deviceReqs.limits.maxVertexBuffers = adapterLimits.limits.maxVertexBuffers;
	deviceReqs.limits.maxVertexAttributes = adapterLimits.limits.maxVertexAttributes;
	deviceReqs.limits.maxVertexBufferArrayStride = adapterLimits.limits.maxVertexBufferArrayStride;
	deviceReqs.limits.maxInterStageShaderComponents = adapterLimits.limits.maxInterStageShaderComponents;
	deviceReqs.limits.maxComputeWorkgroupStorageSize = adapterLimits.limits.maxComputeWorkgroupStorageSize;
	deviceReqs.limits.maxComputeInvocationsPerWorkgroup = adapterLimits.limits.maxComputeInvocationsPerWorkgroup;
	deviceReqs.limits.maxComputeWorkgroupSizeX = adapterLimits.limits.maxComputeWorkgroupSizeX;
	deviceReqs.limits.maxComputeWorkgroupSizeY = adapterLimits.limits.maxComputeWorkgroupSizeY;
	deviceReqs.limits.maxComputeWorkgroupSizeZ = adapterLimits.limits.maxComputeWorkgroupSizeZ;
	deviceReqs.limits.maxComputeWorkgroupsPerDimension = adapterLimits.limits.maxComputeWorkgroupsPerDimension;
	
	

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
	pipelineDescriptor.primitive.frontFace = FrontFace::CW;
	pipelineDescriptor.primitive.cullMode = CullMode::Back;

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



	
	//Model model("F:\\Extracted\\ESO\\sfpts\\model\\1520043.gr2", device, queue); //elstemple
	Model model("F:\\Extracted\\ESO\\sfpts\\model\\2774573.gr2", device, queue); //bendu

	//vertex buffer layouts
	vector<VertexBufferLayout> vertexBufferLayouts(2);


	vector<VertexAttribute> vertexBufferAttributes(2);
	//position
	vertexBufferAttributes[0].shaderLocation = 0;
	vertexBufferAttributes[0].offset = 0;
	vertexBufferAttributes[0].format = VertexFormat::Float32x3;
	//color
	vertexBufferAttributes[1].shaderLocation = 1;
	vertexBufferAttributes[1].offset = 4 * sizeof(float);
	vertexBufferAttributes[1].format = VertexFormat::Float32x3;

	vertexBufferLayouts[0].attributeCount = static_cast<uint32_t>(vertexBufferAttributes.size());
	vertexBufferLayouts[0].attributes = vertexBufferAttributes.data();
	vertexBufferLayouts[0].arrayStride = 32;
	vertexBufferLayouts[0].stepMode = VertexStepMode::Vertex;


	

	int instanceCount = 32;

	//create instance vert buffer
	BufferDescriptor instanceVertBufferDesc;
	instanceVertBufferDesc.size = instanceCount * sizeof(mat4);
	instanceVertBufferDesc.usage = BufferUsage::CopyDst | BufferUsage::Vertex;
	instanceVertBufferDesc.mappedAtCreation = false;
	instanceVertBufferDesc.label = "instance vert buffer";
	Buffer instanceVertBuffer = device.createBuffer(instanceVertBufferDesc);

	vector<VertexAttribute> instancePosVertAttribute(4);
	instancePosVertAttribute[0].shaderLocation = 2;
	instancePosVertAttribute[0].offset = 0;
	instancePosVertAttribute[0].format = VertexFormat::Float32x4;
	instancePosVertAttribute[1].shaderLocation = 3;
	instancePosVertAttribute[1].offset = 4 * sizeof(float);
	instancePosVertAttribute[1].format = VertexFormat::Float32x4;
	instancePosVertAttribute[2].shaderLocation = 4;
	instancePosVertAttribute[2].offset = 8 * sizeof(float);
	instancePosVertAttribute[2].format = VertexFormat::Float32x4;
	instancePosVertAttribute[3].shaderLocation = 5;
	instancePosVertAttribute[3].offset = 12 * sizeof(float);
	instancePosVertAttribute[3].format = VertexFormat::Float32x4;
	vertexBufferLayouts[1].attributeCount = static_cast<uint32_t>(instancePosVertAttribute.size());
	vertexBufferLayouts[1].attributes = instancePosVertAttribute.data();
	vertexBufferLayouts[1].arrayStride = sizeof(mat4);
	vertexBufferLayouts[1].stepMode = VertexStepMode::Instance;




	pipelineDescriptor.vertex.bufferCount = static_cast<uint32_t>(vertexBufferLayouts.size());
	pipelineDescriptor.vertex.buffers = vertexBufferLayouts.data();



	int bindingCount = 1;
	std::vector<BindGroupLayoutEntry> uniformLayoutEntries(bindingCount, Default);
	//global uniform layout
	uniformLayoutEntries[0].binding = 0;
	uniformLayoutEntries[0].visibility = ShaderStage::Vertex;
	uniformLayoutEntries[0].buffer.type = BufferBindingType::Uniform;
	uniformLayoutEntries[0].buffer.minBindingSize = sizeof(Uniforms);
	//bindgroup layout
	BindGroupLayoutDescriptor uniformLayoutDesc;
	uniformLayoutDesc.entryCount = bindingCount;
	uniformLayoutDesc.entries = uniformLayoutEntries.data();
	BindGroupLayout uniformLayout = device.createBindGroupLayout(uniformLayoutDesc);


	//global uniform buffer
	BufferDescriptor uniformBufferDesc;
	uniformBufferDesc.size = sizeof(Uniforms);
	uniformBufferDesc.usage = BufferUsage::CopyDst | BufferUsage::Uniform;
	uniformBufferDesc.mappedAtCreation = false;
	uniformBufferDesc.label = "uniform buffer";
	Buffer uniformBuffer = device.createBuffer(uniformBufferDesc);
	

	std::vector<BindGroupEntry> uniformEntries(bindingCount, Default);
	//global uniform binding
	uniformEntries[0].binding = 0;
	uniformEntries[0].buffer = uniformBuffer;
	uniformEntries[0].offset = 0;
	uniformEntries[0].size = sizeof(Uniforms);
	//bindgroup
	BindGroupDescriptor uniformGroupDesc;
	uniformGroupDesc.layout = uniformLayout;
	uniformGroupDesc.entryCount = bindingCount;
	uniformGroupDesc.entries = uniformEntries.data();
	BindGroup uniformGroup = device.createBindGroup(uniformGroupDesc);




	PipelineLayoutDescriptor layoutDesc;
	layoutDesc.bindGroupLayoutCount = 1;
	layoutDesc.bindGroupLayouts = &(WGPUBindGroupLayout)uniformLayout;
	PipelineLayout layout = device.createPipelineLayout(layoutDesc);
	pipelineDescriptor.layout = layout;
	//pipelineDescriptor.layout = nullptr;

	RenderPipeline pipeline = device.createRenderPipeline(pipelineDescriptor);


	Uniforms uniformData;
	vector<mat4> instanceData(instanceCount);
	for (int i = 0; i < instanceData.size(); i++) instanceData[i] = mat4(1);
	queue.writeBuffer(instanceVertBuffer, 0, instanceData.data(), instanceCount * sizeof(mat4)); //also updating rotation speed


	//proj
	float near = 0.001f;
	float far = 100.0f;
	float ratio = ((float)windowWidth) / ((float)windowHeight);
	float fov = glm::radians(45.f);
	uniformData.proj = glm::perspective(fov, ratio, near, far);

	//view
	uniformData.view = glm::translate(mat4(1), vec3(0, 0, -3));
	uniformData.view = glm::rotate(uniformData.view, glm::radians(-60.f), vec3(1, 0, 0));

	//model

	float modelPos[]{ 0.f, 0.f, 0.f };
	float modelRot[] = { 90.f, 0.f, 0.f };

	float modelScale = 1.f;

	uniformData.time = (float)glfwGetTime();
	uniformData.rotationSpeed = 1.0f;
	queue.writeBuffer(uniformBuffer, 0, &uniformData, sizeof(Uniforms));


	//IMGUI
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io; //????
	ImGui_ImplGlfw_InitForOther(window, true);
	ImGui_ImplWGPU_Init(device, 3, swapChainFormat, depthTextureFormat);


	cout << "Hello CMake." << endl;

	while (!glfwWindowShouldClose(window)) {
		glfwPollEvents();

		uniformData.time = (float)glfwGetTime();
		queue.writeBuffer(uniformBuffer, offsetof(Uniforms, time), &uniformData.time, sizeof(float) * 2); //also updating rotation speed

		for (int i = 0; i < instanceCount; i++) {
			instanceData[i] = glm::rotate(mat4(1), uniformData.time * uniformData.rotationSpeed + glm::two_pi<float>() / instanceCount * i, vec3(0.f, 0.f, 1.f));
			instanceData[i] = glm::translate(instanceData[i], vec3(modelPos[0], modelPos[1], modelPos[2]));
			instanceData[i] = glm::scale(instanceData[i], vec3(modelScale));
			instanceData[i] = glm::rotate(instanceData[i], glm::radians(modelRot[2]), vec3(0.f, 0.f, 1.f));
			instanceData[i] = glm::rotate(instanceData[i], glm::radians(modelRot[1]), vec3(0.f, 1.f, 0.f));
			instanceData[i] = glm::rotate(instanceData[i], glm::radians(modelRot[0]), vec3(1.f, 0.f, 0.f));
		}

		//queue.writeBuffer(uniformBuffer, offsetof(Uniforms, model), &uniformData.model, sizeof(mat4));
		queue.writeBuffer(instanceVertBuffer, 0, instanceData.data(), instanceCount * sizeof(mat4)); //also updating rotation speed


		
		TextureView nextFrame = swapChain.getCurrentTextureView();
		if (!nextFrame) {
			std::cerr << "Cannot acquire next swap chain texture" << std::endl;
			break;
		}
		//std::cout << "next frame: " << nextFrame << std::endl;




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
		renderPass.setBindGroup(0, uniformGroup, 0, nullptr);
		renderPass.setVertexBuffer(0, model.vertBuffer, 0, model.vertBufferSize);
		renderPass.setVertexBuffer(1, instanceVertBuffer, 0, instanceCount * sizeof(mat4));

		renderPass.setIndexBuffer(model.idxBuffer, model.idx32 ? IndexFormat::Uint32 : IndexFormat::Uint16, 0, model.idxBufferSize);
		renderPass.drawIndexed(model.idxCount, instanceCount, 0, 0, 0);
		//imgui
		ImGui_ImplGlfw_NewFrame();
		ImGui_ImplWGPU_NewFrame();
		ImGui::NewFrame();

		ImGui::Text("Test");
		ImGui::DragFloat3("Position", modelPos, 0.01f);
		ImGui::DragFloat3("Rotation", modelRot);
		ImGui::DragFloat("Scale", &modelScale, 0.01f);
		ImGui::DragFloat("Speed", &uniformData.rotationSpeed, 0.01f);
		ImGui::Render();
		ImGui_ImplWGPU_RenderDrawData(ImGui::GetDrawData(), renderPass);
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
