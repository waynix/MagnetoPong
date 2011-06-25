#include "Application.h"
#include "Bat.h"
#include "Ball.h"
#include <sstream>
#include <iostream>
#include "Player.h"
#include "MouseInputDevice.h"
#include "KinectInputDevice.h"
#include "OnScreenMessage.h"
#include "TGString.h"
#include "Ball.h"
#include <exception>
#include <stdlib.h>


Application* Application::myself;
int Application::detail;
int Application::x_res;
int Application::y_res;

void PlayerCallback::playerRecognized(int nr)
{
	Application* app = Application::get();
	if(app->players[Application::PLAYER_RIGHT] == 0) {
		Application::get()->osmRight.setMessage("Welcome. Please form a PSI to calibrate.", 5.0f);
	}
	else if(app->players[Application::PLAYER_LEFT] == 0) {
		Application::get()->osmLeft.setMessage("Welcome. Please form a PSI to calibrate.", 5.0f);
	}

}

void PlayerCallback::calibrationStart(int nr)
{
	Application* app = Application::get();
	if(app->players[Application::PLAYER_RIGHT] == 0) {
		Application::get()->osmRight.setMessage("Calibrating...");
	}
	else if(app->players[Application::PLAYER_LEFT] == 0) {
		Application::get()->osmLeft.setMessage("Calibrating...");
	}
	else {
		Application::get()->osmCenter.setMessage("Too many players, please go away!", 5.0f);
	}
}

void PlayerCallback::calibrationFailed(int nr)
{
	Application* app = Application::get();
	if(app->players[Application::PLAYER_RIGHT] == 0) {
		Application::get()->osmRight.setMessage("Calibration failed", 2.0f);
	}
	else if(app->players[Application::PLAYER_LEFT] == 0) {
		Application::get()->osmLeft.setMessage("Calibration failed", 2.0f);
	}
}

void PlayerCallback::playerCalibrated(int nr)
{
	Application::get()->osmRight.hide();
	Application::get()->osmLeft.hide();
	Application* app = Application::get();
	unsigned int playerSlot = Application::PLAYER_RIGHT;
	if(app->players[Application::PLAYER_RIGHT] != 0) {
		playerSlot = Application::PLAYER_LEFT;
	}
	if(app->players[Application::PLAYER_LEFT] != 0) {
		return;
	}

	KinectInputDevice* device = new KinectInputDevice(nr, playerSlot == Application::PLAYER_LEFT);
	Player* player = new Player(app, device);
	player->setNumber(nr);
	player->getBat()->setColor(playerColors[playerSlot]);

	if(playerSlot == Application::PLAYER_LEFT) {
		std::cout << "You play LEFT!" << std::endl;
	}
	if(playerSlot == Application::PLAYER_RIGHT) {
		std::cout << "You play RIGHT!" << std::endl;
	}

	app->addPlayer(player, playerSlot);
}

void PlayerCallback::playerLost(int nr)
{
	Application* app = Application::get();
	for(int playerSlot = 0; playerSlot < app->players.size(); playerSlot++) {
		Player* player = app->players[playerSlot];
		if(player != 0 && player->getNumber() == nr) {
			std::cout << "Lost player on side " << playerSlot << std::endl;
			app->remPlayer(playerSlot);
		}
	}
}

Application::Application(void)
{

	CL_FontDescription font_desc;
	font_desc.set_typeface_name("tahoma");
	font_desc.set_height(30);
	osmCenter = OnScreenMessage(CL_Pointf(x_res / 2, y_res / 2), font_desc, CL_Colorf::darkslateblue);
	osmLeft = OnScreenMessage(CL_Pointf(x_res / 4, y_res / 2), font_desc,
			playerColors[PLAYER_LEFT]);
	osmRight = OnScreenMessage(CL_Pointf(x_res * 3 / 4, y_res / 2),
			font_desc, playerColors[PLAYER_RIGHT]);
}

