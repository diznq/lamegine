#include "DeviceResources.h"
#include <stdio.h>
DeviceResources* DeviceResources::g_deviceResources = 0;

HRESULT DeviceResources::createDeviceResources(HWND hWnd)
{
	HRESULT hr = S_OK;
	
	D3D_FEATURE_LEVEL levels[] = {
		D3D_FEATURE_LEVEL_11_1,
		D3D_FEATURE_LEVEL_11_0,
	};
	
	UINT deviceFlags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;
	
	#if defined(DEBUG) || defined(_DEBUG)
	deviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
	#endif
	
	DXGI_SWAP_CHAIN_DESC desc;
	ZeroMemory(&desc, sizeof(DXGI_SWAP_CHAIN_DESC));
	desc.Windowed = FALSE;
	desc.BufferCount = 2;
	desc.BufferDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
	desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	desc.SampleDesc.Count = 1;
	desc.SampleDesc.Quality = 0;
	desc.SwapEffect = DXGI_SWAP_EFFECT_SEQUENTIAL;
	desc.OutputWindow = hWnd;
	
	if (!vsync) {
		desc.BufferDesc.RefreshRate.Denominator = 1;
		desc.BufferDesc.RefreshRate.Numerator = 0;
	}
	
	Microsoft::WRL::ComPtr<ID3D11Device> device;
	Microsoft::WRL::ComPtr<ID3D11DeviceContext> context;
	Microsoft::WRL::ComPtr<IDXGISwapChain> swapChain;
	
	hr = D3D11CreateDeviceAndSwapChain(
		nullptr,
		D3D_DRIVER_TYPE::D3D_DRIVER_TYPE_HARDWARE,
		nullptr,
		deviceFlags,
		levels,
		ARRAYSIZE(levels),
		D3D11_SDK_VERSION,
		&desc,
		swapChain.GetAddressOf(),
		device.GetAddressOf(),
		&m_featureLevel,
		context.GetAddressOf()
	);

	if (FAILED(hr) || !swapChain) {
		char msg[200];
		sprintf(msg, "D3D11CreateDeviceAndSwapChain failed with %X", hr);
		MessageBoxA(0, msg, "Renderer error", MB_ICONERROR);
	}
	
	device.As(&m_pd3dDevice);
	context.As(&m_pd3dDeviceContext);
	swapChain.As(&m_pDXGISwapChain);
	
	hr = m_pDXGISwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&m_pBackBuffer);
	
	m_pBackBuffer->GetDesc(&m_bbDesc);
	
	ZeroMemory(&m_viewport, sizeof(D3D11_VIEWPORT));
	
	m_viewport.Height = (float)m_bbDesc.Height;
	m_viewport.Width = (float)m_bbDesc.Width;
	m_viewport.MinDepth = 0;
	m_viewport.MaxDepth = 1;
	
	m_pd3dDeviceContext->RSSetViewports(1, &m_viewport);
	
	hr = m_pd3dDevice->CreateRenderTargetView(m_pBackBuffer.Get(), nullptr, m_pRenderTarget.GetAddressOf());
	
	return hr;
}


HRESULT DeviceResources::createDeviceResources()
{
	HRESULT hr = S_OK;
	
	D3D_FEATURE_LEVEL levels[] = {
		D3D_FEATURE_LEVEL_11_1,
		D3D_FEATURE_LEVEL_11_0,
	};
	
	UINT deviceFlags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;
	
	#if defined(DEBUG) || defined(_DEBUG)
	deviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
	#endif
	
	
	Microsoft::WRL::ComPtr<ID3D11Device>        device;
	Microsoft::WRL::ComPtr<ID3D11DeviceContext> context;
	
	hr = D3D11CreateDevice(
		nullptr,                    
		D3D_DRIVER_TYPE_HARDWARE,   
		0,                          
		deviceFlags,                
		levels,                     
		ARRAYSIZE(levels),          
		D3D11_SDK_VERSION,          
		&device,                    
		&m_featureLevel,            
		&context                    
	);
	
	if (FAILED(hr))
	{
		char msg[200];
		sprintf(msg, "D3D11CreateDevice failed with %X", hr);
		MessageBoxA(0, msg, "Renderer error", MB_ICONERROR);
	}
	
	device.As(&m_pd3dDevice);
	context.As(&m_pd3dDeviceContext);
	
	return hr;
}

