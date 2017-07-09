// Copyright (c) 2017 nyorain
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt

#include <render.hpp>
#include <engine.hpp>

#include <nytl/mat.hpp>
#include <vpp/vk.hpp>
#include <vpp/util/file.hpp>
#include <vpp/renderPass.hpp>
#include <vpp/swapchain.hpp>

#include <dlg/dlg.hpp> // dlg
using namespace dlg::literals;

// shader data
#include <shaders/compiled/triangle.frag.h>
#include <shaders/compiled/triangle.vert.h>

vk::Pipeline createGraphicsPipelines(const vpp::Device&, vk::RenderPass, vk::PipelineLayout,
	vk::SampleCountBits sampleCount);
vpp::RenderPass createRenderPass(const vpp::Swapchain&, vk::SampleCountBits sampleCount);

Renderer::Renderer(Engine& engine, vk::SampleCountBits sampleCount) : engine_(engine)
{
	// buffer
	vk::BufferCreateInfo bufInfo;
	bufInfo.usage = vk::BufferUsageBits::vertexBuffer;
	bufInfo.size = sizeof(float) * 3 * 5;
	auto mem = device().memoryTypeBits(vk::MemoryPropertyBits::hostVisible);
	vertexBuffer_ = {device(), bufInfo, mem};

	// fill it
	vertexBuffer_.assureMemory();
	float data[] = {
		// pos	  // color
		-.8f, .5f,  0.5f, 0.8f, 0.5f,
		.8f, .5f,   0.2f, 0.5f, 0.5f,
		0.f, -.5f,   0.5f, 0.5f, 0.3f
	};

	auto mmap = vertexBuffer_.memoryMap();
	std::memcpy(mmap.ptr(), data, sizeof(float) * 3 * 5);

	acquireComplete_ = {device()};
	renderComplete_ = {device()};

	setupPipeline(sampleCount);
}

void Renderer::setupPipeline(vk::SampleCountBits sampleCount)
{
	// graphics
	sampleCount_ = sampleCount;
	graphicsLayout_ = {device(), {}, {}};
	renderPass_ = createRenderPass(engine_.swapchain(), sampleCount);
	auto pipeline = createGraphicsPipelines(device(), renderPass_, graphicsLayout_, sampleCount);
	trianglePipeline_ = {device(), pipeline};

	createBuffers();
}

void Renderer::createMultisampleTarget()
{
	auto width = engine_.swapchain().size().width;
	auto height = engine_.swapchain().size().height;

	// img
	vk::ImageCreateInfo img;
	img.imageType = vk::ImageType::e2d;
	img.format = engine_.swapchain().format();
	img.extent.width = width;
	img.extent.height = height;
	img.extent.depth = 1;
	img.mipLevels = 1;
	img.arrayLayers = 1;
	img.sharingMode = vk::SharingMode::exclusive;
	img.tiling = vk::ImageTiling::optimal;
	img.samples = sampleCount_;
	img.usage = vk::ImageUsageBits::transientAttachment | vk::ImageUsageBits::colorAttachment;
	img.initialLayout = vk::ImageLayout::undefined;

	// view
	vk::ImageViewCreateInfo view;
	view.viewType = vk::ImageViewType::e2d;
	view.format = img.format;
	view.components.r = vk::ComponentSwizzle::r;
	view.components.g = vk::ComponentSwizzle::g;
	view.components.b = vk::ComponentSwizzle::b;
	view.components.a = vk::ComponentSwizzle::a;
	view.subresourceRange.aspectMask = vk::ImageAspectBits::color;
	view.subresourceRange.levelCount = 1;
	view.subresourceRange.layerCount = 1;

	// create the viewable image
	// will set the created image in the view info for us
	multisampleTarget_ = {device(), {img, view}};
}

void Renderer::createBuffers()
{
	auto msaa = sampleCount_ != vk::SampleCountBits::e1;
	if(msaa) createMultisampleTarget();

	// create render buffers
	vpp::Framebuffer::ExtAttachments attachments;
	auto swapchainID = 0u;

	if(msaa) {
		attachments = {{0, multisampleTarget_.vkImageView()}};
		swapchainID = 1u;
	}

	// render buffers
	auto images = engine_.swapchain().renderBuffers();
	renderQueue_ = device().queue(vk::QueueBits::graphics);
	auto cmdBuffers = device().commandProvider().get(renderQueue_->family(), images.size());

	renderBuffers_.resize(images.size());
	for(auto i = 0u; i < renderBuffers_.size(); ++i) {
		renderBuffers_[i].commandBuffer = std::move(cmdBuffers[i]);
		attachments[swapchainID] = images[i].imageView;
		renderBuffers_[i].framebuffer = {device(), renderPass_, engine_.swapchain().size(),
			{}, attachments};
	}

	record();
}

