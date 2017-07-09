// Copyright (c) 2017 nyorain
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt

#pragma once

#include <vpp/device.hpp> // vpp::Device
#include <vpp/queue.hpp> // vpp::Queue
#include <vpp/buffer.hpp> // vpp::Buffer
#include <vpp/pipeline.hpp> // vpp::Pipeline
#include <vpp/descriptor.hpp> // vpp::DescriptorSet
#include <vpp/commandBuffer.hpp>
#include <vpp/renderPass.hpp>
#include <vpp/framebuffer.hpp>
#include <vpp/sync.hpp>
#include <vpp/queue.hpp>

class Engine;

// vpp::RendererBuilder implementation
class Renderer {
public:
	Renderer(Engine& engine, vk::SampleCountBits sampleCount);
	~Renderer() = default;

	void setupPipeline(vk::SampleCountBits sampleCount);
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
	vpp::RenderPass renderPass_;
	vk::SampleCountBits sampleCount_;
	const vpp::Queue* renderQueue_;

	vpp::Semaphore acquireComplete_;
	vpp::Semaphore renderComplete_;

	struct RenderBuffer {
		vpp::CommandBuffer commandBuffer;
		vpp::Framebuffer framebuffer;
	};

	std::vector<RenderBuffer> renderBuffers_;
};
