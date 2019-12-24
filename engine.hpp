// Copyright (c) 2017 nyorain
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt

#pragma once

#include <ny/fwd.hpp>
#include <vpp/fwd.hpp>
#include <nytl/vec.hpp>
#include <memory>

class Renderer;

/// Central Engine class.
/// Hirachy root, manages all other classes.
/// Entrypoint class from the main function.
class Engine {
public:
	Engine();
	~Engine();

	ny::AppContext& appContext() const;
	ny::WindowContext& windowContext() const;

	vpp::Instance& vulkanInstance() const;
	vpp::Device& vulkanDevice() const;

	Renderer& renderer() const;

	void resize(nytl::Vec2ui size);
	void mainLoop();
	void stop();

protected:
	struct Impl;
	std::unique_ptr<Impl> impl_;
	bool run_;
};
