#pragma once
#ifndef __INTERFACES__
#define _CRT_SECURE_NO_WARNINGS
#define __INTERFACES__
#ifndef WIN32_LEAN_AND_MEAN 
#define WIN32_LEAN_AND_MEAN 
#endif
#ifndef _ENABLE_EXTENDED_ALIGNED_STORAGE
#define _ENABLE_EXTENDED_ALIGNED_STORAGE
#endif
#include <wrl.h>
#include <memory>
#include <vector>
#include <map>
#include <functional>
#include <string>
#include <thread>
#include <atomic>
#include <deque>
#include <algorithm>
#include <unordered_map>
#include <sstream>
#include "VectorMath.h"
#include "Constants.h"

#define LAMEGINE_SDK_VERSION 0x00010000

struct Vertex;
struct IndexBuffer;

struct ITexture;
struct IMaterial;
struct IDrawData;
struct IEngine;
struct IGame;
struct ISound;
struct ISoundSystem;
struct IObject3D;
struct IPhysics;
struct IPhysicsSystem;
struct IWorld;
struct IDynamicLight;
struct ILightSystem;
struct IWindow;
struct ICamera;
struct IKeyFrame;
struct IJoint;

struct IRenderer;
struct IRenderTarget;
struct IDepthStencil;
struct IShader;
struct IBuffer;
struct IIndexBuffer;
struct IVertexBuffer;
struct IConstantBuffer;
struct ISampler;
struct IRenderState;
struct IBlendState;

struct RadiosityMap;
struct CubeMap;
struct TaskBase;
struct ShapeInfo;

enum DXGI_FORMAT;

class btRigidBody;

#ifdef gcnew
#undef gcnew
#endif
#define gcnew std::make_shared

#ifndef Ptr
#define Ptr std::shared_ptr
#endif

#ifndef APIPtr
#define APIPtr Microsoft::WRL::ComPtr
#endif

struct CollisionContact {
	float3 pos[2];
	float3 normal;
	float distance;
	float lifeTime;
	float impulse;
};

#define eBT_Index		0x01
#define eBT_Vertex		0x02
#define eBT_Constant	0x04
#define eBT_Dynamic		0x08
#define eBT_Immutable	0x10
#define eBT_Default		0x20

enum ETopology {
	eTL_TriangleList
};

enum ERasterizerState {
	eRS_SolidFront,
	eRS_SolidBack,
	eRS_SolidNone,
	eRS_Wireframe
};

typedef int BufferTypes;

struct CollisionInfo {
	Ptr<IObject3D> self;
	Ptr<IObject3D> object;
	const std::vector<CollisionContact>& contacts;
	int index;
	float dt;
	CollisionInfo(Ptr<IObject3D> pSelf, Ptr<IObject3D> pObject, const std::vector<CollisionContact>& aContacts, int idx, float t)
		: self(pSelf), object(pObject), contacts(aContacts), index(idx), dt(t) {}
};

struct RayTraceParams {
	float mouseX = 0.0f;
	float mouseY = 0.0f;
	float distance = 256.0f;
	bool center = true;
	IObject3D *except = 0;
	int exceptType = -1;
	float3 hitPoint;
	float3 hitNormal;
	size_t ib = 0;
	bool is3d = false;
	float3 sourcePos;
	float3 viewDir;
};

struct Viewport {
	float TopLeftX;
	float TopLeftY;
	float Width;
	float Height;
	float MinDepth;
	float MaxDepth;
};

typedef std::function<void(btRigidBody*)> PhysicsCallback;
typedef std::function<void(const CollisionInfo& ci)> CollisionCallback;
typedef std::function<Ptr<IObject3D>(void*)> ClassFactory;
typedef std::function < Light(const Light& l)> LightCallback;

#define KEY_UP 0
#define KEY_DOWN 1
#define KEY_PRESS 2
#define MOUSE_PRESS 2
#define MOUSE_UP 0
#define MOUSE_DOWN 1
#define MOUSE_LEFT 0
#define MOUSE_RIGHT 1

typedef unsigned int Index_t;

struct Vertex {
	Math::Vec3 pos;
	Math::Vec2 texcoord;
	Math::Vec3 normal;
	Math::Vec3 tangent;
	uint3 jointIds;
	Math::Vec3 weights;
	Vertex(Math::Vec3 a, Math::Vec2 b, Math::Vec3 c) :pos(a), texcoord(b), normal(c) {}
	Vertex(Math::Vec3 a, Math::Vec2 b, Math::Vec3 c, Math::Vec3 tang) :pos(a), texcoord(b), normal(c), tangent(tang) {}
	Vertex() {}
	Vertex(const Vertex& vtx) :pos(vtx.pos), texcoord(vtx.texcoord), normal(vtx.normal), tangent(vtx.tangent), jointIds(vtx.jointIds), weights(vtx.weights) {
		
	}
	Vertex& operator=(const Vertex& vtx) {
		pos = vtx.pos;
		texcoord = vtx.texcoord;
		normal = vtx.normal;
		tangent = vtx.tangent;
		jointIds = vtx.jointIds;
		weights = vtx.weights;
		return *this;
	}
};

