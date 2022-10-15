#include "Physics.h"
#include "../Lamegine/VectorMathExt.h"
#include <btBulletDynamicsCommon.h>
#include <BulletCollision/CollisionShapes/btHeightfieldTerrainShape.h>

#include <BulletCollision/CollisionDispatch/btGhostObject.h>
#include <BulletDynamics/Character/btKinematicCharacterController.h>

#define ACTOR_HEIGHT 1.7f

#pragma comment(lib, "LinearMath")
#pragma comment(lib, "BulletCollision")
#pragma comment(lib, "BulletDynamics")

Ptr<IPhysics> PhysicsSystem::createPhysics() {
	return gcnew<Physics>(this);
}

PhysicsSystem::~PhysicsSystem() {
	destroy();
}

IObject3D* PhysicsSystem::raytraceObject(RayTraceParams& params) {
	IWindow* window = engine->getWindow();
	IWorld* pWorld = engine->getWorld();
	Ptr<ICamera> pCamera = engine->getCamera();
	params.ib = -1;
	float3 position, dir;
	if (params.is3d) {
		dir = params.viewDir;
		position = params.sourcePos;
	} else {
		dir = pCamera->getDir();
		position = pCamera->getPos();
		RECT wnd;
		GetClientRect((HWND)window->GetHandle(), &wnd);

		float ww = (float)(wnd.right - wnd.left);
		float wh = (float)(wnd.bottom - wnd.top);

		float curX = params.mouseX;
		float curY = params.mouseY;

		if (params.center) {
			curX = ww / 2.0f;
			curY = wh / 2.0f;
		}

		curX = (2.0f * curX) / ww - 1.0f;
		curY = 1.0f - (2.0f * curY) / wh;

		float3 nearPoint(curX, curY, 0.0f);
		float3 farPoint(curX, curY, 1.0f);

		matrix inverseViewProj = (pWorld->mainPass.proj * pWorld->mainPass.view).inverse();

		float3 origin = TransformTransposedFloat3(nearPoint, inverseViewProj);
		float3 end = TransformTransposedFloat3(farPoint, inverseViewProj);
		dir = (end - origin).normalize();
	}


	btVector3 start(position.x, position.y, position.z);
	btVector3 end = start + (btVector3(dir.x, dir.y, dir.z) * params.distance);
	if (params.except || params.exceptType != -1) {
		btCollisionWorld::AllHitsRayResultCallback rayCallback(start, end);
		world->rayTest(start, end, rayCallback);

		if (rayCallback.hasHit())
		{
			float closest = params.distance;
			IObject3D *best = 0;
			for (int i = 0; i < rayCallback.m_collisionObjects.size(); i++) {
				btRigidBody* gameObj = (btRigidBody*)(rayCallback.m_collisionObjects[i]);
				if (gameObj != 0)
				{
					IObject3D *pObj = (IObject3D*)gameObj->getUserPointer();
					if (pObj == params.except || (params.exceptType != -1 && pObj && pObj->getType() == params.exceptType)) continue;
					float dst = rayCallback.m_hitPointWorld[i].distance(start);
					if (dst < closest) {
						params.hitPoint = Math::toVec3(rayCallback.m_hitPointWorld[i]);
						params.hitNormal = Math::toVec3(rayCallback.m_hitNormalWorld[i]);
						params.ib = gameObj->getUserIndex2();
						best = pObj;
						closest = dst;
					}
				}
			}
			return best;
		}
	} else {
		btCollisionWorld::ClosestRayResultCallback rayCallback(start, end);
		world->rayTest(start, end, rayCallback);

		if (rayCallback.hasHit())
		{
			btRigidBody* gameObj = (btRigidBody*)(rayCallback.m_collisionObject);

			if (gameObj != 0)
			{
				params.hitPoint = Math::toVec3(rayCallback.m_hitPointWorld);
				params.hitNormal = Math::toVec3(rayCallback.m_hitNormalWorld);
				params.ib = gameObj->getUserIndex2();
				return (IObject3D*)gameObj->getUserPointer();
			}
		}
	}

	return 0;
}

