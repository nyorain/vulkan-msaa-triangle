#pragma once

#include <ny/windowListener.hpp>
#include <nytl/vec.hpp>

class Engine;

// ny::WindowListener implementation
class MainWindowListener : public ny::WindowListener {
public:
	MainWindowListener(Engine& engine) : engine_(engine) {};
	~MainWindowListener() = default;

	void mouseButton(const ny::MouseButtonEvent&) override;
	void mouseWheel(const ny::MouseWheelEvent&) override;
	void mouseMove(const ny::MouseMoveEvent&) override;
	void mouseCross(const ny::MouseCrossEvent&) override;
	void key(const ny::KeyEvent&) override;
	void state(const ny::StateEvent&) override;
	void close(const ny::CloseEvent&) override;
	void resize(const ny::SizeEvent&) override;

protected:
	ny::AppContext& ac() const;
	ny::WindowContext& wc() const;

protected:
	Engine& engine_;
	nytl::Vec2ui size_;
	ny::ToplevelState toplevelState_;
};