struct IndexBuffer {
	Ptr<IMaterial> pTextures;
	std::string name;
	int offset;
	int start;
	Math::Vec3 relativePos;
	Math::Vec3 axisRadius;
	float radius;
	int type;
	void *extra;
	unsigned long long lastFrame;
	unsigned long id;
	unsigned long ddid;
	IndexBuffer() :
		start(0), radius(0.0f), lastFrame(0), id(0), ddid(0), type(0), extra(0), name(""), offset(0), pTextures(0), relativePos(Math::Vec3(0.0f, 0.0f, 0.0f))
	{

	}
	IndexBuffer(unsigned int off, Ptr<IMaterial> pMaterial, Math::Vec3 relPos = Math::Vec3(0.0f, 0.0f, 0.0f), std::string szName = "", unsigned long nId=0)
		: start(0), radius(0.0f), lastFrame(0), id(nId), ddid(0), type(0), extra(0), name(szName), offset(off), pTextures(pMaterial), relativePos(relPos)
	{
		//...
	}
};

struct JointTransform {
	float3 position;
	float4 rotation;
	JointTransform() {}
	JointTransform(float3& p, float4& r) :position(p), rotation(r) {}
	matrix getLocalTransform() const {
		return matrix(XMMatrixTranslationFromVector(position.raw())).transpose() * matrix(XMMatrixRotationQuaternion(rotation.raw())).transpose();
	}
	static JointTransform interpolate(JointTransform& frameA, JointTransform& frameB, float progression) {
		float3 pos(XMVectorLerp(frameA.position.raw(), frameB.position.raw(), progression));
		float4 rot = XMQuaternionSlerp(frameA.rotation.raw(), frameB.rotation.raw(), progression);
		return JointTransform(pos, rot);
	}
};

struct Message {
	std::shared_ptr<void> hdl;
	std::string msg;
};


namespace std {
	static std::string to_string(const float2& f) {
		std::stringstream ss; ss << f.x << ", " << f.y; return ss.str();
	}
	static std::string to_string(const float3& f) {
		std::stringstream ss; ss << f.x << ", " << f.y << ", " << f.z; return ss.str();
	}
	static std::string to_string(const float4& f) {
		std::stringstream ss; ss << f.x << ", " << f.y << ", " << f.z << ", " << f.w; return ss.str();
	}
	static std::string to_string(const XMVECTOR& v) {
		std::stringstream ss; ss << float4(v); return ss.str();
	}
}

struct Any : public std::enable_shared_from_this<Any> {
	union {
		unsigned char u8;
		char i8;
		unsigned short u16;
		short i16;
		unsigned int u32;
		int i32;
		unsigned long long u64;
		long long i64;
		float f;
		XMVECTOR f4;
	} NumVal;
	std::string StrVal;
	std::map<std::string, std::shared_ptr<Any>> Objs;
	std::vector<std::shared_ptr<Any>> Arr;

	Any() { }
	Any(const Any& a) : NumVal(a.NumVal), StrVal(a.StrVal), Objs(a.Objs), Arr(a.Arr) {}
	Any& operator=(const Any& a) {
		NumVal = a.NumVal;
		StrVal = a.StrVal;
		Objs = a.Objs;
		Arr = a.Arr;
		return *this;
	}

#define Construct(t,w)\
	Any(t v) {NumVal.w = v; StrVal = std::to_string(v); }\
	Any& operator=(t v) { NumVal.w = v; StrVal = std::to_string(v); return *this; }

	static Any& create() {
		return *std::make_shared<Any>();
	}

	Construct(unsigned char, u8)
	Construct(unsigned short, u16)
	Construct(unsigned int, u32)
	Construct(unsigned long long, u64)
	Construct(char, i8)
	Construct(short, i16)
	Construct(int, i32)
	Construct(long long, i64)
	Construct(float, f)
	Construct(XMVECTOR, f4)

	Any(const std::string& v) : StrVal(v) {}
	Any& operator=(const std::string& v) { StrVal = v; return *this; }

	operator unsigned char() const { return NumVal.u8; }
	operator unsigned short() const { return NumVal.u16; }
	operator unsigned int() const { return NumVal.u32; }
	operator unsigned long long() const { return NumVal.u64; }

	operator char() const { return NumVal.i8; }
	operator short() const { return NumVal.i16; }
	operator int() const { return NumVal.i32; }
	operator long long() const { return NumVal.i64; }

	operator float() const { return NumVal.f; }
	operator std::string() const { return StrVal; }

	Any& operator[](const char* v) {
		auto it = Objs.find(v);
		if (it == Objs.end()) {
			std::shared_ptr<Any> a = std::make_shared<Any>();
			Objs.insert(std::make_pair(std::string(v), a));
			return *a;
		}
		return *it->second;
	}
	Any& operator[](const std::string&& v) { return operator[](v.c_str()); }
	Any& operator[](const size_t index) {
		return *Arr[index];
	}
	const size_t size() const { return Arr.size(); }
	std::shared_ptr<Any> has(const char *v) {
		auto it = Objs.find(v);
		if (it != Objs.end()) return it->second;
		return nullptr;
	}
	std::shared_ptr<Any> has(const std::string&& v) { return has(v.c_str()); }
	Any& push(Any& a) {
		Arr.push_back(a.shared_from_this());
		return *this;
	}
};

typedef Any SerializeData;
struct ISerializable {
	virtual ~ISerializable() = default;
	virtual SerializeData serialize() = 0;
	virtual void deserialize(const SerializeData& params) = 0;
};

struct IWorld : public ISerializable {
	double delta;

	struct {
		float renderFov;
		float shadowQuality;
		float ssrQuality;
		float reflectionQuality;
		float refractionQuality;
		float renderQuality;
		bool wireFrame;
		bool vsync;
		float terrainScale;
		float renderDistance;
		float width;
		float height;
		bool fullScreen;
		bool editor;
		bool depthOfField;
		int convertArg;
		bool tessellate;
		bool solidCulling;
		int msaa;
		int anisotropy;
		int cubemapQuality;
		float shadowCascades[16];
	} settings;

