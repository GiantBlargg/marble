#pragma once

#include "entities/entity_manager.hpp"
#include "window.hpp"

class Engine {
  private:
	Engine();
	Engine(const Engine&) = delete;
	static Engine* inst;

  public:
	static void init() { inst = new Engine(); };
	static Engine* get_instance() { return inst; }
	void run();

  public:
	Window window;
	Render::Render& render;
	EntityManager e_manager;
	double time;
};