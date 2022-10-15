#define _CRT_SECURE_NO_WARNINGS
#include "../Lamegine/Interfaces.h"
#include "bmplib.h"

#define ENEMY 107
#define ENEMY_BULLET 108
#define MYSELF 109
#define MY_BULLET 110
#define WALL 106

class CGame : public IGame {
public:
	IEngine *pEngine;
	std::shared_ptr<IObject3D> player;

	int lastKey;
	float3 start, end;
	std::vector<float3> checkpoints;


	bool finished = false;
	float3 mode;
	float totalImpulse = 0.0f;

	class Camera : public ICamera {
	public:
		CGame *parent;
		float3 rot;
		float4 rotation;
		Camera(CGame *game) : parent(game) {
			rotation = XMQuaternionIdentity();
		}
		virtual SerializeData serialize() {
			return SerializeData();
		}
		virtual void deserialize(const SerializeData& params) {

		}
		virtual Math::Vec3 getPos() {
			float3 pos = parent->player->getPos() +float3(0.0f, 0.0f, 4.0f);
			return pos;
		}
		virtual Math::Vec3 getDir() {
			return float3(0.0f, +0.001f, -1.0f).normalize();
		}
		virtual void setPos(Math::Vec3 pos) {

		}
		virtual void setDir(Math::Vec3 dir) {

		}
		virtual void move(float zAngle, float speed, double dt, bool backwards = false) {

		}
		virtual void stopMoving() {

		}
		virtual void jump(double dt) {

		}
		virtual void rotate(float left, float up, const float sens = 0.25) {

		}
		virtual void initPhysics() {

		}
		virtual void update(double dt) {

		}
		virtual bool isPhysical() {
			return false;
		}
		virtual void enablePhysics(bool enable) {

		}
	};

	Ptr<Camera> pCamera;

	struct Enemy {
		Enemy(std::shared_ptr<IObject3D> h) : handle(h) {}
		std::shared_ptr<IObject3D> handle;
		std::shared_ptr<IObject3D> operator->() {
			return handle;
		}
		float getSpeed() {
			return health / 100.0f;
		}
		IObject3D* get() { return handle.get(); }
		float3 lastSeen;
		double lastShot = 0.0;
		bool sawEnemy = false;
		bool seesEnemy = false;
		float lastDistance = 1.0f;
		float integral = 0.0f;
		int health = 100;
	};

	std::vector<std::shared_ptr<Enemy>> enemies;

	int keyStates[256] = { 0 };

	virtual bool overrideLoader() {
		return true;
	}

	float3 playerDir;

