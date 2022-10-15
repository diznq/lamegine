#include "DeviceResources.h"
#include "FrameBuffer.h"
#include <iostream>
using namespace std;
int FrameTexture::totalSize = 0;

FrameTexture::FrameTexture(int w, int h, int bpp, bool hasDepth, int offx, int offy, const char *szName, int msaa, int mipLevels) {
	realBpp = bpp;
	density = bpp & 0x1FFF;
	name = szName;
	if (w < 1) w = 1;
	if (h < 1) h = 1;
	width = w;
	height = h;
	renderTargets = 1;
	D3D11_TEXTURE2D_DESC textureDesc;
	D3D11_RENDER_TARGET_VIEW_DESC renderTargetViewDesc;
	D3D11_SHADER_RESOURCE_VIEW_DESC shaderResourceViewDesc;
	hasDepthStencil = hasDepth;
	depthWrapper = gcnew<DepthTexture>(this);
	ZeroMemory(&textureDesc, sizeof(textureDesc));

	ZeroMemory(&viewport, sizeof(D3D11_VIEWPORT));
	viewport.Height = (float)h;
	viewport.Width = (float)w;
	viewport.MinDepth = 0;
	viewport.MaxDepth = 1;
	viewport.TopLeftX = (float)offx;
	viewport.TopLeftY = (float)offy;
	
	textureDesc.Width = width;
	textureDesc.Height = height;
	textureDesc.MipLevels = mipLevels;
	textureDesc.ArraySize = 1;
	if (bpp == -1) {
		textureDesc.SampleDesc.Count = 1;
	} else {
		if (bpp & DOUBLE_DEPTH_BUFFER) {
			switch (density) {
			case 32:
				textureDesc.Format = DXGI_FORMAT_R32G32_FLOAT;
				calcBpp = 8;
				break;
			case 16:
				textureDesc.Format = DXGI_FORMAT_R16G16_FLOAT;
				calcBpp = 4;
				break;
			case 8:
				textureDesc.Format = DXGI_FORMAT_R8G8_UINT;
				calcBpp = 2;
				break;
			}
		} else if (bpp & DEPTH_BUFFER) {
			switch (density) {
			case 32:
				textureDesc.Format = DXGI_FORMAT_R32_FLOAT;
				calcBpp = 4;
				break;
			case 16:
				textureDesc.Format = DXGI_FORMAT_R16_FLOAT;
				calcBpp = 2;
				break;
			case 8:
				textureDesc.Format = DXGI_FORMAT_R8_UINT;
				calcBpp = 1;
				break;
			}
		} else {
			switch (density) {
			case 32:
				textureDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
				calcBpp = 16;
				break;
			case 16:
				textureDesc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
				calcBpp = 8;
				break;
			case 11:
			case 10:
				textureDesc.Format = DXGI_FORMAT_R11G11B10_FLOAT;
				calcBpp = 4;
				break;
			case 8:
				textureDesc.Format = (bpp & SIGNED) ? DXGI_FORMAT_R8G8B8A8_SNORM : DXGI_FORMAT_R8G8B8A8_UNORM;
				calcBpp = 4;
				break;
			}
		}


		format = textureDesc.Format;

		textureDesc.SampleDesc.Count = 1;
		if (msaa) {
			textureDesc.SampleDesc.Count = msaa;
			HRESULT hr;
			UINT maxQuality;
			hr = DeviceResources::getInstance()->getDevice()->CheckMultisampleQualityLevels(textureDesc.Format, textureDesc.SampleDesc.Count, &maxQuality);
			if (FAILED(hr)) {
				cout << "Failed to get max quality: " << hr << endl;
			}
			textureDesc.SampleDesc.Quality = 0; // maxQuality - 1;
			cout << "MSAA Quality: " << textureDesc.SampleDesc.Quality << endl;
		}
		if (textureDesc.SampleDesc.Count < 2) msaa = 0;
		textureDesc.Usage = D3D11_USAGE_DEFAULT;
		textureDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
		textureDesc.MiscFlags = 0;
		if (mipLevels > 1) {
			textureDesc.MiscFlags = D3D11_RESOURCE_MISC_GENERATE_MIPS;
		}
		textureDesc.CPUAccessFlags = 0;
		
		if (FAILED(DeviceResources::getInstance()->getDevice()->CreateTexture2D(&textureDesc, nullptr, pTexture.GetAddressOf()))) {
			return;
		}

		renderTargetViewDesc.Format = textureDesc.Format;
		renderTargetViewDesc.ViewDimension = msaa ? D3D11_RTV_DIMENSION_TEXTURE2DMS : D3D11_RTV_DIMENSION_TEXTURE2D;
		renderTargetViewDesc.Texture2D.MipSlice = 0;

		if (FAILED(DeviceResources::getInstance()->getDevice()->CreateRenderTargetView(pTexture.Get(), &renderTargetViewDesc, pRenderTargetView.GetAddressOf()))) {
			return;
		}

		shaderResourceViewDesc.Format = textureDesc.Format;
		shaderResourceViewDesc.ViewDimension = msaa ? D3D11_SRV_DIMENSION_TEXTURE2DMS : D3D11_SRV_DIMENSION_TEXTURE2D;
		shaderResourceViewDesc.Texture2D.MostDetailedMip = 0;
		shaderResourceViewDesc.Texture2D.MipLevels = mipLevels;

		if (FAILED(DeviceResources::getInstance()->getDevice()->CreateShaderResourceView(pTexture.Get(), &shaderResourceViewDesc, pShaderView.GetAddressOf()))) {
			return;
		}

	}
	
	if (hasDepthStencil) {
		CD3D11_TEXTURE2D_DESC depthStencilDesc(
			DXGI_FORMAT_R32_TYPELESS,
			static_cast<UINT> (width),
			static_cast<UINT> (height),
			1,
			1,
			D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE
		);

		depthStencilDesc.SampleDesc.Count = textureDesc.SampleDesc.Count;
		depthStencilDesc.SampleDesc.Quality = textureDesc.SampleDesc.Quality;

		D3D11_SHADER_RESOURCE_VIEW_DESC sr_desc;
		memset(&sr_desc, 0, sizeof(sr_desc));
		sr_desc.Format = DXGI_FORMAT_R32_FLOAT;
		sr_desc.ViewDimension = msaa?D3D11_SRV_DIMENSION_TEXTURE2DMS:D3D11_SRV_DIMENSION_TEXTURE2D;
		sr_desc.Texture2D.MostDetailedMip = 0;
		sr_desc.Texture2D.MipLevels = mipLevels;

		D3D11_DEPTH_STENCIL_VIEW_DESC dsv_desc;
		memset(&dsv_desc, 0, sizeof(dsv_desc));
		dsv_desc.Flags = 0;
		dsv_desc.Format = DXGI_FORMAT_D32_FLOAT;
		dsv_desc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;

		HRESULT hr = DeviceResources::getInstance()->getDevice()->CreateTexture2D(&depthStencilDesc, nullptr, pDepthStencil.GetAddressOf());
		if (FAILED(hr)) {
			return;
		}
		hr = DeviceResources::getInstance()->getDevice()->CreateDepthStencilView(pDepthStencil.Get(), &dsv_desc, pDepthStencilView.GetAddressOf());
		if (FAILED(hr)) {
			return;
		}
		hr = DeviceResources::getInstance()->getDevice()->CreateShaderResourceView(pDepthStencil.Get(), &sr_desc, pDepthShaderView.GetAddressOf());
		if (FAILED(hr)) {
			return;
		}
	}
	
	onInit();
}

