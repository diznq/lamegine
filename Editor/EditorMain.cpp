#define _CRT_SECURE_NO_WARNINGS
#include "../Lamegine/Interfaces.h"
#include <sstream>
#include <string>
#include <stdlib.h>
#include "../Lamegine/JSON.h"

#pragma warning(disable : 4244)

using namespace nlohmann;

class CEditor : public IGame {

	enum EEditMode {
		eEM_Spawn,
		eEM_AttachSound
	};
	
	EEditMode editMode = eEM_Spawn;

	IEngine *pEngine;
	IWindow *pWindow;
	IWorld *pWorld;
	std::shared_ptr<IObject3D> player;

	int lastKey;
	
	int activeMode = 0;
	int modeDirKey = 0;
	int paint = 0;
	float3 modeDir = float3(0.0f, 0.0f, 1.0f);

	bool finished = false;

	int component = 0;

	std::vector<std::shared_ptr<ISound>> sounds;
	std::shared_ptr<ISound> jmp_sound, start_sound, end_sound, bgm;

	int console = 0;
	char cmd[255];
	int cmdAt = 0;

	std::vector<std::shared_ptr<void>> subscribers;

	int keyStates[256] = { 0 };

	std::string selectedPath = "";

	virtual bool overrideLoader() {
		return false;
	}

	int mouseUps = 0;

	virtual void init(IEngine *pEngine) {
		if (!pEngine->checkCompatibility(LAMEGINE_SDK_VERSION)) {
			char msg[100];
			int v = pEngine->getVersion();
#define MAJ(v)  (((v & 0xFFFF0000)>>16))
#define MID1(v) (((v & 0x0000F000)>>12))
#define MID2(v) (((v & 0x00000F00)>>8))
#define MIN(v)  ((v & 0xFF))
			sprintf(msg, "You are running incompatibile version of engine\n Mod SDK version: %d.%d.%d.%d (%08X)\n Engine SDK version: %d.%d.%d.%d (%08X)",
				MAJ(LAMEGINE_SDK_VERSION), MID1(LAMEGINE_SDK_VERSION), MID2(LAMEGINE_SDK_VERSION), MIN(LAMEGINE_SDK_VERSION), LAMEGINE_SDK_VERSION,
				MAJ(v), MID1(v), MID2(v), MIN(v), v
			);
			pEngine->shutdown(msg);
			return;
		}

		cmd[0] = 0;
		cmdAt = 0;

		srand((unsigned int)time(0));
		this->pEngine = pEngine;
		pWindow = pEngine->getWindow();
		pWorld = pEngine->getWorld();
	}
	virtual void onUpdate(double dt) {
		static double lastX = 0.0f, lastY = 0.0f;
		double mouseX = 0.0f, mouseY = 0.0f;
		pWindow->GetCursorPos(&mouseX, &mouseY);

		char msg[512];
		IObject3D *pSelObj = pEngine->getSelectedObject();

		if (pSelObj && activeMode && pWindow->IsMouseDown(MOUSE_LEFT)) {
			RayTraceParams params;
			params.center = false;
			params.mouseX = float(mouseX);
			params.mouseY = float(mouseY);
			params.distance = 1024.0f;
			params.except = pSelObj;
			pEngine->raytraceObject(params);
			float3 pos = pSelObj->getPos();
			pos.x = params.hitPoint.x; pos.y = params.hitPoint.y;
			switch (activeMode) {
			case 'G':
				pSelObj->setPos(pos);
				break;
			}
		}

		if (pWindow->IsKeyDown('P') && selectedPath.length()) {
			spawnEntity(selectedPath, false);
		}

		const char *modeName = "none";
		switch (activeMode) {
		case 'G':
			modeName = "move";
			break;
		case 'R':
			modeName = "rotate";
			break;
		case 'K':
			modeName = "exposure";
			break;
		case 'C':
			modeName = "scale";
			break;
		case 'J':
			modeName = "gamma";
			break;
		case 'H':
			modeName = "shadow";
			break;
		}

		sprintf(msg, "Active mode: %s\nSelected path: %s\nExposure: %f, gamma: %f, shadow: %f", modeName, selectedPath.c_str(), pWorld->constants.exposure, pWorld->constants.gamma, pWorld->constants.shadowStrength);
		pEngine->drawText(20.0f, 78.0f, msg);

		lastX = mouseX;
		lastY = mouseY;
	}

	void executeConsoleCommand() {
		std::stringstream ss;
		ss << cmd;
		std::string key, val;
		if (ss >> key >> val) {
			std::map<std::string, std::string> args;
			args[key] = val;
			pWorld->load(args);
		}
	}

