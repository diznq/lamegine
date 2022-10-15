#include "Renderer.h"
#include "ShaderProgram.h"
#include "FrameBuffer.h"
#include "Texture.h"
#include "Buffer.h"

RendererD3D11* RendererD3D11::instance = 0;

RendererD3D11::RendererD3D11(IEngine *pEngine) {
	instance = this;
	this->pEngine = pEngine;
	deviceResources = gcnew<DeviceResources>();
	textureManager = gcnew<TextureManager>();
}

bool RendererD3D11::createWindowResources(IWindow *pWindow) {
	return SUCCEEDED(deviceResources->createWindowResources((HWND)pWindow->GetHandle()));
}

bool RendererD3D11::createDeviceResources() {
	if (!SUCCEEDED(deviceResources->createDeviceResources())) return false;

	D3D11_BLEND_DESC omDesc;
	ZeroMemory(&omDesc, sizeof(D3D11_BLEND_DESC));
	omDesc.RenderTarget[0].BlendEnable = true;
	omDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
	omDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
	omDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
	omDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
	omDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
	omDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
	omDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

	HRESULT hr = deviceResources->getDevice()->CreateBlendState(&omDesc, blendState.GetAddressOf());

	D3D11_SAMPLER_DESC primarySamplerDesc;
	ZeroMemory(&primarySamplerDesc, sizeof(primarySamplerDesc));
	primarySamplerDesc.Filter = D3D11_FILTER::D3D11_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR;
	primarySamplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
	primarySamplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
	primarySamplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
	primarySamplerDesc.MinLOD = 0;
	primarySamplerDesc.MaxLOD = D3D11_FLOAT32_MAX;
	primarySamplerDesc.ComparisonFunc = D3D11_COMPARISON_LESS;

	IWorld *pWorld = pEngine->getWorld();

	if (pWorld->settings.anisotropy > 0) {
		primarySamplerDesc.MaxAnisotropy = pWorld->settings.anisotropy;
		primarySamplerDesc.Filter = D3D11_FILTER_ANISOTROPIC;
	}

	D3D11_SAMPLER_DESC secondarySamplerDesc;

	ZeroMemory(&secondarySamplerDesc, sizeof(secondarySamplerDesc));
	secondarySamplerDesc.Filter = D3D11_FILTER::D3D11_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR;
	secondarySamplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
	secondarySamplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
	secondarySamplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
	secondarySamplerDesc.MinLOD = 0;
	secondarySamplerDesc.MaxLOD = D3D11_FLOAT32_MAX;
	secondarySamplerDesc.ComparisonFunc = D3D11_COMPARISON_LESS;

	if (FAILED(deviceResources->getDevice()->CreateSamplerState(&primarySamplerDesc, primarySampler.GetAddressOf()))) {
		return false;
	}
	if (FAILED(deviceResources->getDevice()->CreateSamplerState(&secondarySamplerDesc, secondarySampler.GetAddressOf()))) {
		return false;
	}

	D3D11_RASTERIZER_DESC solidCullFrontDesc, solidCullBackDesc, solidCullNoneDesc, wireframeDesc;
	ZeroMemory(&solidCullFrontDesc, sizeof(solidCullFrontDesc));

	solidCullFrontDesc.FillMode = D3D11_FILL_SOLID;
	solidCullFrontDesc.CullMode = D3D11_CULL_FRONT;
	solidCullFrontDesc.FrontCounterClockwise = false;

	solidCullFrontDesc.DepthBias = 0;
	solidCullFrontDesc.SlopeScaledDepthBias = 0.0f;
	solidCullFrontDesc.DepthBiasClamp = 0.0f;
	solidCullFrontDesc.DepthClipEnable = true;

	solidCullFrontDesc.ScissorEnable = false;
	solidCullFrontDesc.MultisampleEnable = false;
	solidCullFrontDesc.AntialiasedLineEnable = true;

	memcpy(&solidCullBackDesc, &solidCullFrontDesc, sizeof(solidCullFrontDesc));
	memcpy(&solidCullNoneDesc, &solidCullFrontDesc, sizeof(solidCullFrontDesc));
	memcpy(&wireframeDesc, &solidCullFrontDesc, sizeof(solidCullFrontDesc));

	wireframeDesc.FillMode = D3D11_FILL_WIREFRAME;
	solidCullBackDesc.CullMode = D3D11_CULL_BACK;
	solidCullNoneDesc.CullMode = D3D11_CULL_NONE;

	if (FAILED(deviceResources->getDevice()->CreateRasterizerState(&solidCullFrontDesc, solidCullFront.GetAddressOf()))) {
		return false;
	}
	if (FAILED(deviceResources->getDevice()->CreateRasterizerState(&solidCullBackDesc, solidCullBack.GetAddressOf()))) {
		return false;
	}
	if (FAILED(deviceResources->getDevice()->CreateRasterizerState(&solidCullNoneDesc, solidCullNone.GetAddressOf()))) {
		return false;
	}
	if (FAILED(deviceResources->getDevice()->CreateRasterizerState(&wireframeDesc, wireframe.GetAddressOf()))) {
		return false;
	}

	return true;
}

