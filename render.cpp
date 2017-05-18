#include <render.hpp>
#include <engine.hpp>

#include <nytl/mat.hpp>
#include <vpp/vk.hpp>
#include <vpp/util/file.hpp>
#include <vpp/bufferOps.hpp>
#include <ny/log.hpp>

// shader data
#include <shaders/compiled/triangle.frag.h>
#include <shaders/compiled/triangle.vert.h>

vk::Pipeline createGraphicsPipelines(const vpp::Device&, vk::RenderPass,
	vk::PipelineLayout);

Renderer::Renderer(Engine& engine) : engine_(engine)
{
	// graphics
	graphicsLayout_ = {device(), {}, {}};
	auto& renderPass = engine_.vulkanRenderPass();
	auto pipeline = createGraphicsPipelines(device(), renderPass, graphicsLayout_);
	trianglePipeline_ = {device(), pipeline};

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
		-.5f, .5f,  1.f, 0.f, 0.f,
		.5f, .5f,   0.f, 1.f, 0.f,
		0.f, -.5f,   0.f, 0.f, 1.f
	};

	auto mmap = vertexBuffer_.memoryMap();
	std::memcpy(mmap.ptr(), data, sizeof(float) * 3 * 5);
}

void Renderer::build(unsigned int, const vpp::RenderPassInstance& ini)
{
	auto cmdBuffer = ini.vkCommandBuffer();

	vk::cmdBindPipeline(cmdBuffer, vk::PipelineBindPoint::graphics, trianglePipeline_);
	vk::cmdBindVertexBuffers(cmdBuffer, 0, {vertexBuffer_}, {0});
	vk::cmdDraw(cmdBuffer, 3, 1, 0, 0);

}
std::vector<vk::ClearValue> Renderer::clearValues(unsigned int)
{
	return {{{{{0.0f, 0.0f, 0.0f, 1.0f}}}}};
}
void Renderer::frame(unsigned int)
{
	// nothing to do
}

void Renderer::beforeRender(vk::CommandBuffer)
{
	// nothing to do
}

vpp::Device& Renderer::device() const { return engine_.vulkanDevice(); }

// utility
vk::Pipeline createGraphicsPipelines(const vpp::Device& device,
	vk::RenderPass renderPass, vk::PipelineLayout layout)
{
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
	multisampleInfo.rasterizationSamples = vk::SampleCountBits::e1;
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
