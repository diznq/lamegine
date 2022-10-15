#include "ShaderProgram.h"
#include "DeviceResources.h"
#include <iostream>

using namespace std;

Shader::Shader(void *p, const char *path, int type) {
	//printf("Creating shader %s (type: %d)\n", path, type);
	
	this->type = type;
	ok = false;
	IEngine *pEngine = (IEngine*)p;
	//printf("pEngine: %p\n", pEngine); 
	pVertexShader = 0;
	pGenShader = 0;
	char hlslPath[256];
	strncpy(hlslPath, path, sizeof(hlslPath));
	FILE *f = fopen(path, "rb");
	if (!f) {
		strcpy(strstr(hlslPath, ".cso"), ".hlsl");
		f = fopen(hlslPath, "rb");
		if (f) {
			fclose(f);
			cout << "Compiling shader " << hlslPath << " offline" << endl;
			char cmd[150];
			const char *stype = "ps";
			if (type == VERTEX_SHADER) stype = "vs";
			else if (type == DOMAIN_SHADER) stype = "ds";
			else if (type == HULL_SHADER) stype = "hs";
			sprintf(cmd, "utils\\fxc.exe /T %s_5_0 \"%s\" /Fo \"%s\" /O3 /nologo", stype, hlslPath, path);
			system(cmd);
			f = fopen(path, "rb");
		}
		if(!f) {
			cout << "Failed to load shader " << path << ": " << strerror(errno) << endl;
			return;
		}
	}
	//printf("file opened, seeking for end\n"); 
	fseek(f, 0, SEEK_END);
	int sz = (int)ftell(f);
	fseek(f, 0, SEEK_SET);
	//printf("malloc size: %d\n", sz);
	
	char *data = new char[sz + 16];
	if (!data) {
		cout << "Failed to load shader " << path << ": not enough memory" << endl;
		return;
	}

	fread(data, sz, 1, f);
	fclose(f);

	ID3D11Device* device = DeviceResources::getInstance()->getDevice();

	if (type == VERTEX_SHADER) {
		size_t read = sz;
		HRESULT hr = device->CreateVertexShader(data, read, nullptr, &pVertexShader);
		if (FAILED(hr)) {
			pEngine->terminate(L"Failed to create vertex shader");
		} else {
			D3D11_INPUT_ELEMENT_DESC iaDesc[] =
			{
				{ "POSITION",		0, DXGI_FORMAT_R32G32B32_FLOAT,		0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
				{ "TEXCOORD",		0, DXGI_FORMAT_R32G32_FLOAT,		0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
				{ "NORMAL",			0, DXGI_FORMAT_R32G32B32_FLOAT,		0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
				{ "TANGENT",		0, DXGI_FORMAT_R32G32B32_FLOAT,		0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
				{ "BLENDINDICES",	0, DXGI_FORMAT_R32G32B32_UINT,		0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
				{ "BLENDWEIGHT",	0, DXGI_FORMAT_R32G32B32_FLOAT,		0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
			};
			
			hr = device->CreateInputLayout( iaDesc, ARRAYSIZE(iaDesc), data, read, &pInputLayout );
			if (FAILED(hr)) {
				cout << "Fail: " << path << ", error: " << hex << hr << dec << endl;
				pEngine->terminate(L"Failed to create input layout");
			} else ok = true;
		}
	} else if (type == FRAGMENT_SHADER) {
		HRESULT hr = device->CreatePixelShader(data, sz, nullptr, &pPixelShader);
		if (FAILED(hr)) {
			pEngine->terminate(L"Failed to create pixel shader");
		}
		else ok = true;
	} else if (type == HULL_SHADER) {
		HRESULT hr = device->CreateHullShader(data, sz, nullptr, &pHullShader);
		if (FAILED(hr)) {
			pEngine->terminate(L"Failed to create hull shader");
		}
		else ok = true;
	} else if (type == DOMAIN_SHADER) {
		HRESULT hr = device->CreateDomainShader(data, sz, nullptr, &pDomainShader);
		if (FAILED(hr)) {
			pEngine->terminate(L"Failed to create pixel shader");
		}
		else ok = true;
	}

	links = 0;
	file = path;
}
Shader::~Shader() {

}

ShaderProgram::ShaderProgram(const char *id) {
	name = id;
	program = 0;
}
ShaderProgram::~ShaderProgram() {
	shaders.clear();
}
ShaderProgram& ShaderProgram::use() {
	return *this;
}
ShaderProgram& ShaderProgram::attach(Shader *shader) {
	shaders.push_back(shader);
	return *this;
}
ShaderProgram& ShaderProgram::attach(const char *f, int type) {
	Shader *shader = new Shader(0, f, type);
	shaders.push_back(shader);
	return *this;
}
ShaderProgram& ShaderProgram::link(const char *f) {
	return *this;
}