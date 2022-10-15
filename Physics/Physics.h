#pragma once
#ifndef __PHYSICS_H__
#define __PHYSICS_H__
#include "../Lamegine/Interfaces.h"
#include <vector>
#include <functional>

class btRigidBody;
class btCollisionShape;
class btDiscreteDynamicsWorld;
class btTriangleMesh;
class btBvhTriangleMeshShape;
class btDynamicsWorld;
class btKinematicCharacterController;
class btPairCachingGhostObject;

struct IObject3D;
struct btDefaultMotionState;

class PhysicsSystem : public IPhysicsSystem {
	int simulation_steps = 10;
	float ts = 1.0f;
	btDiscreteDynamicsWorld* world;
	friend class Physics;
	IEngine *engine;
public:
	PhysicsSystem(IEngine *pEngine) : engine(pEngine) { init(); }
	virtual Ptr<IPhysics> createPhysics();
	virtual Ptr<IPhysics> createCameraPhysics(bool physical, float3 position, float height);
	virtual ~PhysicsSystem();
	virtual void init();
	virtual void destroy();
	virtual void update(double dt);
	virtual void setSteps(int steps) { simulation_steps = steps; }
	virtual int getSteps() const { return simulation_steps; }
	virtual void setTimeScale(float timescale) { ts = timescale; }
	virtual float getTimeScale() const { return ts; }

	virtual IObject3D* raytraceObject(RayTraceParams& params);
	virtual void* getHandle() { return world; }
	virtual void** getHandleRef() { return (void**)&world; }
private:
	static void globalCallback(btDynamicsWorld *dynamicsWorld, float timeStep);
};

class Physics : public IPhysics {
private:
	std::vector<btDefaultMotionState*> motionStates;
	std::vector<btRigidBody*> bodies;
	std::vector<btTriangleMesh*> meshes;
	std::vector<btCollisionShape*> meshShapes;
	CollisionCallback collisionCallback = nullptr;
	bool enabled;
	btRigidBody *body;
	PhysicsSystem *system;
	btKinematicCharacterController *m_character;
	btPairCachingGhostObject *m_ghostObject;
	float mass;
public:
	virtual operator bool() const { return enabled; }

	Physics(PhysicsSystem *psystem) : system(psystem), body(0), enabled(false) {}
	virtual ~Physics();

	void initCamera(float3 position, float height);

	std::vector<btRigidBody*>& getBodies() { return bodies; }
	btRigidBody* getBody() { return body; }

	virtual void init(IObject3D* parent, float mass = 0.0f, ShapeInfo shape = ShapeInfo(ePS_Generic), PhysicsCallback cb = nullptr);

	virtual void enable() { enabled = true; }
	virtual void disable() { enabled = false; }
	virtual void push(float3 dir, float force);
	virtual void release();
	virtual bool isEnabled() { return enabled; }
	virtual IPhysics& setGravity(float gravity);
	virtual IPhysics& setResistution(float rest);
	virtual IPhysics& setDamping(float lin_damping, float ang_damping);
	virtual IPhysics& setFriction(float friction);
	virtual IPhysics& setHitFriction(float friction);
	virtual IPhysics& setRollingFriction(float friction);
	virtual IPhysics& setSpinningFriction(float friction);
	virtual IPhysics& setAnisotropicFriction(float3 friction, int mode=1);
	virtual IPhysics& setAngularVelocity(float3 av);
	virtual IPhysics& setAngularFactor(float af);
	virtual IPhysics& setDeactivationTime(float dt);
	virtual IPhysics& setCollisionCallback(CollisionCallback cb);
	virtual IPhysics& activate();
	virtual CollisionCallback getCollisionCallback() const;
	virtual IPhysics& setScale(float3 scale);
	virtual float getMass() const;


	virtual bool onGround();
	virtual void jump(float3 dir);
	virtual float3 getPos();
	virtual void setPos(float3 pos);
	virtual float4 getRotation();
	virtual void setRotation(float4 rot);
	virtual float3 getGhostPos();
	virtual void setGhostPos(float3 pos);
	virtual void setWalkDirection(float3 dir);
	virtual matrix getTransform();

	virtual void* getHandle() { return body; }
	virtual void** getHandleRef() { return (void**)&body; }

	virtual SerializeData serialize();
	virtual void deserialize(const SerializeData& params);
};
#endif