void FrameTexture::onInit() {
	size = width * height * calcBpp + (hasDepthStencil ? (width * height * 4) : 0);
	totalSize += size;
}

int FrameTexture::getSize() {
	return size;
}

FrameTexture::FrameTexture(
	Microsoft::WRL::ComPtr<ID3D11RenderTargetView> pRTV,
	Microsoft::WRL::ComPtr<ID3D11DepthStencilView> pDSV,
	D3D11_VIEWPORT vp,
	int w, int h, const char *szName)
	: pRenderTargetView(pRTV), density(0), pDepthStencilView(pDSV),  viewport(vp),  width(w),  height(h),  renderTargets(1),  name(szName){
	if (width < 1) width = 1;
	if (height < 1)height = 1;
	calcBpp = 4;
	hasDepthStencil = true;
	depthWrapper = gcnew<DepthTexture>(this);
	onInit();
}

Microsoft::WRL::ComPtr<ID3D11Texture2D>& FrameTexture::getResource() {
	return pTexture;
}

int FrameTexture::getWidth() const {
	return width;
}

Microsoft::WRL::ComPtr<ID3D11DepthStencilView>& FrameTexture::getDepthStencilView() {
	return pDepthStencilView;
}

Microsoft::WRL::ComPtr<ID3D11RenderTargetView>& FrameTexture::getRenderTargetView() {
	return pRenderTargetView;
}