void RendererD3D11::goFullScreen() {
	deviceResources->goFullScreen();
}

void RendererD3D11::goWindowed() {
	deviceResources->goWindowed();
}

void RendererD3D11::setVsync(bool state) {
	deviceResources->setVsync(state);
}

void RendererD3D11::resetMaterials() {
	Material::resetActive();
}

Ptr<ITexture> RendererD3D11::loadTexture(const char *f, const char *nm, bool generateMips, int slot, bool srgb) {
	return textureManager->getTexture(nm, f, generateMips, slot, srgb);
}

Ptr<IShader> RendererD3D11::loadShader(const char *f, int type) {
	return gcnew<Shader>(pEngine, f, type);
}

Ptr<IFrameBuffer> RendererD3D11::createFrameBuffer(const char *name, int width, int height, Ptr<IShader> vertexShader, Ptr<IShader> pixelShader, int bpp, bool hasDepth, int mipLevel) {
	return gcnew<FrameBuffer>(name, width, height, vertexShader, pixelShader, bpp, hasDepth, 0, mipLevel);
}

Ptr<IFrameBuffer> RendererD3D11::createFrameBuffer(const char *name, Ptr<ITexture> texture, Ptr<IShader> vertexShader, Ptr<IShader> pixelShader, float left, float top) {
	return gcnew<FrameBuffer>(name, std::dynamic_pointer_cast<FrameTexture>(texture), vertexShader, pixelShader, left, top, 0);
}

Ptr<IFrameBuffer> RendererD3D11::createScreenOutputFrameBuffer(int width, int height, Ptr<IShader> vertexShader, Ptr<IShader> pixelShader) {
	
	return gcnew<FrameBuffer>(
		deviceResources->getCOMRenderTarget(),
		deviceResources->getCOMDepthStencil(),
		deviceResources->getViewport(),
		width, height, vertexShader, pixelShader, 0
	);
}

Ptr<IMaterial> RendererD3D11::createMaterial(const char *name) {
	return gcnew<Material>(name);
}

void RendererD3D11::setShader(Ptr<IShader> shader) {
	ID3D11DeviceContext* ctx = deviceResources->getDeviceContext();
	if (!shader || !ctx) return;
	Shader *pShader = (Shader*)shader.get();
	void *pGenShader = pShader->getShader();
	ID3D11InputLayout *pInputLayout = pShader->getInputLayout();
	int type = pShader->getType();
	
	if (!pGenShader) {
		return;
	}
	switch (type) {
	case FRAGMENT_SHADER:
		ctx->PSSetShader(pShader->getPixelShader(), 0, 0);
		break;
	case VERTEX_SHADER:
		ctx->IASetInputLayout(pInputLayout);
		ctx->VSSetShader(pShader->getVertexShader(), 0, 0);
		break;
	case HULL_SHADER:
		ctx->HSSetShader(pShader->getHullShader(), 0, 0);
		break;
	case DOMAIN_SHADER:
		ctx->DSSetShader(pShader->getDomainShader(), 0, 0);
		break;
	}
}

void RendererD3D11::setPixelShaderResources(int slot, const std::vector<Ptr<ITexture>>& res) {
	ID3D11DeviceContext* ctx = deviceResources->getDeviceContext();
	ID3D11ShaderResourceView *views[8];
	for (size_t i = 0, j = res.size(); i < j; i++) {
		views[i] = (ID3D11ShaderResourceView*)res[i]->getShaderView();
	}
	ctx->PSSetShaderResources(slot, UINT(res.size()), views);
}

