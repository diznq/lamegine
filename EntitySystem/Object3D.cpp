#include "shared.h"
#include "Object3D.h"
#include "../Lamegine/VectorMathExt.h"
#include <btBulletDynamicsCommon.h>

Object3D::Object3D() :
	destroyed(false),
	refs(0),
	pos(0.0f),
	scale(1.0f),
	model(1.0f),
	pigment(1.0f),
	rotation(0.0f, 0.0f, 0.0f, 1.0f),
	type(DEFAULT_OBJECT),
	flags(ENT_ALL),
	entityId(0),
	tess(3),
	instances(1),
	maxInstances(1),
	txGroup(0),
	vbupdate(false),
	visible(true),
	usePigment(false),
	lifeTime(-1.0f),
	transformCached(false),
	staticFlag(false),
	name("")
{
	char buff[128];
	sprintf(buff, "Obj#%p", this);
	name = buff;
}

bool Object3D::isAnimated() const {
	return false;
}

IObject3D& Object3D::add(bool autoInit, bool cleanUp) {
	engine->addObject(shared_from_this(), autoInit, cleanUp);
	return *this;
}
IObject3D& Object3D::setLifeTime(float lifeTime) {
	this->lifeTime = (float)(engine->getGameTime() + lifeTime);
	return *this;
}
IObject3D& Object3D::setPos(const Vec3& newPos) {
	pos = newPos;
	if (auto phys = getPhysics()) {
		phys->setPos(newPos);
	}
	transformCached = false;
	return *this;
}
Math::Vec4& Object3D::getInternalRotation() {
	return internalRotation;
}
IObject3D& Object3D::setInternalRotation(const Math::Vec4& rot) {
	internalRotation = rot;
	return *this;
}
IObject3D& Object3D::setRotation(const Vec4& rot) {
	rotation = rot;
	if (auto phys = getPhysics()) {
		phys->setRotation(rot);
	}
	transformCached = false;
	return *this;
}
bool Object3D::inViewport(const IWorld *pWorld, float radius, float mnd) {
	auto c = engine->getCamera();
	float x = pos.x;
	float y = pos.y;
	float z = pos.z;
	Vec3 position = c->getPos();
	Vec3 dir = c->getDir();
	float cx = (float)position.x;
	float cy = (float)position.y;
	float cz = (float)position.z;
	float dx = x - cx, dy = y - cy, dz = z - cz;
	float dist = sqrt(dx*dx + dy * dy);
	float realDist = sqrt(dx*dx + dy * dy + dz * dz);
	if (realDist < radius) return true;
	float angle = atan2(dx, dy);
	float camAngle = atan2(dir.x, dir.y);
	float diff = camAngle - angle;
	float pi = PI;
	while (diff >= pi) diff -= 2 * pi;
	while (diff <= -pi) diff += 2 * pi;
	return !(((abs(diff) > pi*0.5f) && dist > radius * 1.41f) || dist > ((IWorld*)pWorld)->settings.renderDistance / 1.75f || dist < mnd);
}
bool Object3D::intersects(const Vec3& o, const Vec3& dirParam, Vec3& intersection) {
	return false;
}

bool Object3D::attachTo(Ptr<IObject3D> parent, const std::string& jointName) {
	pAttachmentObject = parent;
	IObject3D *pObject = parent.get();
	IAnimated* pAnimated = dynamic_cast<IAnimated*>(pObject);
	if (pAnimated) {
		pAttachmentJoint = pAnimated->getJointByName(jointName);
		if (pAttachmentJoint) {
			return true;
		}
	}
	return false;
}

IObject3D& Object3D::detach() {
	pAttachmentJoint = nullptr;
	pAttachmentObject = nullptr;
	return *this;
}

