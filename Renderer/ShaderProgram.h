#pragma once
#include "../Lamegine/Interfaces.h"
#include <D3D11.h>

class Shader : public IShader {
public:
	Shader(void *pEngine, const char *path, int type);
	~Shader();

	virtual void* getShader() const {
		return pGenShader;
	}
	ID3D11VertexShader* getVertexShader() {
		return pVertexShader;
	}
	ID3D11PixelShader* getPixelShader() {
		return pPixelShader;
	}
	ID3D11HullShader* getHullShader() {
		return pHullShader;
	}
	ID3D11DomainShader* getDomainShader() {
		return pDomainShader;
	}
	ID3D11InputLayout* getInputLayout() {
		return pInputLayout;
	}

	virtual int getType() const {
		return type;
	}

	virtual std::string getPath() const { return file; }
private:
	int links;
	std::string file;
	int type;
	unsigned int shader;
	bool ok;

	union {
		ID3D11VertexShader *pVertexShader;
		ID3D11PixelShader *pPixelShader;
		ID3D11HullShader *pHullShader;
		ID3D11DomainShader *pDomainShader;
		void *pGenShader;
	};

	ID3D11InputLayout *pInputLayout;
};

struct ShaderProgram {
	ShaderProgram(const char *id);
	~ShaderProgram();
	ShaderProgram& use();
	ShaderProgram& attach(Shader *shader);
	ShaderProgram& attach(const char *fileName, int type);
	ShaderProgram& link(const char *fragLoc);
	unsigned int program;
	std::vector<Shader*> shaders;
	const char *name;
};