	struct {
		float jumpHeight;
		float stepHeight;
	} controls;

	struct {
		matrix world, view, proj;
	} mainPass;

	struct {
		float3 lightPos;
		float gravity;
		float waterLevel;
	} environment;

	SceneConstants constants;
	EntityConstants entityConstants;
	AnimationConstants animationConstants;

	virtual void load(const char *file) = 0;
	virtual void load(std::map<std::string, std::string> args) = 0;
};

enum TextureSlot {
	TextureSlotDiffuse = 0,
	TextureSlotNormal = 1,
	TextureSlotMisc = 2,
	TextureSlotAmbient = 3,
	TextureSlotAlpha = 4,
	TextureSlotDiffuse2 = 5,
	TextureSlotNormal2 = 6,
	TextureSlotMisc2 = 7,
	TextureSlotDiffuse3 = 8,
	TextureSlotNormal3 = 9,
	TextureSlotMisc3 = 10,
	TextureEnd = 11
};

struct ITexture : public std::enable_shared_from_this<ITexture>, public ISerializable {
	virtual ~ITexture() = default;
	virtual void attach(IEngine *pEngine, int slot) = 0;
	virtual size_t getMemoryUsage() const = 0;
	virtual void* getShaderView() = 0;
	virtual void** getShaderViewRef() = 0;
	virtual std::string getPath() = 0;
	virtual std::string getName() = 0;
	virtual void addRef() = 0;
	virtual void release() = 0;
	virtual int getSlot() const = 0;
	virtual int getWidth() const = 0;
	virtual int getHeight() const = 0;
	virtual int getBitsPerPixel() const = 0;
	virtual bool read(std::vector<unsigned char>& data) const = 0;
	virtual bool read(std::vector<float>& data) const = 0;
	virtual bool read(std::vector<float2>& data) const = 0;
	virtual bool read(std::vector<float3>& data) const = 0;
	virtual bool read(std::vector<float4>& data) const = 0;
	virtual DXGI_FORMAT getFormat() const = 0;
	virtual void* getHandle() = 0;
	virtual void** getHandleRef() = 0;
	virtual void refresh(bool force = false) = 0;
	virtual void generateMips() = 0;
};

struct IFrameBuffer : public std::enable_shared_from_this<IFrameBuffer> {
	virtual Ptr<ITexture> getTexture() = 0;
	virtual Ptr<ITexture> getDepthTexture() = 0;

	virtual Ptr<IRenderTarget> getRenderTarget() = 0;
	virtual Ptr<IDepthStencil> getDepthStencil() = 0;

	virtual matrix* getView() = 0;
	virtual matrix* getProj() = 0;
	virtual matrix* getWorld() = 0;

	virtual Viewport& getViewport() = 0;
	virtual void setViewport(Viewport vp) = 0;
	virtual bool getVPW(matrix* v, matrix* p, matrix* w) = 0;
	virtual void setVPW(matrix* v, matrix* p, matrix* w) = 0;

	virtual void setPixelShader(Ptr<IShader> shader) = 0;
	virtual void setVertexShader(Ptr<IShader> shader) = 0;

	virtual Ptr<IShader> getPixelShader() = 0;
	virtual Ptr<IShader> getVertexShader() = 0;
};

struct IMaterial : public std::map<int, Ptr<ITexture> >, public ISerializable {
	virtual ~IMaterial() = default;
	virtual bool isTransparent() const = 0;
	virtual IMaterial& add(Ptr<ITexture> tx) = 0;
	virtual const char *getName() const = 0;
	virtual int attach(IEngine *pEngine, int slot) = 0;

	virtual Math::Vec4& ambient() = 0;
	virtual Math::Vec4& diffuse() = 0;
	virtual Math::Vec4& specular() = 0;
	virtual Math::Vec4& pbr1() = 0;
	virtual Math::Vec4& pbr2() = 0;
	virtual float& alpha() = 0;
	virtual float2& scale() = 0;
	virtual bool& isRefractive() = 0;
};

struct IDrawData : public std::enable_shared_from_this<IDrawData> {
	virtual ~IDrawData() = default;
	virtual void addRef(IObject3D* pObj) = 0;
	virtual void release(IObject3D* pObj) = 0;
	virtual void finish() = 0;
	virtual bool& isBound() = 0;
	virtual bool& needsCleanUp() = 0;
	virtual Ptr<IBuffer> getIndexBuffer() = 0;
	virtual Ptr<IBuffer> getVertexBuffer() = 0;
	virtual void setIndexBuffer(Ptr<IBuffer> buffer) = 0;
	virtual void setVertexBuffer(Ptr<IBuffer> buffer) = 0;
	virtual std::vector<Vertex>& getVertices() = 0;
	virtual std::vector<Index_t>& getIndices() = 0;
	virtual std::vector<Ptr<IndexBuffer>>& getObjects() = 0;
	virtual unsigned int getTotalIndices() const = 0;
	virtual unsigned int getBufferId() const = 0;
	virtual void setBufferId(unsigned int id) = 0;
};

struct IDrawDataManager : public std::enable_shared_from_this<IDrawDataManager> {
	virtual ~IDrawDataManager() = default;
	virtual bool remove(std::string path) = 0;
	virtual Ptr<IDrawData> getDrawData(std::string path, bool cache) = 0;
};

enum PhysicsShape {
	ePS_Sphere,
	ePS_Box,
	ePS_ConvexHull,
	ePS_Generic
};

