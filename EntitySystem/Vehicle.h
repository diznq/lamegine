#pragma once
#include "Model3D.h"
#include <BulletDynamics/Vehicle/btRaycastVehicle.h>

struct CarInterface;

class Vehicle : public Model3D, public IVehicle {
	float speed, currentSpeed, forwardForce, steering;
	CarInterface *controller;
	bool controlled;

public:
	Vehicle(std::string path);
	virtual IObject3D& init(IEngine *pEngine);

	virtual IObject3D& buildPhysics(float mass = 0.0f, ShapeInfo shapeInfo = ShapeInfo(), PhysicsCallback callback = nullptr);

	virtual void update(const IWorld* pWorld);
	virtual void forward(float speed);
	virtual void turn(float angle);

	virtual void setSpeed(float speed) {
		this->speed = speed;
	}

	virtual void setController(void *ctl) {
		controller = (CarInterface*)ctl;
	}

	virtual void* getController() {
		return (void*)controller;
	}

	virtual bool isControlled();

	virtual unsigned int getType() const { return VEHICLE; }

	virtual Mat4 getTransform(const Ptr<const IndexBuffer> ib);

	void moveVertices(Ptr<IndexBuffer> ib, Vec3 dir);
private:
	btDefaultVehicleRaycaster* vehicleRaycaster;
	btRaycastVehicle* vehicle;
	btRaycastVehicle::btVehicleTuning tuning;
};