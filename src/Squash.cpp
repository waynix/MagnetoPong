/*
 * Squash.cpp
 *
 *  Created on: 03.07.2011
 *      Author: matthas
 */

#include "Squash.h"
#include "Application.h"
#include "Ball.h"
#include "Bat.h"
#include "InputDevice.h"
#include <stdlib.h>
#include <math.h>

Squash::Squash(Application* app)
:MagnetoEngine(app)
{
   solidSides = Entity::BOTTOMSIDE | Entity::TOPSIDE;
         //| Entity::LEFTSIDE;

   inMatch = false;
   timeToMatch = 2000;
   difficulty = 1;
}

Squash::~Squash()
{
   // TODO Auto-generated destructor stub
}
//---------------------------------------------------------------------------

void Squash::run(float timediff)
{
   if(app->players[app->PLAYER_RIGHT] == 0) return;

   CL_Draw::line(app->getGC(), app->x_res/3, 0,
                 app->x_res/3, app->y_res, CL_Colorf::blue);

   //--Spieler input verarbeiten und Spielerskelett Zeichnen
   if(app->players[app->PLAYER_RIGHT] != 0)
   {
      Player* pl = app->players[app->PLAYER_RIGHT];
      pl->processInput(timediff);
      Vec2d pos = pl->getBat()->getPosition();
      if(pos.x < app->x_res/3)
      {
         pos.x = app->x_res/3;
         pl->getBat()->setPosition(pos);
      }
      pl->draw(Player::SKELETON | Player::BAR);
   }

   if(app->players[app->PLAYER_LEFT] != 0)
   {
      if(app->players[app->PLAYER_LEFT]->getExit())
      {
         app->switchTo(app->GS_MENU);
      }
   }
   if(app->players[app->PLAYER_RIGHT] != 0)
   {
      if(app->players[app->PLAYER_RIGHT]->getExit())
      {
         app->switchTo(app->GS_MENU);
      }
   }

   //--Ball einfügen
   if(spawnBall)
   {
      timeToSpawnBall -= timediff;
      if(timeToSpawnBall <= 0.0f)
      {
         doSpawnBall();
      }
   }

   drawScores(app->players[app->PLAYER_RIGHT]->getScore());

   if(!inMatch)
   {
      clearBalls();
      spawnBall = false;
      timeToMatch -= timediff;
      if(timeToMatch <= 0.0f)
      {
         startMatch();
      }
   }

   if(calcForces(timediff)) //--bei Kollision sound ausgeben (nur wenn Spieler drin sind)
   {
      app->soundPlayer->effect("collision");
   }
}
//---------------------------------------------------------------------------

void Squash::startMatch(void)
{
   inMatch = true;
   spawnBall = false;
   app->osmShout.setMessage("FIGHT", 3.0f);
   app->soundPlayer->effect("fight");

   if(app->players[app->PLAYER_RIGHT] != 0)
   {
      app->players[app->PLAYER_RIGHT]->setScore(0);
   }

   makeBall();
}
//---------------------------------------------------------------------------

void Squash::restartMatch()
{
   spawnBall = false;
   inMatch   = false;
   timeToMatch = 5000.0f;
   app->osmCenter.setMessage("Get ready...", 3.0f);
}
//---------------------------------------------------------------------------

void Squash::setDifficulty(float dif)
{
   difficulty = dif;
}
//---------------------------------------------------------------------------

// return true if ball is out
bool Squash::checkBall(Ball* ball)
{
   bool ballOut = false;
   Vec2d pos = ball->getPosition();

   if(pos.x < 0)
   {
      app->soundPlayer->effect("point");
      Vec2d speed = ball->getSpeed();
      speed.x = -speed.x;
      double points = sqrt(speed.x*speed.x + speed.y*speed.y) * 40*difficulty*difficulty;
      int score = app->players[app->PLAYER_RIGHT]->getScore();
      score += points;
      app->players[app->PLAYER_RIGHT]->setScore(score);

      speed *= difficulty;
      ball->setSpeed(speed);
      pos.x = 0;
      ball->setPosition(pos);
   }
   else if(pos.x >= app->x_res)
   {
      this->ballOut(app->PLAYER_RIGHT);
      ballOut = true;
   }
   else if(!(pos.y >= 0 && pos.y < app->y_res))
   {
      makeBall();
      ballOut = true;
   }

   return ballOut;
}
//---------------------------------------------------------------------------

void Squash::ballOut(int player)
{
   app->soundPlayer->effect("win");
   app->osmHuge.setMessage("GAMEOVER", 3.0f);
   restartMatch();
}
//---------------------------------------------------------------------------

void Squash::drawScores(int s1)
{
   CL_FontDescription scoreFont_desc;
   scoreFont_desc.set_typeface_name("Verdana");
   scoreFont_desc.set_height(80);
   CL_Font_System scoreFont(app->getGC(), scoreFont_desc);

   TGString txt = s1;
   CL_Size scoreSize = scoreFont.get_text_size(app->getGC(), txt);
   CL_Pointf scoreLeftPos((Application::x_res - scoreSize.width) / 2, scoreSize.height);
   scoreFont.draw_text(app->getGC(), scoreLeftPos, txt, CL_Colorf::black);
}
//---------------------------------------------------------------------------