struct ShapeInfo {
	PhysicsShape type;
	float radius;
	float x, y, z;
	ShapeInfo() : type(ePS_Generic), radius(0.0f), x(0.0f), y(0.0f), z(0.0f) {}
	ShapeInfo(PhysicsShape t, float r = 1.0f) : type(t), radius(r) {}
	ShapeInfo(PhysicsShape t, float half_x, float half_y, float half_z) : type(t), x(half_x), y(half_y), z(half_z) {}
};

struct IPhysicsSystem {
	virtual ~IPhysicsSystem() = default;
	virtual Ptr<IPhysics> createPhysics() = 0;
	virtual void update(double dt) = 0;
	virtual void setSteps(int steps) = 0;
	virtual int getSteps() const = 0;
	virtual void setTimeScale(float timescale) = 0;
	virtual float getTimeScale() const = 0;
	virtual IObject3D *raytraceObject(RayTraceParams& params) = 0;
	virtual Ptr<IPhysics> createCameraPhysics(bool physical, float3 position, float height) = 0;
	virtual void* getHandle() = 0;
	virtual void** getHandleRef() = 0;
};

struct IPhysics : public std::enable_shared_from_this<IPhysics>, public ISerializable {
	virtual ~IPhysics() = default;
	virtual operator bool() const = 0;
	virtual void init(IObject3D* parent, float mass = 0.0f, ShapeInfo shapeInfo = ShapeInfo(ePS_Generic), PhysicsCallback cb = nullptr) = 0;
	virtual void enable() = 0;
	virtual void disable() = 0;
	virtual bool isEnabled() = 0;
	virtual void push(float3 dir, float force) = 0;
	virtual void release() = 0;
	virtual IPhysics& setScale(float3 scale) = 0;
	virtual IPhysics& setGravity(float gravity) = 0;
	virtual IPhysics& setResistution(float rest) = 0;
	virtual IPhysics& setDamping(float lin_damping, float ang_damping) = 0;
	virtual IPhysics& setFriction(float friction) = 0;
	virtual IPhysics& setHitFriction(float friction) = 0;
	virtual IPhysics& setRollingFriction(float friction) = 0;
	virtual IPhysics& setSpinningFriction(float friction) = 0;
	virtual IPhysics& setAnisotropicFriction(float3 friction, int mode=1) = 0;
	virtual IPhysics& setAngularVelocity(float3 av) = 0;
	virtual IPhysics& setAngularFactor(float af) = 0;
	virtual IPhysics& setDeactivationTime(float dt) = 0;
	virtual IPhysics& setCollisionCallback(CollisionCallback cb) = 0;
	virtual IPhysics& activate() = 0;

	virtual bool onGround() = 0;
	virtual void jump(float3 dir) = 0;
	virtual float3 getPos() = 0;
	virtual void setPos(float3 pos) = 0;
	virtual float4 getRotation() = 0;
	virtual void setRotation(float4 rot) = 0;
	virtual float3 getGhostPos() = 0;
	virtual void setGhostPos(float3 pos) = 0;
	virtual void setWalkDirection(float3 dir) = 0;
	virtual matrix getTransform() = 0;

	virtual void* getHandle() = 0;
	virtual void** getHandleRef() = 0;

	virtual float getMass() const = 0;
	virtual CollisionCallback getCollisionCallback() const = 0;
};

struct IAnimation : public std::enable_shared_from_this<IAnimation> {
	virtual ~IAnimation() = default;
	virtual std::vector<Ptr<IKeyFrame>>& getKeyFrames() = 0;
	virtual float getLength() const = 0;
};

struct IKeyFrame : public std::enable_shared_from_this<IKeyFrame> {
	virtual ~IKeyFrame() = default;
	virtual std::unordered_map<std::string, JointTransform>& getJointTransforms() = 0;
	virtual float getTimestamp() const = 0;
};

struct IJoint : public std::enable_shared_from_this<IJoint> {
	virtual ~IJoint() = default;
	virtual void reset(int jointId, const std::string& jointName, matrix mtx) = 0;
	virtual void addChild(Ptr<IJoint> joint) = 0;
	virtual std::string& getName() = 0;
	virtual matrix& getTransform() = 0;
	virtual void setTransform(matrix& mtx) = 0;
	virtual matrix& getBindTransform() = 0;
	virtual matrix& getInverseBindTransform() = 0;
	virtual void setPosition(float3 pos) = 0;
	virtual float3 getPosition() const = 0;
	virtual void setRotation(float4 pos) = 0;
	virtual float4 getRotation() const = 0;
	virtual void calcInverseBindTransform(const matrix& parentBindTransform) = 0;
	virtual std::vector<Ptr<IJoint>>& getChildren() = 0;
	virtual int getId() const = 0;
	virtual void printHierarchy(Ptr<IJoint> parent, int offset) const = 0;
	virtual Ptr<IJoint> getJointByName(const std::string& name) = 0;
};

struct IAnimator : public std::enable_shared_from_this<IAnimator> {
	virtual ~IAnimator() = default;
	virtual void startAnimation(Ptr<IAnimation> anim) = 0;
	virtual void update(float dt) = 0;
};

struct IAnimated {
	virtual ~IAnimated() = default;
	virtual int getJointCount() const = 0;
	virtual Ptr<IJoint> getRootJoint() = 0;
	virtual Ptr<IAnimator> getAnimator() = 0;
	virtual std::vector<matrix>& getJointTransforms() = 0;
	virtual void startAnimation(Ptr<IAnimation> animation) = 0;
	virtual Ptr<IAnimation> getAnimation(const std::string& name) = 0;
	virtual std::unordered_map<std::string, Ptr<IAnimation>>& getAnimations() = 0;
	virtual Ptr<IJoint> getJointByName(const std::string& name) = 0;
};