void PhysicsSystem::init() {
	btBroadphaseInterface* broadphase = new btDbvtBroadphase();

	btDefaultCollisionConfiguration* collisionConfiguration(new btDefaultCollisionConfiguration());
	btCollisionDispatcher* dispatcher(new btCollisionDispatcher(collisionConfiguration));
	btSequentialImpulseConstraintSolver* solver(new btSequentialImpulseConstraintSolver);

	world = new btDiscreteDynamicsWorld(dispatcher, broadphase, solver, collisionConfiguration);
	world->setGravity(btVector3(0, 0, engine->getWorld()->environment.gravity));
	world->setInternalTickCallback(&PhysicsSystem::globalCallback);

	/*
	btCollisionShape* groundShape = new btStaticPlaneShape(btVector3(0, 0, 1), 1.0f);
	btDefaultMotionState* groundMotionState = new btDefaultMotionState(btTransform(btQuaternion(0, 0, 0, 1), btVector3(0, 0, -2)));
	btRigidBody::btRigidBodyConstructionInfo groundRigidBodyCI(0, groundMotionState, groundShape, btVector3(0, 0, 0));
	btRigidBody* groundRigidBody = new btRigidBody(groundRigidBodyCI);

	physicSpec(groundRigidBody);
	*/
}

void PhysicsSystem::destroy() {

}

void PhysicsSystem::update(double dt) {
	for(int i=0; i<simulation_steps;i++)
		world->stepSimulation(btScalar((dt/simulation_steps) * ts), 0);
}

Ptr<IPhysics> PhysicsSystem::createCameraPhysics(bool physical, float3 position, float height) {
	if (!physical) return nullptr;
	auto phys = gcnew<Physics>(this);
	phys->initCamera(position, height);
	return phys;
}

float Physics::getMass() const {
	return mass;
}

