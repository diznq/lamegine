#pragma once
#include "../Lamegine/Interfaces.h"
#include "DeviceResources.h"
#include "Texture.h"

using Microsoft::WRL::ComPtr;

class RendererD3D11 : public IRenderer {
private:
	IEngine *pEngine = 0;
	Ptr<DeviceResources> deviceResources;
	Ptr<TextureManager> textureManager;
	static RendererD3D11* instance;
	ComPtr<ID3D11BlendState> blendState;
	ComPtr<ID3D11SamplerState> primarySampler, secondarySampler;
	ComPtr<ID3D11RasterizerState> solidCullFront, solidCullBack, solidCullNone, wireframe;
public:
	RendererD3D11(IEngine *engine);
	static RendererD3D11* getInstance() {
		return instance;
	}

	virtual void resetMaterials();

	virtual Ptr<ITexture> loadTexture(const char *f, const char *nm, bool generateMips, int slot, bool srgb);
	virtual Ptr<IShader> loadShader(const char *f, int type);

	virtual Ptr<IFrameBuffer> createFrameBuffer(const char *name, int width, int height, Ptr<IShader> vertexShader, Ptr<IShader> pixelShader, int bpp, bool hasDepth, int mipLevel=1);
	virtual Ptr<IFrameBuffer> createFrameBuffer(const char *name, Ptr<ITexture> texture, Ptr<IShader> vertexShader, Ptr<IShader> pixelShader, float left, float top);
	virtual Ptr<IFrameBuffer> createScreenOutputFrameBuffer(int width, int height, Ptr<IShader> vertexShader, Ptr<IShader> pixelShader);

	virtual void setShader(Ptr<IShader> shader);
	virtual void setPixelShaderResources(int slot, const std::vector<Ptr<ITexture>>& res);
	virtual void setVertexShaderResources(int slot, const std::vector<Ptr<ITexture>>& res);

	virtual void setPixelShaderResources(int slot, Ptr<ITexture> resource);
	virtual void setVertexShaderResources(int slot, Ptr<ITexture> resource);

	virtual void setPixelConstantBuffers(int slot, const std::vector<Ptr<IBuffer>>& buffers);
	virtual void setVertexConstantBuffers(int slot, const std::vector<Ptr<IBuffer>>& buffers);

	virtual void setPixelConstantBuffers(int slot, Ptr<IBuffer> buffers);
	virtual void setVertexConstantBuffers(int slot, Ptr<IBuffer> buffers);

	virtual void present();

	virtual void setRenderTargets(const std::vector<Ptr<IRenderTarget>>& renderTargets, Ptr<IDepthStencil> depthStencil);

	virtual void clearRenderTarget(Ptr<IRenderTarget> renderTarget, const float *color);
	virtual void clearDepthStencil(Ptr<IDepthStencil> depthStencil);

	virtual void copyResource(Ptr<ITexture> dest, Ptr<ITexture> src);

	virtual bool createDeviceResources();
	virtual bool createWindowResources(IWindow *pWindow);
	virtual void goFullScreen();
	virtual void goWindowed();
	virtual void setVsync(bool state);
	virtual void enableBlending();
	virtual void disableBlending();
	virtual void setRasterizer(ERasterizerState rs);

	virtual void setDrawBuffers(Ptr<IBuffer> vertexBuffer, Ptr<IBuffer> indexBuffer);
	virtual void setDrawData(Ptr<IDrawData> drawData);
	virtual void setRawDrawData(IDrawData *pDrawData);

	virtual void setTopology(ETopology topology);
	virtual void drawIndexedInstanced(int indexCount, int instanceCount, int offset);

	virtual void assignSamplers();

	virtual Ptr<IBuffer> createBuffer(BufferTypes flags, size_t size, void *data);

	virtual void setViewports(int no, const Viewport* vp);

	virtual Ptr<IMaterial> createMaterial(const char *name);

	template<class T> bool readTextureGeneric(const Ptr<const ITexture> pTexture, Microsoft::WRL::ComPtr<ID3D11Texture2D> pRawTex, DXGI_FORMAT format, std::vector<T>& data) const {
		Microsoft::WRL::ComPtr<ID3D11Texture2D> pCPUTex = 0;
		bool allocated = false;
		if (!allocated) {
			D3D11_TEXTURE2D_DESC textureDesc;
			ZeroMemory(&textureDesc, sizeof(textureDesc));
			textureDesc.Width = pTexture->getWidth();
			textureDesc.Height = pTexture->getHeight();
			textureDesc.MipLevels = 1;
			textureDesc.ArraySize = 1;
			textureDesc.SampleDesc.Count = 1;
			textureDesc.BindFlags = 0;
			textureDesc.Usage = D3D11_USAGE_STAGING;
			textureDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ | D3D11_CPU_ACCESS_WRITE;
			textureDesc.Format = format;
			textureDesc.MiscFlags = 0;
			if (FAILED(DeviceResources::getInstance()->getDevice()->CreateTexture2D(&textureDesc, nullptr, pCPUTex.GetAddressOf()))) {
				return false;
			}
			allocated = true;
		}
		DeviceResources::getInstance()->getDeviceContext()->CopyResource(pCPUTex.Get(), pRawTex.Get());
		D3D11_MAPPED_SUBRESOURCE sub;

		HRESULT hr = DeviceResources::getInstance()->getDeviceContext()->Map(pCPUTex.Get(), 0, D3D11_MAP_READ, 0, &sub);
		if (FAILED(hr)) {
			return false;
		}
		unsigned char* pData = (unsigned char*)sub.pData;
		data.clear();
		data.assign((T*)(pData), (T*)(pData + pTexture->getWidth() * pTexture->getHeight() * pTexture->getBitsPerPixel() / 8));
		DeviceResources::getInstance()->getDeviceContext()->Unmap(pCPUTex.Get(), 0);
		return true;
	}
};