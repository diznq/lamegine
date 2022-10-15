#include "Buffer.h"
#include "DeviceResources.h"
#include <D3D11.h>

Buffer::~Buffer() {
	if (buffer) {
		buffer->Release();
		buffer = 0;
	}
}

void *Buffer::beginCopy() {
	ID3D11DeviceContext* ctx = DeviceResources::getInstance()->getDeviceContext();
	static D3D11_MAPPED_SUBRESOURCE ms;
	if (FAILED(ctx->Map(buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &ms))) return 0;
	return ms.pData;
}

bool Buffer::copyData(void *data, size_t len, size_t off) {
	ID3D11DeviceContext* ctx = DeviceResources::getInstance()->getDeviceContext();
	static D3D11_MAPPED_SUBRESOURCE ms;
	if (FAILED(ctx->Map(buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &ms))) return false;
	memcpy((void*)(((unsigned char*)ms.pData) + off), data, len);
	ctx->Unmap(buffer, 0);
	return true;
}

bool Buffer::update(void *data) {
	ID3D11DeviceContext* ctx = DeviceResources::getInstance()->getDeviceContext();
	ctx->UpdateSubresource(buffer, 0, 0, data, 0, 0);
	return true;
}

void Buffer::endCopy() {
	ID3D11DeviceContext* ctx = DeviceResources::getInstance()->getDeviceContext();
	ctx->Unmap(buffer, 0);
}

size_t Buffer::getSize() {
	return size;
}

BufferTypes Buffer::getType() {
	return type;
}