#pragma once
#include "../Lamegine/Interfaces.h"
#include <D3D11.h>
class Engine;

class Texture : public ITexture {
private:
	Microsoft::WRL::ComPtr<ID3D11Texture2D> pTexture;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> pShaderView;
	DXGI_FORMAT format;
	std::string name;
	std::string path;
	int width;
	int height;
	int flags;
	unsigned int tex;
	int links;
	int slot;
	unsigned long mem;
	int refs;
	int bpp;
	static unsigned long usageCtr;
	time_t lastm = 0;
	void load(const char *texName, const char *texPath, bool generateMips = true, int slot = -1, bool srgb = false);
public:
	Texture(const char *texName, const char *texPath, bool generateMips=true, int slot=-1, bool srgb=false);
	virtual ~Texture();
	virtual void attach(IEngine *pEngine, int slot);
	virtual size_t getMemoryUsage() const;
	virtual ID3D11ShaderResourceView** getShaderViewUnsafe();
	virtual void* getShaderView();
	virtual std::string getPath();
	virtual std::string getName();
	virtual void addRef();
	virtual void release();
	virtual int getSlot() const;
	virtual void* getHandle() { return pTexture.Get(); }
	virtual void** getHandleRef() { return (void**)pTexture.GetAddressOf(); }

	virtual void **getShaderViewRef() { return (void**)pShaderView.GetAddressOf(); }

	virtual int getWidth() const;
	virtual int getHeight() const;
	virtual int getBitsPerPixel() const;
	virtual bool read(std::vector<unsigned char>& data) const;
	virtual bool read(std::vector<float>& data) const;
	virtual bool read(std::vector<float2>& data) const;
	virtual bool read(std::vector<float3>& data) const;
	virtual bool read(std::vector<float4>& data) const;

	virtual void generateMips();

	virtual SerializeData serialize();
	virtual void deserialize(const SerializeData& params);

	virtual DXGI_FORMAT getFormat() const;
	virtual void refresh(bool force = false);
};

class TextureManager {
	std::map<std::string, std::weak_ptr<Texture> > textures;
public:
	Ptr<Texture> getTexture(const char *txName, const char *txPath, bool mips = true, int slot = -1, bool srgb=false);
	std::map<std::string, std::weak_ptr<Texture> >& getTextures();
	TextureManager() {

	}
	~TextureManager() {

	}
	void releaseTexture(std::string name);
	void print();
};

class Material : public IMaterial {
private:
	static int groupIdCtr;
	static int activeGroupId;
	int groupId;
	std::string name;
	ID3D11ShaderResourceView* shaderViews[16];
	int numViews = 0;
	float4 ambient_, diffuse_, specular_, pbrInfo_, pbrExtra_;
	float2 scale_;
	float alpha_;
	bool refractive;
public:
	virtual bool isTransparent() const;
	Material(std::string name = "");
	virtual ~Material();
	virtual IMaterial& add(Ptr<ITexture> tex);
	virtual const char *getName() const;
	static void resetActive();
	virtual int attach(IEngine *pEngine, int slot);

	virtual Math::Vec4& ambient();
	virtual Math::Vec4& diffuse();
	virtual Math::Vec4& specular();
	virtual Math::Vec4& pbr1();
	virtual Math::Vec4& pbr2();
	virtual Math::Vec2& scale();
	virtual float& alpha();
	virtual bool& isRefractive();

	virtual SerializeData serialize();
	virtual void deserialize(const SerializeData& params);
};