	virtual void init(IEngine *pEngine) {
		if (!pEngine->checkCompatibility(LAMEGINE_SDK_VERSION)) {
			char msg[100];
			int v = pEngine->getVersion();
#define MAJ(v)  (((v & 0xFFFF0000)>>16))
#define MID1(v) (((v & 0x0000F000)>>12))
#define MID2(v) (((v & 0x00000F00)>>8))
#define MIN(v)  ((v & 0xFF))
			sprintf(msg, "You are running incompatibile version of engine\n Mod SDK version: %d.%d.%d.%d (%08X)\n Engine SDK version: %d.%d.%d.%d (%08X)",
				MAJ(LAMEGINE_SDK_VERSION), MID1(LAMEGINE_SDK_VERSION), MID2(LAMEGINE_SDK_VERSION),	MIN(LAMEGINE_SDK_VERSION), LAMEGINE_SDK_VERSION,
				MAJ(v), MID1(v), MID2(v), MIN(v), v
			);
			pEngine->shutdown(msg);
			return;
		}

		pCamera = gcnew<Camera>(this);
		pEngine->setCamera(pCamera);

		srand((unsigned int)time(0));
		this->pEngine = pEngine;
		
		IWorld *pWorld = pEngine->getWorld();
		
		pWorld->constants.sunColor = float4(0.1f, 0.105f, 0.15f, 1.0f);

		pWorld->settings.shadowCascades[1] = 0.25f;
		pWorld->settings.shadowCascades[2] = 0.125f;
		pWorld->settings.shadowCascades[3] = 0.125f;
		
		bmp mapka;
		bmp_read(&mapka, "maze/map.bmp");
		ModelSpawnParams boxParams;
		boxParams.path = "maze/ground.lmm";
		auto bgObj = pEngine->getClass("Model3D", &boxParams);
		bgObj->
			setPos(float3(0.0f, 0.0f, -513.0f))
			.setScale(float3(512.0f, 512.0f, 512.0f))
			.buildPhysics(0.0f, ShapeInfo(ePS_Box, 512.0f, 512.0f, 512.0f))
			.add();
		boxParams.path = "maze/box.lmm";

		if (mapka.data) {
			for (int y = 0; y < mapka.h; y++) {
				for (int x = 0; x < mapka.w; x++) {
					int color = bmp_get(&mapka, x, y);
					float X = float(x) - mapka.w / 2.0f;
					float Y = float(y) - mapka.h / 2.0f;
					float3 pos(2.0f * X, 2.0f * Y, 0.0f);

					if (!color || color == 0x0000FF) {
						auto obj = pEngine->getClass("Model3D", &boxParams);
						obj->setPos(pos).buildPhysics(color == 0x0000FF ? 1.0f : 0.0f, ShapeInfo(ePS_Box, 1.0f, 1.0f, 1.0f)).add();
						if (color == 0x0000FF) {
							obj->setUsePigment(true);
							obj->setPigment(float4(10.0f, 0.0f, 5.765f, 1.0f));
							obj->getPhysics()->setFriction(0.4f);
							pEngine->getLightSystem()->addLight(pEngine->getLightSystem()->createDynamicLight(Light(float3(1.0f, 0.0f, 0.5765f), float3(0.0f, 0.0f, 1.3f), 15.0f), nullptr, obj.get(), float3(0.0f, 0.0f, 2.0f)));

						} else {
							obj->getPhysics()->setResistution(1.0f);
							obj->setUsePigment(false);
							obj->setPigment(float4(0.9f, 0.9f, 0.9f, 1.0f));
						}
						//obj->getPhysics()->setResistution(1.0f);
					} else {
						if (color == 0x00FF00) {
							start = pos;
						} else if (color == 0xFF0000) {
							end = pos;
							auto endObj = pEngine->getClass("Model3D", &boxParams);
							endObj->setPos(pos).setVisible(true).add();
						} else if (color == 0xFFFF00) {
							checkpoints.push_back(pos);
						} else {
							if (rand() % 33 == 11) {
								ModelSpawnParams enemyParams;
								enemyParams.path = "maze/sphere.lmm";
								auto handle = pEngine->getClass("Model3D", &enemyParams);
								const Ptr<Enemy> enemy = gcnew<Enemy>(handle);
								handle->setPos(pos).setScale(float3(0.4f, 0.4f, 0.4f));
								handle->buildPhysics(5.0f, ShapeInfo(ePS_Sphere, 0.4f), nullptr);
								handle->setType(ENEMY);
								handle->setUsePigment(true).setPigment(float4(1.0f, 0.1f, 0.1f, 1.0f));
								handle->add();
								handle->setParent(enemy.get());
								auto dynLight = pEngine->getLightSystem()->createDynamicLight(Light(float3(1.0f, 0.0f, 0.0f), float3(0.0f, 0.0f, 0.0f), 10.0f), [=](Light l) -> Light {
									float gameTime = (float)pEngine->getGameTime();
									l.intensity += sin(2 * PI * 1.0f * 0.1f * gameTime);
									if (enemy->health <= 0) {
										l.color = float3(1.0f, 0.1f, 0.1f);
									} else {
										if (enemy->seesEnemy) {
											l.color = float3(0.09f, 0.1f, 0.09f) * getHealthColor(enemy->health);
										}
										else l.color = float3(0.1f, 0.1f, 0.8f);
									}
									l.color.y += abs(0.3f * cos(2 * PI * 1.0f * 0.1f * gameTime));
									return l;
								}, handle.get());
								pEngine->getLightSystem()->addLight(dynLight);
								enemy->health = 100 + rand() % 200;
								enemies.push_back(enemy);
							}
						}
					}
				}
			}
		}

		ModelSpawnParams spawnParams;
		spawnParams.path = "maze/sphere.lmm";
		player = pEngine->getClass("Model3D", &spawnParams);
		player->setPos(start + float3(0.0f, 0.0f, 0.5f)).setScale(float3(0.707f, 0.707f, 0.707f));
		player->buildPhysics(10.0f, ShapeInfo(ePS_Sphere, 0.707f), nullptr);
		player->add();

		/*
		auto dynLight = pEngine->getLightSystem()->createDynamicLight(Light(float3(1.0f, 0.6f, 0.1f), float3(0.0f, 0.0f, 0.0f), 10.0f), [=](Light l) -> Light {
			l.position = player->getPos() + float3(0.0f, 0.0f, 1.4f);
			return l;
		});

		pEngine->getLightSystem()->addLight(dynLight);
		*/
	}

