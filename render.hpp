#pragma once

#include <vpp/device.hpp> // vpp::Device
#include <vpp/queue.hpp> // vpp::Queue
#include <vpp/renderer.hpp> // vpp::SwapchainRenderer
#include <vpp/buffer.hpp> // vpp::Buffer
#include <vpp/pipeline.hpp> // vpp::Pipeline
#include <vpp/descriptor.hpp> // vpp::DescriptorSet

class Engine;

// vpp::RendererBuilder implementation
class Renderer : public vpp::RendererBuilder {
public:
	Renderer(Engine& engine);
	~Renderer() = default;

	void beforeRender(vk::CommandBuffer) override;
	void build(unsigned int, const vpp::RenderPassInstance&) override;
	std::vector<vk::ClearValue> clearValues(unsigned int) override;
	void frame(unsigned int id) override;

	vpp::Device& device() const;

protected:
	Engine& engine_;

	vpp::Pipeline trianglePipeline_;
	vpp::PipelineLayout graphicsLayout_;
	vpp::Buffer vertexBuffer_;
};
