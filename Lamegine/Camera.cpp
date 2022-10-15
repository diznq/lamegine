#include "shared.h"
#include "Camera.h"
#include "VectorMathExt.h"
#include "World.h"

float3 m_cameraPosition;

Camera::~Camera() {

}

void Camera::move(float q, float speed, double dt, bool backwards) {
	if (physical && !dynamic) speed *= ((float)dt)/engine->getPhysicsSystem()->getSteps();
	q = q / DEG;

	Vec3 _dir = dir;
	if (!World::getInstance()->settings.editor) {
		_dir.z = 0.0f;
		_dir = _dir.normalize();
	}

	float dx = _dir.x * cos(q) - _dir.y * sin(q);
	float dy = _dir.x * sin(q) + _dir.y * cos(q);

	moveDir.x = dx;
	moveDir.y = dy;
	moveDir.z = dir.z * (backwards ? -1 : 1);

	if (!physical) {
		int iq = (int)(q* DEG);
		if (!(iq == 0 || iq == 180)) moveDir.z = 0.0f;
		moveDir = moveDir.normalize();
	}

	moveDir = moveDir * speed;

	if (physical) {
		physics->setWalkDirection(float3(dx, dy, 0.0f).normalize() * speed);
	}
}

void Camera::stopMoving() {
	moveDir.x = 0.0f;
	moveDir.y = 0.0f;
	moveDir.z = 0.0f;
	if (physical) {
		physics->setWalkDirection(float3(0.0f, 0.0f, 0.0f));
	}
}

void Camera::jump(double dt) {
	static int ctr = 0;
	moveDir.z = World::getInstance()->controls.jumpHeight;
	if (physical) {
		if (!physics->onGround()) {
			if (!ctr) ctr++;
			else return; 
		} else ctr = 0;
		physics->jump(float3(0.0f, 0.0f, World::getInstance()->controls.jumpHeight));
	}
}

void Camera::rotate(float left, float up, const float sens) {
	anglx += left * sens;
	anglz += up * sens;
	if (anglz >= 89) anglz = 89;
	else if (anglz <= -89) anglz = -89;
	if (anglx >= 360) anglx -= 360;
	else if (anglx < 0) anglx += 360;
	dir = Vec3(sin(anglx / DEG) * cos(anglz / DEG), cos(anglx / DEG) * cos(anglz / DEG), sin(anglz / DEG)).normalize();
}

Vec3 Camera::getPos() {
	if (physical) {
		Vec3 pos = m_cameraPosition;
		pos.z += 1.7f / 2.0f - 0.1f;
		return pos;
	}
	return position;
}

void Camera::setPos(Math::Vec3 p) {
	if (physical) {
		m_cameraPosition = p;
		syncCameraPosition(true);
	} else {
		position = p;
		syncCameraPosition(false);
	}
}

Vec3 Camera::getDir() {
	return dir;
}

void Camera::setDir(Math::Vec3 d) {
	dir = d;
}

void Camera::syncCameraPosition(bool fromPhysToFree) {
	if (fromPhysToFree) {
		position = m_cameraPosition;
		physics->setGhostPos(m_cameraPosition);
	} else {
		m_cameraPosition = position;
	}
}

void Camera::initPhysics() {
	physics = engine->getPhysicsSystem()->createCameraPhysics(true, position, 1.7f);
}

void Camera::update(double dt) {
	if (physical) {
		m_cameraPosition = physics->getGhostPos();
	} else {
		position += moveDir * (float)dt;
	}
}

Camera::Camera(IEngine *pEngine, float x, float y, float z, bool phys)
	: position(x, y, z), engine(pEngine), jumped(false), physical(phys), dir(1.0f, 0.0f, 0.0f), anglx(0.0f), anglz(0.0f), jx(0.0f), jy(0.0f)
{
	dynamic = false;
}

void Camera::enablePhysics(bool enable) {
	if (!physical && enable) {
		syncCameraPosition(true);
		m_cameraPosition = position;
	} else if (physical && !enable) {
		syncCameraPosition(false);
	}
	physical = enable;
}

bool Camera::isPhysical() {
	return physical;
}

SerializeData Camera::serialize() {
	SerializeData data;
	return data;
}

void Camera::deserialize(const SerializeData& params) {

}