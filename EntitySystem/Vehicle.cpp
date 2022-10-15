#include "Vehicle.h"
#include "CarInterface.h"
#include <set>
#include <btBulletDynamicsCommon.h>


Vehicle::Vehicle(std::string ppath) : Model3D(ppath) {
	controller = 0;
	controlled = false;
	forwardForce = 0.0f;
	steering = 0.0f;
	type = VEHICLE;
}

IObject3D& Vehicle::init(IEngine *pEngine) {
	Model3D::init(pEngine);
	return *this;
}

IObject3D& Vehicle::buildPhysics(float mass, ShapeInfo shapeInfo, PhysicsCallback callback) {
	physics = engine->createPhysics();
	
	float maxEngineForce = 1000.f;
	float maxBreakingForce = 1000.f;
	float steeringIncrement = 0.0004f;
	float wheelFriction = 100.0f;
	float suspensionStiffness = 20.0f;
	float suspensionDamping = 2.3f;
	float suspensionCompression = 4.4f;
	float rollInfluence = 0.025f;
	float suspLength = 0.5f;

	physics->enable();
	vehicleRaycaster = new btDefaultVehicleRaycaster((btDynamicsWorld*)engine->getPhysicsSystem()->getHandle());

	tuning.m_suspensionStiffness = suspensionStiffness;
	tuning.m_frictionSlip = wheelFriction;
	
	Vec3 chasisSize(1.0f, 1.0f, 1.0f);
	Vec3 chasisPos(0.0f, 0.0f, 0.0f);
	bool found = false;
	for (auto& it : getDrawData()->getObjects()) {
		if (it->name.find("Chasis") != std::string::npos && !found) {
			printf("chasis pos: %f %f %f\n", it->relativePos.x, it->relativePos.y, it->relativePos.z);
			chasisPos = it->relativePos;
			chasisSize = it->axisRadius;
			//moveVertices(it, chasisPos);
			//break;
			found = true;
		}
	}
	for (auto& it : getDrawData()->getObjects()) {
		if (it->name.find("Wheel") == std::string::npos) {
			moveVertices(it, Vec3(0.0f, 0.0f, -chasisPos.z + 0.1f));
		}
	}

	printf("chasis size: %f x %f x %f\n", chasisSize.x, chasisSize.y, chasisSize.z);

	physics->init(this, mass, ShapeInfo(ePS_Box, chasisSize.x, chasisSize.y, chasisSize.z));
	
	vehicle = new btRaycastVehicle(tuning, (btRigidBody*)physics->getHandle(), vehicleRaycaster);
	btVector3 wheelDirectionCS0(0, 0, -1);
	btVector3 wheelAxleCS(1, 0, 0);

	Vec4 wheelInfo[4];
	bool foundInfo[4] = { false, false, false, false };

	for (auto& it : getDrawData()->getObjects()) {
		auto name = it->name;
		if (name.find("RearWheel2") != std::string::npos) {
			if(!foundInfo[3])
				wheelInfo[3] = Vec4(it->relativePos, it->axisRadius.z);
			it->type = VEHICLE | 1;
			it->extra = (void*)3;
			foundInfo[3] = true;
			//moveVertices(it, Vec3(-it->relativePos.x, -it->relativePos.y, -it->relativePos.z));
		}
		if (name.find("RearWheel1") != std::string::npos) {
			if (!foundInfo[2])
				wheelInfo[2] = Vec4(it->relativePos, it->axisRadius.z);
			it->type = VEHICLE | 1;
			it->extra = (void*)2;
			foundInfo[2] = true;
			//moveVertices(it, Vec3(-it->relativePos.x, -it->relativePos.y, -it->relativePos.z));
		}
		if (name.find("FrontWheel2") != std::string::npos) {
			if (!foundInfo[1])
				wheelInfo[1] = Vec4(it->relativePos, it->axisRadius.z);
			it->type = VEHICLE | 1;
			it->extra = (void*)1;
			foundInfo[1] = true;
			//moveVertices(it, Vec3(-it->relativePos.x, -it->relativePos.y, -it->relativePos.z));
		}
		if (name.find("FrontWheel1") != std::string::npos) {
			if (!foundInfo[0])
				wheelInfo[0] = Vec4(it->relativePos, it->axisRadius.z);
			it->type = VEHICLE | 1;
			it->extra = (void*)0;
			foundInfo[0] = true;
			//moveVertices(it, Vec3(-it->relativePos.x, -it->relativePos.y, -it->relativePos.z));
		}
	}
	for (auto& it : getDrawData()->getObjects()) {
		if (it->type == (VEHICLE | 1)) {
			int wheel = *(int*)&it->extra;
			Vec3 offset = wheelInfo[wheel];
			moveVertices(it, -offset);
		}
	}
	for (int i = 0; i < 4; i++) {
		Vec4& p = wheelInfo[i];
		printf("Wheel %d pos: %f %f %f, radius: %f\n", i, p.x, p.y, p.z, p.w);
		vehicle->addWheel(btVector3(p.x, p.y, p.z), wheelDirectionCS0, wheelAxleCS, p.w, suspLength, tuning, i<2);
	}

	for (int i = 0; i < vehicle->getNumWheels(); i++)
	{
		btWheelInfo& wheel = vehicle->getWheelInfo(i);
		wheel.m_suspensionStiffness = suspensionStiffness;
		wheel.m_wheelsDampingRelaxation = suspensionDamping;
		wheel.m_wheelsDampingCompression = suspensionCompression;
		wheel.m_frictionSlip = wheelFriction;
		wheel.m_rollInfluence = rollInfluence;
	}

	vehicle->getRigidBody()->setUserPointer(this);
	((btDynamicsWorld*)engine->getPhysicsSystem()->getHandle())->addVehicle(vehicle);
	
	return *this;
}

