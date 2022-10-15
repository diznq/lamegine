#pragma once
#include "Interfaces.h"


class Camera : public ICamera {
private:
	bool physical;
	Math::Vec3 moveDir;
	Math::Vec3 position;
	Math::Vec3 dir;
	bool jumped;
	float jx, jy, dz;
	float anglx, anglz;
	bool dynamic = false;
	Ptr<IPhysics> physics;
	IEngine *engine;
	void syncCameraPosition(bool dir);
public:
	Camera(IEngine *pEngine, float x, float y, float z, bool phys = true);
	virtual ~Camera();
	virtual Math::Vec3 getPos();
	virtual Math::Vec3 getDir();
	virtual void setPos(Math::Vec3 p);
	virtual void setDir(Math::Vec3 d);
	virtual void move(float zAngle, float speed, double dt, bool backwards = false);
	virtual void stopMoving();
	virtual void jump(double dt);
	virtual void rotate(float left, float up, const float sens = 0.25);
	virtual void initPhysics();
	virtual void update(double dt);
	virtual bool isPhysical();
	virtual void enablePhysics(bool enable);

	virtual SerializeData serialize();
	virtual void deserialize(const SerializeData& params);
};