HRESULT DeviceResources::createWindowResources(HWND hWnd)
{
	HRESULT hr = S_OK;
	
	DXGI_SWAP_CHAIN_DESC desc;
	ZeroMemory(&desc, sizeof(DXGI_SWAP_CHAIN_DESC));
	desc.Windowed = TRUE; 
	desc.BufferCount = 2;
	desc.BufferDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
	desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	desc.SampleDesc.Count = 1; 
	desc.SampleDesc.Quality = 0;    
	desc.SwapEffect = DXGI_SWAP_EFFECT_SEQUENTIAL;
	desc.OutputWindow = hWnd;
	
	if (!vsync) {
		desc.BufferDesc.RefreshRate.Denominator = 1;
		desc.BufferDesc.RefreshRate.Numerator = 0;
	}
	
	Microsoft::WRL::ComPtr<IDXGIDevice> dxgiDevice;
	m_pd3dDevice.As(&dxgiDevice);
	
	Microsoft::WRL::ComPtr<IDXGIAdapter> adapter;
	Microsoft::WRL::ComPtr<IDXGIFactory> factory;
	
	hr = dxgiDevice->GetAdapter(&adapter);
	
	if (SUCCEEDED(hr))
	{
		adapter->GetParent(IID_PPV_ARGS(&factory));
		
		hr = factory->CreateSwapChain(m_pd3dDevice.Get(), &desc, &m_pDXGISwapChain);
		if (FAILED(hr) || !m_pDXGISwapChain) {
			MessageBoxA(0, "DeviceResources::createWindowResources Failed to create swap chain", 0, 0);
		}
	}

	hr = configureBackBuffer();
	
	return hr;
}

HRESULT DeviceResources::configureBackBuffer()
{
	HRESULT hr = S_OK;

	hr = m_pDXGISwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&m_pBackBuffer);
	
	hr = m_pd3dDevice->CreateRenderTargetView(m_pBackBuffer.Get(), nullptr, m_pRenderTarget.GetAddressOf());
	m_pBackBuffer->GetDesc(&m_bbDesc);
	
	ZeroMemory(&m_viewport, sizeof(D3D11_VIEWPORT));
	m_viewport.Height = (float)m_bbDesc.Height;
	m_viewport.Width = (float)m_bbDesc.Width;
	m_viewport.MinDepth = 0;
	m_viewport.MaxDepth = 1;
	
	m_pd3dDeviceContext->RSSetViewports(1, &m_viewport);
	
	return hr;
}

HRESULT DeviceResources::releaseBackBuffer()
{
	HRESULT hr = S_OK;
	
	m_pRenderTarget.Reset();
	m_pBackBuffer.Reset();
	
	m_pDepthStencilView.Reset();
	m_pDepthStencil.Reset();
	
	m_pd3dDeviceContext->Flush();
	return hr;
}

HRESULT DeviceResources::goFullScreen()
{
	HRESULT hr = S_OK;
	hr = m_pDXGISwapChain->SetFullscreenState(TRUE, NULL);
	if (FAILED(hr)) {
		MessageBoxA(0, "SetFullScreenState failed", 0, 0);
	}
	releaseBackBuffer();
	hr = m_pDXGISwapChain->ResizeBuffers(0, 0, 0, DXGI_FORMAT_UNKNOWN, 0);
	hr = configureBackBuffer();
	
	return hr;
}

HRESULT DeviceResources::goWindowed()
{
	HRESULT hr = S_OK;
	
	hr = m_pDXGISwapChain->SetFullscreenState(FALSE, NULL);
	if (FAILED(hr)) MessageBoxA(0, "DeviceResources::goWindowed SetFullScreenState failed", 0, 0);
	releaseBackBuffer();
	hr = m_pDXGISwapChain->ResizeBuffers(0, 0, 0, DXGI_FORMAT_UNKNOWN, 0);
	if (FAILED(hr)) MessageBoxA(0, "DeviceResources::goWindowed ResizeBuffers failed", 0, 0); 
	hr = configureBackBuffer();
	if (FAILED(hr)) MessageBoxA(0, "DeviceResources::goWindowed ConfigureBackBuffer failed", 0, 0);

	return hr;
}

float DeviceResources::getAspectRatio()
{
	return static_cast<float>(m_bbDesc.Width) / static_cast<float>(m_bbDesc.Height);
}

void DeviceResources::present()
{
	m_pDXGISwapChain->Present(vsync, 0);
}

Microsoft::WRL::ComPtr<ID3D11DepthStencilView> DeviceResources::getCOMDepthStencil() { return m_pDepthStencilView; }

Microsoft::WRL::ComPtr<ID3D11RenderTargetView> DeviceResources::getCOMRenderTarget() { return m_pRenderTarget; }

DeviceResources* DeviceResources::getInstance() { return g_deviceResources; }

void DeviceResources::setVsync(bool vs) {
	vsync = vs;
}

ID3D11DepthStencilView* DeviceResources::getDepthStencil() { return m_pDepthStencilView.Get(); }

ID3D11RenderTargetView* DeviceResources::getRenderTarget() { return m_pRenderTarget.Get(); }

ID3D11Device*           DeviceResources::getDevice() { return m_pd3dDevice.Get(); }

ID3D11DeviceContext*    DeviceResources::getDeviceContext() { return m_pd3dDeviceContext.Get(); }