struct IObject3D : public std::enable_shared_from_this<IObject3D>, public ISerializable {
	virtual ~IObject3D() = default;
	virtual size_t getSize() const = 0;
	virtual IObject3D& create(void *params = 0) = 0;
	virtual IObject3D& init(IEngine *pEngine) = 0;
	virtual void destroy() = 0;

	virtual IObject3D& add(bool autoInit = true, bool cleanUp = false) = 0;

	virtual Ptr<IPhysics> getPhysics() = 0;
	virtual IObject3D& setPhysics(Ptr<IPhysics> physics) = 0;

	virtual IObject3D& enablePhysics() = 0;
	virtual IObject3D& disablePhysics() = 0;

	virtual IObject3D& buildPhysics(float mass = 0.0f, ShapeInfo shapeInfo = ShapeInfo(), PhysicsCallback callback = nullptr) = 0;

	virtual void addRef() = 0;
	virtual void release() = 0;
	virtual IObject3D& setLifeTime(float lifeTime) = 0;
	virtual float getLifeTime() const = 0;

	virtual Vertex* getVertices(size_t& sz) = 0;
	virtual Index_t* getIndices(size_t& sz) = 0;
	virtual std::vector<Ptr<IndexBuffer>>& getAllIndices() = 0;
	virtual Ptr<IDrawData> getDrawData() = 0;
	virtual bool queryDrawData(Ptr<IDrawDataManager> pMgr, const char *name = 0, bool cache = true) = 0;

	virtual void update(const IWorld *pWorld) = 0;
	virtual bool isVisible() const = 0;
	virtual IObject3D& setVisible(bool visible) = 0;

	virtual float getLOD() const = 0;
	virtual Ptr<IObject3D>* getObjects(int* sz) = 0;
	virtual std::string getClassName() const = 0;
	virtual std::string getName() const = 0;
	virtual IObject3D& setName(std::string name) = 0;

	virtual Math::Mat4& getModel() = 0;
	virtual IObject3D& setModel(const Math::Mat4& mdl) = 0;

	virtual Math::Vec4& getPigment() = 0;
	virtual IObject3D& setPigment(const Math::Vec4& pigment) = 0;
	virtual IObject3D& setUsePigment(bool state = false) = 0;
	virtual bool usesPigment() = 0;

	virtual unsigned int getType() const = 0;
	virtual IObject3D& setType(const unsigned int type) = 0;

	virtual unsigned int getEntityId() const = 0;
	virtual IObject3D& setEntityId(const unsigned int id) = 0;

	virtual Math::Vec3& getScale() = 0;
	virtual IObject3D& setScale(const Math::Vec3& scale) = 0;

	virtual unsigned int getFlags() const = 0;
	virtual IObject3D& setFlags(const int flags) = 0;

	virtual Math::Vec3& getPos() = 0;
	virtual IObject3D& setPos(const Math::Vec3& pos) = 0;

	virtual Math::Vec4& getRotation() = 0;
	virtual IObject3D& setRotation(const Math::Vec4& rot) = 0;

	virtual Math::Vec4& getInternalRotation() = 0;
	virtual IObject3D& setInternalRotation(const Math::Vec4& rot) = 0;

	virtual Math::Mat4 getTransform(const Ptr<const IndexBuffer> ib) = 0;

	virtual unsigned int getTessellation() const = 0;
	virtual IObject3D& setTessellation(const unsigned int tess) = 0;

	virtual Ptr<IMaterial>& getTextureGroup() = 0;
	virtual IObject3D& setTextureGroup(Ptr<IMaterial> group) = 0;

	virtual IObject3D& setEngineBufferId(const unsigned int id) = 0;
	virtual unsigned int getEngineBufferId() const = 0;

	virtual unsigned int getInstancesCount() const = 0;
	virtual IObject3D& setInstancesCount(const unsigned int c) = 0;

	virtual unsigned int getMaxInstancesCount() const = 0;
	virtual IObject3D& setMaxInstancesCount(const unsigned int c) = 0;

	virtual bool requiresVBUpdate() const = 0;
	virtual void flagVBUpdate(const bool state) = 0;

	virtual bool inViewport(const IWorld *pWorld, float radius, float mnd = 0.0f) = 0;
	virtual bool intersects(const Math::Vec3& origin, const Math::Vec3& dir, Math::Vec3& intersection) = 0;

	virtual int getCulling() const = 0;

	virtual IObject3D& attachSound(Ptr<ISound> sound, Math::Vec3 relPos = Math::Vec3(0.0f, 0.0f, 0.0f)) = 0;
	virtual std::vector<Ptr<ISound>> getAttachedSounds() const = 0;

	virtual IObject3D& attachLight(Ptr<IDynamicLight> light, Math::Vec3 relPos = Math::Vec3(0.0f, 0.0f, 0.0f)) = 0;
	virtual bool attachTo(Ptr<IObject3D> parent, const std::string& jointName) = 0;
	virtual IObject3D& detach() = 0;
	virtual std::vector<Ptr<IDynamicLight>> getAttachedLights() const = 0;

	virtual bool isStatic() const = 0;
	virtual IObject3D& setStatic(bool flag) = 0;

	virtual bool isAnimated() const = 0;

	virtual bool exportObject(const std::string& path, const std::string& pathPrefix) = 0;
	virtual bool importObject(const std::string& path, const std::string& pathPrefix) = 0;

	virtual IObject3D& setParent(void *parent, unsigned int idx = 0) = 0;
	virtual void* getParent(unsigned int idx = 0) const = 0;

