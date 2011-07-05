/*
 * Bat.cpp
 *
 *  Created on: 24.06.2011
 *      Author: sebastian
 */

#include "Ball.h"
#include "Application.h"
#include "stdlib.h"

float Ball::ballacc;
float Ball::radius;

Ball::Ball(Application* application, Vec2d windowFrame)
: Entity(application)
{
	setColor(CL_Colorf(0.0f, 0.0f, 0.0f, 0.6f));
	setRadius(Ball::radius);
	this->windowFrame = windowFrame;
}

Ball::~Ball()
{

}
//---------------------------------------------------------------------------

void Ball::draw(void)
{
	Entity::draw();
}
//---------------------------------------------------------------------------

void Ball::initializePosition()
{
	Vec2d startpos = windowFrame/2;
	startpos.x += (rand() % 21)-10;
	startpos.y  =  rand() % (int)windowFrame.y;
	this->setPosition(startpos);

}
//---------------------------------------------------------------------------

void Ball::updateposition(float timedifference, int solidSides)
{
	speed += this->force * timedifference;
	Vec2d newpos = this->getPosition()+this->speed*timedifference;

	if(solidSides & Entity::BOTTOMSIDE)
	{
      if(newpos.y + speed.y + getRadius() > windowFrame.y)
      {
         speed.y = - speed.y;
         newpos.y= this->windowFrame.y-getRadius();
      }
	}
	if(solidSides & Entity::TOPSIDE)
   {
      if(newpos.y - speed.y - getRadius() < 0)
      {
         speed.y = - speed.y;
         newpos.y= getRadius();
      }
   }
	if(solidSides & Entity::RIGHTSIDE)
   {
      if(newpos.x + speed.x + getRadius() > windowFrame.x)
      {
         speed.x = - speed.x;
         newpos.x= this->windowFrame.x-getRadius();
      }
   }
   if(solidSides & Entity::LEFTSIDE)
   {
      if(newpos.x - speed.x - getRadius() < 0)
      {
         speed.x = - speed.x;
         newpos.x= getRadius();
      }
   }

	this->setPosition(newpos);
	force = Vec2d(0,0);
}
//---------------------------------------------------------------------------

Vec2d Ball::getForce()
{
	return this->force;
}
//---------------------------------------------------------------------------

void Ball::addForce(Vec2d force)
{
   this->force += force;
}
//---------------------------------------------------------------------------

void Ball::setSpeed(Vec2d v)
{
   speed = v;
}
//---------------------------------------------------------------------------
