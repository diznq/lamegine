#define _CRT_SECURE_NO_WARNINGS
#include "../Lamegine/Interfaces.h"
#include <iostream>
#include <map>
#include <array>
#include <vector>
#include "../Lamegine/utils/bmp.h"
using namespace std;

class CShooter : public IGame {
	IEngine *pEngine;
	IWindow *pWindow;
	IWorld *pWorld;
	std::shared_ptr<IObject3D> player, deagle;

	int lastKey;

	int activeMode = 0;
	int modeDirKey = 0;
	float3 modeDir = float3(0.0f, 0.0f, 1.0f);

	bool finished = false;
	bool firstPerson = true;

	std::vector<std::shared_ptr<ISound>> sounds;
	std::shared_ptr<ISound> jmp_sound, start_sound, end_sound, bgm;
	Ptr<ITexture> pSnow, pSnowDDN, pSnowMisc;
	Ptr<IObject3D> vehicle;

	std::map<int, Ptr<IMaterial>> materials;

	Ptr<ISound> hitSound;

	std::array<std::vector<float3>, 32> blocks;
	std::array<Ptr<IObject3D>, 32> block_objects;
	std::array<bool, 32> blocks_ready;

	Ptr<IObject3D> cube;

	int keyStates[256] = { 0 };
	int currentLayer = 1;

	bool flyMode = false;

	virtual bool overrideLoader() {
		return true;
	}

	virtual void init(IEngine *pEngine) {
		if (!pEngine->checkCompatibility(LAMEGINE_SDK_VERSION)) return;

		srand((unsigned int)time(0));
		this->pEngine = pEngine;
		pWindow = pEngine->getWindow();
		pWorld = pEngine->getWorld();

		ModelSpawnParams cubeParams;
		cubeParams.path = "crafty/cube.obj";
		cube = pEngine->getClass("Model3D", &cubeParams);
		cube->add();

		auto world = pEngine->getWorld();
		//world->constants.skyColor = float3(0.02f, 0.04f, 0.1f);

		for (auto i = 0; i < block_objects.size(); i++) {
			block_objects[i] = 0;
			blocks_ready[i] = false;
			blocks[i].reserve(2048);
		}

		for (int y = -32; y < 32; y++) {
			for (int x = -32; x < 32; x++) {
				spawnBlock(0, float3((float)x, (float)y, 0.0f) * 2.0f, 1.0f);
			}
		}
	}

	void spawnPlayer(bool attachGun) {
		ModelSpawnParams playerParams;
		playerParams.path = "data/objects/de_dust2/body.dae";
		player = pEngine->getClass("Model3D", &playerParams);
		player->setPos(pEngine->getCameraPos()).add();
		if (attachGun) {
			ModelSpawnParams deagleParams;
			deagleParams.path = "data/objects/de_dust2/gun.obj";
			deagle = pEngine->getClass("Model3D", &deagleParams);
			deagle->setPos(pEngine->getCameraPos() + 1.0f * pEngine->getCameraDir() - float3(0.0f, 0.0f, 1.0f)).add();

			deagle->attachTo(player, "metarig_palm_02_R");
		}
	}

	void spawnBlock(int type, float3 pos, float size = 1.0f) {
		blocks[type].push_back(pos);
		blocks_ready[type] = false;
	}

	void removeBlock(int type, float3 pos) {
		std::vector<float3> positions;
		positions.reserve(blocks[type].size());
		for (auto& bpos : blocks[type]) {
			float dst = bpos.dist(pos);
			if (dst > 0.5f) {
				positions.push_back(bpos);
			}
		}
		blocks[type] = positions;
		blocks_ready[type] = false;
	}

	virtual void onUpdate(double dt) {
		static double t = 0;
		static float3 lastPos = pEngine->getCameraPos();
		float3 pos = pEngine->getCameraPos();
#if 1
		float3 diff = lastPos - pos;
		if (diff.z > 0.0f) {
			float zSpeed = diff.z;
			pEngine->setDistortion(zSpeed);
		} else pEngine->setDistortion(0.0f);
#endif
		t += dt;

		if (player) {
			updatePlayer(dt, t);
		}

		for (auto i = 0; i < blocks_ready.size(); i++) {
			if (!blocks_ready[i]) {
				refreshBlocks(i);
			}
		}

		
		lastPos = pos;
	}

	void refreshBlocks(size_t layer) {
		if (block_objects[layer]) {
			pEngine->removeObject(block_objects[layer].get());
			block_objects[layer] = 0;
		}

		if (blocks[layer].size()) {
			static char path[256];
			sprintf(path, "crafty/%04d.obj", (int)layer);
			auto cubeDD = cube->getDrawData();
			auto& cubeVerts = cubeDD->getVertices();
			auto& cubeIndices = cubeDD->getIndices();
			std::vector<Vertex> verts;
			std::vector<Index_t> idxs;
			size_t counter = 0;
			for (auto& pos : blocks[layer]) {
				for (auto vtx : cubeVerts) {
					vtx.pos += pos;
					verts.push_back(vtx);
				}
				for (auto idx : cubeIndices) {
					idxs.push_back((Index_t)(idx + cubeVerts.size() * counter));
				}
				counter++;
			}
			ModelSpawnParams cubeParams;
			cubeParams.path = path;

			auto pObj = pEngine->getClass("Model3D", &cubeParams);
			pObj->getDrawData()->getVertices() = verts;
			pObj->getDrawData()->getIndices() = idxs;
			pObj->getDrawData()->getObjects()[0]->offset = (int)idxs.size();
			pObj->postInit();
			pObj->buildPhysics();
			pObj->add();
			block_objects[layer] = pObj;
		}
		blocks_ready[layer] = true;
	}