	virtual void postInit() = 0;
};

struct IVehicle {
	virtual ~IVehicle() = default;
	virtual void forward(float speed) = 0;
	virtual void turn(float angle) = 0;
	virtual void setSpeed(float speed) = 0;
	virtual void setController(void *ctl) = 0;
	virtual void* getController() = 0;
	virtual bool isControlled() = 0;
};

struct ISoundSystem : public std::enable_shared_from_this<ISoundSystem> {
	virtual ~ISoundSystem() = default;
	virtual Ptr<ISound> getSound(const char *path, bool is3d = true) = 0;
	virtual Ptr<ISound> getSound(const wchar_t *path, bool is3d = true) = 0;
	virtual void onUpdate() = 0;
	virtual void setReverb(int id) = 0;
};

struct ISound : public std::enable_shared_from_this<ISound>, public ISerializable {
	virtual ~ISound() = default;
	virtual void setRelativePos(Math::Vec3 pos) = 0;
	virtual Math::Vec3 getRelativePos() = 0;
	virtual void play() = 0;
	virtual void pause() = 0;
	virtual void stop() = 0;
	virtual void resume() = 0;
	virtual void setPan(float pan) = 0;
	virtual void setVolume(float vol) = 0;
	virtual void setPitch(float pitch) = 0;
	virtual bool isPlaying() = 0;
	virtual bool is3D() = 0;
	virtual void update(Math::Vec3 pos, Math::Vec4 rotation, Math::Vec3 listenerPos, Math::Vec3 listenerDir, float dt) = 0;
};

struct IDynamicLight : public ISerializable {
	virtual ~IDynamicLight() = default;
	virtual Light update() = 0;
	virtual Light& getLight() = 0;
	virtual Math::Vec3& getRelativePos() = 0;
	virtual IObject3D*& getParent() = 0;
	virtual LightCallback& getCallback() = 0;
	virtual void attach(IObject3D* object, Math::Vec3 relPos = Math::Vec3(0.0f, 0.0f, 0.0f)) = 0;
	virtual Light& getFinalLight() = 0;
	virtual void setFinalLight(const Light& l) = 0;
};

struct ILightSystem {
	virtual ~ILightSystem() = default;
	virtual ILightSystem& addLight(Ptr<IDynamicLight> def) = 0;
	virtual std::vector<Ptr<IDynamicLight>> getNearbyLights(Math::Vec3 pos, float radius, int limit = 10) = 0;
	virtual Ptr<IDynamicLight> createDynamicLight(Light lightSpec, LightCallback callback = nullptr, IObject3D* parent = nullptr, Math::Vec3 relPos = Math::Vec3(0.0f, 0.0f, 0.0f)) = 0;
	virtual void removeLight(Ptr<IDynamicLight> light) = 0;
};

struct ICamera : public std::enable_shared_from_this<ICamera>, public ISerializable {
	virtual ~ICamera() = default;
	virtual Math::Vec3 getPos() = 0;
	virtual Math::Vec3 getDir() = 0;
	virtual void setPos(Math::Vec3 pos) = 0;
	virtual void setDir(Math::Vec3 dir) = 0;
	virtual void move(float zAngle, float speed, double dt, bool backwards = false) = 0;
	virtual void stopMoving() = 0;
	virtual void jump(double dt) = 0;
	virtual void rotate(float left, float up, const float sens = 0.25) = 0;
	virtual void initPhysics() = 0;
	virtual void update(double dt) = 0;
	virtual bool isPhysical() = 0;
	virtual void enablePhysics(bool enable) = 0;
};

struct IShader : public std::enable_shared_from_this<IShader> {
	virtual ~IShader() = default;
	virtual void *getShader() const = 0;
	virtual int getType() const = 0;
};

struct IRenderTarget : public std::enable_shared_from_this<IRenderTarget> {
	virtual ~IRenderTarget() = default;
	virtual void* getHandle() = 0;
	virtual void** getHandleRef() = 0;
};

struct IDepthStencil : public std::enable_shared_from_this<IDepthStencil> {
	virtual ~IDepthStencil() = default;
	virtual void* getHandle() = 0;
	virtual void** getHandleRef() = 0;
};

struct IBuffer : public std::enable_shared_from_this<IBuffer> {
	virtual void* getHandle() = 0;
	virtual void** getHandleRef() = 0;
	virtual void *beginCopy() = 0;
	virtual bool copyData(void *data, size_t len, size_t off) = 0;
	virtual bool update(void *data) = 0;
	virtual void endCopy() = 0;
	virtual size_t getSize() = 0;
	virtual BufferTypes getType() = 0;
};

struct IRenderer : public std::enable_shared_from_this<IRenderer> {
	virtual ~IRenderer() = default;
	virtual Ptr<IShader> loadShader(const char *path, int type) = 0;
	virtual Ptr<ITexture> loadTexture(const char *f, const char *nm, bool generateMips, int slot, bool srgb) = 0;
	virtual Ptr<IFrameBuffer> createFrameBuffer(const char *name, int width, int height, Ptr<IShader> vertexShader, Ptr<IShader> pixelShader, int bpp, bool hasDepth, int mipLevel=1) = 0;
	virtual Ptr<IFrameBuffer> createFrameBuffer(const char *name, Ptr<ITexture> texture, Ptr<IShader> vertexShader, Ptr<IShader> pixelShader, float left, float top) = 0;
	virtual Ptr<IFrameBuffer> createScreenOutputFrameBuffer(int width, int height, Ptr<IShader> vertexShader, Ptr<IShader> pixelShader) = 0;
	virtual Ptr<IMaterial> createMaterial(const char *name) = 0;