Mat4 Object3D::getTransform(const Ptr<const IndexBuffer> ib) {
	if (!transformCached) {

		if (pAttachmentJoint && pAttachmentObject) {
			float3 jointPos(pAttachmentJoint->getPosition());
			XMVECTOR trans, scale, rot;
			XMMatrixDecompose(&scale, &rot, &trans, pAttachmentJoint->getTransform().raw());
			jointPos = mul(jointPos, pAttachmentJoint->getTransform());
			jointPos = mul(jointPos, matrix(XMMatrixRotationQuaternion(pAttachmentObject->getRotation())).transpose());

			setPos(pAttachmentObject->getPos() + jointPos);
			setRotation(
				XMQuaternionMultiply(
					XMQuaternionMultiply(rot,
						XMQuaternionMultiply(
							XMQuaternionRotationRollPitchYaw(PI / 2.0f, 0.0f, -PI),
							pAttachmentObject->getRotation().raw()
						)
					), internalRotation.raw()
				)
			);
		}

		auto physics = getPhysics();
		if (physics) {
			auto t = physics->getPos();
			pos = physics->getPos();
			rotation = physics->getRotation();
			transform = physics->getTransform() *  Mat4(XMMatrixScaling(scale.x, scale.y, scale.z));
			transformCached = staticFlag;
		} else {
			transformCached = true;
			transform = Mat4(XMMatrixTranslationFromVector(getPos().raw())).transpose() * Mat4(XMMatrixRotationQuaternion(getRotation().raw())).transpose() * Mat4(XMMatrixScaling(scale.x, scale.y, scale.z));
		}
		return transform;
	}
	return transform;
}

IObject3D& Object3D::create(void *params) { return *this; }

IObject3D& Object3D::init(IEngine *pEngine) { engine = pEngine; return *this; }

Object3D::~Object3D() {
	destroy();
}

void Object3D::destroy() {
	if (!destroyed) {
		if (drawData) {
			drawData->release(this);
		}
		destroyed = true;
	}
}

void Object3D::addRef() { refs++; }
void Object3D::release() { refs--; if (!refs) { if (!destroyed) destroy(); delete this; } }

float Object3D::getLifeTime() const { return lifeTime; }

Vec4& Object3D::getPigment() { return pigment; }
IObject3D& Object3D::setPigment(const Vec4& pigment) { this->pigment = pigment; return *this; }
IObject3D& Object3D::setUsePigment(bool state) { usePigment = state; return *this; }
bool Object3D::usesPigment() { return usePigment; }

Ptr<IPhysics> Object3D::getPhysics() {
	return physics;
}
IObject3D& Object3D::setPhysics(Ptr<IPhysics> phys) {
	physics = phys;
	return *this;
}
IObject3D& Object3D::enablePhysics() { physics->enable(); return *this; }
IObject3D& Object3D::disablePhysics() { physics->disable(); return *this; }

IObject3D& Object3D::buildPhysics(float mass, ShapeInfo shapeInfo, PhysicsCallback callback) {
	if (!physics) {
		physics = engine->createPhysics();
	}
	physics->init(this, mass, shapeInfo, callback);
	return *this;
}

Vertex* Object3D::getVertices(size_t& sz) { sz = drawData->getVertices().size();  return drawData->getVertices().data(); }
Index_t* Object3D::getIndices(size_t& sz) { sz = drawData->getVertices().size();  return drawData->getIndices().data(); }
std::vector<Ptr<IndexBuffer>>& Object3D::getAllIndices() { return drawData->getObjects(); }

Ptr<IDrawData> Object3D::getDrawData() {
	return drawData;
}

bool Object3D::queryDrawData(Ptr<IDrawDataManager> pMgr, const char *name, bool cache) {
	try {
		if (drawData && drawData->getVertices().size()) return false;
		static char buff[40];
		if (name == 0) {
			sprintf(buff, "%p", this);
			name = buff;
		}
		bool hadDrawData = drawData != 0;
		Ptr<IDrawData> pData = pMgr->getDrawData(name, cache);
		if (pData && !hadDrawData) pData->addRef(this);
		drawData = pData;
		return pData.get() != nullptr;
	}
	catch (std::exception& ex) {
		std::cout << "Failed to queryDrawData: " << ex.what() << std::endl;
		return false;
	}
}