int FrameTexture::getTotalSize() {
	return totalSize;
}

bool FrameTexture::hasDepth() {
	return hasDepthStencil;
}

int FrameTexture::getHeight() const {
	return height;
}

int FrameTexture::getBitsPerPixel() const {
	return calcBpp << 3;
}

Microsoft::WRL::ComPtr<ID3D11Texture2D>& FrameTexture::getDepthStencil() {
	return pDepthStencil;
}

Ptr<IShader> FrameBuffer::getPixelShader() { return pPixelShader; }

Microsoft::WRL::ComPtr<ID3D11DepthStencilView>& FrameBuffer::getDepthStencilView() {
	return texture->getDepthStencilView();
}

Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>& FrameBuffer::getShaderView() {
	return texture->getShaderViewSafe();
}

Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>& FrameBuffer::getDepthShaderView() {
	return texture->getDepthShaderView();
}

DXGI_FORMAT FrameTexture::getFormat() const {
	return format;
}

bool FrameTexture::read(std::vector<unsigned char>& data) const {
	//Engine* pEngine = Engine::getInstance();
	//return pEngine->readTexture<unsigned char>(shared_from_this(), pTexture, format, data);
	return false;
}

bool FrameTexture::read(std::vector<float>& data) const {
	//Engine* pEngine = Engine::getInstance();
	//return pEngine->readTexture<float>(shared_from_this(), pTexture, format, data);
	return false;
}

bool FrameTexture::read(std::vector<float2>& data) const {
	//Engine* pEngine = Engine::getInstance();
	//return pEngine->readTexture<float2>(shared_from_this(), pTexture, format, data);
	return false;
}

bool FrameTexture::read(std::vector<float3>& data) const {
	//Engine* pEngine = Engine::getInstance();
	//return pEngine->readTexture<float3>(shared_from_this(), pTexture, format, data);
	return false;
}

bool FrameTexture::read(std::vector<float4>& data) const {
	//Engine* pEngine = Engine::getInstance();
	//return pEngine->readTexture<float4>(shared_from_this(), pTexture, format, data);
	return false;
}

void FrameTexture::generateMips() {
	DeviceResources::getInstance()->getDeviceContext()->GenerateMips(pShaderView.Get());
}

void FrameBuffer::setPixelShader(Ptr<IShader> pShader) {
	pPixelShader = pShader;
}


SerializeData FrameTexture::serialize() {
	SerializeData data;
	return data;
}