	virtual void setShader(Ptr<IShader> pShader) = 0;
	virtual void setPixelShaderResources(int slot, const std::vector<Ptr<ITexture>>& resources) = 0;
	virtual void setVertexShaderResources(int slot, const std::vector<Ptr<ITexture>>& resources) = 0;
	virtual void setPixelShaderResources(int slot, Ptr<ITexture> resource) = 0;
	virtual void setVertexShaderResources(int slot, Ptr<ITexture> resource) = 0;
	virtual void setRenderTargets(const std::vector<Ptr<IRenderTarget>>& renderTargets, Ptr<IDepthStencil> depthStencil) = 0;

	virtual void clearRenderTarget(Ptr<IRenderTarget> renderTarget, const float *color) = 0;
	virtual void clearDepthStencil(Ptr<IDepthStencil> depthStencil) = 0;

	virtual void copyResource(Ptr<ITexture> dest, Ptr<ITexture> src) = 0;

	virtual void resetMaterials() = 0;
	virtual void present() = 0;

	virtual void setPixelConstantBuffers(int slot, const std::vector<Ptr<IBuffer>>& buffers) = 0;
	virtual void setVertexConstantBuffers(int slot, const std::vector<Ptr<IBuffer>>& buffers) = 0;
	virtual void setPixelConstantBuffers(int slot, Ptr<IBuffer> buffers) = 0;
	virtual void setVertexConstantBuffers(int slot, Ptr<IBuffer> buffers) = 0;

	virtual void setDrawBuffers(Ptr<IBuffer> vertexBuffer, Ptr<IBuffer> indexBuffer) = 0;
	virtual void setDrawData(Ptr<IDrawData> drawData) = 0;
	virtual void setRawDrawData(IDrawData* drawData) = 0;

	virtual void setRasterizer(ERasterizerState rs) = 0;

	virtual void assignSamplers() = 0;

	virtual void enableBlending() = 0;
	virtual void disableBlending() = 0;

	virtual bool createDeviceResources() = 0;
	virtual bool createWindowResources(IWindow *window) = 0;
	virtual void setVsync(bool state) = 0;
	virtual void goFullScreen() = 0;
	virtual void goWindowed() = 0;

	virtual void setViewports(int no, const Viewport* viewports) = 0;
	virtual void setTopology(ETopology topology) = 0;

	virtual void drawIndexedInstanced(int indexCount, int instanceCount, int offset) = 0;

	virtual Ptr<IBuffer> createBuffer(BufferTypes flags, size_t size, void *data) = 0;
};

struct IEngine {
	virtual int getVersion() = 0;
	virtual void shutdown(const char *msg) = 0;
	virtual bool checkCompatibility(int ver) = 0;
	virtual void getVersionAsString(char *out, int ver) const = 0;

	virtual ~IEngine() = default;

	virtual void terminate(const char *text) = 0;
	virtual void terminate(const wchar_t *text) = 0;

	virtual void setSoundSystem(Ptr<ISoundSystem> pSS) = 0;
	virtual void setRenderer(Ptr<IRenderer> pRenderer) = 0;
	virtual Ptr<IRenderer> getRenderer() const = 0;
	virtual Ptr<IPhysicsSystem> getPhysicsSystem() const = 0;

	virtual IWindow* init(IWindow *pWindow, float w, float h, bool fullscreen = false, const char *nm = "Lamegine", IWorld *world = 0, bool silent = false) = 0;

	virtual void addTask(TaskBase* task) = 0;
	virtual void queueMessage(const Message& msg) = 0;
	virtual void replyMessage(const Message& msg) = 0;

	virtual void setGame(IGame *gameObject) = 0;
	virtual IGame* getGame() = 0;
	virtual IWorld* getWorld() = 0;

	virtual Math::Vec3 getCameraPos() const = 0;
	virtual Math::Vec3 getCameraDir() const = 0;

	virtual IWindow* getWindow() = 0;

	virtual Ptr<ISoundSystem> getSoundSystem() = 0;
	virtual Ptr<ILightSystem> getLightSystem() = 0;
	virtual Ptr<IDrawDataManager> getDrawDataManager() = 0;

	virtual void registerClass(const char *className, ClassFactory fn) = 0;
	virtual ClassFactory getClass(const char *className) = 0;
	virtual Ptr<IObject3D> getClass(const char *className, void *params, bool init = true) = 0;
	
	virtual void onUpdate(double dt) = 0;

	virtual Ptr<ITexture> loadTexture(const char *file, const char *name, bool generateMips = true, int slot = -1, bool srgb = false) = 0;
	
	virtual void drawText(float x, float y, const char *text, Math::Vec3 color = Math::Vec3(1.0f, 1.0f, 1.0f)) = 0;

	virtual void setLUT(const char *path) = 0;

	virtual void addObject(Ptr<IObject3D> object, bool autoInit = false, bool cleanUp = true) = 0;
	virtual void selectObject(IObject3D *obj) = 0;
	virtual bool removeObject(IObject3D *obj) = 0;
	virtual std::vector<Ptr<IObject3D>>& getObjects() = 0;
	virtual IObject3D* getSelectedObject() = 0;
	virtual const std::vector<Ptr<IObject3D>>::iterator getObjectIterator(IObject3D *obj) = 0;
	virtual IObject3D *raytraceObject(RayTraceParams& params) = 0;
	
	virtual void setRadiosityMap(RadiosityMap rm) = 0;
	virtual RadiosityMap& getRadiosityMap() = 0;
	virtual void loadRadiosityMap(const char *path, Math::Vec4 resolution) = 0;