	bool vis = true;

	virtual void onUpdate(double dt) {
		static clock_t lastSound = clock();
		static float3 lastPos = player->getPos();
		//char msg[80];

		char msg[50];
		sprintf(msg, "impulse total: %f\n", totalImpulse);
		pEngine->drawText(10.0f, 100.0f, msg);

		float3 moveDir;
		bool move = false;
		if (keyStates[VK_UP] || keyStates['W']) {
			moveDir += float3(0.0f, 1.0f, 0.0f);
			move = true;
		}
		if (keyStates[VK_DOWN] || keyStates['S']) {
			moveDir += float3(0.0f, -1.0f, 0.0f);
			move = true;
		}
		if (keyStates[VK_LEFT] || keyStates['A']) {
			moveDir += float3(-1.0f, 0.0f, 0.0f);
			move = true;
		}
		if (keyStates[VK_RIGHT] || keyStates['D']) {
			moveDir += float3(1.0f, 0.0f, 0.0f);
			move = true;
		}
		moveDir.z = -0.5f;
		if (move){
			moveDir = moveDir.normalize();
			player->getPhysics()->push(moveDir, float(100.0f * dt));
		}

		std::vector<std::shared_ptr<Enemy>> dead;

		// najbrutanejsie AI sveta
		for (auto parent : enemies) {
			auto enemy = parent->handle;
			// ideme zistit ci nepriatel moze vidiet hraca
			RayTraceParams trace;
			trace.is3d = true;
			trace.sourcePos = enemy->getPos();
			trace.except = enemy.get();
			trace.viewDir = (player->getPos() - enemy->getPos()).normalize();
			auto res = pEngine->raytraceObject(trace);
			if (parent->health < 0) parent->health = 0;
			enemy->setPigment(getHealthColor(parent->health));
			/*if (parent->health <= 0) {
				dead.push_back(parent);
				continue;
			}*/
			// ak nepriatel vidi hraca, nech ho sleduje
			if (res == player.get() && vis) {
				parent->sawEnemy = true;
				parent->seesEnemy = true;
				enemy->getPhysics()->push(trace.viewDir, 1.0f * parent->getSpeed());
				parent->lastSeen = player->getPos();
				float dst = player->getPos().dist(enemy->getPos());
				// ak je hrac dalej ako 2m, nech po nom nepriatel vystreli
				if (dst > 2.0f && parent->health > 0) {
					double t = pEngine->getGameTime();
					// ak cas od poslednej strely > 0.2s (cize max 5 vystrelov/s), strelime v smere hraca
					if (t - parent->lastShot > 0.2f) {
						ModelSpawnParams spawnParams;
						spawnParams.path = "maze/sphere.lmm";
						auto shot = pEngine->getClass("Model3D", &spawnParams);
						shot->setPos(enemy->getPos() + trace.viewDir*float3(1.0f, 1.0f, 0.0f) + float3(0.0f, 0.0f, 0.2f)).setScale(float3(0.1f, 0.1f, 0.1f));
						shot->setUsePigment(true).setPigment(float4(10.0f, 5.0f, 0.0f, 1.0f));
						shot->buildPhysics(0.2f, ShapeInfo(ePS_Sphere, 0.1f), nullptr);
						shot->add();
						shot->setLifeTime(5.0f);
						shot->getPhysics()->push(trace.viewDir, 1.0f);
						shot->getPhysics()->setResistution(1.0f);
						shot->setParent(enemy.get());
						parent->lastShot = t;
					}
				}
			} else {
				float dst = parent->lastSeen.dist(enemy->getPos());
				parent->seesEnemy = false;
				// ak sme hraca videli niekedy v minulosti ale uz nevidime, snazime sa dostat na miesto kde sme ho naposledy videli
				if (dst > 0.0f && parent->sawEnemy) {
					float df = dst - parent->lastDistance;
					float P = 1.0f, D = 0.1f, I = 0.0001f;
					enemy->getPhysics()->push((parent->lastSeen - enemy->getPos()).normalize(), parent->getSpeed() * (P * dst + D * df + I * parent->integral));
					parent->lastDistance = dst;
					parent->integral += df;
				}
			}
		}

		// odstranime mrtve AI
		for (auto d : dead) {
			for (auto it = enemies.begin(); it != enemies.end(); it++) {
				if(*it == d) {
					enemies.erase(it);
					break;
				}
			}
			pEngine->removeObject(d->handle.get());
		}

		playerDir = (player->getPos() - lastPos).normalize();
		lastPos = player->getPos();
		//pEngine->getWindow()->HideCursor();
	}

