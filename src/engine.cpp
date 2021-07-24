#include "engine.hpp"
#include <chrono>

Engine* Engine::inst = nullptr;

Engine::Engine() : render(window.getRender()) {}

void Engine::run() {
	float time = 0;
	auto past = std::chrono::high_resolution_clock::now();

	// Loop will continue until "X" on window is clicked.
	// We may want more complex closing behaviour
	while (!window.shouldClose()) {
		window.beginFrame();

		auto now = std::chrono::high_resolution_clock::now();
		double timestep = std::chrono::duration_cast<std::chrono::duration<double>>(now - past).count();
		past = now;

		time += timestep;

		e_manager.update(timestep);

		window.endFrame();
	}
}