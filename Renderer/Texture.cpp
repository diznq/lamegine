#include "DeviceResources.h"
#include "Texture.h"
#include <DDSTextureLoader.h>
#include <WICTextureLoader.h>
#include <iostream>

using namespace std;

int Material::groupIdCtr = 0;
int Material::activeGroupId = -1;
unsigned long Texture::usageCtr = 0;

Texture::Texture(const char *texName, const char *texPath, bool mips, int slotNo, bool srgb) {
	load(texName, texPath, mips, slotNo, srgb);
}

void Texture::load(const char *texName, const char *texPath, bool mips, int slotNo, bool srgb) {
	path = texPath;
	refs = 0;
	name = texName;
	slot = slotNo;
	char texPathLower[256];
	for (int i = 0; texPath[i]; i++) {
		texPathLower[i] = tolower(texPath[i]);
		texPathLower[i + 1] = 0;
	}
	const char *isDDS = strstr(texPathLower, ".dds");

	FILE *f = fopen(texPath, "rb");
	if (!f) return;

	struct _stat64i32 Stats;
	if (_stat(texPath, &Stats) == 0) {
		lastm = Stats.st_mtime;
	}

	fseek(f, 0, SEEK_END);
	int sz = ftell(f);
	fseek(f, 0, SEEK_SET);
	unsigned char *img = (unsigned char*)malloc(sz);
	if (!img) {
		fclose(f); return;
	}
	fread(img, sz, 1, f);
	fclose(f);
	DeviceResources *res = DeviceResources::getInstance();
	HRESULT hr;
	if (pShaderView) pShaderView.ReleaseAndGetAddressOf();
	if (!isDDS) {
		if(mips)
			hr = CreateWICTextureFromMemory(res->getDevice(), res->getDeviceContext(), img, (size_t)sz, nullptr, pShaderView.GetAddressOf());
		else hr = CreateWICTextureFromMemory(res->getDevice(), img, (size_t)sz, nullptr, pShaderView.GetAddressOf());
	} else {
		if(mips)
			hr = CreateDDSTextureFromMemory(res->getDevice(), res->getDeviceContext(), img, (size_t)sz, nullptr, pShaderView.GetAddressOf());
		else hr = CreateDDSTextureFromMemory(res->getDevice(), img, (size_t)sz, nullptr, pShaderView.GetAddressOf());
	}
	free(img);
	bpp = 32;
	if (FAILED(hr)) return;
	mem = sz;
	usageCtr += mem;
}

void Texture::release() {
	
}

Texture::~Texture() {
	usageCtr -= mem;
}

void Texture::attach(IEngine *pEngine, int slotNo) {
	int oslot = slotNo;
	if (slot != -1) slotNo = slot;
	//printf("attaching texture %s to %d (orig: %d)\n", name.c_str(), slotNo, oslot);
	auto ctx = DeviceResources::getInstance()->getDeviceContext();
	ctx->PSSetShaderResources(slotNo, 1, pShaderView.GetAddressOf());
	//printf("attached texture %s to %d / %d / %d\n", path.c_str(), slotNo, oslot, slot);
}

int Material::attach(IEngine *pEngine, int slot) {
	if (activeGroupId == groupId) {
		return (int)size();
	}
	
	int st = slot;
	auto ctx = DeviceResources::getInstance()->getDeviceContext();

	if (numViews == 0 /* || (pEngine->getFrameNumber() & 63)==0 */) {
		for (auto& sv : shaderViews) {
			sv = 0;
		}
		for (auto& it : *this) {
			//it.second->refresh();
			shaderViews[it.first] = (ID3D11ShaderResourceView*)it.second->getShaderView();
			if (it.first > numViews) {
				numViews = it.first;
			}
		}
	}

	ctx->PSSetShaderResources(0, numViews + 1, shaderViews);
	
	activeGroupId = groupId;
	return slot - st;
}

DXGI_FORMAT Texture::getFormat() const { return format; }

void TextureManager::releaseTexture(std::string name) {
	auto it = textures.find(name);
	if (it != textures.end()) {
		textures.erase(it);
	}
}

void TextureManager::print() {
	
}

std::map<std::string, std::weak_ptr<Texture> >& TextureManager::getTextures() {
	return textures;
}

