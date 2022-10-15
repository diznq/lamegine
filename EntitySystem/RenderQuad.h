#pragma once
#include "Object3D.h"

struct RenderQuad : public Object3D {
	RenderQuad();
	virtual const char* getName();
	virtual IObject3D& init(IEngine *pEngine);
	bool isVisible() const;
};