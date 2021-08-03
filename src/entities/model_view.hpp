#pragma once

#include "engine.hpp"
#include "render/gltf.hpp"
#include "render/import.hpp"

#include <string>

class ModelView : public Entity {
  private:
	Render::Model model;
	Render::TextureHandle skyboxTexture;
	std::optional<Render::ModelInstance> modelInstance;

  public:
	ModelView(
		std::string modelPath = "assets/DamagedHelmet.glb",
		std::string skyboxPath = "assets/neurathen_rock_castle_4k.hdr")
		: model(Render::load_gltf(modelPath, Engine::get_instance()->render)),
		  skyboxTexture(Render::importSkybox(skyboxPath)){};
	void enter() override {

		Engine::get_instance()->render.set_skybox_rect_texture(skyboxTexture);

		modelInstance.emplace(model);

		float pi = glm::pi<float>();
		Engine::get_instance()->render.create_dir_light({pi, pi, pi}, {1, 1, 1});
		// render.create_dir_light({2, 2, 2}, {-1, 0, 1});
		// render.create_dir_light({1, 1, 1}, {0, 0, -1});
	}

	void update(double) override {}
};