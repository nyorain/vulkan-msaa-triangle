// Copyright (c) 2017 nyorain
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt

#include <engine.hpp>
#include <window.hpp>
#include <render.hpp>

#include <ny/backend.hpp> // ny::Backend
#include <ny/appContext.hpp> // ny::AppContext
#include <ny/windowContext.hpp> // ny::WindowContext
#include <ny/windowListener.hpp> // ny::WindowListener
#include <ny/windowSettings.hpp> // ny::WindowSettings
#include <ny/loopControl.hpp> // ny::LoopControl

#include <vpp/instance.hpp> // vpp::Instance
#include <vpp/device.hpp> // vpp::Device
#include <vpp/queue.hpp> // vpp::Queue
#include <vpp/swapchain.hpp> // vpp::Swapchain
#include <vpp/renderer.hpp> // vpp::SwapchainRenderer
#include <vpp/debug.hpp> // vpp::DebugCallback

#include <dlg/dlg.hpp> // dlg
using namespace dlg::literals;

#include <chrono>
using Clock = std::chrono::high_resolution_clock;

struct Engine::Impl {
	std::unique_ptr<ny::AppContext> appContext_;
	vpp::Instance instance;
	std::unique_ptr<vpp::DebugCallback> debugCallback;
	std::unique_ptr<ny::WindowContext> windowContext_;
	std::unique_ptr<vpp::Device> device;
	vpp::Swapchain swapchain {};
	const vpp::Queue* presentQueue {};
	vpp::DefaultSwapchainSettings scSettings;

	Clock::time_point lastFrame_; // used for frame delta

	MainWindowListener windowListener;
	std::unique_ptr<Renderer> renderer_ {};

	Impl(Engine& engine) : windowListener(engine) {}
};

Engine::Engine()
{
	// for now hardcoded stuff
	constexpr auto startSize = nytl::Vec2ui{1100, 800};
	constexpr auto useValidation = true;
	constexpr auto startMsaa = vk::SampleCountBits::e1;
	constexpr auto layerName = "VK_LAYER_LUNARG_standard_validation";

	impl_ = std::make_unique<Impl>(*this);

	// ny backend and appContext
	auto& backend = ny::Backend::choose();
	if(!backend.vulkan())
		throw std::runtime_error("Engine: ny backend has no vulkan support!");

	impl_->appContext_ = backend.createAppContext();

	// vulkan init
	// instance
	auto iniExtensions = impl_->appContext_->vulkanExtensions();
	iniExtensions.push_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);

	vk::ApplicationInfo appInfo ("msaa-triangle", 1, "msaa-triangle", 1, VK_API_VERSION_1_0);
	vk::InstanceCreateInfo instanceInfo;
	instanceInfo.pApplicationInfo = &appInfo;

	instanceInfo.enabledExtensionCount = iniExtensions.size();
	instanceInfo.ppEnabledExtensionNames = iniExtensions.data();

	if(useValidation) {
		instanceInfo.enabledLayerCount = 1;
		instanceInfo.ppEnabledLayerNames = &layerName;
	}

	try {
		impl_->instance = {instanceInfo};
		if(!impl_->instance.vkInstance())
			throw std::runtime_error("vkCreateInstance returned a nullptr");
	} catch(const std::exception& error) {
		dlg_source("Engine"_module);
		dlg_error("Vulkan instance creation failed: {}", error.what());
		dlg_error("\tThis may indicate that your system that does support vulkan");
		dlg_error("\tThis application requires vulkan to work; rethrowing");
		throw;
	}

	// debug callback
	if(useValidation)
		impl_->debugCallback = std::make_unique<vpp::DebugCallback>(impl_->instance);

	// init ny window
	auto vkSurface = vk::SurfaceKHR {};
	auto ws = ny::WindowSettings {};

	ws.surface = ny::SurfaceType::vulkan;
	ws.listener = &impl_->windowListener;
	ws.size = startSize;
	ws.vulkan.instance = (VkInstance) impl_->instance.vkHandle();
	ws.vulkan.storeSurface = &(std::uintptr_t&) (vkSurface);

	impl_->windowContext_ = impl_->appContext_->createWindowContext(ws);

	// finish vpp init
	impl_->device = std::make_unique<vpp::Device>(impl_->instance, vkSurface, impl_->presentQueue);
	auto& dev = *impl_->device;

	impl_->scSettings.prefPresentMode = vk::PresentModeKHR::immediate;
	impl_->scSettings.errorAction = vpp::DefaultSwapchainSettings::ErrorAction::none;
	impl_->swapchain = {dev, vkSurface, {startSize[0], startSize[1]}, impl_->scSettings};

	// renderer
	impl_->renderer_ = std::make_unique<Renderer>(*this, startMsaa);
}

Engine::~Engine()
{
}

void Engine::mainLoop()
{
	constexpr auto printFrames = true;

	using secf = std::chrono::duration<float, std::ratio<1, 1>>;

	auto lastFrame = Clock::now();
	auto secCounter = 0.f;
	auto fpsCounter = 0u;

	run_ = true;

	// TODO: to make this work on android an additional idle-check
	// loop is needed. See other android-working implementations using ny and
	// vpp for examples.
	while(run_) {
		if(!impl_->appContext_->dispatchEvents()) {
			dlg_info("::Engine::mainLoop"_src, "dispatchEvents returned false");
			return;
		}

		auto now = Clock::now();
		auto deltaCount = std::chrono::duration_cast<secf>(now - lastFrame).count();
		lastFrame = now;

		renderer().renderBlock(*impl_->presentQueue);

		if(printFrames) {
			++fpsCounter;
			secCounter += deltaCount;
			if(secCounter >= 1.f) {
				dlg_info("::Engine::mainLoop"_src, "{} fps", fpsCounter);
				secCounter = 0.f;
				fpsCounter = 0;
			}
		}
	}
}

void Engine::resize(nytl::Vec2ui size)
{
	if(!impl_->swapchain)
		return;

	impl_->swapchain.resize({size[0], size[1]}, impl_->scSettings);
	impl_->renderer_->createBuffers();
}

void Engine::stop() { run_ = false; }

// get functions
ny::AppContext& Engine::appContext() const { return *impl_->appContext_; }
ny::WindowContext& Engine::windowContext() const { return *impl_->windowContext_; }

vpp::Instance& Engine::vulkanInstance() const { return impl_->instance; }
vpp::Device& Engine::vulkanDevice() const { return *impl_->device; }
vpp::Swapchain& Engine::swapchain() const { return impl_->swapchain; }
Renderer& Engine::renderer() const { return *impl_->renderer_; }
