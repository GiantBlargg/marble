#pragma once

class Entity {
  public:
	virtual ~Entity(){};

	virtual void enter() = 0;
	virtual void update(double dTime) = 0;
};