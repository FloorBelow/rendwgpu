// rendwgpu.cpp : Defines the entry point for the application.
//

#include <iostream>
#include "GLFW\glfw3.h"

#include "webgpu\webgpu.h"
#include "webgpu\wgpu.h"

#include "glfw3webgpu\glfw3webgpu.h"
#include <cassert>

using namespace std;

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

	WGPUInstanceDescriptor instanceDesc;
	instanceDesc.nextInChain = nullptr;
	WGPUInstance instance = wgpuCreateInstance(&instanceDesc);
	if (!instance) {
		std::cerr << "Could not initialize WebGPU!" << std::endl;
		return 1;
	}

	WGPUSurface surface = glfwGetWGPUSurface(instance, window);

	 
	WGPURequestAdapterOptions adapterOptions;
	adapterOptions.nextInChain = nullptr;
	adapterOptions.compatibleSurface = surface;
	WGPUAdapter adapter = requestAdapter(instance, &adapterOptions);
	if(!adapter) {
		std::cerr << "Could not get adapter!" << std::endl;
		return 1;
	}

	WGPUDeviceDescriptor deviceDescriptor;
	deviceDescriptor.nextInChain = nullptr;
	deviceDescriptor.label = "Default Device";
	deviceDescriptor.requiredFeaturesCount = 0;
	deviceDescriptor.requiredLimits = nullptr;
	deviceDescriptor.defaultQueue.nextInChain = nullptr;
	deviceDescriptor.defaultQueue.label = "Default Queue";

	WGPUDevice device = requestDevice(adapter, &deviceDescriptor);
	if (!device) {
		std::cerr << "Could not get device!" << std::endl;
		return 1;
	}

	//TODO figure out what this means
	auto onDeviceError = [](WGPUErrorType type, char const* message, void* /* pUserData */) {
		std::cout << "Uncaptured device error: type " << type;
		if (message) std::cout << " (" << message << ")";
		std::cout << std::endl;
	};
	wgpuDeviceSetUncapturedErrorCallback(device, onDeviceError, nullptr /* pUserData */);

	
	
	WGPUSwapChainDescriptor swapChainDescriptor;
	swapChainDescriptor.nextInChain = nullptr;
	swapChainDescriptor.width = 1600;
	swapChainDescriptor.height = 900;
	swapChainDescriptor.usage = WGPUTextureUsage_RenderAttachment;
	WGPUTextureFormat swapChainFormat = wgpuSurfaceGetPreferredFormat(surface, adapter);
	swapChainDescriptor.format = swapChainFormat;
	swapChainDescriptor.presentMode = WGPUPresentMode_Fifo; //TODO - mailbox?
	WGPUSwapChain swapChain = wgpuDeviceCreateSwapChain(device, surface, &swapChainDescriptor);
	
	
	WGPUQueue queue = wgpuDeviceGetQueue(device);
	


	/*

	WGPUCommandEncoder encoder = wgpuDeviceCreateCommandEncoder(device, &encoderDescriptor);
	wgpuCommandEncoderInsertDebugMarker(encoder, "Test 1");
	wgpuCommandEncoderInsertDebugMarker(encoder, "Test 2");

	
	

	wgpuQueueSubmit(queue, 1, &commandBuffer);
	*/
	
	WGPUCommandEncoderDescriptor encoderDescriptor;
	encoderDescriptor.nextInChain = nullptr;
	encoderDescriptor.label = "Default Encoder";

	WGPUCommandBufferDescriptor bufferDescriptor;
	bufferDescriptor.nextInChain = nullptr;
	bufferDescriptor.label = "Default command buffer";


	cout << "Hello CMake." << endl;

	while (!glfwWindowShouldClose(window)) {
		glfwPollEvents();


		
		WGPUTextureView nextFrame = wgpuSwapChainGetCurrentTextureView(swapChain);
		if (!nextFrame) {
			std::cerr << "Cannot acquire next swap chain texture" << std::endl;
			break;
		}
		std::cout << "nextTexture: " << nextFrame << std::endl;




		WGPURenderPassColorAttachment renderPassColorAttachment;
		renderPassColorAttachment.view = nextFrame;
		renderPassColorAttachment.resolveTarget = nullptr;
		renderPassColorAttachment.loadOp = WGPULoadOp_Clear;
		renderPassColorAttachment.storeOp = WGPUStoreOp_Store;
		renderPassColorAttachment.clearValue = WGPUColor{ 0.05, 0.1, 0.11, 1.0 };

		WGPURenderPassDescriptor renderPassDescriptor;
		renderPassDescriptor.label = "Default Render Pass";
		renderPassDescriptor.nextInChain = nullptr;
		renderPassDescriptor.colorAttachmentCount = 1;
		renderPassDescriptor.colorAttachments = &renderPassColorAttachment;
		renderPassDescriptor.depthStencilAttachment = nullptr;
		renderPassDescriptor.timestampWriteCount = 0;
		renderPassDescriptor.timestampWrites = nullptr;


		//swapChain.present();
		WGPUCommandEncoder encoder = wgpuDeviceCreateCommandEncoder(device, &encoderDescriptor);
		WGPURenderPassEncoder renderPass = wgpuCommandEncoderBeginRenderPass(encoder, &renderPassDescriptor);
		wgpuRenderPassEncoderEnd(renderPass);

		wgpuTextureViewDrop(nextFrame);


		WGPUCommandBuffer commandBuffer = wgpuCommandEncoderFinish(encoder, &bufferDescriptor);
		wgpuQueueSubmit(queue, 1, &commandBuffer);

		wgpuSwapChainPresent(swapChain);

		
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