void RendererD3D11::setVertexShaderResources(int slot, const std::vector<Ptr<ITexture>>& res) {
	ID3D11DeviceContext* ctx = deviceResources->getDeviceContext(); 
	ID3D11ShaderResourceView *views[8];
	for (size_t i = 0, j = res.size(); i < j; i++) {
		views[i] = (ID3D11ShaderResourceView*)res[i]->getShaderView();
	}
	ctx->VSSetShaderResources(slot, UINT(res.size()), views);
}

void RendererD3D11::setPixelShaderResources(int slot, Ptr<ITexture> res) {
	
	ID3D11DeviceContext* ctx = deviceResources->getDeviceContext();
	ctx->PSSetShaderResources(slot, 1, (ID3D11ShaderResourceView**)res->getShaderViewRef());
}

void RendererD3D11::setVertexShaderResources(int slot, Ptr<ITexture> res) {
	
	ID3D11DeviceContext* ctx = deviceResources->getDeviceContext();
	ctx->VSSetShaderResources(slot, 1, (ID3D11ShaderResourceView**)res->getShaderViewRef());
}

void RendererD3D11::setPixelConstantBuffers(int slot, const std::vector<Ptr<IBuffer>>& res) {
	
	ID3D11DeviceContext* ctx = deviceResources->getDeviceContext();
	ID3D11Buffer *views[8];
	for (size_t i = 0, j = res.size(); i < j; i++) {
		views[i] = (ID3D11Buffer*)res[i]->getHandle();
	}
	ctx->PSSetConstantBuffers(slot, UINT(res.size()), views);
}

void RendererD3D11::setVertexConstantBuffers(int slot, const std::vector<Ptr<IBuffer>>& res) {
	
	ID3D11DeviceContext* ctx = deviceResources->getDeviceContext();
	ID3D11Buffer *views[8];
	for (size_t i = 0, j = res.size(); i < j; i++) {
		views[i] = (ID3D11Buffer*)res[i]->getHandle();
	}
	ctx->VSSetConstantBuffers(slot, UINT(res.size()), views);
}

void RendererD3D11::setPixelConstantBuffers(int slot, Ptr<IBuffer> res) {
	
	ID3D11DeviceContext* ctx = deviceResources->getDeviceContext();
	ctx->PSSetConstantBuffers(slot, 1, (ID3D11Buffer**)res->getHandleRef());
}

void RendererD3D11::setVertexConstantBuffers(int slot, Ptr<IBuffer> res) {
	
	ID3D11DeviceContext* ctx = deviceResources->getDeviceContext();
	ctx->VSSetConstantBuffers(slot, 1, (ID3D11Buffer**)res->getHandleRef());
}

void RendererD3D11::setRenderTargets(const std::vector<Ptr<IRenderTarget>>& renderTargets, Ptr<IDepthStencil> depthStencil) {
	
	ID3D11DeviceContext* ctx = deviceResources->getDeviceContext();
	ID3D11RenderTargetView *views[8];
	for (size_t i = 0, j = renderTargets.size(); i < j; i++) {
		views[i] = (ID3D11RenderTargetView*)renderTargets[i]->getHandle();
	}
	ctx->OMSetRenderTargets(UINT(renderTargets.size()), views, depthStencil?(ID3D11DepthStencilView*)depthStencil->getHandle():0);
}

void RendererD3D11::setViewports(int no, const Viewport *vps) {
	
	ID3D11DeviceContext* ctx = deviceResources->getDeviceContext();
	ctx->RSSetViewports(no, (D3D11_VIEWPORT*)vps);
}

