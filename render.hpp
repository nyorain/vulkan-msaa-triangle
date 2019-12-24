// Copyright (c) 2017 nyorain
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt

#pragma once

#include <vpp/device.hpp> // vpp::Device
#include <vpp/queue.hpp> // vpp::Queue
#include <vpp/buffer.hpp> // vpp::Buffer
#include <vpp/pipeline.hpp> // vpp::Pipeline
#include <vpp/renderer.hpp> // vpp::DefaultRenderer
#include <vpp/descriptor.hpp> // vpp::DescriptorSet
#include <vpp/handles.hpp>
#include <vpp/queue.hpp>
#include <vpp/vk.hpp> // FIXME
#include <nytl/vec.hpp>

class Engine;

class Renderer : public vpp::DefaultRenderer {
public:
	Renderer(const vpp::Device&, vk::SurfaceKHR, vk::SampleCountBits samples,
		const vpp::Queue& present);
	~Renderer() = default;

	void resize(nytl::Vec2ui size);
	void samples(vk::SampleCountBits);

protected:
	void createMultisampleTarget(const vk::Extent2D& size);
	void record(const RenderBuffer&) override;
	void initBuffers(const vk::Extent2D&, nytl::Span<RenderBuffer>) override;

protected:
	vpp::Pipeline trianglePipeline_;
	vpp::PipelineLayout graphicsLayout_;
	vpp::Buffer vertexBuffer_;
	vpp::ViewableImage multisampleTarget_;
	vpp::RenderPass renderPass_;
	vk::SampleCountBits sampleCount_;
	vk::SwapchainCreateInfoKHR scInfo_;
};
