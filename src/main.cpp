#include "entities/entity_manager.hpp"
#include "entities/model_view.hpp"
#include "entities/orbit_cam.hpp"
#include "window.hpp"
#include <chrono>
#include <iostream>

int main(int argc, char* argv[]) {
	// Read in the command line args
	std::vector<std::string> args;
	args.assign(argv, argv + argc);

	Window window;

	Render::Render& render = window.getRender();

	// Create entitity manager
	EntityManager e_manager;

	// Set up variables for fixed and variable timestep
	float timestep = 1.f / 60.f;
	float time = 0;
	auto past = std::chrono::high_resolution_clock::now();

	if (args.size() > 1) {
		e_manager.addEntity(std::make_unique<ModelView>(render, args.at(1)));
	} else {
		e_manager.addEntity(std::make_unique<ModelView>(render));
	}
	e_manager.addEntity(std::make_unique<OrbitCam>(render, window));

	// Loop will continue until "X" on window is clicked.
	// We may want more complex closing behaviour
	while (!window.shouldClose()) {
		window.beginFrame();

		// 1 for variable, 0 for fixed
		if (0) {
			auto now = std::chrono::high_resolution_clock::now();
			timestep = std::chrono::duration_cast<std::chrono::duration<float>>(now - past).count();
			past = now;
		}
		time += timestep;

		e_manager.update(timestep);

		window.endFrame();
	}
	return 0;
}