void RendererD3D11::clearRenderTarget(Ptr<IRenderTarget> renderTarget, const float *color) {
	
	ID3D11DeviceContext* ctx = deviceResources->getDeviceContext();
	ctx->ClearRenderTargetView((ID3D11RenderTargetView*)renderTarget->getHandle(), color);
}
void RendererD3D11::clearDepthStencil(Ptr<IDepthStencil> depthStencil) {
	ID3D11DeviceContext* ctx = deviceResources->getDeviceContext();
	ctx->ClearDepthStencilView((ID3D11DepthStencilView*)depthStencil->getHandle(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
}

void RendererD3D11::present() {
	deviceResources->present();
}

void RendererD3D11::copyResource(Ptr<ITexture> dest, Ptr<ITexture> src) {
	ID3D11DeviceContext* ctx = deviceResources->getDeviceContext();
	ctx->CopyResource((ID3D11Texture2D*)dest->getHandle(), (ID3D11Texture2D*)src->getHandle());
}

Ptr<IBuffer> RendererD3D11::createBuffer(BufferTypes flags, size_t size, void *data) {
	D3D11_USAGE usage = D3D11_USAGE_DEFAULT;
	if (flags & eBT_Immutable) usage = D3D11_USAGE_IMMUTABLE;
	else if (flags & eBT_Dynamic) usage = D3D11_USAGE_DYNAMIC;
	D3D11_BIND_FLAG type = D3D11_BIND_CONSTANT_BUFFER;
	if (flags & eBT_Vertex) type = D3D11_BIND_VERTEX_BUFFER;
	else if (flags & eBT_Index) type = D3D11_BIND_INDEX_BUFFER;

	CD3D11_BUFFER_DESC bufferDesc(UINT(size), type, usage);

	if (usage == D3D11_USAGE_DYNAMIC) {
		bufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	}

	D3D11_SUBRESOURCE_DATA bufferData;
	ZeroMemory(&bufferData, sizeof(D3D11_SUBRESOURCE_DATA));
	bufferData.pSysMem = data;
	bufferData.SysMemPitch = 0;
	bufferData.SysMemSlicePitch = 0;

	ID3D11Buffer *buffer = 0;
	HRESULT res = deviceResources->getDevice()->CreateBuffer(&bufferDesc, data?&bufferData:nullptr, &buffer);
	if (FAILED(res)) {
		
		pEngine->terminate("ERROR");
		return nullptr;
	}
	return gcnew<Buffer>(buffer, size, flags);
}

void RendererD3D11::setDrawBuffers(Ptr<IBuffer> vertexBuffer, Ptr<IBuffer> indexBuffer) {
	ID3D11DeviceContext* ctx = deviceResources->getDeviceContext();
	UINT stride = sizeof(Vertex);
	UINT offset = 0;
	ctx->IASetVertexBuffers(0, 1, (ID3D11Buffer**)vertexBuffer->getHandleRef(), &stride, &offset);
	ctx->IASetIndexBuffer((ID3D11Buffer*)indexBuffer->getHandle(), DXGI_FORMAT_R32_UINT, 0);
}

void RendererD3D11::setDrawData(Ptr<IDrawData> pDD) {
	setDrawBuffers(pDD->getVertexBuffer(), pDD->getIndexBuffer());
}

void RendererD3D11::setRawDrawData(IDrawData *pDD) {
	setDrawBuffers(pDD->getVertexBuffer(), pDD->getIndexBuffer());
}

void RendererD3D11::setTopology(ETopology tl) {
	ID3D11DeviceContext* ctx = deviceResources->getDeviceContext();
	switch (tl) {
	case eTL_TriangleList:
		ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		break;
	}
}

void RendererD3D11::drawIndexedInstanced(int indexCount, int instance, int offset) {
	ID3D11DeviceContext* ctx = deviceResources->getDeviceContext();
	ctx->DrawIndexedInstanced(indexCount, instance, offset, 0, 0);
}

void RendererD3D11::enableBlending() {
	ID3D11DeviceContext* ctx = deviceResources->getDeviceContext();
	float arr[] = { 0.5f, 0.5f, 0.5f, 0.5f };
	ctx->OMSetBlendState(blendState.Get(), arr, 0xFFFFFFFF);
}

void RendererD3D11::disableBlending() {
	ID3D11DeviceContext* ctx = deviceResources->getDeviceContext();
	ctx->OMSetBlendState(0, 0, 0xFFFFFFFF);
}

void RendererD3D11::setRasterizer(ERasterizerState rs) {
	ID3D11DeviceContext* ctx = deviceResources->getDeviceContext();
	switch (rs) {
	case eRS_SolidBack:
		ctx->RSSetState(solidCullBack.Get());
		break;
	case eRS_SolidFront:
		ctx->RSSetState(solidCullFront.Get());
		break;
	case eRS_SolidNone:
		ctx->RSSetState(solidCullNone.Get());
		break;
	case eRS_Wireframe:
		ctx->RSSetState(wireframe.Get());
		break;
	}
}

void RendererD3D11::assignSamplers() {
	ID3D11DeviceContext* ctx = deviceResources->getDeviceContext();
	ctx->PSSetSamplers(0, 1, primarySampler.GetAddressOf());
	ctx->PSSetSamplers(1, 1, secondarySampler.GetAddressOf());
}


extern "C" {
	__declspec(dllexport) IRenderer* CreateRenderer(IEngine *pEngine) {
		return new RendererD3D11(pEngine);
	}
}