void Renderer::record()
{
	// record buffers
	vk::CommandBufferBeginInfo cmdBufInfo;
	auto clearValues = std::vector<vk::ClearValue>{{{{{0.0f, 0.0f, 0.0f, 1.0f}}}}}; // clear

	auto width = engine_.swapchain().size().width;
	auto height = engine_.swapchain().size().height;

	vk::RenderPassBeginInfo rbeginInfo;
	rbeginInfo.renderPass = renderPass_;
	rbeginInfo.renderArea = {{0, 0}, {width, height}};
	rbeginInfo.clearValueCount = clearValues.size();
	rbeginInfo.pClearValues = clearValues.data();

	vk::Viewport viewport;
	viewport.width = width;
	viewport.height = height;
	viewport.minDepth = 0.f;
	viewport.maxDepth = 1.f;

	vk::Rect2D scissor;
	scissor.extent = {width, height};
	scissor.offset = {0, 0};

	for(auto& buf : renderBuffers_) {
		rbeginInfo.framebuffer = buf.framebuffer;

		vk::beginCommandBuffer(buf.commandBuffer, cmdBufInfo);
		vk::cmdBeginRenderPass(buf.commandBuffer, rbeginInfo, vk::SubpassContents::eInline);

		vk::cmdSetViewport(buf.commandBuffer, 0, 1, viewport);
		vk::cmdSetScissor(buf.commandBuffer, 0, 1, scissor);

		build(buf.commandBuffer);

		vk::cmdEndRenderPass(buf.commandBuffer);
		vk::endCommandBuffer(buf.commandBuffer);
	}
}

void Renderer::build(vk::CommandBuffer cmdBuffer)
{
	vk::cmdBindPipeline(cmdBuffer, vk::PipelineBindPoint::graphics, trianglePipeline_);
	vk::cmdBindVertexBuffers(cmdBuffer, 0, {vertexBuffer_}, {0});
	vk::cmdDraw(cmdBuffer, 3, 1, 0, 0);
}

void Renderer::renderBlock(const vpp::Queue& present)
{
	unsigned int currentBuffer;
	auto result = engine_.swapchain().acquire(currentBuffer, acquireComplete_);
	if(result == vk::Result::errorOutOfDateKHR) {
		// we simply skip the frame since the engine should resize/recreate
		// the swapchain (and the renderer data) in the next loop during
		// dispatching of events since we must have gotten a resize event.
		dlg_info("render"_module, "Swapchain out of date. Skipping frame");
		return;
	}

	auto& cmdBuf = renderBuffers_[currentBuffer].commandBuffer;
	vk::PipelineStageFlags flag = vk::PipelineStageBits::colorAttachmentOutput;

	vk::SubmitInfo submitInfo;
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = &acquireComplete_.vkHandle();
	submitInfo.pWaitDstStageMask = &flag;
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = &renderComplete_.vkHandle();

	vpp::CommandExecutionState execState;
	device().submitManager().add(*renderQueue_, {cmdBuf}, submitInfo, &execState);
	device().submitManager().submit(*renderQueue_);

	engine_.swapchain().present(present, currentBuffer, renderComplete_);
	execState.wait();
}

vpp::Device& Renderer::device() const { return engine_.vulkanDevice(); }

