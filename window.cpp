#include <window.hpp>
#include <engine.hpp>

#include <ny/log.hpp> // ny::log
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
					ny::log("Starting to resize window");
					wc().beginResize(ev.eventData, resizeEdges);
				} else {
					ny::log("Starting to move window");
					wc().beginMove(ev.eventData);
				}
			}
		}
	}
}
void MainWindowListener::mouseWheel(const ny::MouseWheelEvent&)
{
	// unused
}
void MainWindowListener::mouseMove(const ny::MouseMoveEvent&)
{
	// unused
}
void MainWindowListener::mouseCross(const ny::MouseCrossEvent&)
{
	// unused
}
void MainWindowListener::key(const ny::KeyEvent& keyEvent)
{
	auto keycode = keyEvent.keycode;
	if(keyEvent.pressed && ac().keyboardContext()->modifiers() & ny::KeyboardModifier::shift) {
		if(keycode == ny::Keycode::f) {
			ny::log("f pressed. Toggling fullscreen");
			if(toplevelState_ != ny::ToplevelState::fullscreen) {
				wc().fullscreen();
				toplevelState_ = ny::ToplevelState::fullscreen;
			} else {
				wc().normalState();
				toplevelState_ = ny::ToplevelState::normal;
			}
		} else if(keycode == ny::Keycode::n) {
			ny::log("n pressed. Resetting window to normal state");
			wc().normalState();
		} else if(keycode == ny::Keycode::escape) {
			ny::log("escape pressed. Closing window and exiting");
			engine_.stop();
		} else if(keycode == ny::Keycode::m) {
			ny::log("m pressed. Toggle window maximize");
			if(toplevelState_ != ny::ToplevelState::maximized) {
				wc().maximize();
				toplevelState_ = ny::ToplevelState::maximized;
			} else {
				wc().normalState();
				toplevelState_ = ny::ToplevelState::normal;
			}
		} else if(keycode == ny::Keycode::i) {
			ny::log("i pressed, Minimizing window");
			toplevelState_ = ny::ToplevelState::minimized;
			wc().minimize();
		} else if(keycode == ny::Keycode::d) {
			ny::log("d pressed. Trying to toggle decorations");
			wc().customDecorated(!wc().customDecorated());
		}
	}
	/*
	else if(keyEvent.pressed) {
		if(keycode == ny::Keycode::l) {
			renderer->renderLines = !renderer->renderLines;
			swapchainRenderer->record();
		}
	}
	*/
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
	// ny::log("resize: ", ev.size, " -- ", swapchain->vkHandle());
	size_ = ev.size;
	engine_.resize(ev.size);
}

ny::AppContext& MainWindowListener::ac() const { return engine_.appContext(); }
ny::WindowContext& MainWindowListener::wc() const { return engine_.windowContext(); }
