#include <engine.hpp>
#include <window.hpp>
#include <render.hpp>

#include <ny/backend.hpp> // ny::Backend
#include <ny/appContext.hpp> // ny::AppContext
#include <ny/windowContext.hpp> // ny::WindowContext
#include <ny/windowListener.hpp> // ny::WindowListener
#include <ny/windowSettings.hpp> // ny::WindowSettings
#include <ny/loopControl.hpp> // ny::LoopControl
#include <ny/log.hpp> // ny::error

#include <vpp/instance.hpp> // vpp::Instance
#include <vpp/device.hpp> // vpp::Device
#include <vpp/queue.hpp> // vpp::Queue
#include <vpp/swapchain.hpp> // vpp::Swapchain
#include <vpp/renderer.hpp> // vpp::SwapchainRenderer
#include <vpp/debug.hpp> // vpp::DebugCallback

#include <chrono>
using Clock = std::chrono::high_resolution_clock;

constexpr vk::SampleCountBits sampleCounts = vk::SampleCountBits::e8;

struct Engine::Impl {
	std::unique_ptr<ny::AppContext> appContext_;
	vpp::Instance instance;
	std::unique_ptr<vpp::DebugCallback> debugCallback;
	std::unique_ptr<ny::WindowContext> windowContext_;
	std::unique_ptr<vpp::Device> device;
	vpp::Swapchain swapchain {};
	const vpp::Queue* presentQueue {};
	vpp::DefaultSwapchainSettings scSettings;
	vpp::RenderPass renderPass;

	Clock::time_point lastFrame_; // used for frame delta

	MainWindowListener windowListener;
	std::unique_ptr<Renderer> renderer_ {};

	Impl(Engine& engine) : windowListener(engine) {}
};

// utility
vpp::RenderPass createRenderPass(const vpp::Swapchain&);

Engine::Engine()
{
	// settings and (for now) hardcoded stuff
	constexpr auto layerName = "VK_LAYER_LUNARG_standard_validation";
	constexpr auto debug = true;
	constexpr auto startSize = nytl::Vec2ui{1100, 800};

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

	if(debug) {
		instanceInfo.enabledLayerCount = 1;
		instanceInfo.ppEnabledLayerNames = &layerName;
	}

	try {
		impl_->instance = {instanceInfo};
		if(!impl_->instance.vkInstance())
			throw std::runtime_error("vkCreateInstance returned a nullptr");
	} catch(const std::exception& error) {
		ny::error("Engine: Vulkan instance creation failed:\n");
		ny::error(error.what());
		ny::error("\nThis may indicate that your computer does not support vulkan\n");
		ny::error("This application will not work without vulkan support. Rethrowing.\n");
		throw;
	}

	// debug callback
	if(debug) impl_->debugCallback = std::make_unique<vpp::DebugCallback>(impl_->instance);

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
	impl_->renderPass = createRenderPass(impl_->swapchain);

	// renderer
	impl_->renderer_ = std::make_unique<Renderer>(*this);
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
			ny::log("Engine::mainLoop: dispatchEvents returned false");
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
				ny::log(fpsCounter, " fps");
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
vpp::RenderPass& Engine::vulkanRenderPass() const { return impl_->renderPass; }
Renderer& Engine::renderer() const { return *impl_->renderer_; }

// utility impl
vpp::RenderPass createRenderPass(const vpp::Swapchain& swapchain)
{
	vk::AttachmentDescription attachments[2] {};

	// multisample color attachment
	attachments[0].format = swapchain.format();
	attachments[0].samples = sampleCounts;
	attachments[0].loadOp = vk::AttachmentLoadOp::clear;
	attachments[0].storeOp = vk::AttachmentStoreOp::dontCare;
	attachments[0].stencilLoadOp = vk::AttachmentLoadOp::dontCare;
	attachments[0].stencilStoreOp = vk::AttachmentStoreOp::dontCare;
	attachments[0].initialLayout = vk::ImageLayout::undefined;
	attachments[0].finalLayout = vk::ImageLayout::presentSrcKHR;

	// swapchain color attachments we want to resolve to
	attachments[1].format = swapchain.format();
	attachments[1].samples = vk::SampleCountBits::e1;
	attachments[1].loadOp = vk::AttachmentLoadOp::dontCare;
	attachments[1].storeOp = vk::AttachmentStoreOp::store;
	attachments[1].stencilLoadOp = vk::AttachmentLoadOp::dontCare;
	attachments[1].stencilStoreOp = vk::AttachmentStoreOp::dontCare;
	attachments[1].initialLayout = vk::ImageLayout::undefined;
	attachments[1].finalLayout = vk::ImageLayout::presentSrcKHR;

	vk::AttachmentReference colorReference;
	colorReference.attachment = 0;
	colorReference.layout = vk::ImageLayout::colorAttachmentOptimal;

	vk::AttachmentReference resolveReference;
	resolveReference.attachment = 1;
	resolveReference.layout = vk::ImageLayout::colorAttachmentOptimal;

	// only subpass
	vk::SubpassDescription subpass;
	subpass.pipelineBindPoint = vk::PipelineBindPoint::graphics;
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &colorReference;
	subpass.pResolveAttachments = &resolveReference;

	vk::RenderPassCreateInfo renderPassInfo;
	renderPassInfo.attachmentCount = 2;
	renderPassInfo.pAttachments = attachments;
	renderPassInfo.subpassCount = 1;
	renderPassInfo.pSubpasses = &subpass;

	return {swapchain.device(), renderPassInfo};
}
