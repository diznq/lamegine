#pragma once
#include "Interfaces.h"

class Engine;
class Vehicle;
class GameMain : public IGame {
private:
	IEngine *pEngine;
	IVehicle *activeVehicle;
	IObject3D *activeVehicleObj;
public:
	virtual void init(IEngine *pEngine);
	virtual void onUpdate(double dt);
	virtual void onKeyPressed(int key);
	virtual void onKeyDown(int key);
	virtual void onKeyUp(int key);
	virtual void onMouseDown(int btn);
	virtual void onMouseUp(int btn);
	virtual void onMousePressed(int btn);
	virtual void onScroll(int dir);
	virtual void onMessage(const Message& msg);
	virtual bool isActorGhost();
	virtual bool overrideCamera();
	virtual bool overrideLoader();
	virtual bool overrideKeyboard();
	virtual bool overrideMouse();
	virtual bool overrideMovement();
	virtual bool getCameraPos(Math::Vec3& out);
	virtual bool getCameraDir(Math::Vec3& out);
	virtual const char *getName();
	virtual void release();
};