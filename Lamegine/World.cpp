#include "shared.h"
#include "World.h"
#include <fstream>
#include <sstream>

#define setFloat(k) settings.k = (float)atof(v.c_str())
#define setInt(k) settings.k = (int)atoi(v.c_str())
#define setBool(k) settings.k = (v[0] != '0')
#define setFloatGen(p,k) p.k = (float)atof(v.c_str())
#define setIntGen(p,k) p.k = (int)atoi(v.c_str())
#define setBoolGen(p,k) p.k = (v[0] != '0')

#define Serialize(n, k) data[#n] = std::to_string(settings.k)
#define SerializeGen(n, p, k) data[#n] = std::to_string(p.k)

World* World::instance = 0;

void World::load(const char *file) {
	std::ifstream f(file);
	if (f && f.is_open()) {
		std::string line;
		std::map<std::string, std::string> args;
		while (std::getline(f, line)) {
			std::stringstream ss;
			ss << line;
			auto pos = line.find('=');
			if (pos == std::string::npos) {
				std::string key, value;
				ss >> key >> value;
				if (key[0] == '#') continue;
				if (value == "=") ss >> value;
				args[key] = value;
			} else {
				ss.clear();
				std::string key, value;
				key = line.substr(0, pos);
				value = line.substr(pos + 1);
				args[key] = value;
			}
		}
		f.close();
		load(args);
	}
}

void World::load(std::map<std::string, std::string> args) {
	for (auto& it : args) {
		std::string k = it.first;
		std::string v = it.second;
		trim(k); trim(v);
		if (k[0] == '#') continue;
		params[k] = v;
		if (k == "fov") setFloat(renderFov);
		else if (k == "shq") {
			setFloat(shadowQuality);
			setFloatGen(constants, shadowQuality);
		}
		else if (k == "rq" || k == "renq") setFloat(renderQuality);
		else if (k == "refq") setFloat(refractionQuality);
		else if (k == "fs") setBool(fullScreen);
		else if (k == "w") setFloat(width);
		else if (k == "h") setFloat(height);
		else if (k == "wf") setBool(wireFrame);
		else if (k == "vsync") setBool(vsync);
		else if (k == "tess") setBool(tessellate);
		else if (k == "jump") setFloatGen(controls, jumpHeight);
		else if (k == "step") setFloatGen(controls, stepHeight);
		else if (k == "editor") setBool(editor);
		else if (k == "ssr") setFloat(ssrQuality);
		else if (k == "dof") setBool(depthOfField);
		else if (k == "bloom") setFloatGen(constants, bloomStrength);
		else if (k == "shadow") setFloatGen(constants, shadowStrength);
		else if (k == "exposure") setFloatGen(constants, exposure);
		else if (k == "gamma") setFloatGen(constants, gamma);
		else if (k == "convert" || k == "conv") settings.convertArg = 1;
		else if (k == "cull") setBool(solidCulling);
		else if (k == "msaa") setInt(msaa);
		else if (k == "anisotropy") setInt(anisotropy);
		else if (k == "cq") setInt(cubemapQuality);
		else if (k == "hdr_min" || k == "hdr_max") {
			if (k == "hdr_min") {
				float omax = 1.0f / constants.hdrSettings.y + constants.hdrSettings.x;
				float nmin = (float)atof(v.c_str());
				constants.hdrSettings.x = nmin;
				constants.hdrSettings.y = 1.0f / (omax - nmin);
			} else if (k == "hdr_max") {
				float nmax = (float)atof(v.c_str());
				constants.hdrSettings.y = 1.0f / (nmax - constants.hdrSettings.x);
			}
		}
	}
}

World* World::getInstance() {
	if (!instance) instance = new World();
	return instance;
}

void World::init() {
	settings.shadowCascades[0] = 1.0f;
	settings.shadowCascades[1] = 1.0f;
	settings.shadowCascades[2] = 1.0f;
	settings.shadowCascades[3] = 1.0f;
}


SerializeData World::serialize() {
	SerializeData data;
	Serialize(fov, renderFov);
	Serialize(shq, shadowQuality);
	Serialize(rq, renderQuality);
	Serialize(refq, refractionQuality);
	Serialize(fs, fullScreen);
	Serialize(w, width);
	Serialize(h, height);
	Serialize(wf, wireFrame);
	Serialize(vsync, vsync);
	Serialize(tess, tessellate);
	SerializeGen(jump, controls, jumpHeight);
	SerializeGen(step, controls, stepHeight);
	Serialize(editor, editor);
	Serialize(ssr, ssrQuality);
	Serialize(dof, depthOfField);
	SerializeGen(bloom, constants, bloomStrength);
	SerializeGen(shadow, constants, shadowStrength);
	SerializeGen(exposure, constants, exposure);
	SerializeGen(gamma, constants, gamma);
	Serialize(cull, solidCulling);
	Serialize(msaa, msaa);
	Serialize(anisotropy, anisotropy);
	Serialize(cq, cubemapQuality);
	SerializeGen(hdr_min, constants, hdrSettings.x);
	float omax = 1.0f / constants.hdrSettings.y + constants.hdrSettings.x;
	data["hdr_max"] = std::to_string(omax);
	return data;
}
void World::deserialize(const SerializeData& params) {
	//load(params);
}