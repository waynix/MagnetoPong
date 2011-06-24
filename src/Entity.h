/*
 * Entity.h
 *
 *  Created on: 24.06.2011
 *      Author: sebastian
 */

#ifndef ENTITY_H_
#define ENTITY_H_

#include "Vec2d.h"
#include <set>
#include <ClanLib/core.h>
#include <ClanLib/display.h>
#include <ClanLib/gl.h>
#include <ClanLib/core.h>

class Application;
class Entity;

typedef std::set<Entity*> EntitySet;

class Entity {
public:
	Entity(Application* application);
	virtual ~Entity();

	virtual void draw(void);

	virtual Vec2d getPosition(void);
	virtual float getX(void);
	virtual float getY(void);
	virtual void setPosition(const Vec2d& position);
	virtual float getCharge(void);
	virtual void setCharge(float charge);
	virtual float getMass(void);
	virtual void setMass(float mass);
	virtual CL_Colorf getColor();
	virtual void setColor(CL_Colorf color);

	virtual void updateforces(const EntitySet& objects, float timedifference) {}
	virtual void updateposition(float timedifference) {}

protected:
	Application* application;
private:
	Vec2d position;
	float charge;
	float mass;
	float radius;
	CL_Colorf color;
};


#endif /* ENTITY_H_ */
