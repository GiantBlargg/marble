#pragma once

#include "entity.hpp"
#include "render/render.hpp"
#include "window.hpp"
#include <imgui.h>

class OrbitCam : public Entity {
  private:
	const float speed = -1.0f / 200.0f;
	float xAngle = 0, yAngle = 0;
	const float pi = glm::pi<float>();

	float dist = 3;
	const float scroll_speed = -1.0f / 2.0f;

  public:
	OrbitCam(){};
	void enter() override { Engine::get_instance()->render.camera_set_fov(50); }

	void update(double) override {
		Window& window = Engine::get_instance()->window;
		if (window.mouseButton(GLFW_MOUSE_BUTTON_LEFT)) {
			window.setCursorMode(Window::CursorMode::Disabled);
			xAngle += window.cursor.deltax * speed;
			yAngle += window.cursor.deltay * speed;
			yAngle = glm::clamp<float>(yAngle, -pi / 2, pi / 2);

		} else {
			window.setCursorMode(Window::CursorMode::Normal);
		}

		dist += window.scroll.deltay * scroll_speed;
		dist = glm::max(dist, 0.0f);

		glm::mat4 cameraPos = glm::rotate(glm::mat4(1.0f), xAngle, {0, 1, 0}) *
			glm::rotate(glm::mat4(1.0f), yAngle, {1, 0, 0}) * glm::translate(glm::mat4(1.0f), glm::vec3({0, 0, dist}));

		Engine::get_instance()->render.camera_set_pos(cameraPos);
	}
};