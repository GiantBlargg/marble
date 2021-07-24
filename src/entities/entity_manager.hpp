#pragma once

#include "entity.hpp"
#include <memory>
#include <vector>

class EntityManager {
  private:
	std::vector<std::unique_ptr<Entity>> entities;
	std::vector<std::unique_ptr<Entity>> entities_to_add;
	std::vector<Entity*> entities_to_remove;

  protected:
	void processChangedEntities();

  public:
	EntityManager();

	void addEntity(std::unique_ptr<Entity> e);

	void update(double dTime);

	void removeEntity(Entity* e);
};