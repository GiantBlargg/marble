#include "engine.hpp"
#include "entities/model_view.hpp"
#include "entities/orbit_cam.hpp"
#include <chrono>
#include <iostream>

int main(int argc, char* argv[]) {
	// Read in the command line args
	std::vector<std::string> args;
	args.assign(argv, argv + argc);

	Engine::init();

	if (args.size() > 1) {
		Engine::get_instance()->e_manager.addEntity(std::make_unique<ModelView>(args.at(1)));
	} else {
		Engine::get_instance()->e_manager.addEntity(std::make_unique<ModelView>());
	}
	Engine::get_instance()->e_manager.addEntity(std::make_unique<OrbitCam>());

	Engine::get_instance()->run();
}