void Application::run(void)
{
	quit = false;
	Application::myself = this;

	players.resize(2);
	players[PLAYER_LEFT] = 0;
	players[PLAYER_RIGHT] = 0;
	playersActive = 0;

	kinect.setPlayerCallback(&playerCallback);

	unsigned int start =  CL_System::get_time();
	CL_DisplayWindowDescription window_desc;
	window_desc.set_size(CL_Size(Application::x_res, Application::y_res), true);
	window_desc.set_title("MagnetoPong!!!11einself");
	CL_DisplayWindow window(window_desc);

	CL_Slot slot_quit = window.sig_window_close().connect(this, &Application::on_window_close);

	graphicContext = window.get_gc();
	gc = graphicContext;
	CL_InputDevice keyboard = window.get_ic().get_keyboard();
	CL_InputDevice mouse = window.get_ic().get_mouse(0);

	CL_ResourceManager resources("resources.xml");

	CL_Sprite boat_sprite(gc, "Boat", &resources);

	CL_FontDescription font_desc;
	font_desc.set_typeface_name("tahoma");
	font_desc.set_height(30);
	CL_Font_System font(gc, font_desc);

	osmCenter.setMessage("Welcome to MagnetoPong!", 5.0f);

	clearBalls();

	void addPlayer(int num);
	while (!quit)
	{
		kinect.update();

		int timediff = CL_System::get_time() - start ;
		start = CL_System::get_time();


		if(keyboard.get_keycode(CL_KEY_ESCAPE) == true)
			quit = true;


		for(EntitySet::iterator it = entities.begin(); it != entities.end(); it++) {
			(*it)->updateforces(entities,timediff);
		}

		osmCenter.tick((float)timediff / 1000.0f);
		osmLeft.tick((float)timediff / 1000.0f);
		osmRight.tick((float)timediff / 1000.0f);

		gc.clear(CL_Colorf::white);

		for(std::vector<Player*>::iterator it = players.begin();
				it != players.end(); it++) {
			Player* pl = *it;
			if(pl != 0) {
				pl->processInput();
				kinect.drawPlayer(pl->getNumber());
			}
		}

		for(EntitySet::iterator it = entities.begin(); it != entities.end();) {
			EntitySet::iterator next = it;
			next++;
			(*it)->updateposition(timediff);
			(*it)->draw();

			if(Ball* ball = dynamic_cast<Ball*>(*it)) {
				if(checkBall(ball)) {
					remEntity(ball);
					delete ball;
				}
			}
			it = next;
		}

		osmCenter.draw();
		osmLeft.draw();
		osmRight.draw();

//		TGString s = TGString("b1(") + ball.getPosition().x + "|" + ball.getPosition().y + ") b2(" + ball2.getPosition().x + "|" + ball2.getPosition().y + ")";
	//	font.draw_text(Application::myself->gc, 10, 20, s.c_str(), CL_Colorf::black);

		window.flip();
		CL_KeepAlive::process();
	}
}

void Application::addEntity(Entity* entity)
{
	entities.insert(entity);
}

void Application::remEntity(Entity* entity)
{
	entities.erase(entity);
}

void Application::addPlayer(Player* player, int playerSlot)
{
	if(players[playerSlot] != 0) {
		remPlayer(playerSlot);
	}
	players[playerSlot] = player;
	playersActive++;

	clearBalls();

	switch(playersActive) {
	case 0: {
		break;
	}
	case 1: {
		osmCenter.setMessage("Waiting for Player 2");
		break;
	}
	case 2: {
		osmCenter.setMessage("FIGHT", 3.0f);
		break;
	}
	default: throw std::exception(); break;
	}
}

void Application::remPlayer(int playerSlot)
{
	//delete player;
	Player* player = players[playerSlot];
	player->quit();
	players[playerSlot] = 0;
	playersActive--;
}

// return true if ball is out
bool Application::checkBall(Ball* ball)
{
	bool ballOut = false;
	Vec2d pos = ball->getPosition();

	if(pos.x < 0) {
		ballOutLeft(ball);
		ballOut = true;
	}
	else if(pos.x >= x_res) {
		ballOutRight(ball);
		ballOut = true;
	}
	else if(!(pos.y >= 0 && pos.y < y_res)) {
		ballGone(ball);
		ballOut = true;
	}

	if(ballOut) {
		switch(playersActive) {
		case 0: {
			makeBall();
			break;
		}
		case 1: {
			makeBall();
			break;
		}
		case 2: {
			makeBall();
			break;
		}
		default: throw std::exception(); break;
		}
	}

	return ballOut;
}

void Application::ballOutLeft(Ball* ball) {
	if(playersActive == 2) {
		osmCenter.setMessage("Left Player Scores", 2.0f);
	}
}

void Application::ballOutRight(Ball* ball) {
	if(playersActive == 2) {
		osmCenter.setMessage("Right Player Scores", 2.0f);
	}
}

void Application::ballGone(Ball* ball) {

}

void Application::clearBalls(void) {
	for(EntitySet::iterator it = entities.begin(); it != entities.end();) {
		EntitySet::iterator next = it;
		next++;
		if(Ball* ball = dynamic_cast<Ball*>(*it)) {
			remEntity(ball);
			delete ball;
		}
		it = next;
	}

	switch(playersActive) {
	case 0: {
		makeBall();
		makeBall();
		break;
	}
	case 1: {
		makeBall();
		break;
	}
	case 2: {
		makeBall();
		break;
	}
	default: throw std::exception(); break;
	}
}

void Application::makeBall(void)
{
	Ball* b1 = new Ball(this,Vec2d(Application::x_res, Application::y_res));
	b1->initializePosition();
	int ch = rand() % 2;
	b1->setCharge(ch ? 1.0f : -1.0f);
	addEntity(b1);

}
