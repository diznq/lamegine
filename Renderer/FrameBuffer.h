#pragma once
#include "../Lamegine/Interfaces.h"
#include <Windows.h>
#include <D3D11.h>
#pragma comment(lib, "D3D11")
#pragma comment(lib, "DirectXTK")

#include "ShaderProgram.h"
#include <map>

class DepthTexture;

struct RenderTarget : public IRenderTarget {
private:
	ID3D11RenderTargetView* renderTarget;
public:
	RenderTarget(ID3D11RenderTargetView *view) : renderTarget(view) {}
	virtual void* getHandle() {
		return renderTarget;
	}
	virtual void** getHandleRef() {
		return (void**)&renderTarget;
	}
};

struct DepthStencil : public IDepthStencil {
private:
	ID3D11DepthStencilView* depthStencil;
public:
	DepthStencil(ID3D11DepthStencilView *view) : depthStencil(view) {}
	virtual void* getHandle() {
		return depthStencil;
	}
	virtual void** getHandleRef() {
		return (void**)&depthStencil;
	}
};

class FrameTexture : public ITexture {
private:
	static int totalSize;
	Ptr<ITexture> depthWrapper;
	Microsoft::WRL::ComPtr<ID3D11Texture2D> pTexture;
	Microsoft::WRL::ComPtr<ID3D11Texture2D> pDepthStencil;
	Microsoft::WRL::ComPtr<ID3D11DepthStencilView> pDepthStencilView;
	Microsoft::WRL::ComPtr<ID3D11RenderTargetView> pRenderTargetView;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> pShaderView;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> pDepthShaderView;
	D3D11_VIEWPORT viewport;
	DXGI_FORMAT format;
	int width;
	int height;
	int density;
	int renderTargets;
	int realBpp;
	int calcBpp;
	int size;
	std::string name;
	bool hasDepthStencil;
public:
	FrameTexture(int w, int h, int bpp=8, bool hasDepth=true, int offx=0, int offy=0, const char *szName=0, int msaa=0, int mipLevels=1);
	FrameTexture(Microsoft::WRL::ComPtr<ID3D11RenderTargetView> pRTV,
		Microsoft::WRL::ComPtr<ID3D11DepthStencilView> pDSV,
		D3D11_VIEWPORT vp,
		int w,
		int h,
		const char *szName);

	int getSize();

	static int getTotalSize();

	~FrameTexture() {
		totalSize -= size;
	}

	Microsoft::WRL::ComPtr<ID3D11Texture2D>& getResource();
	Microsoft::WRL::ComPtr<ID3D11RenderTargetView>& getRenderTargetView();
	Microsoft::WRL::ComPtr<ID3D11Texture2D>& getDepthStencil();
	Microsoft::WRL::ComPtr<ID3D11DepthStencilView>& getDepthStencilView();

	virtual void attach(IEngine *pEngine, int slot) { }
	virtual size_t getMemoryUsage() const { return 0; }

	virtual ID3D11ShaderResourceView** getShaderViewUnsafe() { return pShaderView.GetAddressOf(); }
	virtual Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>& getShaderViewSafe() { return pShaderView; }

	virtual void* getShaderView() { return pShaderView.Get(); }
	virtual void **getShaderViewRef() { return (void**)pShaderView.GetAddressOf(); }

	virtual ID3D11ShaderResourceView** getDepthShaderViewUnsafe() { return pDepthShaderView.GetAddressOf(); }
	virtual Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>& getDepthShaderView() { return pDepthShaderView; }

	virtual std::string getPath() { return "://VRAM"; }
	virtual std::string getName() { return name; }
	virtual void addRef() { }
	virtual void release() { }
	virtual int getSlot() const { return 0; }

	virtual int getWidth() const ;
	virtual int getHeight() const ;
	virtual int getBitsPerPixel() const;
	virtual bool read(std::vector<unsigned char>& data) const;
	virtual bool read(std::vector<float>& data) const;
	virtual bool read(std::vector<float2>& data) const;
	virtual bool read(std::vector<float3>& data) const;
	virtual bool read(std::vector<float4>& data) const;

	virtual void generateMips();

	virtual DXGI_FORMAT getFormat() const;

	bool hasDepth();
	void onInit();

	virtual SerializeData serialize();
	virtual void deserialize(const SerializeData& params);

	virtual Ptr<ITexture> getDepthTexture() {
		return depthWrapper;
	}

	virtual void* getHandle() { return pTexture.Get(); }
	virtual void** getHandleRef() { return (void**)pTexture.GetAddressOf(); }

	virtual void refresh(bool force = false) {}
};


class DepthTexture : public ITexture {
	FrameTexture *parent;
public:
	DepthTexture(FrameTexture *par) : parent(par) {}
	~DepthTexture() {

	}

