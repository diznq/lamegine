#include "shared.h"
#include "Engine.h"
#include "ScriptSystem.h"
#include "World.h"
#include <btBulletCollisionCommon.h>
#include <btBulletDynamicsCommon.h>

static int spawnModel(lua_State *L) {
	int numArgs = lua_gettop(L);
	if (numArgs < 5) {
		lua_pushnil(L);
		return 1;
	}
	float weight = 0.0f;
	const char *name = lua_tostring(L, 1);
	const char *path = lua_tostring(L, 2);
	float x = (float)lua_tonumber(L, 3);
	float y = (float)lua_tonumber(L, 4);
	float z = (float)lua_tonumber(L, 5);
	if (numArgs > 5) {
		weight = (float)lua_tonumber(L, 6);
	}
	cout << " [ss] Spawning " << name << " (" << path << ") at " << x << ", " << y << ", " << z << ", mass: " << weight << endl;
	Vec3 pos = Vec3(x, y, z);
	if (path) {
		auto physicSpec = [=](btRigidBody* body) -> void {
			body->setFriction(0.5f);
			body->setRollingFriction(0.5f);
			body->setSpinningFriction(0.5f);
		};
		IEngine *pEngine = Engine::getInstance();
		if (pEngine) {
			ModelSpawnParams params;
			params.path = path;
			auto model = pEngine->getClass("Model3D", &params);
			model->setPos(pos);
			if (name) {
				model->setName(name);
			}
			//if (weight>0.0f) {
			model->buildPhysics(weight, ShapeInfo(), physicSpec);// .disablePhysics();
			//}
			model->add(true, false);
			lua_pushlightuserdata(L, model.get());
			return 1;
		}
	}
	lua_pushnil(L);
	return 1;
}

static int getCameraPos(lua_State *L) {
	IEngine *pEngine = Engine::getInstance();
	auto pos = pEngine->getCamera()->getPos();
	lua_pushnumber(L, pos.x);
	lua_pushnumber(L, pos.y);
	lua_pushnumber(L, pos.z);
	return 3;
}

static int getCameraDir(lua_State *L) {
	IEngine *pEngine = Engine::getInstance();
	auto pos = pEngine->getCamera()->getPos();
	lua_pushnumber(L, pos.x);
	lua_pushnumber(L, pos.y);
	lua_pushnumber(L, pos.z);
	return 3;
}

static int getObjectByName(lua_State *L) {
	int numArgs = lua_gettop(L);
	if(numArgs < 1) {
		lua_pushnil(L);
		return 1;
	}
	const char *name = lua_tostring(L, 1);
	Engine *pEngine = Engine::getInstance();

	auto obj = pEngine->getObjectByName<IObject3D>(name);
	if (obj) {
		lua_pushlightuserdata(L, obj.get());
		return 1;
	}

	lua_pushnil(L);
	return 1;
}

static int setObjectPos(lua_State *L) {
	int numArgs = lua_gettop(L);
	if (numArgs < 4) {
		return 0;
	}
	IObject3D* pObj = (IObject3D*)lua_touserdata(L, 1);
	float x = (float)lua_tonumber(L, 2);
	float y = (float)lua_tonumber(L, 3);
	float z = (float)lua_tonumber(L, 4);
	if (pObj) {
		if (pObj->getPhysics()) {
			/// TODO
			//btRigidBody* body = pObj->getPhysics()->getBody();
			//body->getWorldTransform().setOrigin(btVector3(x, y, z));
		} else pObj->setPos(Vec3(x, y, z));
	}
	return 0;
}

static int getObjectPos(lua_State *L) {
	int numArgs = lua_gettop(L);
	if (numArgs < 1) {
		return 0;
	}
	IObject3D* pObj = (IObject3D*)lua_touserdata(L, 1);
	if (pObj) {
		auto& pos = pObj->getPos();
		lua_pushnumber(L, pos.x);
		lua_pushnumber(L, pos.y);
		lua_pushnumber(L, pos.z);
		return 3;
	}
	return 0;
}

ScriptSystem::ScriptSystem() :argCounter(0) {
	L = luaL_newstate();
	luaL_openlibs(L);

	lua_register(L, "GetObjectByName", getObjectByName);
	lua_register(L, "GetCameraPos", getCameraPos);
	lua_register(L, "GetCameraDir", getCameraDir);

	lua_register(L, "SetObjectPos", setObjectPos);
	lua_register(L, "GetObjectPos", getObjectPos);

	lua_register(L, "SpawnModel", spawnModel);
}
ScriptSystem::~ScriptSystem() {
	lua_close(L);
}
bool ScriptSystem::loadFile(const char *name) {
	int state = luaL_loadfilex(L, name, NULL);
	if (state) {
		cout << " [ScriptSystem] Failed to load file " << name << ", error code: " << state << endl;
		return false;
	}
	lua_pcall(L, 0, 0, 0);
	return true;
}
void ScriptSystem::registerFunction(const char *name, int(*function)(lua_State*)) {
	lua_register(L, name, function);
}