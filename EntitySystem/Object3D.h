#pragma once
#include "../Lamegine/Interfaces.h"

using namespace Math;

class Object3D : public IObject3D {
protected:
	Ptr<IDrawData> drawData;
	Ptr<IMaterial> txGroup;
	std::vector<Ptr<ISound>> sounds;
	std::vector<Ptr<IDynamicLight>> lights;
	Ptr<IPhysics> physics;
	Ptr<IJoint> pAttachmentJoint;
	Ptr<IObject3D> pAttachmentObject;
	std::string name;
	Vec3 pos;
	Vec3 scale;
	Vec4 rotation, internalRotation{ 0.0f, 0.0f, 0.0f, 1.0f };
	Vec4 pigment;
	Mat4 model;
	Mat4 transform;
	bool staticFlag;
	bool transformCached;
	unsigned int type;
	unsigned int flags;
	unsigned int entityId;
	unsigned int tess;
	unsigned int instances;
	unsigned int maxInstances;
	bool vbupdate;
	bool visible;
	int refs;
	bool destroyed;
	float lifeTime;
	bool usePigment;
	IEngine *engine;
	std::map<unsigned int, void*> parents;
public:
	Object3D();
	virtual ~Object3D();
	virtual size_t getSize() const { return sizeof(Object3D); }
	virtual IObject3D& create(void *params = 0);
	virtual IObject3D& init(IEngine *pEngine);
	virtual void postInit(){}
	virtual void destroy();

	virtual IObject3D& add(bool autoInit = true, bool cleanUp = false);

	virtual Ptr<IPhysics> getPhysics();
	virtual IObject3D& setPhysics(Ptr<IPhysics> physics);

	virtual IObject3D& enablePhysics();
	virtual IObject3D& disablePhysics();


	virtual bool attachTo(Ptr<IObject3D> parent, const std::string& jointName);
	virtual IObject3D& detach();

	virtual IObject3D& buildPhysics(float mass = 0.0f, ShapeInfo shapeInfo = ShapeInfo(), PhysicsCallback callback = nullptr);

	virtual void addRef();
	virtual void release();
	virtual IObject3D& setLifeTime(float lifeTime);
	virtual float getLifeTime() const;

	virtual Vertex* getVertices(size_t& sz);
	virtual Index_t* getIndices(size_t& sz);
	virtual std::vector<Ptr<IndexBuffer>>& getAllIndices();
	virtual Ptr<IDrawData> getDrawData();
	virtual bool queryDrawData(Ptr<IDrawDataManager> pMgr, const char *name = 0, bool cache = true);

	virtual void update(const IWorld *pWorld);
	virtual bool isVisible() const;
	virtual IObject3D& setVisible(bool visible);

	virtual float getLOD() const;
	virtual Ptr<IObject3D>* getObjects(int* sz);
	virtual std::string getClassName() const;
	virtual std::string getName() const;
	virtual IObject3D& setName(std::string name);

	virtual Math::Mat4& getModel();
	virtual IObject3D& setModel(const Math::Mat4& mdl);

	virtual Math::Vec4& getPigment();
	virtual IObject3D& setPigment(const Math::Vec4& pigment);
	virtual IObject3D& setUsePigment(bool state = false);
	virtual bool usesPigment();

	virtual unsigned int getType() const;
	virtual IObject3D& setType(const unsigned int type);

	virtual unsigned int getEntityId() const;
	virtual IObject3D& setEntityId(const unsigned int id);

	virtual Math::Vec3& getScale();
	virtual IObject3D& setScale(const Math::Vec3& scale);

	virtual unsigned int getFlags() const;
	virtual IObject3D& setFlags(const int flags);

	virtual Math::Vec3& getPos();
	virtual IObject3D& setPos(const Math::Vec3& pos);

	virtual Math::Vec4& getRotation();
	virtual IObject3D& setRotation(const Math::Vec4& rot);

	virtual Math::Vec4& getInternalRotation();
	virtual IObject3D& setInternalRotation(const Math::Vec4& rot);

	virtual Math::Mat4 getTransform(const Ptr<const IndexBuffer> ib);

	virtual unsigned int getTessellation() const;
	virtual IObject3D& setTessellation(const unsigned int tess);

	virtual Ptr<IMaterial>& getTextureGroup();
	virtual IObject3D& setTextureGroup(Ptr<IMaterial> group);

	virtual IObject3D& setEngineBufferId(const unsigned int id);
	virtual unsigned int getEngineBufferId() const;

	virtual unsigned int getInstancesCount() const;
	virtual IObject3D& setInstancesCount(const unsigned int c);

	virtual unsigned int getMaxInstancesCount() const;
	virtual IObject3D& setMaxInstancesCount(const unsigned int c);

	virtual bool requiresVBUpdate() const;
	virtual void flagVBUpdate(const bool state);

	virtual bool inViewport(const IWorld *pWorld, float radius, float mnd = 0.0f);
	virtual bool intersects(const Math::Vec3& origin, const Math::Vec3& dir, Math::Vec3& intersection);

	virtual int getCulling() const;

	virtual IObject3D& attachSound(Ptr<ISound> sound, Math::Vec3 relPos = Math::Vec3(0.0f, 0.0f, 0.0f));
	virtual std::vector<Ptr<ISound>> getAttachedSounds() const;

	virtual IObject3D& attachLight(Ptr<IDynamicLight> light, Math::Vec3 relPos = Math::Vec3(0.0f, 0.0f, 0.0f));
	virtual std::vector<Ptr<IDynamicLight>> getAttachedLights() const;

	virtual bool isStatic() const;
	virtual IObject3D& setStatic(bool flag);

	virtual bool isAnimated() const;

	virtual SerializeData serialize();
	virtual void deserialize(const SerializeData& params);

	virtual bool exportObject(const std::string& path, const std::string& pathPrefix) { return false; }
	virtual bool importObject(const std::string& path, const std::string& pathPrefix) { return false; }
	virtual IObject3D& setParent(void *parent, unsigned int idx = 0);
	virtual void* getParent(unsigned int idx = 0) const;
};