// utility
vk::Pipeline createGraphicsPipelines(const vpp::Device& device,
	vk::RenderPass renderPass, vk::PipelineLayout layout, vk::SampleCountBits sampleCount)
{
	// auto msaa = sampleCount != vk::SampleCountBits::e1;
	auto lightVertex = vpp::ShaderModule(device, triangle_vert_data);
	auto lightFragment = vpp::ShaderModule(device, triangle_frag_data);

	vpp::ShaderProgram lightStages({
		{lightVertex, vk::ShaderStageBits::vertex},
		{lightFragment, vk::ShaderStageBits::fragment}
	});

	vk::GraphicsPipelineCreateInfo trianglePipe;

	trianglePipe.renderPass = renderPass;
	trianglePipe.layout = layout;

	trianglePipe.stageCount = lightStages.vkStageInfos().size();
	trianglePipe.pStages = lightStages.vkStageInfos().data();

	constexpr auto stride = sizeof(float) * 5; // vec2 pos, vec3 color
	vk::VertexInputBindingDescription bufferBinding {0, stride, vk::VertexInputRate::vertex};

	// vertex position attribute
	vk::VertexInputAttributeDescription attributes[2];
	attributes[0].format = vk::Format::r32g32Sfloat; // pos
	attributes[1].format = vk::Format::r32g32b32Sfloat; // color
	attributes[1].location = 1;

	vk::PipelineVertexInputStateCreateInfo vertexInfo;
	vertexInfo.vertexBindingDescriptionCount = 1;
	vertexInfo.pVertexBindingDescriptions = &bufferBinding;
	vertexInfo.vertexAttributeDescriptionCount = 2;
	vertexInfo.pVertexAttributeDescriptions = attributes;
	trianglePipe.pVertexInputState = &vertexInfo;

	vk::PipelineInputAssemblyStateCreateInfo assemblyInfo;
	assemblyInfo.topology = vk::PrimitiveTopology::triangleFan;
	trianglePipe.pInputAssemblyState = &assemblyInfo;

	vk::PipelineRasterizationStateCreateInfo rasterizationInfo;
	rasterizationInfo.polygonMode = vk::PolygonMode::fill;
	rasterizationInfo.cullMode = vk::CullModeBits::none;
	rasterizationInfo.frontFace = vk::FrontFace::counterClockwise;
	rasterizationInfo.depthClampEnable = false;
	rasterizationInfo.rasterizerDiscardEnable = false;
	rasterizationInfo.depthBiasEnable = false;
	rasterizationInfo.lineWidth = 1.f;
	trianglePipe.pRasterizationState = &rasterizationInfo;

	vk::PipelineMultisampleStateCreateInfo multisampleInfo;
	multisampleInfo.rasterizationSamples = sampleCount;
	multisampleInfo.sampleShadingEnable = false;
	multisampleInfo.alphaToCoverageEnable = false;
	trianglePipe.pMultisampleState = &multisampleInfo;

	vk::PipelineColorBlendAttachmentState blendAttachment;
	blendAttachment.blendEnable = true;
	blendAttachment.alphaBlendOp = vk::BlendOp::add;
	blendAttachment.srcColorBlendFactor = vk::BlendFactor::srcAlpha;
	blendAttachment.dstColorBlendFactor = vk::BlendFactor::oneMinusSrcAlpha;
	blendAttachment.srcAlphaBlendFactor = vk::BlendFactor::one;
	blendAttachment.dstAlphaBlendFactor = vk::BlendFactor::zero;
	blendAttachment.colorWriteMask =
		vk::ColorComponentBits::r |
		vk::ColorComponentBits::g |
		vk::ColorComponentBits::b |
		vk::ColorComponentBits::a;

	vk::PipelineColorBlendStateCreateInfo blendInfo;
	blendInfo.attachmentCount = 1;
	blendInfo.pAttachments = &blendAttachment;
	trianglePipe.pColorBlendState = &blendInfo;

	vk::PipelineViewportStateCreateInfo viewportInfo;
	viewportInfo.scissorCount = 1;
	viewportInfo.viewportCount = 1;
	trianglePipe.pViewportState = &viewportInfo;

	const auto dynStates = {vk::DynamicState::viewport, vk::DynamicState::scissor};

	vk::PipelineDynamicStateCreateInfo dynamicInfo;
	dynamicInfo.dynamicStateCount = dynStates.size();
	dynamicInfo.pDynamicStates = dynStates.begin();
	trianglePipe.pDynamicState = &dynamicInfo;

	// setup cache
	constexpr auto cacheName = "graphicsCache.bin";

	vpp::PipelineCache cache;
	if(vpp::fileExists(cacheName)) cache = {device, cacheName};
	else cache = {device};

	vk::Pipeline ret;
	vk::createGraphicsPipelines(device, cache, 1, trianglePipe, nullptr, ret);
	vpp::save(cache, cacheName);

	return ret;
}