Ptr<Texture> TextureManager::getTexture(const char *txName, const char *txPath, bool mips, int slot, bool srgb) {
	std::string name = txName;
	std::string path = std::string(txPath) + ":" + std::to_string(slot);
	auto it = textures.find(path);
	if (it != textures.end()) {
		if(!it->second.expired())
			return it->second.lock();
	}
	auto tx = gcnew<Texture>(txName, txPath, mips, slot, srgb);
	textures[path] = tx;
	return tx;
}

void Texture::addRef() { refs++; }

int Texture::getSlot() const { return slot; }

std::string Texture::getPath() { return path; }

std::string Texture::getName() { return name; }

void* Texture::getShaderView() { return pShaderView.Get(); }

size_t Texture::getMemoryUsage() const { return sizeof(*this) + sizeof(void*) + mem; }

ID3D11ShaderResourceView** Texture::getShaderViewUnsafe() { return pShaderView.GetAddressOf(); }


int Texture::getWidth() const {
	return width;
}
int Texture::getHeight() const {
	return height;
}
int Texture::getBitsPerPixel() const {
	return bpp;
}
bool Texture::read(std::vector<unsigned char>& data) const {
	return false;
}
bool Texture::read(std::vector<float>& data) const {
	return false;
}
bool Texture::read(std::vector<float2>& data) const {
	return false;
}
bool Texture::read(std::vector<float3>& data) const {
	return false;
}
bool Texture::read(std::vector<float4>& data) const {
	return false;
}


SerializeData Texture::serialize() {
	SerializeData data;
	data["path"] = path;
	switch (slot) {
	case TextureSlotDiffuse:
		data["type"] = "diffuse";
		break;
	case TextureSlotNormal:
		data["type"] = "normal";
		break;
	case TextureSlotAmbient:
		data["type"] = "ambient";
		break;
	case TextureSlotMisc:
		data["type"] = "misc";
		break;
	default:
		data["type"] = "unknown";
		break;
	}
	data["name"] = name;
	return data;
}
void Texture::deserialize(const SerializeData& p) {
	SerializeData params = p;
	typedef std::string Str;
	path = Str(params["path"]);
	Str type = params["type"];
	if (type == "misc") {
		slot = TextureSlotMisc;
	} else if (type == "normal") {
		slot = TextureSlotNormal;
	} else if (type == "diffuse") {
		slot = TextureSlotDiffuse;
	} else if (type == "ambient") {
		slot = TextureSlotAmbient;
	} else if (type == "alpha") {
		slot = TextureSlotAlpha;
	}
	load(((Str)params["name"]).c_str(), path.c_str(), true, slot);
}

void Texture::refresh(bool force) {
	struct _stat64i32 Stats;
	if (_stat(path.c_str(), &Stats) == 0) {
		if (Stats.st_mtime > lastm) {
			load(name.c_str(), path.c_str(), true, slot);
			lastm = Stats.st_mtime;
		}
	}
}

void Texture::generateMips() {
	DeviceResources::getInstance()->getDeviceContext()->GenerateMips(pShaderView.Get());
}

IMaterial& Material::add(Ptr<ITexture> tex) {
	(*this)[tex->getSlot()] = tex;
	return *this;
}

void Material::resetActive() {
	activeGroupId = -1;
}

bool Material::isTransparent() const { return alpha_ < 1.0f; }

Material::Material(std::string name) {
	groupId = ++groupIdCtr;
	ambient_ = float4(0.0f, 0.0f, 0.0f, 1.0f);
	diffuse_ = float4(1.0f, 1.0f, 1.0f, 1.0f);
	specular_ = float4(0.0f, 0.0f, 0.0f, 1.0f);
	pbrInfo_ = float4(1.0f, 1.0f, 1.0f, 1.0f);
	scale_ = float2(1.0f, 1.0f);
	alpha_ = 1.0f;
	refractive = false;
	this->name = name.length() ? name : "(null)";
}

Material::~Material() {
	for (auto& it : *this) {
		it.second->release();
	}
}

const char *Material::getName() const {
	return name.c_str();
}

float4& Material::ambient() {
	return ambient_;
}
float4& Material::diffuse() {
	return diffuse_;
}
float4& Material::specular() {
	return specular_;
}
float4& Material::pbr1() {
	return pbrInfo_;
}
float4& Material::pbr2() {
	return pbrExtra_;
}
float& Material::alpha() {
	return alpha_;
}
bool& Material::isRefractive() {
	return refractive;
}
float2& Material::scale() {
	return scale_;
}

SerializeData Material::serialize() {
	SerializeData data;
	return data;
}

void Material::deserialize(const SerializeData& params) {

};