	virtual void onKeyPressed(int keyCode) {
		if (keyCode == VK_SPACE) {
			player->getPhysics()->push(float3(0.0f, 0.0f, 1.0f), 35.0f);
		} else if (keyCode == 'V') {
			vis = !vis;
		} else if (keyCode == VK_F3) {
			pEngine->generateCubemap();
		}
		if (keyCode == 'X') mode = float3(1.0f, 0.0f, 0.0f);
		if (keyCode == 'Y') mode = float3(0.0f, 1.0f, 0.0f);
		if (keyCode == 'Z') mode = float3(0.0f, 0.0f, 1.0f);
		if (keyCode == 'S') shoot();
	}

	virtual void onKeyUp(int keyCode) {
		keyStates[keyCode] = 0;
		lastKey = 0;
	}

	virtual void onKeyDown(int keyCode) {
		keyStates[keyCode] = 1;
		lastKey = keyCode;
	}

	virtual void onMousePressed(int mouseBtn) {
		
	}

	virtual void onMouseUp(int mouseBtn) {

	}

	virtual void onMouseDown(int mouseBtn) {
		shoot();
	}

	float3 getHealthColor(int health) {
		if (health > 200) {
			return float3(10.0f, 10.0f, (health - 200.0f) / 10.0f);
		}
		if (health > 100) {
			return float3(10.0f, (health - 100.0f) / 10.0f, 0.0f);
		}
		return float3((health) / 10.0f, 0.0f, 0.0f);
	}

	void shoot() {
		double cx, cy;
		pEngine->getWindow()->GetCursorPos(&cx, &cy);
		RayTraceParams params;
		params.mouseX = float(cx);
		params.mouseY = float(cy);
		params.center = false;
		if (pEngine->raytraceObject(params)) {
			ModelSpawnParams spawnParams;
			spawnParams.path = "maze/sphere.lmm";
			auto shot = pEngine->getClass("Model3D", &spawnParams);

			float3 dir = params.hitPoint - player->getPos();
			//dir.z = 0.0f;
			dir = dir.normalize();

			shot->setPos(player->getPos() + dir * 0.5f).setScale(float3(0.1f, 0.1f, 0.1f));
			shot->setUsePigment(true).setPigment(float4(0.0f, 10.0f, 20.0f, 1.0f));
			shot->buildPhysics(0.2f, ShapeInfo(ePS_Sphere, 0.1f), nullptr);
			shot->setLifeTime(5.0f);
			shot->getPhysics()->push(dir, 2.0f);
			shot->getPhysics()->setResistution(1.0f);
			shot->setParent(player.get());
			shot->getPhysics()->setCollisionCallback([=](const CollisionInfo& ci) -> void {
				if (ci.object && ci.object->getType() == ENEMY && ci.object.get() != shot->getParent() && ci.contacts.size()) {
					void* par = ci.object->getParent();
					if (par) {
						//totalImpulse += 1.0f;
						Enemy *e = (Enemy*)par;
						e->health -= 2;
					}
				}
			});
			shot->add();
		}
	}

	virtual bool isActorGhost() {
		return true;
	}

	virtual bool overrideCamera() {
		return false;
	}

	virtual bool overrideKeyboard() {
		return true;
	}

	virtual bool overrideMouse() {
		return false;
	}

	virtual bool getCameraPos(float3& pos) {
		return false;
	}

	virtual bool getCameraDir(float3& dir) {
		return false;
	}

	virtual const char *getName() {
		return "Dungeon master";
	}

	virtual void release() {

	}

	virtual void onScroll(int dir) {
		pCamera->rot += mode * (float)dir / 20.0f;
	}

	virtual bool overrideMovement() {
		return false;
	}

	virtual void onMessage(const Message& msg) {
		
	}
};

extern "C" {
	__declspec(dllexport) IGame* CreateGame() {
		return new CGame();
	}
}