void Physics::init(IObject3D* parent, float mass, ShapeInfo shapeInfo, PhysicsCallback cb) {
	body = 0;
	this->mass = mass;
	auto& rot = parent->getRotation();
	auto& pos = parent->getPos();
	enabled = true;
	btQuaternion rotation(rot.x, rot.y, rot.z, rot.w);
	btVector3 position(pos.x, pos.y, pos.z);
	btDefaultMotionState* fallMotionState = new btDefaultMotionState(btTransform(rotation, position));
	btVector3 fallInertia(0, 0, 0);
	if (shapeInfo.type != ePS_Generic && shapeInfo.type != ePS_ConvexHull) {
		btCollisionShape *shape = 0;
		if (shapeInfo.type == ePS_Sphere) {
			shape = new btSphereShape(shapeInfo.radius);
		} else if (shapeInfo.type == ePS_Box) {
			shape = new btBoxShape(btVector3(shapeInfo.x, shapeInfo.y, shapeInfo.z));
		}
		if(mass > 0.0f)
			shape->calculateLocalInertia((btScalar)mass, fallInertia);
		btRigidBody::btRigidBodyConstructionInfo fallRigidBodyCI((btScalar)mass, fallMotionState, shape, fallInertia);

		body = new btRigidBody(fallRigidBodyCI);
		body->setUserPointer(parent);
		body->setUserIndex2(0);
		body->setCcdMotionThreshold(0.1f);
		body->setRestitution(0.0f);
		body->setFriction(1.0f);
		
		bodies.push_back(body);
		motionStates.push_back(fallMotionState);

		if (cb) cb(body);
		system->world->addRigidBody(body);
		meshShapes.push_back(shape);
		if (mass <= 0.0f) enabled = false;
	} else if(shapeInfo.type == ePS_ConvexHull || shapeInfo.type == ePS_Generic) {
		auto& objects = parent->getDrawData()->getObjects();
		auto& indices = parent->getDrawData()->getIndices();
		auto& vertices = parent->getDrawData()->getVertices();
		int type = parent->getType();
		if (vertices.size()) {
			size_t last = 0;
			this->mass = mass = 0;

			btRigidBody* sceneRB = 0;
			int idx = 0;
			//for (auto& object : objects) {
			btTriangleMesh* sceneMesh = new btTriangleMesh;

			//cout << "Object start: " << object.start << ", offset: " << object.offset << endl;
			for (size_t i = 0, j = indices.size(); i < j; i += 3) {
				int index0 = indices[i];
				int index1 = indices[i + 1];
				int index2 = indices[i + 2];
				btVector3 vertex0(vertices[index0].pos.x, vertices[index0].pos.y, vertices[index0].pos.z);
				btVector3 vertex1(vertices[index1].pos.x, vertices[index1].pos.y, vertices[index1].pos.z);
				btVector3 vertex2(vertices[index2].pos.x, vertices[index2].pos.y, vertices[index2].pos.z);
				sceneMesh->addTriangle(vertex0, vertex1, vertex2);
			}

			btCollisionShape* shape = 0;
			if (shapeInfo.type == ePS_Generic) {
				btBvhTriangleMeshShape *solid = new btBvhTriangleMeshShape(sceneMesh, true, true);
				solid->buildOptimizedBvh();
				shape = solid;
			} else {
				btConvexHullShape* hull = new btConvexHullShape();
			}

			btRigidBody::btRigidBodyConstructionInfo srbci((btScalar)mass, fallMotionState, shape);
			sceneRB = new btRigidBody(srbci);
			sceneRB->setUserPointer(parent);
			sceneRB->setUserIndex2(idx++);

			motionStates.push_back(fallMotionState);
			meshes.push_back(sceneMesh);
			meshShapes.push_back(shape);
			bodies.push_back(sceneRB);
			system->world->addRigidBody(sceneRB);
			//}

			body = sceneRB;

			body->setRestitution(0.0f);
			body->setFriction(1.0f);

			if (cb) cb(sceneRB);
			if (mass <= 0.0f) enabled = false;
		} else enabled = false;
	}
}


Physics::~Physics() {
	release();
}

void Physics::release() {
	for (auto body : bodies) {
		system->world->removeRigidBody(body);
		delete body;
	}
	for (auto state : motionStates) {
		delete state;
	}
	for (auto mesh : meshes) {
		delete mesh;
	}
	for (auto meshShape : meshShapes) {
		delete meshShape;
	}
	bodies.clear();
	motionStates.clear();
	meshes.clear();
	meshShapes.clear();
	enabled = false;
}

void Physics::push(float3 dir, float power) {
	if (body && enabled) {
		body->activate(true);
		btVector3 force = btVector3(dir.x, dir.y, dir.z) * power;
		body->applyImpulse(force, btVector3(0.0f, 0.0f, 0.0f));
	}
}

IPhysics& Physics::setScale(float3 scale) {
	for (auto& shape : meshShapes) {
		shape->setLocalScaling(btVector3(scale.x, scale.y, scale.z));
	}
	for (auto& body : bodies) {
		system->world->updateSingleAabb(body);
	}
	return *this;
}

IPhysics& Physics::setGravity(float gravity) {
	for (auto body : bodies) {
		body->setGravity(btVector3(0.0f, 0.0f, gravity));
	}
	return *this;
}

IPhysics& Physics::setResistution(float rest) {
	for (auto body : bodies) {
		body->setRestitution(rest);
	}
	return *this;
}

IPhysics& Physics::setDamping(float lin_damping, float ang_damping) {
	for (auto body : bodies) {
		body->setDamping(lin_damping, ang_damping);
	}
	return *this;
}

IPhysics& Physics::setFriction(float friction) {
	for (auto body : bodies) {
		body->setFriction(friction);
	}
	return *this;
}

IPhysics& Physics::setHitFriction(float friction) {
	for (auto body : bodies) {
		body->setHitFraction(friction);
	}
	return *this;
}

