#include "shared.h"
#include "Engine.h"
#include "Camera.h"
#include "World.h"
#include "VectorMathExt.h"
#include "ScriptSystem.h"
#include "Timer.h"
#include "SceneLoader.h"
#include "GameLogics.h"
#include <btBulletDynamicsCommon.h>
#include <thread>
#include "utils/bmp.h"

#if defined(_MSC_VER)
extern "C" {
	__declspec(dllexport) DWORD NvOptimusEnablement = 1;
	__declspec(dllexport) DWORD AmdPowerXpressRequestHighPerformance = 1;
}
#endif

void fixWindow() {
	HMODULE shcoreLib = LoadLibraryW(L"shcore.dll");
	if (shcoreLib) 
	{
		enum DPIAwareness
		{
			DPI_UNAWARE = 0,
			SYSTEM_DPI_AWARE = 1,
			PER_MONITOR_DPI_AWARE = 2
		};
		typedef HRESULT(__stdcall *f_SetDPIAware)(DPIAwareness);
		f_SetDPIAware SetDPIAware = (f_SetDPIAware)GetProcAddress(shcoreLib, "SetProcessDpiAwareness");
		if (SetDPIAware) SetDPIAware(PER_MONITOR_DPI_AWARE);
		FreeLibrary(shcoreLib);
	}
}