	virtual void addReflectionMap(Ptr<CubeMap> cubemap) = 0;
	virtual std::vector<Ptr<CubeMap>>& getReflectionMaps() = 0;
	virtual Ptr<CubeMap> getClosestReflectionMap(float3 pos) = 0;
	virtual void setReflectionMap(Ptr<CubeMap> cubemap) = 0;
	virtual void setReflectionMap(Ptr<ITexture> cubemap) = 0;
	virtual void setReflectionMapPosition(float3 centre) = 0;
	virtual void setReflectionMapBbox(float3 bboxA, float3 bboxB) = 0;
	virtual Ptr<CubeMap> getActiveReflectionMap() = 0;

	virtual Ptr<ITexture> getDefaultDiffuse() = 0;
	virtual Ptr<ITexture> getDefaultBump() = 0;
	virtual Ptr<ITexture> getDefaultAlpha() = 0;
	virtual Ptr<ITexture> getDefaultMisc() = 0;
	virtual Ptr<ITexture> getDefaultTexture() = 0;

	virtual void setActiveBGM(Ptr<ISound> bgm) = 0;
	virtual Ptr<ISound> getActiveBGM() = 0;

	virtual void setGlobalQuality(float q) = 0;
	virtual float getGlobalQuality() = 0;

	virtual void setCamera(Ptr<ICamera> pCamera) = 0;
	virtual Ptr<ICamera> getCamera() = 0;

	virtual void setTimeScale(float timeScale) = 0;
	virtual float getTimeScale() const = 0;

	virtual void setDistortion(float distortion) = 0;
	virtual float getDistortion() const = 0;

	virtual Ptr<IObject3D> getObject(unsigned int entityId) = 0;
	virtual Ptr<IPhysics> createPhysics() = 0;

	virtual double getGameTime() = 0;
	virtual unsigned int getFrameNumber()const = 0;

	virtual void generateCubemap() = 0;
};

struct IWindow {
	virtual ~IWindow() = default;
	virtual int *GetKeys() = 0;
	virtual int *GetMouseStates() = 0;
	virtual void* GetHandle() = 0;
	virtual void SetTitle(const char *text) = 0;
	virtual void SetTitle(const wchar_t *text) = 0;
	virtual int GetKeyState(int key) = 0;
	virtual bool IsMouseDown(int mouseBtn) = 0;
	virtual bool IsKeyPressed(int key) = 0;
	virtual bool IsKeyUp(int key) = 0;
	virtual bool IsKeyDown(int key) = 0;
	virtual bool IsMovementKeys(float& zAngle) = 0;
	virtual int GetMouseState(int btn) = 0;
	virtual void KeepMouseDown(int btn) = 0;
	virtual bool ShouldClose() = 0;
	virtual void Close() = 0;
	virtual bool IsActive() = 0;
	virtual void GetCursorPos(double* x, double *y) = 0;
	virtual void ShowCursor() = 0;
	virtual void HideCursor() = 0;
	virtual void SetCursorPos(double x, double y) = 0;
	virtual void SwapBuffers() = 0;
	virtual bool Poll() = 0;
};

struct IGame {
public:
	virtual ~IGame() = default;
	virtual void init(IEngine *pEngine) = 0;
	virtual void onUpdate(double dt) = 0;
	virtual void onKeyPressed(int key) = 0;
	virtual void onKeyDown(int key) = 0;
	virtual void onKeyUp(int key) = 0;
	virtual void onMouseDown(int btn) = 0;
	virtual void onMouseUp(int btn) = 0;
	virtual void onMousePressed(int btn) = 0;
	virtual void onScroll(int dir) = 0;
	virtual void onMessage(const Message& msg) = 0;
	virtual bool isActorGhost() = 0;

	virtual bool overrideCamera() = 0;
	virtual bool overrideLoader() = 0;
	virtual bool overrideKeyboard() = 0;
	virtual bool overrideMouse() = 0;
	virtual bool overrideMovement() = 0;

	virtual bool getCameraPos(Math::Vec3& out) = 0;
	virtual bool getCameraDir(Math::Vec3& out) = 0;
	virtual const char *getName() = 0;
	virtual void release() = 0;
};

struct RadiosityMap {
	Ptr<ITexture> texture;
	Math::Vec4 resolution;
	Math::Vec4 settings;
	RadiosityMap() : settings(1.0f, 1.0f, 0.0f, 1.0f) {}
};

struct CubeMap {
	Ptr<ITexture> texture;
	float3 position;
	float3 bbox1, bbox2;
	bool local;
};

struct TaskBase {
public:
	virtual void exec() = 0;
	virtual void finish() = 0;
	virtual bool isFinished() = 0;
};

template<class T>
class Task : public TaskBase {
private:
	std::function<T()> fn;
	std::function<void(T)> cb;
	T result;
	std::atomic<bool> finished;
public:
	Task(std::function<T()> fun, std::function<void(T)> callback = nullptr) : fn(fun), cb(callback) {}
	virtual void exec() {
		finished = false;
		result = fn();
		finished = true;
	}
	virtual bool isFinished() {
		return finished.load();
	}
	virtual void finish() {
		cb(result);
	}
};

template<typename Base, size_t Size>
struct Extend : public Base {
private:
	unsigned char Data[Size];
public:
	void setParent(void *Ref) {
		memmove(Data, Ref, Size);
	}
	Base* getParent() {
		return (Base*)Data;
	}
};

template<class T, class B>
T* GetComponent(B base) {
	return dynamic_cast<T*>(base.get());
}

struct ModelSpawnParams {
	std::string path;
};

#endif