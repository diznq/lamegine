#include "Model3D.h"
#include "Vehicle.h"
#include "RenderQuad.h"

extern "C" {
	__declspec(dllexport) void CreateEntitySystem(IEngine* pEngine) {

		pEngine->registerClass("Model3D", [](void* params) -> Ptr<IObject3D> {
			auto ptr = gcnew<Model3D>();
			ptr->create(params);
			return ptr;
		});

		pEngine->registerClass("RenderQuad", [pEngine](void* params) -> Ptr<IObject3D> {
			auto ptr = gcnew<RenderQuad>();
			ptr->create(params);
			return ptr;
		});

		pEngine->registerClass("Vehicle", [](void *params) -> Ptr<IObject3D> {
			ModelSpawnParams *path = (ModelSpawnParams*)params;
			auto ptr = gcnew<Vehicle>(path->path);
			ptr->create(params);
			return ptr;
		});
	}
}