IPhysics& Physics::setRollingFriction(float friction) {
	for (auto body : bodies) {
		body->setRollingFriction(friction);
	}
	return *this;
}

IPhysics& Physics::setSpinningFriction(float friction) {
	for (auto body : bodies) {
		body->setSpinningFriction(friction);
	}
	return *this;
}

IPhysics& Physics::setAnisotropicFriction(float3 friction, int mode) {
	for (auto body : bodies) {
		body->setAnisotropicFriction(btVector3(friction.x, friction.y, friction.z), mode);
	}
	return *this;
}

IPhysics& Physics::setAngularVelocity(float3 vel) {
	for (auto body : bodies) {
		body->setAngularVelocity(btVector3(vel.x, vel.y, vel.z));
	}
	return *this;
}

IPhysics& Physics::setAngularFactor(float af) {
	for (auto body : bodies) {
		body->setAngularFactor(af);
	}
	return *this;
}

IPhysics& Physics::setDeactivationTime(float dt) {
	for (auto body : bodies) {
		body->setDeactivationTime(dt);
	}
	return *this;
}

IPhysics& Physics::setCollisionCallback(CollisionCallback cb) {
	collisionCallback = cb;
	return *this;
}

CollisionCallback Physics::getCollisionCallback() const {
	return collisionCallback;
}

IPhysics& Physics::activate() {
	for (auto& body : bodies) {
		body->activate();
	}
	return *this;
}

void PhysicsSystem::globalCallback(btDynamicsWorld *dynamicsWorld, float timeStep) {
	int numManifolds = dynamicsWorld->getDispatcher()->getNumManifolds();
	for (int i = 0; i < numManifolds; i++) {
		btPersistentManifold *contactManifold = dynamicsWorld->getDispatcher()->getManifoldByIndexInternal(i);
		const btCollisionObject *objA = contactManifold->getBody0();
		const btCollisionObject *objB = contactManifold->getBody1();
		IObject3D *pObjA, *pObjB;
		if (objA && objB && (pObjA = (IObject3D*)objA->getUserPointer()) && (pObjB = (IObject3D*)objB->getUserPointer())) {
			int numContacts = contactManifold->getNumContacts();
			std::vector<CollisionContact> contacts(numContacts);
			for (int j = 0; j < numContacts; j++) {
				btManifoldPoint& pt = contactManifold->getContactPoint(j);
				const btVector3& ptA = pt.getPositionWorldOnA();
				const btVector3& ptB = pt.getPositionWorldOnB();
				const btVector3& normalOnB = pt.m_normalWorldOnB;
				CollisionContact contact;
				contact.pos[0] = float3(ptA.getX(), ptA.getY(), ptA.getZ());
				contact.pos[1] = float3(ptB.getX(), ptB.getY(), ptB.getZ());
				contact.normal = float3(normalOnB.getX(), normalOnB.getY(), normalOnB.getZ());
				contact.distance = pt.getDistance();
				contact.lifeTime = float(pt.getLifeTime());
				contact.impulse = pt.getAppliedImpulse();
				
				contacts.push_back(contact);
			}
			auto physA = pObjA->getPhysics();
			auto physB = pObjB->getPhysics();
			CollisionCallback cbA, cbB;
			if (physA && (cbA = physA->getCollisionCallback())) {
				cbA(CollisionInfo(
					pObjA->shared_from_this(),
					pObjB->shared_from_this(),
					contacts,
					0,
					timeStep));
			}
			if (physB && (cbB = physB->getCollisionCallback())) {
				cbB(CollisionInfo(
					pObjB->shared_from_this(),
					pObjA->shared_from_this(),
					contacts,
					1,
					timeStep));
			}
		}
	}
}

SerializeData Physics::serialize() {
	SerializeData data;
	data["mass"] = std::to_string(mass);
	return data;
}

void Physics::deserialize(const SerializeData& params) {

}

