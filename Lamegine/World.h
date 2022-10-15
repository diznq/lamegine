#pragma once
#include "Interfaces.h"
#include "Constants.h"
#include "ConstantBuffers.h"
#include "VectorMath.h"

#include <map>
#include <string>

using Math::Mat4;
using Math::Vec3;

class World : public IWorld {
private:
	static World* instance;
public:
	World() {
		instance = this;
		init();
	}
	static World* getInstance();
	void init();
	Mat4 *pLightView[NUM_CASCADES], *pLightProj[NUM_CASCADES];

	struct {
		Mat4 world, view, proj;
	} shadowPass;

	LightConstants lightConstants;
	TextConstants texts;

	std::map<std::string, std::string> params;

	virtual void load(const char *fileName);
	virtual void load(std::map<std::string, std::string> args);

	virtual SerializeData serialize();
	virtual void deserialize(const SerializeData& params);
};