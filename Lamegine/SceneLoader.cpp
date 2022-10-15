#include "shared.h"
#include "World.h"
#include "VectorMathExt.h"
#include "SceneLoader.h"
#include "JSON.h"
#include "Engine.h"
#include <btBulletDynamicsCommon.h>

Scene::~Scene() {
	
}

float3 readFloat3(nlohmann::json pos) {
	return float3(
		pos[0].get<float>(),
		pos[1].get<float>(),
		pos[2].get<float>()
	);
}

float4 readFloat4(nlohmann::json pos) {
	return float4(
		pos[0].get<float>(),
		pos[1].get<float>(),
		pos[2].get<float>(),
		pos[3].get<float>()
	);
}

Scene::Scene(const char *path) {
	bgmVolume = 1.0f;
	bgmPitch = 0.0f;
	bgm = "";
	auto physicSpec = [=](btRigidBody* body) -> void {
		body->setFriction(1.0f);
		body->setRollingFriction(0.5f);
		body->setSpinningFriction(0.5f);
	};
	try {
		IEngine *pEngine = Engine::getInstance();
		FILE *f = fopen(path, "rb");
		if (!f) {
			cout << "[error] " << strerror(errno) << endl;
			Engine::Terminate("Failed to load scene");
			return;
		}
		long sz = 0;
		fseek(f, 0, SEEK_END);
		sz = ftell(f);
		fseek(f, 0, SEEK_SET);
		char *data = new char[sz + 1];
		data[sz] = 0;
		fread(data, sz, 1, f);
		fclose(f);
		using namespace nlohmann;
		json scene = json::parse(data);
		name = "Unknown";
		if (scene.count("name")) {
			name = scene["name"].get<std::string>();
		}
		if (scene.count("objects")) {
			auto objs = scene["objects"];
			for (auto& it : objs.items()) {
				float mass = 0.0f;
				auto object = it.value();
				Vec3 position(0.0f, 0.0f, 0.0f);
				Vec4 rotation(0.0f, 0.0f, 0.0f, 1.0f);
				Vec3 scale(1.0f, 1.0f, 1.0f);
				if (object.count("pos")) {
					auto pos = object["pos"];
					position = (Vec3(
						pos[0].get<float>(),
						pos[1].get<float>(),
						pos[2].get<float>()
					));
				}
				if (object.count("rotation")) {
					auto rot = object["rotation"];
					rotation = (Vec4(
						rot[0].get<float>(),
						rot[1].get<float>(),
						rot[2].get<float>(),
						rot[3].get<float>()
					));
				}
				if (object.count("scale")) {
					scale = readFloat3(object["scale"]);
				}
				bool physics = true;
				if (object.count("physics")) {
					auto physObj = object["physics"];
					if (physObj.count("enabled")) {
						physics = physObj["enabled"].get<bool>();
					}
					if (physObj.count("mass")) {
						mass = physObj["mass"].get<float>();
					}
				}
				std::string type("object");
				if (object.count("type")) type = object["type"].get<std::string>();
				
				IObject3DPtr mdl;
				if (type == "vehicle") {
					ModelSpawnParams params;
					params.path = object["path"].get<std::string>().c_str();
					auto veh = pEngine->getClass("Vehicle", &params, false);
					if (object.count("vehicle")) {
						auto vehCtl = std::dynamic_pointer_cast<IVehicle>(veh);
						if (vehCtl) {
							auto vehProps = object["vehicle"];
							if (vehProps.count("controller")) {
								HMODULE hLib = LoadLibraryA(vehProps["controller"].get<std::string>().c_str());
								if (hLib) {
									typedef void*(*PFNCONTROLLER)();
									PFNCONTROLLER ctl = (PFNCONTROLLER)GetProcAddress(hLib, "getController");
									if (!ctl) {
										Engine::Terminate("Invalid controller DLL");
										break;
									}
									vehCtl->setController(ctl());
								}
							}
						}
					}
					mdl = veh;
				} else {
					ModelSpawnParams params;
					params.path = object["path"].get<std::string>().c_str();
					mdl = pEngine->getClass("Model3D", &params, false);
				}
				mdl->setScale(scale);
				mdl->init(pEngine);
				mdl->setPos(position).setRotation(rotation);
				if (type == "terrain") {
					mdl->setType(TERRAIN);
				}
				if (physics) {
					mdl->buildPhysics(mass, ShapeInfo(), physicSpec);
				}
				if (object.count("lights")) {
					for (auto lightIt : object["lights"].items()) {
						auto lightObj = lightIt.value();
						auto pLS = pEngine->getLightSystem();
						Light light;
						light.intensity = 0.0f;
						Vec3 relPos;
						if (lightObj.count("pos")) {
							relPos = readFloat3(lightObj["pos"]);
						}
						if (lightObj.count("color")) {
							light.color = readFloat3(lightObj["color"]);
						}
						if (lightObj.count("intensity")) {
							light.intensity = lightObj["intensity"].get<float>();
						}
						
						auto dynLight = (pLS->createDynamicLight(light, nullptr, mdl.get(), relPos ));
						auto rPos = dynLight->getRelativePos();
						//printf("rpos: %f %f %f | %f %f %f | %p\n", rPos.x, rPos.y, rPos.z, relPos.x, relPos.y, relPos.z, mdl.get());
						pLS->addLight(dynLight);
					}
				}
				if (object.count("sounds")) {
					for (auto soundIt : object["sounds"].items()) {
						auto soundObj = soundIt.value();
						auto pSS = pEngine->getSoundSystem();
						if (soundObj.count("path")) {
							auto sound = pSS->getSound(soundObj["path"].get<std::string>().c_str());
							if (soundObj.count("pos")) {
								auto soundPos = soundObj["pos"];
								sound->setRelativePos(Vec3(
									soundPos[0].get<float>(),
									soundPos[1].get<float>(),
									soundPos[2].get<float>()
								));
							}
							float vol = 1.0f;
							if (soundObj.count("volume")) {
								vol = soundObj["volume"].get<float>();
							}
							sound->setVolume(vol);
							mdl->attachSound(sound, sound->getRelativePos());
						}
					}
				}
				objects.push_back(mdl);
			}
		}
		if (scene.count("cubemap")) {
			auto cubemapObj = scene["cubemap"];
			if (cubemapObj.count("path")) {
				pEngine->setReflectionMap(pEngine->loadTexture(cubemapObj["path"].get<std::string>().c_str(), "Cubemap", true));
			}
			if (cubemapObj.count("pos")) {
				pEngine->setReflectionMapPosition(readFloat3(cubemapObj["pos"]));
			}
			if (cubemapObj.count("size")) {
				float3 size = readFloat3(cubemapObj["size"]) * 0.5f;
				pEngine->setReflectionMapBbox(size * (-1.0f), size);
			}
		}
		if (scene.count("radiosity")) {
			auto radiosityObj = scene["radiosity"];
			radiosity.texture = pEngine->loadTexture(radiosityObj["map"].get<std::string>().c_str(), "Radiosity", false);
			auto res = radiosityObj["resolution"];
			radiosity.resolution = Vec4(
				res[0].get<float>(),
				res[1].get<float>(),
				res[2].get<float>(),
				res[3].get<float>()
			);
			if (radiosityObj.count("multiply")) {
				auto mult = radiosityObj["multiply"];
				if (mult.count("power")) {
					radiosity.settings.y = mult["power"].get<float>();
				}
				if (mult.count("factor")) {
					radiosity.settings.x = mult["factor"].get<float>();
				}
			}
			if (radiosityObj.count("add")) {
				auto add = radiosityObj["add"];
				if (add.count("power")) {
					radiosity.settings.w = add["power"].get<float>();
				}
				if (add.count("factor")) {
					radiosity.settings.z = add["factor"].get<float>();
				}
			}
		}
		World* pWorld = (World*)pEngine->getWorld();
		if (scene.count("sun")) {
			auto sun = scene["sun"];
			if (sun.count("color")) {
				pWorld->constants.sunColor = readFloat3(sun["color"]);
			}
			if (sun.count("dir")) {
				pWorld->constants.sunDir = readFloat3(sun["dir"]);
			}
		}
		if (scene.count("sky")) {
			auto sky = scene["sky"];
			if (sky.count("color")) {
				pWorld->constants.skyColor = readFloat3(sky["color"]);
			}
		}
		if (scene.count("fog")) {
			auto fog = scene["fog"];
			if (fog.count("color")) {
				float strength = pWorld->constants.fogSettings.w;
				pWorld->constants.fogSettings = readFloat3(fog["color"]);
				pWorld->constants.fogSettings.w = strength;
			}
			if (fog.count("density")) {
				pWorld->constants.fogSettings.w = fog["density"].get<float>();
			}
		}
		if (scene.count("cascades")) {
			auto cascadesObj = scene["cascades"];
			float4 cascades = readFloat4(cascadesObj);
			pWorld->settings.shadowCascades[0] = cascades.x;
			pWorld->settings.shadowCascades[1] = cascades.y;
			pWorld->settings.shadowCascades[2] = cascades.z;
			pWorld->settings.shadowCascades[3] = cascades.w;
		}
		if (scene.count("music")) {
			bgm = scene["music"]["path"].get<std::string>();
			if (scene["music"].count("volume")) {
				bgmVolume = scene["music"]["volume"].get<float>();
			}
			if (scene["music"].count("pitch")) {
				bgmPitch = scene["music"]["pitch"].get<float>();
			}
		}
	} catch (std::exception& ex) {
		std::cout << "Failed to parse scene " << path << ", reason: " << ex.what() << std::endl;
	}
}

void Scene::load() {
	auto engine = Engine::getInstance();
	engine->setRadiosityMap(radiosity);
	for (auto it : objects) {
		it->add(false, true);
		for (auto sound : it->getAttachedSounds()) {
			auto p = sound->getRelativePos();
			sound->play();
		}
	}
	if (bgm.length() > 0) {
		auto pSS = engine->getSoundSystem();
		auto pSound = pSS->getSound(bgm.c_str(), false);
		pSound->setVolume(bgmVolume);
		pSound->setPitch(bgmPitch);
		engine->setActiveBGM(pSound);
		pSound->play();
	}
}