void Vehicle::moveVertices(Ptr<IndexBuffer> ib, Vec3 dir) {
	auto& verts = getDrawData()->getVertices();
	auto& indices = getDrawData()->getIndices();
	std::set<unsigned int> affected;
	for (int i = ib->start; i < ib->offset; i++) {
		unsigned int idx = indices[i];
		if (affected.find(idx) == affected.end()) {
			Vertex& vtx = verts[idx];
			vtx.pos += dir;
			affected.insert(idx);
		}
	}
}

void Vehicle::forward(float speed) {
	forwardForce = speed;
}
void Vehicle::turn(float angle) {
	angle = -angle;
	steering = (float)(angle / (180.0f / PI));
}

Mat4 Vehicle::getTransform(const Ptr<const IndexBuffer> ib) {
	if (ib->type == (VEHICLE | 1)) {
		auto t = vehicle->getWheelTransformWS(*(int*)(&ib->extra));
		float mtx[16];
		t.getOpenGLMatrix(mtx);
		return Mat4(mtx).transpose() * Mat4(-1.0f);
	}
	return Object3D::getTransform(ib);
}

void Vehicle::update(const IWorld* pWorld) {
	static bool autoCtl = false;
	static Vec3 lastPos = getPos();
	if (engine->getWindow()->IsKeyPressed('Y')) autoCtl = !autoCtl;
	currentSpeed = getPos().dist(lastPos) / (float)pWorld->delta;
	lastPos = getPos();
	controlled = autoCtl;
	if (autoCtl) {
		if (controller) {
			controller->clearLine();
			//controller->writeCam(readCam());
			if (!controller->hasControl()) {
				controller->setSpeed(speed / 100.0f);
				controller->setSteering(-steering * (180.0f / (float)PI) / 45.0f);
			}
			controller->onUpdate((float)pWorld->delta);
			const char *text = controller->getLine();
			if (text) {
				engine->drawText(10.0f, 150.0f, text, Vec3(0.0f, 0.0f, 0.0f));
			}
			if (controller->hasControl()) {
				setSpeed(controller->getSpeed() * 100.0f);
				turn(controller->getSteering() * 45.0f);
			}
		}
	}

	if (abs(speed) > 0.0f) {
		float df = (speed - currentSpeed);
		forward((float)(3.0f * df * physics->getMass() * pWorld->delta));
	}

	//cout << "Forward force: " << forwardForce << ", speed: "<<speed<<", currentSpeed: "<<currentSpeed<< ", mass: " << physics->getMass() << endl;

	vehicle->applyEngineForce(forwardForce, 2);
	vehicle->applyEngineForce(forwardForce, 3);

	vehicle->setSteeringValue(btScalar(steering), 0);
	vehicle->setSteeringValue(btScalar(steering), 1);
}

bool Vehicle::isControlled() {
	return controlled && controller->hasControl();
}