	virtual void attach(IEngine *pEngine, int slot) { }
	virtual size_t getMemoryUsage() const { return 0; }

	virtual ID3D11ShaderResourceView** getShaderViewUnsafe() { return parent->getDepthShaderViewUnsafe(); }
	virtual Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>& getShaderViewSafe() { return parent->getDepthShaderView(); }

	virtual void* getShaderView() { return parent->getDepthShaderView().Get(); }
	virtual void** getShaderViewRef() { return (void**)parent->getDepthShaderViewUnsafe(); }

	virtual ID3D11ShaderResourceView** getDepthShaderViewUnsafe() { return parent->getDepthShaderViewUnsafe(); }
	virtual Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>& getDepthShaderView() { return parent->getDepthShaderView(); }

	virtual std::string getPath() { return "://VRAM"; }
	virtual std::string getName() { return "://Depth"; }
	virtual void addRef() { }
	virtual void release() { }
	virtual int getSlot() const {
		return parent->getSlot();
	}

	virtual int getWidth() const {
		return parent->getWidth();
	}
	virtual int getHeight() const {
		return parent->getHeight();
	}
	virtual int getBitsPerPixel() const {
		return 32;
	}
	virtual bool read(std::vector<unsigned char>& data) const {
		return false;
	}
	virtual bool read(std::vector<float>& data) const {
		return false;
	}
	virtual bool read(std::vector<float2>& data) const {
		return false;
	}
	virtual bool read(std::vector<float3>& data) const {
		return false;
	}
	virtual bool read(std::vector<float4>& data) const {
		return false;
	}

	virtual void generateMips() {

	}

	virtual DXGI_FORMAT getFormat() const {
		return DXGI_FORMAT::DXGI_FORMAT_R24G8_TYPELESS;
	}

	virtual SerializeData serialize() {
		return SerializeData();
	}
	virtual void deserialize(const SerializeData& params) {

	}
	virtual Ptr<ITexture> getDepthTexture() {
		return shared_from_this();
	}

	virtual void* getHandle() { return parent->getHandle(); }
	virtual void** getHandleRef() { return parent->getHandleRef(); }
	virtual void refresh(bool force = false) {}
};

class FrameBuffer : public IFrameBuffer {
private:
	matrix *view;
	matrix *proj;
	matrix *world;
	Viewport viewport;
	Ptr<FrameTexture> texture;
	Ptr<IShader> pVertexShader, pPixelShader;
	Ptr<RenderTarget> renderTarget;
	Ptr<DepthStencil> depthStencil;
	const char *name;
public:
	FrameBuffer(
		Microsoft::WRL::ComPtr<ID3D11RenderTargetView> pRTV,
		Microsoft::WRL::ComPtr<ID3D11DepthStencilView> pDSV,
		D3D11_VIEWPORT vp,
		int w,
		int h,
		Ptr<IShader> vertexShader, Ptr<IShader> pixelShader, int msaa=0);

	FrameBuffer(const char *szName,
		int w,
		int h,
		Ptr<IShader> vertexShader,
		Ptr<IShader> pixelShader,
		int bpp = 8,
		int rtc = 1,
		int msaa = 0,
		int mipLevels = 1);

	FrameBuffer(const char *szName,
		Ptr<FrameTexture> ftex,
		Ptr<IShader> vertexShader,
		Ptr<IShader> pixelShader,
		float offx, float offy, int msaa=0);

	virtual Ptr<ITexture> getTexture();
	virtual Ptr<IShader> getPixelShader();
	virtual Ptr<IShader> getVertexShader();
	void setTexture(Ptr<FrameTexture> tx);
	virtual void setPixelShader(Ptr<IShader> pShader);
	virtual void setVertexShader(Ptr<IShader> pShader);

	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>& getShaderView();
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>& getDepthShaderView();
	Microsoft::WRL::ComPtr<ID3D11DepthStencilView>& getDepthStencilView();
	virtual Ptr<IRenderTarget> getRenderTarget() { return renderTarget; }
	Microsoft::WRL::ComPtr<ID3D11RenderTargetView>& getRenderTargetView();
	virtual Ptr<IDepthStencil> getDepthStencil() { return depthStencil; }

	virtual Ptr<ITexture> getDepthTexture() {
		return texture->getDepthTexture();
	}

	bool hasDepthStencil();

	virtual Viewport& getViewport();
	void setViewport(Viewport vp);

	bool getVPW(matrix* v, matrix* p, matrix* w);
	void setVPW(matrix* v, matrix* p, matrix* w);

	virtual matrix* getView();
	virtual matrix* getProj();
	virtual matrix* getWorld();
};