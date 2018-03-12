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

// shader data
#include <shaders/triangle.frag.h>
#include <shaders/triangle.vert.h>

vk::Pipeline createGraphicsPipelines(const vpp::Device&, vk::RenderPass, 
	vk::PipelineLayout, vk::SampleCountBits);
vpp::RenderPass createRenderPass(const vpp::Device&, vk::Format, 
	vk::SampleCountBits);

Renderer::Renderer(const vpp::Device& dev, vk::SurfaceKHR surface, 
	vk::SampleCountBits samples, const vpp::Queue& present)
{
	// FIXME: size
	// pipeline
	sampleCount_ = samples;
	scInfo_ = vpp::swapchainCreateInfo(dev, surface, {800u, 500u});

	graphicsLayout_ = {dev, {}, {}};
	renderPass_ = createRenderPass(dev, scInfo_.imageFormat, samples);

	// buffer
	vk::BufferCreateInfo bufInfo;
	bufInfo.usage = vk::BufferUsageBits::vertexBuffer;
	bufInfo.size = sizeof(float) * 3 * 5;
	auto mem = dev.memoryTypeBits(vk::MemoryPropertyBits::hostVisible);
	vertexBuffer_ = {dev, bufInfo, mem};

	// fill it
	vertexBuffer_.ensureMemory();
	float data[] = {
		// pos	  // color
		-.8f, .5f,  0.5f, 0.8f, 0.5f,
		.8f, .5f,   0.2f, 0.5f, 0.5f,
		0.f, -.5f,   0.5f, 0.5f, 0.3f
	};

	auto mmap = vertexBuffer_.memoryMap();
	std::memcpy(mmap.ptr(), data, sizeof(float) * 3 * 5);

	auto pipeline = createGraphicsPipelines(dev, renderPass_, 
		graphicsLayout_, sampleCount_);
	trianglePipeline_ = {dev, pipeline};

	// init renderer
	vpp::DefaultRenderer::init(renderPass_, scInfo_, present);
}

void Renderer::createMultisampleTarget(const vk::Extent2D& size)
{
	auto width = size.width;
	auto height = size.height;

	// img
	vk::ImageCreateInfo img;
	img.imageType = vk::ImageType::e2d;
	img.format = scInfo_.imageFormat;
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
	multisampleTarget_ = {device(), img, view};
}

void Renderer::record(const RenderBuffer& buf)
{
	static const auto clearValue = vk::ClearValue {{0.f, 0.f, 0.f, 1.f}};
	const auto width = scInfo_.imageExtent.width;
	const auto height = scInfo_.imageExtent.height;

	auto cmdBuf = buf.commandBuffer;
	vk::beginCommandBuffer(cmdBuf, {});
	vk::cmdBeginRenderPass(cmdBuf, {
		renderPass(),
		buf.framebuffer,
		{0u, 0u, width, height},
		1,
		&clearValue
	}, {});

	vk::Viewport vp {0.f, 0.f, (float) width, (float) height, 0.f, 1.f};
	vk::cmdSetViewport(cmdBuf, 0, 1, vp);
	vk::cmdSetScissor(cmdBuf, 0, 1, {0, 0, width, height});

	vk::cmdBindPipeline(cmdBuf, vk::PipelineBindPoint::graphics, trianglePipeline_);
	vk::cmdBindVertexBuffers(cmdBuf, 0, {vertexBuffer_}, {0});
	vk::cmdDraw(cmdBuf, 3, 1, 0, 0);

	vk::cmdEndRenderPass(cmdBuf);
	vk::endCommandBuffer(cmdBuf);
}

void Renderer::resize(nytl::Vec2ui size)
{
	vpp::DefaultRenderer::resize({size[0], size[1]}, scInfo_);
}

void Renderer::samples(vk::SampleCountBits samples)
{
	sampleCount_ = samples;
	if(sampleCount_ != vk::SampleCountBits::e1) {
		createMultisampleTarget(scInfo_.imageExtent);
	}

	renderPass_ = createRenderPass(device(), scInfo_.imageFormat, samples);
	vpp::DefaultRenderer::renderPass_ = renderPass_;
	auto pipeline = createGraphicsPipelines(device(), renderPass_, 
		graphicsLayout_, sampleCount_);
	trianglePipeline_ = {device(), pipeline};

	initBuffers(scInfo_.imageExtent, renderBuffers_);
	invalidate();
}

void Renderer::initBuffers(const vk::Extent2D& size, 
	nytl::Span<RenderBuffer> bufs)
{
	if(sampleCount_ != vk::SampleCountBits::e1) {
		createMultisampleTarget(scInfo_.imageExtent);
		vpp::DefaultRenderer::initBuffers(size, bufs, 
			{multisampleTarget_.vkImageView()});
	} else {
		vpp::DefaultRenderer::initBuffers(size, bufs, {});
	}
}

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
	vpp::PipelineCache cache {device, cacheName};

	vk::Pipeline ret;
	vk::createGraphicsPipelines(device, cache, 1, trianglePipe, nullptr, ret);
	vpp::save(cache, cacheName);

	return ret;
}

vpp::RenderPass createRenderPass(const vpp::Device& dev,
	vk::Format format, vk::SampleCountBits sampleCount)
{
	vk::AttachmentDescription attachments[2] {};
	auto msaa = sampleCount != vk::SampleCountBits::e1;

	auto swapchainID = 0u;
	if(msaa) {
		// multisample color attachment
		attachments[0].format = format;
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
	attachments[swapchainID].format = format;
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

	return {dev, renderPassInfo};
}