	void updatePlayer(double dt, double t) {
		if (player) {
			float3 actorPos = pEngine->getCameraPos();
			actorPos.z -= 1.73f;
			float3 dir = pEngine->getCameraDir();
			float3 xyDir = dir;
			xyDir.z = 0.0f;
			xyDir = xyDir.normalize();
			float dst = -0.25f;
			actorPos += xyDir * float3(dst, dst, 0.0f);
			float4 rotation = XMQuaternionIdentity();
			dir = dir.normalize();
			//rotation = XMQuaternionMultiply(rotation.raw(), XMQuaternionRotationRollPitchYaw(asin(dir.z), 0, 0));
			rotation = XMQuaternionMultiply(rotation.raw(), XMQuaternionRotationRollPitchYaw(0.0f, 0, atan2(-dir.x, dir.y)));
			rotation = XMQuaternionMultiply(rotation.raw(), XMQuaternionRotationRollPitchYaw(0.0f, 0.0f, PI));
			player->setPos(actorPos);
			player->setRotation(rotation);
		}

		if (deagle) {
			if (player) {
				IAnimated* pAnimated = GetComponent<IAnimated>(player);
				if (pAnimated) {
					Ptr<IJoint> pChest = pAnimated->getJointByName("metarig_shoulder_R");
					if (pChest) {
						auto dir = pEngine->getCameraDir();
						pChest->setRotation(XMQuaternionRotationRollPitchYaw(asin(-dir.z), 0.0f, 0.0f));
					}
					else cout << "Couldnt find chest bone!" << endl;
				}
			}
			else {
				auto dir = pEngine->getCameraDir();
				dir = dir.normalize();
				deagle->setPos(pEngine->getCameraPos() + 0.1f * dir + float3(0.0f, 0.0f, 0.015f * sin(2 * PI * 0.2f * float(t))));
				float4 rotation = deagle->getRotation();
				rotation = XMQuaternionIdentity();
				rotation = XMQuaternionMultiply(rotation.raw(), XMQuaternionRotationRollPitchYaw(asin(dir.z), 0, 0));
				rotation = XMQuaternionMultiply(rotation.raw(), XMQuaternionRotationRollPitchYaw(0.0f, 0, atan2(-dir.x, dir.y)));
				deagle->setRotation(rotation);
			}
		}
	}

	virtual void onKeyPressed(int keyCode) {
		if (keyCode == 'F') flyMode = !flyMode;
		if (keyCode == 'C') pEngine->getCamera()->setPos(pEngine->getCamera()->getPos() + float3(0.0f, 0.0f, 4.0f));
		if (keyCode >= '0' && keyCode <= '9') {
			currentLayer = keyCode - '0';
		}
	}

	virtual void onKeyUp(int keyCode) {
		keyStates[keyCode] = 0;
		lastKey = 0;
	}

	virtual void onKeyDown(int keyCode) {
		keyStates[keyCode] = 1;
		lastKey = keyCode;
	}

	virtual void onMousePressed(int mouseBtn) {
		
	}

	virtual void onScroll(int dir) {
		
	}

	virtual void onMouseUp(int mouseBtn) {

	}

	virtual void onMouseDown(int mouseBtn) {
		RayTraceParams params;
		params.center = true;
		params.is3d = false;
		IObject3D *pObject = pEngine->raytraceObject(params);
		if (pObject) {
			if (mouseBtn == MOUSE_RIGHT) {
				float3 pos = params.hitPoint * 0.5f - params.hitNormal * 0.5f;
				pos.x = round(pos.x);
				pos.y = round(pos.y);
				pos.z = round(pos.z);
				pos *= 2.0f;
				for (auto i = 0; i < block_objects.size(); i++) {
					if (block_objects[i] && block_objects[i].get() == pObject) {
						removeBlock(i, pos);
						break;
					}
				}
			} else {
				float size = 1.0f;
				float3 pos = params.hitPoint * 0.5f + params.hitNormal * 0.5f;
				pos.x = round(pos.x);
				pos.y = round(pos.y);
				pos.z = round(pos.z);
				pos *= 2.0f;
				spawnBlock(currentLayer, pos, size);
			}
		}
	}

	virtual void onMessage(const Message& msg) {

	}

	virtual bool isActorGhost() {
		return false;
	}

	virtual bool overrideCamera() {
		return false;
	}

	virtual bool overrideKeyboard() {
		return false;
	}

	virtual bool overrideMouse() {
		return true;
	}

	virtual bool overrideMovement() {
		return false;
	}

	virtual bool getCameraPos(float3& pos) {
		if (!firstPerson) {
			float3 dir = pEngine->getCameraDir();
			pos += dir * -2.0f;
			float q = -PI / 2;
			float dx = dir.x * cos(q) - dir.y * sin(q);
			float dy = dir.x * sin(q) + dir.y * cos(q);
			pos.x -= dx;
			pos.y -= dy;
		}
		return false;
	}

	virtual bool getCameraDir(float3& dir) {
		return false;
	}

	virtual const char *getName() {
		return "Shooter";
	}

	virtual void release() {

	}
};

extern "C" {
	__declspec(dllexport) IGame* CreateGame() {
		return new CShooter();
	}
}