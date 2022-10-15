#include "RenderQuad.h"

RenderQuad::RenderQuad() {

}
const char* RenderQuad::getName() {
	return "RenderQuad";
}
IObject3D& RenderQuad::init(IEngine *pEngine) {
	Object3D::init(pEngine);
	if (queryDrawData(pEngine->getDrawDataManager(), "RenderQuad")) {
		auto pDD = getDrawData();
		auto& vertices = pDD->getVertices();
		auto& indices = pDD->getIndices();
		auto& objects = pDD->getObjects();

		vertices.push_back(Vertex(float3(-1.0f, -1.0f, 0.0f),	float2(0.0f, 1.0f), float3(0.0f, 0.0f, 0.0f)));
		vertices.push_back(Vertex(float3(1.0f, -1.0f, 0.0f),	float2(1.0f, 1.0f), float3(0.0f, 0.0f, 0.0f)));
		vertices.push_back(Vertex(float3(-1.0f, 1.0f, 0.0f),	float2(0.0f, 0.0f), float3(0.0f, 0.0f, 0.0f)));
		vertices.push_back(Vertex(float3(1.0f, 1.0f, 0.0f),		float2(1.0f, 0.0f), float3(0.0f, 0.0f, 0.0f)));

		indices.push_back(0);
		indices.push_back(1);
		indices.push_back(2);

		indices.push_back(2);
		indices.push_back(1);
		indices.push_back(3);

		auto mtl = pEngine->getRenderer()->createMaterial("RenderQuadMaterial");
		objects.push_back(gcnew<IndexBuffer>((unsigned int)indices.size(), mtl));
		pDD->finish();
	}
	return *this;
}
bool RenderQuad::isVisible() const {
	return true;
}