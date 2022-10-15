#pragma once
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <wrl.h>
#include <Windows.h>
#include <D3D11.h>
#pragma comment(lib, "D3D11")
#pragma comment(lib, "DirectXTK")

class DeviceResources {
public:
	DeviceResources() : vsync(true) {
		g_deviceResources = this;
	}
	HRESULT createDeviceResources(HWND hWnd);
	HRESULT createDeviceResources();
	HRESULT createWindowResources(HWND hWnd);

	HRESULT configureBackBuffer();
	HRESULT releaseBackBuffer();
	HRESULT goFullScreen();
	HRESULT goWindowed();
	float getAspectRatio();

	void setVsync(bool vs);

	static DeviceResources* getInstance();
	ID3D11Device*           getDevice();
	ID3D11DeviceContext*    getDeviceContext();
	ID3D11RenderTargetView* getRenderTarget();
	ID3D11DepthStencilView* getDepthStencil();
	
	Microsoft::WRL::ComPtr<ID3D11RenderTargetView> getCOMRenderTarget();
	Microsoft::WRL::ComPtr<ID3D11DepthStencilView> getCOMDepthStencil();
	
	ID3D11BlendState*		getBlendState() { return m_pBlendState.Get(); }
	D3D11_VIEWPORT&			getViewport() { return m_viewport; }
	void present();

private:

	static DeviceResources *g_deviceResources;
	char deviceName[128];
	bool vsync;

	//-----------------------------------------------------------------------------
	// Direct3D device
	//-----------------------------------------------------------------------------
	Microsoft::WRL::ComPtr<ID3D11Device>        m_pd3dDevice;
	Microsoft::WRL::ComPtr<ID3D11DeviceContext> m_pd3dDeviceContext; // immediate context
	Microsoft::WRL::ComPtr<IDXGISwapChain>      m_pDXGISwapChain;
	Microsoft::WRL::ComPtr<ID3D11BlendState>	m_pBlendState;


	//-----------------------------------------------------------------------------
	// DXGI swap chain device resources
	//-----------------------------------------------------------------------------
	Microsoft::WRL::ComPtr < ID3D11Texture2D>        m_pBackBuffer;
	Microsoft::WRL::ComPtr < ID3D11RenderTargetView> m_pRenderTarget;


	//-----------------------------------------------------------------------------
	// Direct3D device resources for the depth stencil
	//-----------------------------------------------------------------------------
	Microsoft::WRL::ComPtr<ID3D11Texture2D>         m_pDepthStencil;
	Microsoft::WRL::ComPtr<ID3D11DepthStencilView>  m_pDepthStencilView;


	//-----------------------------------------------------------------------------
	// Direct3D device metadata and device resource metadata
	//-----------------------------------------------------------------------------
	D3D_FEATURE_LEVEL       m_featureLevel;
	D3D11_TEXTURE2D_DESC    m_bbDesc;
	D3D11_VIEWPORT          m_viewport;
};