#pragma once
#include "Object3D.h"
#include "Animation.h"

class Model3D : public Object3D, public Animated {
	std::string path;
	bool auto_rotate;
	bool animated = false;

	std::map<std::string, Ptr<IJoint>> boneHierarchy;
	std::map<int, std::string> revMapping;
public:
	Model3D();
	Model3D(std::string szPath);
	virtual ~Model3D();
	virtual IObject3D& create(void *params = 0);
	virtual IObject3D& init(IEngine *pEngine);
	virtual void postInit();
	//virtual void update(const World *pWorld);
	void readMaterials(IEngine *pEngine, std::string pathPrefix, const char *mtlPath, std::map<std::string, std::shared_ptr<IMaterial> >& mtls);

	void exportModel(std::string path, std::string pathPrefix);
	void importModel(std::string path, std::string pathPrefix);

	void importFbxNode(void *node);
	void importAssimpNode(std::string pathPrefix, Ptr<IJoint> root, const void *s, void *n);
	void importOther(std::string path, std::string pathPrefix);
	void importObjModel(std::string path, std::string pathPrefix);


	virtual bool exportObject(const std::string& path, const std::string& pathPrefix) { exportModel(path, pathPrefix); return true; }
	virtual bool importObject(const std::string& path, const std::string& pathPrefix) { importModel(path, pathPrefix); return true; }

	virtual size_t getSize() const { return sizeof(Model3D); }
	virtual void update(const IWorld *pWorld);

	virtual bool isAnimated() const;
};