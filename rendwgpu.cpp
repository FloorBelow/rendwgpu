// rendwgpu.cpp : Defines the entry point for the application.
//

#include <iostream>
#include "GLFW\glfw3.h"

#include "webgpu\webgpu.h"
#include "webgpu\wgpu.h"

#define WEBGPU_CPP_IMPLEMENTATION
#include "webgpu\webgpu.hpp"

#include "glfw3webgpu\glfw3webgpu.h"
#include <cassert>
#include <fstream>

using namespace std;
using namespace wgpu;

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
	glfwInit();
	if (!glfwInit()) {
		std::cerr << "Could not initialize GLFW!" << std::endl;
		return 1;
	}

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
	GLFWwindow* window = glfwCreateWindow(1600, 900, "render", NULL, NULL);
	if (!window) {
		std::cerr << "Could not open window!" << std::endl;
		glfwTerminate();
		return 1;
	}

	//INSTANCE
	Instance instance = wgpu::createInstance(InstanceDescriptor());

	Surface surface = glfwGetWGPUSurface(instance, window);
	 

	RequestAdapterOptions adapterOptions;
	adapterOptions.compatibleSurface = surface;
	Adapter adapter = instance.requestAdapter(adapterOptions);

	//DEVICE
	DeviceDescriptor deviceDescriptor;
	deviceDescriptor.label = "Default Device";
	deviceDescriptor.requiredFeaturesCount = 0;
	deviceDescriptor.requiredLimits = nullptr;
	deviceDescriptor.defaultQueue.label = "Default Queue";
	Device device = adapter.requestDevice(deviceDescriptor);
	
	//TODO figure out what this means
	auto onDeviceError = [](WGPUErrorType type, char const* message, void* /* pUserData */) {
		std::cout << "Uncaptured device error: type " << type;
		if (message) std::cout << " (" << message << ")";
		std::cout << std::endl;
	};
	wgpuDeviceSetUncapturedErrorCallback(device, onDeviceError, nullptr /* pUserData */);

	
	//SWAPCHAIN
	SwapChainDescriptor swapChainDescriptor;
	swapChainDescriptor.width = 1600;
	swapChainDescriptor.height = 900;
	swapChainDescriptor.usage = WGPUTextureUsage_RenderAttachment;
	TextureFormat swapChainFormat = wgpuSurfaceGetPreferredFormat(surface, adapter);
	swapChainDescriptor.format = swapChainFormat;
	swapChainDescriptor.presentMode = WGPUPresentMode_Fifo; //TODO - mailbox?
	SwapChain swapChain = device.createSwapChain(surface, swapChainDescriptor);
	
	
	Queue queue = device.getQueue();
	
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

	pipelineDescriptor.depthStencil = nullptr;

	pipelineDescriptor.multisample.count = 1;
	pipelineDescriptor.multisample.mask = ~0u;
	pipelineDescriptor.multisample.alphaToCoverageEnabled = false;

	pipelineDescriptor.layout = nullptr;

	RenderPipeline pipeline = device.createRenderPipeline(pipelineDescriptor);


	cout << "Hello CMake." << endl;

	while (!glfwWindowShouldClose(window)) {
		glfwPollEvents();


		
		TextureView nextFrame = swapChain.getCurrentTextureView();
		if (!nextFrame) {
			std::cerr << "Cannot acquire next swap chain texture" << std::endl;
			break;
		}
		std::cout << "next frame: " << nextFrame << std::endl;




		RenderPassColorAttachment renderPassColorAttachment;
		renderPassColorAttachment.view = nextFrame;
		renderPassColorAttachment.resolveTarget = nullptr;
		renderPassColorAttachment.loadOp = WGPULoadOp_Clear;
		renderPassColorAttachment.storeOp = WGPUStoreOp_Store;
		renderPassColorAttachment.clearValue = WGPUColor{ 0.05, 0.1, 0.11, 1.0 };

		RenderPassDescriptor renderPassDescriptor;
		renderPassDescriptor.label = "Default Render Pass";
		renderPassDescriptor.colorAttachmentCount = 1;
		renderPassDescriptor.colorAttachments = &renderPassColorAttachment;
		renderPassDescriptor.depthStencilAttachment = nullptr;
		renderPassDescriptor.timestampWriteCount = 0;
		renderPassDescriptor.timestampWrites = nullptr;


		//swapChain.present();
		CommandEncoder encoder = device.createCommandEncoder(encoderDescriptor);
		RenderPassEncoder renderPass = encoder.beginRenderPass(renderPassDescriptor);
		renderPass.setPipeline(pipeline);
		renderPass.draw(3, 1, 0, 0);
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
