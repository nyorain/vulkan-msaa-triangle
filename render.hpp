#pragma once

#include <vpp/device.hpp> // vpp::Device
#include <vpp/queue.hpp> // vpp::Queue
#include <vpp/buffer.hpp> // vpp::Buffer
#include <vpp/pipeline.hpp> // vpp::Pipeline
#include <vpp/descriptor.hpp> // vpp::DescriptorSet
#include <vpp/commandBuffer.hpp>
#include <vpp/framebuffer.hpp>
#include <vpp/sync.hpp>
#include <vpp/queue.hpp>

class Engine;

// vpp::RendererBuilder implementation
class Renderer {
public:
	Renderer(Engine& engine);
	~Renderer() = default;

	void createBuffers();
	void renderBlock(const vpp::Queue& present);

	vpp::Device& device() const;

protected:
	void createMultisampleTarget();
	void record();
	void build(vk::CommandBuffer cmd);

protected:
	Engine& engine_;

	vpp::Pipeline trianglePipeline_;
	vpp::PipelineLayout graphicsLayout_;
	vpp::Buffer vertexBuffer_;
	vpp::ViewableImage multisampleTarget_;
	const vpp::Queue* renderQueue_;

	vpp::Semaphore acquireComplete_;
	vpp::Semaphore renderComplete_;

	struct RenderBuffer {
		vpp::CommandBuffer commandBuffer;
		vpp::Framebuffer framebuffer;
	};

	std::vector<RenderBuffer> renderBuffers_;
};