void Object3D::update(const IWorld *pWorld) {
	if (pAttachmentJoint && pAttachmentObject) {
		transformCached = false;
	}
	auto phys = getPhysics();
	if (phys) {
		pos = phys->getPos();
		rotation = phys->getRotation();
	}
}
bool Object3D::isVisible() const { return visible; }
IObject3D& Object3D::setVisible(bool vis) { visible = vis; return *this; }

float Object3D::getLOD()  const { return 2; }
Ptr<IObject3D>* Object3D::getObjects(int* sz) { if (sz) *sz = 0; return 0; }
std::string Object3D::getClassName() const { return typeid(*this).name(); }
std::string Object3D::getName() const { return name; }
IObject3D& Object3D::setName(std::string name) { this->name = name; return *this; }

Mat4& Object3D::getModel() { return model; }
IObject3D& Object3D::setModel(const Mat4& mdl) { model = mdl; return *this; }

unsigned int Object3D::getType() const { return type; }
IObject3D& Object3D::setType(const unsigned int tp) { type = tp; return *this; }

unsigned int Object3D::getEntityId() const { return entityId; }
IObject3D& Object3D::setEntityId(const unsigned int id) { entityId = id; return *this; }

Vec3& Object3D::getScale() { return scale; }
IObject3D& Object3D::setScale(const Vec3& s) {
	this->scale = s; 
	if (getPhysics()) {
		getPhysics()->setScale(s);
	}
	return *this;
}

unsigned int Object3D::getFlags() const { return flags; }
IObject3D& Object3D::setFlags(const int flags) { this->flags = flags; return *this; }

Vec3& Object3D::getPos() { return pos; }

Vec4& Object3D::getRotation() { return rotation; }

unsigned int Object3D::getTessellation() const { return tess; }
IObject3D& Object3D::setTessellation(const unsigned int newTess) { tess = newTess; return *this; }

Ptr<IMaterial>& Object3D::getTextureGroup() { return txGroup; }
IObject3D& Object3D::setTextureGroup(Ptr<IMaterial> group) { txGroup = group; return *this; }

unsigned int Object3D::getEngineBufferId() const { return drawData->getBufferId(); }
IObject3D& Object3D::setEngineBufferId(const unsigned int id) { drawData->setBufferId(id); return *this; }

unsigned int Object3D::getInstancesCount() const { return instances; }
IObject3D& Object3D::setInstancesCount(const unsigned int c) { instances = c; return *this; }

unsigned int Object3D::getMaxInstancesCount() const { return maxInstances; }
IObject3D& Object3D::setMaxInstancesCount(const unsigned int c) { maxInstances = c; return *this; }

bool Object3D::requiresVBUpdate() const { return vbupdate; }
void Object3D::flagVBUpdate(const bool f) { vbupdate = f; }

int Object3D::getCulling() const { return CullBack; }

IObject3D& Object3D::attachSound(Ptr<ISound> sound, Math::Vec3 relPos) {
	sound->setRelativePos(relPos);
	sounds.push_back(sound);
	return *this;
}

std::vector<Ptr<ISound> > Object3D::getAttachedSounds() const {
	return sounds;
}

IObject3D& Object3D::attachLight(Ptr<IDynamicLight> light, Math::Vec3 relPos) {
	light->attach(this, relPos);
	lights.push_back(light);
	return *this;
}
std::vector<Ptr<IDynamicLight>> Object3D::getAttachedLights() const {
	return lights;
}

bool Object3D::isStatic() const {
	return staticFlag;
}
IObject3D& Object3D::setStatic(bool flag) {
	staticFlag = flag;
	return *this;
}

SerializeData Object3D::serialize() {
	SerializeData data;
	return data;
}

void Object3D::deserialize(const SerializeData& params) {

}

void* Object3D::getParent(unsigned int idx) const {
	auto par = parents.find(idx);
	if (par != parents.end()) return par->second;
	return 0;
}

IObject3D& Object3D::setParent(void *parent, unsigned int idx) {
	parents[idx] = parent;
	return *this;
}