Ptr<Pass> carPass;

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR     lpCmdLine, int       nShowCmd) {

	int argc = __argc;
	const char **argv = (const char **)__argv;

	CoInitializeEx(nullptr, COINIT_MULTITHREADED);
	fixWindow();

	World* pWorld = World::getInstance();

	auto physicSpec = [=](btRigidBody* body) -> void {
		body->setFriction(0.5f);
		body->setRollingFriction(0.5f);
		body->setSpinningFriction(0.5f);
	};

	unsigned int terrainMask = -1;
	bool music = false;
	bool noTerrain = false;

	pWorld->environment.gravity = -9.81f;
	pWorld->environment.waterLevel = 128.0f;

	pWorld->controls.jumpHeight = 5.0f;
	pWorld->controls.stepHeight = 0.55f;
	
	pWorld->settings.tessellate = false;
	pWorld->settings.renderFov = 75.0f;
	pWorld->settings.renderQuality = 1.0f;
	pWorld->settings.reflectionQuality = 0.0625f;
	pWorld->settings.shadowQuality = 1.0f;
	pWorld->settings.wireFrame = false;
	pWorld->settings.terrainScale = 4096.0f;
	pWorld->settings.renderDistance = 0.65f;
	pWorld->settings.refractionQuality = 0.25f;
	pWorld->settings.ssrQuality = 0.0f;
	pWorld->settings.vsync = true;
	pWorld->settings.editor = true;
	pWorld->settings.depthOfField = false;
	pWorld->settings.convertArg = 0;
	pWorld->settings.width = 1280.0f;
	pWorld->settings.height = 720.0f;
	pWorld->settings.fullScreen = false;
	pWorld->settings.solidCulling = false;
	pWorld->settings.msaa = 0;
	pWorld->settings.anisotropy = 4;
	pWorld->settings.cubemapQuality = 128;

	pWorld->constants.exposure = 6.85f;
	pWorld->constants.gamma = 0.95f;
	pWorld->constants.bloomStrength = 0.10f;
	pWorld->constants.shadowStrength = 0.10f;
	pWorld->constants.skyColor = SKY_COLOR;
	pWorld->constants.fogSettings = float4(SKY_COLOR, 1.0f / 256.0f);
	pWorld->constants.fogSettings.w = 1.0f / 256.0f;
	pWorld->constants.sunColor = SUN_COLOR;
	pWorld->constants.distortionStrength = 0.0f;
	pWorld->constants.hdrSettings.x = 0.0f;
	pWorld->constants.hdrSettings.y = 1.0f;

	float fpsGoal = 60.0f;

	float& renderDistance = pWorld->settings.renderDistance;

	const char *terrainPath = "engine/textures/terrain.bmp";
	const char *cgPath = "engine/grading/palette.bmp";
	const char *modelPath = "engine/sponza/sponza.lmm";
	const char *radiosity = "engine/radiosity/default.dds";

	bool debugging = false;
	bool expectAny = false;

	renderDistance = pWorld->settings.terrainScale * renderDistance;

	float& rq = pWorld->settings.renderQuality;
	float& sq = pWorld->settings.shadowQuality;
	
	const int DIVISOR = CLOCKS_PER_SEC / 1000;

	Engine *pEngine = new Engine();

	std::map<std::string, std::string> settings;

	for (int i = 1; i < argc; i++) {
		if (argv[i][0] == '-' && (i + 1) < argc) {
			settings[argv[i] + 1] = argv[i + 1];
			i++;
			continue;
		}
	}

	pWorld->load("settings.cfg");
	pWorld->load(settings);

	auto it = pWorld->params.find("mdl");
	if (it != pWorld->params.end()) modelPath = it->second.c_str();
	it = pWorld->params.find("cg");
	if (it != pWorld->params.end()) cgPath = it->second.c_str();
	it = pWorld->params.find("fps");
	if (it != pWorld->params.end()) fpsGoal = (float)atof(it->second.c_str());

	if (pWorld->settings.convertArg) {
		pEngine->silentInit(pWorld);
		printf("beginning conversion\n");
		for (int i = 1; i < argc; i++) {
			if (argv[i][0] == '-') continue;
			char savePath[255];
			strcpy(savePath, argv[i]);
			strcpy(strstr(savePath, ".obj"), ".lmm");
			printf("converting %s to %s\n", argv[i], savePath); fflush(stdout);
			ModelSpawnParams params;
			params.path = argv[i];
			auto mdl = pEngine->getClass("Model3D", &params);
			mdl->exportObject(savePath, "");
			//delete mdl;
		}
		return 0;
	}

	cout << "Settings:" << endl
		<< " Render Quality: " << pWorld->settings.renderQuality << endl
		<< " Render FOV: " << pWorld->settings.renderFov << endl
		<< " Wireframe: " << (pWorld->settings.wireFrame ? "on" : "off") << endl
		<< " VSync: " << (pWorld->settings.vsync ? "on" : "off") << endl
		<< " Max lights: " << (NUM_LIGHTS) << endl
		<< " Shadow Quality: " << pWorld->settings.shadowQuality << endl
		<< " Resolution: " << pWorld->settings.width << " x " << pWorld->settings.height << " (final: " << (pWorld->settings.width * rq) << " x " << (pWorld->settings.height * rq) << ")" << endl
		<< " Shadowmap Resolution: " << (1024 * sq) << " x " << (1024 * sq) << endl << endl;

	bool silent = pWorld->params.find("silent") != pWorld->params.end();

	IWindow* window = pEngine->init(0, pWorld->settings.width, pWorld->settings.height, pWorld->settings.fullScreen, "Lamegine", pWorld, silent);

	it = pWorld->params.find("pass");
	if (it != pWorld->params.end()) pEngine->displayPass(it->second.c_str());

	IRenderer* pRenderer = pEngine->getRenderer().get();

	auto ss = pEngine->getScriptSystem();
	ss->loadFile("scripts/main.lua");

	
	pEngine->setCamera(gcnew<Camera>(pEngine, 0.0f, 0.0f, 4.0f, !pWorld->settings.editor));
	
	float& terrainScale = pWorld->settings.terrainScale;

	Vec3 LightPos(0.0f, 0.0f, 868.0f);
	Vec3 LightDir(-0.3f, -0.3f, -0.3f);

	pWorld->environment.lightPos = LightPos;
	pWorld->constants.sunDir = LightDir;

	float scale = 1.0f;

	Mat4 view;
	Mat4 proj = Math::perspective(Math::radians(pWorld->settings.renderFov), 1280.0f / 720.0f, Z_NEAR, Z_FAR);
	
	pEngine->createPass(1.0f, &pWorld->mainPass.view, &pWorld->mainPass.proj);

	pEngine->setLUT(cgPath);
	pEngine->loadRadiosityMap(radiosity, Vec4(64.0f, 64.0f, 16.0f, 4.0f));

	bool move = false;
	double cx = 0, cy = 0, lcx = 0, lcy = 0;
	Mat4 rot(1.0f);
	Vec3 movedir(0.0f, 0.0f, 0.0f);
	bool eventHappened = false;
	float speed = 0.1f;
	double dt = 0.0f;
	float lastEvent = 0.0f;
	float usage = 0.0f;

	
	IGame *pGame = 0;
	bool overrideLoader;
	float overallQuality = 1.0f;

	if (pWorld->params["game"].length()) {
		typedef IGame*(__cdecl *PFNCREATEGAME)();
		PFNCREATEGAME startup = 0;
		HMODULE hLib = LoadLibraryA(pWorld->params["game"].c_str());
		if (hLib) {
			startup = (PFNCREATEGAME)GetProcAddress(hLib, "CreateGame");
			if (startup) {
				pGame = startup();
			}
		}
	} else {
		pGame = new GameMain();
	}

	if (pGame) {
		pEngine->setGame(pGame);
		overrideLoader = pGame->overrideLoader();
	}

	if (!overrideLoader) {
		if (pWorld->params.find("scene") != pWorld->params.end()) {
			Scene *pScene = new Scene(pWorld->params["scene"].c_str());
			pScene->load();
			delete pScene;
		}
	}

	float lastFps = (float)pEngine->getGameTime();

	Vec3 scaleVector(1.0f);

	pEngine->initObjects();

	pWorld->mainPass.proj = proj;
	pWorld->mainPass.view = view;

	float turnAngle = 0.0f;
	double lastDelta = 0.0f;
	float avgFrameRate = 1.0f/60.0f, cummulativeFrameRate = 0.0f;
	int ctr = 1;

	bool overrideCam = pGame && pGame->overrideCamera();
	
	pEngine->getCamera()->enablePhysics(!overrideCam && !pWorld->settings.editor);
	pEngine->getCamera()->initPhysics();

	HMODULE controller = 0;

	Vec3 lastPos = pEngine->getCamera()->getPos();

	auto pSoundSystem = pEngine->getSoundSystem();

	float originalExposure = pWorld->constants.exposure, originalShadow = pWorld->constants.shadowStrength;

	float csz = 10.0f;
	pEngine->setReflectionMapBbox(float3(-csz, -csz, 0.0f), float3(csz, csz, csz * 2));

	float3 bboxPoints[3];
	int bboxId = 0;

	float4 *radiosityMap = new float4[1024 * 1024];
	for (int i = 0; i < 1024 * 1024; i++) {
		radiosityMap[i] = float4(0.0f, 0.0f, 0.0f, 0.0f);
	}

	while (!window->ShouldClose()) {
		double start = (double)pEngine->getGameTime();
		if (window->Poll()) continue;
		
		float zAngle = 0.0f;
		float vehSpeed = 0.0f;

		eventHappened = false;

		speed = 8.0f;
		move = false;
		if (window->IsKeyPressed(VK_ESCAPE))
			window->Close();
		window->GetCursorPos(&cx, &cy);

		double frameDelta = dt;
		
		if (!pGame || !pGame->overrideMovement()) {

			if ((cx != lcx || cy != lcy) && window->IsActive()) {
				if (!pWorld->settings.editor || window->IsMouseDown(MOUSE_RIGHT)) {
					float movRight = (float)(cx - lcx);
					float movDown = (float)(-(cy - lcy));
					float sens = 37.5f;
					pEngine->getCamera()->rotate(movRight * sens * (float)frameDelta, movDown * sens * (float)frameDelta);
					eventHappened = true;
					cx = pWorld->settings.width / 2; cy = pWorld->settings.height / 2;
					window->SetCursorPos(cx, cy);
					window->HideCursor();
				} else window->ShowCursor();
			}

			if (window->IsMovementKeys(zAngle)) {
				move = true;
				eventHappened = true;
			}

			if (!pGame || !pGame->overrideKeyboard()) {
				if (window->IsKeyPressed(VK_F3)) {
					//cubemapStep = 1;
					pEngine->generateCubemap();
				}
				if (window->IsKeyPressed(VK_F4)) {
					RayTraceParams params;
					IObject3D* pHit = pEngine->raytraceObject(params);
					if (pHit) {
						bboxPoints[bboxId++] = params.hitPoint;
						if (bboxId == 3) {
							auto lst = pEngine->getActiveReflectionMap();
							float3 center = lst->position;
							float3 bboxA = bboxPoints[0] - center;
							bboxPoints[1].z = bboxPoints[2].z;
							float3 bboxB = bboxPoints[1] - center;
							lst->bbox1 = bboxA;
							lst->bbox2 = bboxB;
							lst->position = center;
							bboxId = 0;
						}
					}
				}
				if (window->IsKeyPressed(VK_F5)) {
					pEngine->setReflectionMapPosition(pEngine->getCameraPos());
				}
			}
			
			if (window->IsKeyDown(VK_SHIFT)) speed *= 4;
			if (window->IsKeyDown(VK_CONTROL)) speed *= 0.25;
			if (window->IsKeyPressed(VK_SPACE)) pEngine->getCamera()->jump(dt);
		}

		if (!pGame || !pGame->overrideMovement()) {
			if (move) {
				pEngine->getCamera()->move(zAngle, speed, frameDelta, window->IsKeyDown('S'));
			} else {
				pEngine->getCamera()->stopMoving();
			}
			pEngine->getCamera()->update(frameDelta);
		}

		pWorld->delta = frameDelta;
		pEngine->onUpdate(frameDelta);
		ss->call("OnUpdate", (float)frameDelta);

		char titleMsg[255];
		double FPS = 1.0 / frameDelta;
		Vec3 pos = pEngine->getCamera()->getPos();
		double cpuTime = pEngine->getCPUTime();
		double gpuTime = pEngine->getGPUTime();
		double ft = cpuTime + gpuTime;
		double ratio = cpuTime / ft;
		sprintf(titleMsg, "$FFFF00 Game: %s\n$%06X FPS: %6.2f (quality: %5.2f %%, cpu/gpu/ratio: %5.2f/%5.2f/%.2f, tris/dc/maps: %7d/%5d/%d )\n$00FF00 Position: %.3f %.3f %.3f",
			!pGame ? "Default" : pGame->getName(),
			FPS >= 50.0f ? 0x00FF00 :
			FPS >= 28.0f ? 0xFFFF00 :
			FPS >= 14.0f ? 0xFF7F00 :
			0xFF0000,
			FPS, pEngine->getGlobalQuality() * 100.0f,
			cpuTime * 1000,
			gpuTime * 1000,
			ratio,
			pEngine->getTris(),
			pEngine->getDrawCallsCount(),
			pEngine->getMaps(),
			pos.x, pos.y, pos.z);
		pEngine->drawText(10.0f, 10.0f, titleMsg);
		float dFPS = float(FPS - (fpsGoal - 5.0f));
		overallQuality += float(dFPS * frameDelta * frameDelta);
		if (overallQuality > 1.0f) overallQuality = 1.0f;
		if (overallQuality < 0.5f) overallQuality = 0.5f;
		if (isnan(overallQuality) || isinf(overallQuality)) overallQuality = 1.0f;

		Ptr<CubeMap> closest = pEngine->getClosestReflectionMap(pEngine->getCameraPos());
		if (closest) pEngine->setReflectionMap(closest);
		pEngine->setGlobalQuality( round(overallQuality * 10.0f) / 10.0f );
		ctr++;

		lcx = cx;
		lcy = cy;
		lastPos = pEngine->getCamera()->getPos();
		double t = (double)pEngine->getGameTime();
		lastDelta = dt;
		dt = (t - start);
	}
	
	delete pEngine;

	CoUninitialize();

	return 0;
}