vpp::RenderPass createRenderPass(const vpp::Swapchain& swapchain, vk::SampleCountBits sampleCount)
{
	vk::AttachmentDescription attachments[2] {};
	auto msaa = sampleCount != vk::SampleCountBits::e1;

	auto swapchainID = 0u;
	if(msaa) {
		// multisample color attachment
		attachments[0].format = swapchain.format();
		attachments[0].samples = sampleCount;
		attachments[0].loadOp = vk::AttachmentLoadOp::clear;
		attachments[0].storeOp = vk::AttachmentStoreOp::dontCare;
		attachments[0].stencilLoadOp = vk::AttachmentLoadOp::dontCare;
		attachments[0].stencilStoreOp = vk::AttachmentStoreOp::dontCare;
		attachments[0].initialLayout = vk::ImageLayout::undefined;
		attachments[0].finalLayout = vk::ImageLayout::presentSrcKHR;

		swapchainID = 1u;
	}

	// swapchain color attachments we want to resolve to
	attachments[swapchainID].format = swapchain.format();
	attachments[swapchainID].samples = vk::SampleCountBits::e1;
	if(msaa) attachments[swapchainID].loadOp = vk::AttachmentLoadOp::dontCare;
	else attachments[swapchainID].loadOp = vk::AttachmentLoadOp::clear;
	attachments[swapchainID].storeOp = vk::AttachmentStoreOp::store;
	attachments[swapchainID].stencilLoadOp = vk::AttachmentLoadOp::dontCare;
	attachments[swapchainID].stencilStoreOp = vk::AttachmentStoreOp::dontCare;
	attachments[swapchainID].initialLayout = vk::ImageLayout::undefined;
	attachments[swapchainID].finalLayout = vk::ImageLayout::presentSrcKHR;

	// refs
	vk::AttachmentReference colorReference;
	colorReference.attachment = 0;
	colorReference.layout = vk::ImageLayout::colorAttachmentOptimal;

	vk::AttachmentReference resolveReference;
	resolveReference.attachment = 1;
	resolveReference.layout = vk::ImageLayout::colorAttachmentOptimal;

	// deps
	std::array<vk::SubpassDependency, 2> dependencies;

	dependencies[0].srcSubpass = vk::subpassExternal;
	dependencies[0].dstSubpass = 0;
	dependencies[0].srcStageMask = vk::PipelineStageBits::bottomOfPipe;
	dependencies[0].dstStageMask = vk::PipelineStageBits::colorAttachmentOutput;
	dependencies[0].srcAccessMask = vk::AccessBits::memoryRead;
	dependencies[0].dstAccessMask = vk::AccessBits::colorAttachmentRead |
		vk::AccessBits::colorAttachmentWrite;
	dependencies[0].dependencyFlags = vk::DependencyBits::byRegion;

	dependencies[1].srcSubpass = 0;
	dependencies[1].dstSubpass = vk::subpassExternal;
	dependencies[1].srcStageMask = vk::PipelineStageBits::colorAttachmentOutput;;
	dependencies[1].dstStageMask = vk::PipelineStageBits::bottomOfPipe;
	dependencies[1].srcAccessMask = vk::AccessBits::colorAttachmentRead |
		vk::AccessBits::colorAttachmentWrite;
	dependencies[1].dstAccessMask = vk::AccessBits::memoryRead;
	dependencies[1].dependencyFlags = vk::DependencyBits::byRegion;

	// only subpass
	vk::SubpassDescription subpass;
	subpass.pipelineBindPoint = vk::PipelineBindPoint::graphics;
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &colorReference;
	if(sampleCount != vk::SampleCountBits::e1)
		subpass.pResolveAttachments = &resolveReference;

	vk::RenderPassCreateInfo renderPassInfo;
	renderPassInfo.attachmentCount = 1 + msaa;
	renderPassInfo.pAttachments = attachments;
	renderPassInfo.subpassCount = 1;
	renderPassInfo.pSubpasses = &subpass;

	if(msaa) {
		renderPassInfo.dependencyCount = dependencies.size();
		renderPassInfo.pDependencies = dependencies.data();
	}

	return {swapchain.device(), renderPassInfo};
}