	void spawnEntity(std::string path, bool physics) {
		ModelSpawnParams modelParams;
		modelParams.path = path.c_str();
		double mouseX = 0.0f, mouseY = 0.0f;
		pWindow->GetCursorPos(&mouseX, &mouseY);

		RayTraceParams params;
		params.center = false;
		params.mouseX = float(mouseX);
		params.mouseY = float(mouseY);
		params.distance = 1024.0f;
		IObject3D *pObj = pEngine->raytraceObject(params);
		if (editMode == eEM_Spawn) {
			float3 pos = params.hitPoint;

			auto model = pEngine->getClass("Model3D", &modelParams);
			model->setPos(pos);
			if (physics)
				model->buildPhysics();
			model->add();
		} else if(editMode == eEM_AttachSound && pObj) {
			float3 relPos = params.hitPoint - pObj->getPos();
			auto sound = pEngine->getSoundSystem()->getSound(path.c_str(), true);
			pObj->attachSound(sound, relPos);
			sound->play();
		}
	}

	virtual void onKeyPressed(int keyCode) {
		static bool modePick = false;
		static bool dirPick = false;
		if (keyCode == 'N' && selectedPath.c_str()) {
			spawnEntity(selectedPath, true);
		} else if (keyCode == 'L') {
			double cx, cy; pWindow->GetCursorPos(&cx, &cy);
			RayTraceParams params;
			params.mouseX = float(cx);
			params.mouseY = float(cy);
			params.center = false;
			IObject3D *pGameObj = pEngine->raytraceObject(params);
			params.hitPoint += params.hitNormal;
			if (pGameObj) {
				float3 relPos = params.hitPoint - pGameObj->getPos();
				auto pLight = pEngine->getLightSystem()->createDynamicLight(Light(float4(modeDir.x, modeDir.y, modeDir.z, 1.0f), float3(0.0f, 0.0f, 0.0f), 50.0f));
				pGameObj->attachLight(pLight, relPos);
				pEngine->getLightSystem()->addLight(pLight);
			} else pEngine->getLightSystem()->addLight(pEngine->getLightSystem()->createDynamicLight(Light(float4(modeDir.x, modeDir.y, modeDir.z, 1.0f), params.hitPoint, 10.0f)));
		} else {
			if (modePick) {
				switch (keyCode) {
				case 'R':
				case 'G':
				case 'C':
				case 'K':
				case 'J':
				case 'H':
					activeMode = keyCode;
					break;
				case 'M':
					modePick = false;
					dirPick = false;
				}
			} else if (dirPick) {
				switch (keyCode) {
				case 'X':
					modeDir = float3(1.0f, 0.0f, 0.0f);
					break;
				case 'Y':
					modeDir = float3(0.0f, 1.0f, 0.0f);
					break;
				case 'Z':
					modeDir = float3(0.0f, 0.0f, 1.0f);
					break;
				case 'D':
					modePick = false;
					dirPick = false;
				}
			} else {
				switch (keyCode) {
				case 'X':
					modeDir = float3(1.0f, 0.0f, 0.0f);
					break;
				case 'Y':
					modeDir = float3(0.0f, 1.0f, 0.0f);
					break;
				case 'Z':
					modeDir = float3(0.0f, 0.0f, 1.0f);
					break;
				case 'M':
					modePick = true;
					dirPick = false;
					break;
				case 'V':
					pWorld->constants.fogSettings.w *= 2.0f;
					break;
				case 'F':
					pWorld->constants.fogSettings.w *= 0.5f;
					break;
				case 'E':
					pWorld->constants.sunDir = pEngine->getCameraDir();
					break;
				case VK_DELETE:
					if (pEngine->getSelectedObject()) {
						pEngine->removeObject(pEngine->getSelectedObject());
					}
					onObjectUnselected();
					break;
				}
			}
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
		if (mouseBtn == MOUSE_LEFT) {
			IObject3D* pSelObj = pEngine->getSelectedObject();
			double cx, cy;
			pWindow->GetCursorPos(&cx, &cy);
			RayTraceParams params;
			params.mouseX = float(cx);
			params.mouseY = float(cy);
			params.center = false;
			if (IObject3D *pGameObj = pEngine->raytraceObject(params)) {
				if (pGameObj == pSelObj && !activeMode) {
					pGameObj = 0;
					activeMode = 0;
					onObjectUnselected();
				} else {
					if (pGameObj != pSelObj) {
						pEngine->selectObject(pGameObj);
						activeMode = 0;
						onObjectSelected(pGameObj);
					}
				}
			} else {
				pEngine->selectObject(0);
				onObjectUnselected();
			}
		}
	}

	json getObjectInfo(Ptr<IObject3D> pObj) { return getObjectInfo(pObj.get()); }
	json getObjectInfo(IObject3D *pGameObj) {
		json info;

		float3 pos = pGameObj->getPos();
		float4 rot = pGameObj->getRotation();
		float4 internalRot = pGameObj->getInternalRotation();
		float3 scale = pGameObj->getScale();

		info["id"] = pGameObj->getEntityId();
		info["name"] = pGameObj->getName();
		info["class"] = pGameObj->getClassName();
		info["pos"] = { pos.x, pos.y, pos.z };
		info["rotation"] = { rot.x, rot.y, rot.z, rot.w };
		info["internal_rotation"] = { internalRot.x, internalRot.y, internalRot.z, internalRot.w };
		info["scale"] = { scale.x, scale.y, scale.z };
		info["animated"] = pGameObj->isAnimated();
		info["physics"] = pGameObj->getPhysics() && pGameObj->getPhysics()->isEnabled();

		return info;
	}

	virtual void onObjectSelected(IObject3D *pGameObj) {
		if (pGameObj) {
			for (auto sub : subscribers) {
				json response;
				json data;
				data["action"] = "object_selected";
				data["value"] = getObjectInfo(pGameObj);
				response["id"] = 0;
				response["data"] = data;
				pEngine->replyMessage(Message{ sub, response.dump() });
			}
		}
	}

	virtual void onObjectUnselected() {
		for (auto sub : subscribers) {
			json response;
			json data;
			data["action"] = "object_unselected";
			response["id"] = 0;
			response["data"] = data;
			pEngine->replyMessage(Message{ sub, response.dump() });
		}
	}

	virtual void onMessage(const Message& msg) {
		auto obj = json::parse(msg.msg);
		if (obj.count("id") && obj.count("data")) {
			int id = obj["id"].get<int>();
			auto data = obj["data"];
			if (data.count("action")) {
				auto action = data["action"];
				if (action == "subscribe") {
					subscribers.push_back(msg.hdl);
					json response;
					response["id"] = id;
					response["data"] = "ok";
					pEngine->replyMessage(Message{ msg.hdl, response.dump() });
				} else if (action == "set_model_path") {
					selectedPath = data["value"].get<std::string>();
					if (selectedPath.find(".ogg") != std::string::npos) editMode = eEM_AttachSound;
					else editMode = eEM_Spawn;
				}
			}
		}
	}

	virtual void onScroll(int dir) {
		IObject3D* pSelObj = pEngine->getSelectedObject();
		if (pSelObj) {
			if (activeMode == 'G') {
				pSelObj->setPos(pSelObj->getPos() + float3(0.0f, 0.0f, float(dir / 4.0f * pEngine->getWorld()->delta)));
			} else if(activeMode == 'R') {
				float4 rotation = pSelObj->getRotation();
				auto raw = rotation.raw();
				rotation = XMQuaternionMultiply(raw, XMQuaternionRotationRollPitchYaw(modeDir.x * dir / DEG, modeDir.y * dir / DEG, modeDir.z * dir / DEG));
				pSelObj->setRotation(rotation);
			} else if (activeMode == 'C') {
				pSelObj->setScale(pSelObj->getScale() + (modeDir * (dir / 4.0f * pEngine->getWorld()->delta)));
			}
		} else if (activeMode == 'K') {
			pWorld->constants.exposure += 0.01f * (float)dir;
		} else if (activeMode == 'J') {
			pWorld->constants.gamma += 0.01f * (float)dir;
		} else if (activeMode == 'H') {
			pWorld->constants.shadowStrength += 0.01f * (float)dir;
		}
	}

	virtual void onMouseUp(int mouseBtn) {
		mouseUps++;
	}

	virtual void onMouseDown(int mouseBtn) {

	}

	virtual bool isActorGhost() {
		return true;
	}

	virtual bool overrideCamera() {
		return false;
	}

	virtual bool overrideKeyboard() {
		return console;
	}

	virtual bool overrideMouse() {
		return true;
	}

	virtual bool overrideMovement() {
		return console;
	}

	virtual bool getCameraPos(float3& pos) {
		return false;
	}

	virtual bool getCameraDir(float3& dir) {
		return false;
	}

	virtual const char *getName() {
		return "Editor";
	}

	virtual void release() {

	}
};

extern "C" {
	__declspec(dllexport) IGame* CreateGame() {
		return new CEditor();
	}
}