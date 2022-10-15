#pragma once
#include "Interfaces.h"
#include "Constants.h"
#include "VectorMath.h"

class DynamicLight : public IDynamicLight {
private:
	Light light;
	Light finalLight;
	IObject3D* parent;
	Vec3 relativePos;
	LightCallback callback;
public:
	DynamicLight(Light light, LightCallback cb = nullptr);
	virtual Light update();
	virtual Light& getLight();
	virtual Vec3& getRelativePos();
	virtual IObject3D*& getParent();
	virtual std::function<Light(const Light& l)>& getCallback();
	virtual void attach(IObject3D* object, Vec3 relPos = Vec3(0.0f, 0.0f, 0.0f));
	virtual SerializeData serialize();
	virtual void deserialize(const SerializeData& params);
	virtual Light& getFinalLight();
	virtual void setFinalLight(const Light& l);
};

class LightSystem : public ILightSystem {
private:
	std::deque<Ptr<IDynamicLight>> lights;
public:
	virtual LightSystem& addLight(Ptr<IDynamicLight> def);
	virtual std::vector<Ptr<IDynamicLight>> getNearbyLights(Math::Vec3 pos, float radius, int limit = 10);
	virtual Ptr<IDynamicLight> createDynamicLight(Light lightSpec, LightCallback callback = nullptr, IObject3D* parent = nullptr, Math::Vec3 relPos = Math::Vec3(0.0f, 0.0f, 0.0f));
	virtual void removeLight(Ptr<IDynamicLight> light);
};