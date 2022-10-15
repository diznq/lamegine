#include "shared.h"
#include "Light.h"

DynamicLight::DynamicLight(Light light, LightCallback cb) : parent(0) {
	this->light = light;
	callback = cb;
}
void DynamicLight::setFinalLight(const Light& l) {
	finalLight = l;
}
Light& DynamicLight::getFinalLight() {
	return finalLight;
}
Light DynamicLight::update() {
	if (callback != nullptr) {
		return finalLight = callback(light);
	}
	return finalLight = light;
}
Light&  DynamicLight::getLight() {
	return light;
}
Vec3&  DynamicLight::getRelativePos() {
	return relativePos;
}
IObject3D*&  DynamicLight::getParent() {
	return parent;
}

SerializeData DynamicLight::serialize() {
	SerializeData data;
	return data;
}
void DynamicLight::deserialize(const SerializeData& params) {

}


std::function<Light(const Light& l)>&  DynamicLight::getCallback() {
	return callback;
}
void DynamicLight::attach(IObject3D* object, Vec3 relPos) {
	parent = object;
	relativePos = relPos;
}

LightSystem& LightSystem::addLight(Ptr<IDynamicLight> def) {
	lights.push_back(def);
	return *this;
}
std::vector<Ptr<IDynamicLight>> LightSystem::getNearbyLights(Math::Vec3 pos, float radius, int limit) {
	std::vector<Ptr<IDynamicLight>> nearby;
	std::deque<Ptr<IDynamicLight>> candidates;
	nearby.reserve(limit);
	for (auto& it : this->lights) {
		it->update();
		Light& l = it->getFinalLight();
		Vec3& lightPos = l.position;
		if (auto parent = it->getParent()) {
			lightPos = parent->getPos() + (it->getRelativePos() + l.position) * Mat4(XMMatrixRotationQuaternion(parent->getRotation().raw()));
		}
		if (lightPos.dist(pos) < radius) {
			candidates.push_back(it);
		}
	}
	if (candidates.size() < limit) {
		nearby.assign(candidates.begin(), candidates.end());
	} else {
		std::sort(candidates.begin(), candidates.end(), [pos](const Ptr<IDynamicLight>& d1, const Ptr<IDynamicLight>& d2) -> bool {
			float dst1 = d1->getFinalLight().position.dist(pos);
			float dst2 = d2->getFinalLight().position.dist(pos);
			return dst1 < dst2;
		});
		for (int i = 0; i < limit; i++) {
			nearby.push_back(candidates[i]);
		}
	}
	return nearby;
}
Ptr<IDynamicLight> LightSystem::createDynamicLight(Light lightSpec, LightCallback callback, IObject3D* parent, Math::Vec3 relPos) {
	Ptr<DynamicLight> dynLight = gcnew<DynamicLight>(lightSpec, callback);
	if (parent) {
		dynLight->attach(parent, relPos);
		parent->attachLight(dynLight, relPos);
	}
	return dynLight;
}
void LightSystem::removeLight(Ptr<IDynamicLight> light) {
	for (auto it = lights.begin(); it != lights.end(); it++) {
		if ((*it) == light) {
			lights.erase(it);
			return;
		}
	}
}