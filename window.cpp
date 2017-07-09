// Copyright (c) 2017 nyorain
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt

#include <window.hpp>
#include <engine.hpp>
#include <render.hpp>

#include <dlg/dlg.hpp> // dlg
#include <ny/key.hpp> // ny::Keycode
#include <ny/mouseButton.hpp> // ny::MouseButton
#include <ny/image.hpp> // ny::Image
#include <ny/event.hpp> // ny::*Event
#include <ny/keyboardContext.hpp> // ny::KeyboardContext
#include <ny/appContext.hpp> // ny::AppContext
#include <ny/windowContext.hpp> // ny::WindowContext
#include <ny/windowSettings.hpp> // ny::WindowEdge

// TODO: to make this work for android, implement the surfaceCreated/surfaceDestroyed
// methods

using namespace dlg::literals;

void MainWindowListener::mouseButton(const ny::MouseButtonEvent& ev)
{
	if(ev.pressed) {
		if(ev.button == ny::MouseButton::left) {
			bool alt = ac().keyboardContext()->modifiers() & ny::KeyboardModifier::alt;

			if(wc().customDecorated() && alt) {
				if(ev.position[0] < 0 || ev.position[1] < 0 ||
					static_cast<unsigned int>(ev.position[0]) > size_[0] ||
					static_cast<unsigned int>(ev.position[1]) > size_[1])
						return;

				ny::WindowEdges resizeEdges = ny::WindowEdge::none;
				if(ev.position[0] < 100)
					resizeEdges |= ny::WindowEdge::left;
				else if(static_cast<unsigned int>(ev.position[0]) > size_[0] - 100)
					resizeEdges |= ny::WindowEdge::right;

				if(ev.position[1] < 100)
					resizeEdges |= ny::WindowEdge::top;
				else if(static_cast<unsigned int>(ev.position[1]) > size_[1] - 100)
					resizeEdges |= ny::WindowEdge::bottom;

				if(resizeEdges != ny::WindowEdge::none) {
					dlg_info("window"_module, "Starting to resize window");
					wc().beginResize(ev.eventData, resizeEdges);
				} else {
					dlg_info("window"_module, "Starting to move window");
					wc().beginMove(ev.eventData);
				}
			}
		}
	}
}
void MainWindowListener::key(const ny::KeyEvent& keyEvent)
{
	static const dlg::Source source = "window"_module;

	auto keycode = keyEvent.keycode;
	if(keyEvent.pressed && ac().keyboardContext()->modifiers() & ny::KeyboardModifier::shift) {
		if(keycode == ny::Keycode::f) {
			dlg_info(source, "f pressed. Toggling fullscreen");
			if(toplevelState_ != ny::ToplevelState::fullscreen) {
				wc().fullscreen();
				toplevelState_ = ny::ToplevelState::fullscreen;
			} else {
				wc().normalState();
				toplevelState_ = ny::ToplevelState::normal;
			}
		} else if(keycode == ny::Keycode::n) {
			dlg_info(source, "n pressed. Resetting window to normal state");
			wc().normalState();
		} else if(keycode == ny::Keycode::escape) {
			dlg_info(source, "escape pressed. Closing window and exiting");
			engine_.stop();
		} else if(keycode == ny::Keycode::m) {
			dlg_info(source, "m pressed. Toggle window maximize");
			if(toplevelState_ != ny::ToplevelState::maximized) {
				wc().maximize();
				toplevelState_ = ny::ToplevelState::maximized;
			} else {
				wc().normalState();
				toplevelState_ = ny::ToplevelState::normal;
			}
		} else if(keycode == ny::Keycode::i) {
			dlg_info(source, "i pressed, Minimizing window");
			toplevelState_ = ny::ToplevelState::minimized;
			wc().minimize();
		} else if(keycode == ny::Keycode::d) {
			dlg_info(source, "d pressed. Trying to toggle decorations");
			wc().customDecorated(!wc().customDecorated());
		}
	} else if(keyEvent.pressed) {
		if(keycode == ny::Keycode::k1) {
			dlg_info(source, "Using no multisampling");
			engine_.renderer().setupPipeline(vk::SampleCountBits::e1);
		} else if(keycode == ny::Keycode::k8) {
			dlg_info(source, "Using 8 multisamples");
			engine_.renderer().setupPipeline(vk::SampleCountBits::e8);
		}
	}
}
void MainWindowListener::state(const ny::StateEvent& stateEvent)
{
	if(stateEvent.state != toplevelState_)
		toplevelState_ = stateEvent.state;
}
void MainWindowListener::close(const ny::CloseEvent&)
{
	engine_.stop();
}
void MainWindowListener::resize(const ny::SizeEvent& ev)
{
	dlg_info("window"_module, "resize: {}", ev.size);
	size_ = ev.size;
	engine_.resize(ev.size);
}

ny::AppContext& MainWindowListener::ac() const { return engine_.appContext(); }
ny::WindowContext& MainWindowListener::wc() const { return engine_.windowContext(); }