void FrameTexture::deserialize(const SerializeData& params) {

}
FrameBuffer::FrameBuffer(const char *szName,Ptr<FrameTexture> ftex,Ptr<IShader> vertexShader,Ptr<IShader> pixelShader,float offx, float offy, int msaa)
	: name(szName), texture(ftex), pVertexShader(vertexShader), pPixelShader(pixelShader),  view(0), proj(0), world(0){
	ZeroMemory(&viewport, sizeof(D3D11_VIEWPORT));
	viewport.Height = (float)ftex->getHeight();
	viewport.Width = (float)ftex->getWidth();
	viewport.MinDepth = 0;
	viewport.MaxDepth = 1;
	viewport.TopLeftX = offx;
	viewport.TopLeftY = offy;

	if (texture->getRenderTargetView())
		renderTarget = gcnew<RenderTarget>(texture->getRenderTargetView().Get());
	if (texture->getDepthStencilView())
		depthStencil = gcnew<DepthStencil>(texture->getDepthStencilView().Get());
}

void FrameBuffer::setVPW(matrix* v, matrix* p, matrix* w) {
	view = v;
	proj = p;
	world = w;
}

FrameBuffer::FrameBuffer(Microsoft::WRL::ComPtr<ID3D11RenderTargetView> pRTV,Microsoft::WRL::ComPtr<ID3D11DepthStencilView> pDSV,D3D11_VIEWPORT vp,int w,int h,Ptr<IShader> vertexShader, Ptr<IShader> pixelShader, int msaa)
	: name("Screen"),  texture(gcnew<FrameTexture>(pRTV, pDSV, vp, w, h, "Screen")),  pVertexShader(vertexShader),  pPixelShader(pixelShader),  view(0), proj(0), world(0){
	memcpy(&viewport, &vp, sizeof(vp));
	if (texture->getRenderTargetView())
		renderTarget = gcnew<RenderTarget>(texture->getRenderTargetView().Get());
	if (texture->getDepthStencilView())
		depthStencil = gcnew<DepthStencil>(texture->getDepthStencilView().Get());
}

matrix* FrameBuffer::getProj() { return proj; }

FrameBuffer::FrameBuffer(const char *szName,int w,int h,Ptr<IShader> vertexShader,Ptr<IShader> pixelShader,int bpp,int rtc, int msaa, int mipLevels)
	: name(szName),  texture(gcnew<FrameTexture>(w, h, bpp, rtc, 0, 0, szName, msaa, mipLevels)),  pVertexShader(vertexShader),  pPixelShader(pixelShader),  view(0), proj(0), world(0){
	ZeroMemory(&viewport, sizeof(D3D11_VIEWPORT));
	viewport.Height = (float)h;
	viewport.Width = (float)w;
	viewport.MinDepth = 0;
	viewport.MaxDepth = 1;
	viewport.TopLeftX = 0.0f;
	viewport.TopLeftY = 0.0f;
	if(texture->getRenderTargetView())
		renderTarget = gcnew<RenderTarget>(texture->getRenderTargetView().Get());
	if (texture->getDepthStencilView())
		depthStencil = gcnew<DepthStencil>(texture->getDepthStencilView().Get());
}

matrix* FrameBuffer::getView() { return view; }

matrix* FrameBuffer::getWorld() { return world; }

void FrameBuffer::setVertexShader(Ptr<IShader> pShader) {
	pPixelShader = pShader;
}

bool FrameBuffer::getVPW(matrix* v, matrix* p, matrix* w) {
	if (view && proj && world) {
		if (v && p && w) {
			*v = *view;
			*p = *proj;
			*w = *world;
		}
		return true;
	}
	return false;
}

bool FrameBuffer::hasDepthStencil() {
	return texture->hasDepth();
}

Viewport& FrameBuffer::getViewport() {
	return viewport;
}

Ptr<IShader> FrameBuffer::getVertexShader() { return pVertexShader; }

Ptr<ITexture> FrameBuffer::getTexture() { return texture; }

void FrameBuffer::setTexture(Ptr<FrameTexture> tx) {
	texture = tx;
}

void FrameBuffer::setViewport(Viewport vp) {
	viewport = vp;
}

Microsoft::WRL::ComPtr<ID3D11RenderTargetView>& FrameBuffer::getRenderTargetView() {
	return texture->getRenderTargetView();
}