void Physics::initCamera(float3 position, float height) {
	btCapsuleShape *m_shape = 0;
	btRigidBody *cameraBody = 0;

	btScalar characterHeight = height;
	btScalar characterWidth = 0.25f;
	btScalar characterMass = 75.0f;

	btTransform startTransform;

	m_shape = new btCapsuleShape(characterWidth, characterHeight);
	btQuaternion rot;
	rot.setEuler(0.0f, PI / 2, 0.0f);
	startTransform.setOrigin(btVector3(position.x, position.y, position.z));
	startTransform.setRotation(rot);
	m_ghostObject = new btPairCachingGhostObject();
	m_ghostObject->setWorldTransform(startTransform);

	system->world->getBroadphase()->getOverlappingPairCache()->setInternalGhostPairCallback(new btGhostPairCallback());


	m_ghostObject->setCollisionShape(m_shape);
	m_ghostObject->setCollisionFlags(btCollisionObject::CF_CHARACTER_OBJECT);


	btScalar stepHeight = btScalar(system->engine->getWorld()->controls.stepHeight);
	btKinematicCharacterController *m_character = new btKinematicCharacterController(m_ghostObject, m_shape, stepHeight, btVector3(0.0f, 0.0f, 1.0f));
	m_character->setJumpSpeed(1.0f);
	m_character->setFallSpeed(55.0f);

	this->m_character = m_character;

	system->world->addCollisionObject(m_ghostObject, btBroadphaseProxy::KinematicFilter, btBroadphaseProxy::AllFilter);
	system->world->addAction(m_character);
}

float3 Physics::getPos() {
	if (body) {
		return Math::toVec3(body->getWorldTransform().getOrigin());
	}
	return float3(0.0f, 0.0f, 0.0f);
}

float4 Physics::getRotation() {
	if (body) {
		return Math::toVec4(body->getWorldTransform().getRotation());
	}
	return float4(0.0f, 0.0f, 0.0f, 1.0f);
}

void Physics::setPos(float3 newPos) {
	for (auto& body : bodies) {
		auto& wt = body->getWorldTransform();
		auto ms = body->getMotionState();
		wt.setOrigin(btVector3(newPos.x, newPos.y, newPos.z));
		body->setWorldTransform(wt); body->setCenterOfMassTransform(wt);
		if (ms) {
			ms->setWorldTransform(wt);
		}
	}
}

void Physics::setRotation(float4 rot) {
	for (auto& body : bodies) {
		auto& wt = body->getWorldTransform();
		auto ms = body->getMotionState();
		wt.setRotation(btQuaternion(rot.x, rot.y, rot.z, rot.w));
		body->setWorldTransform(wt); body->setCenterOfMassTransform(wt);
		if (ms) {
			ms->setWorldTransform(wt);
		}
	}
}

matrix Physics::getTransform() {
	float nums[16];
	body->getWorldTransform().getOpenGLMatrix(nums);
	return matrix(nums).transpose();
}

void Physics::setWalkDirection(float3 dir) {
	if (m_character) m_character->setWalkDirection(btVector3(dir.x, dir.y, dir.z));
}

void Physics::jump(float3 dir) {
	if(m_character) m_character->jump(btVector3(dir.x, dir.y, dir.z));
}

float3 Physics::getGhostPos() {
	if (m_ghostObject) return Math::toVec3(m_ghostObject->getWorldTransform().getOrigin());
	return float3(0.0f, 0.0f, 0.0f);
}

void Physics::setGhostPos(float3 pos) {
	if (m_ghostObject) m_ghostObject->getWorldTransform().setOrigin(btVector3(pos.x, pos.y, pos.z));
}

bool Physics::onGround() {
	if (m_character) return m_character->onGround();
	return false;
}


extern "C" {
	__declspec(dllexport) IPhysicsSystem* CreatePhysicsSystem(IEngine *pEngine) {
		return new PhysicsSystem(pEngine);
	}
}