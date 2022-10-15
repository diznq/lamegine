#pragma once
#include "../Lamegine/Interfaces.h"
struct ID3D11Buffer;

class Buffer : public IBuffer {
private:
	ID3D11Buffer *buffer = 0;
	BufferTypes type;
	size_t size;
public:
	Buffer(ID3D11Buffer *buff, size_t sz, BufferTypes tpe) : buffer(buff), size(sz), type(tpe) {}
	virtual ~Buffer();
	virtual void* getHandle() { return buffer; }
	virtual void *beginCopy();
	virtual bool copyData(void *data, size_t len, size_t off);
	virtual bool update(void *data);
	virtual void endCopy();
	virtual size_t getSize();
	virtual BufferTypes getType();
	virtual void** getHandleRef() { return (void**)&buffer; }
};