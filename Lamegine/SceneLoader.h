#pragma once
#include "Engine.h"

class Scene {
public:
	Scene(const char *path);
	~Scene();
	void load();
private:
	std::string name;
	std::vector<IObject3DPtr> objects;
	std::string bgm;
	float bgmVolume;
	float bgmPitch;
	RadiosityMap radiosity;
	size_t size;
};