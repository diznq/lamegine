#include "shared.h"

#include "VectorMathExt.h"
#include "Engine.h"
#include "DrawData.h"
#include "ScriptSystem.h"
#include "Light.h"
#include "Window.h"
#include "World.h"
#include <unordered_map>
#include <set>
#include "Server.h"

#define PBR_DEPTH 16

std::string Engine::regConstants = "";
unsigned long long Engine::perfFreq = 0;
Engine* Engine::instance = 0;
bool Engine::destroyed = false;


void Engine::worker(Engine* pEngine) {
	while (true) {
		TaskBase* task;
		pEngine->tasks.wait_dequeue(task);
		if (task) {
			task->exec();
			pEngine->finishedTasks.enqueue(task);
		}
	}
}

Engine::Engine() {
	instance = this;
}

Engine::~Engine() {
	if (instance == this) {
		instance = 0;
		destroyed = true;
	}
	destroy();
}

void Engine::Terminate(const wchar_t *msg) {
	IWindow *wnd = 0;
	Engine *inst = Engine::getInstance();
	if (inst) wnd = inst->window;
	MessageBoxW(wnd ? (HWND)wnd->GetHandle() : 0, msg, L"Lamegine", MB_OK | MB_ICONERROR);
	if (wnd) wnd->Close();
}

void Engine::Terminate(const char *msg) {
	IWindow *wnd = 0;
	Engine *inst = Engine::getInstance();
	if (inst) wnd = inst->window;
	MessageBoxA(wnd ? (HWND)wnd->GetHandle() : 0, msg, "Lamegine", MB_OK | MB_ICONERROR);
	if (wnd) wnd->Close();
}

void Engine::silentInit(IWorld *world) {
	drawDataManager = gcnew<DrawDataManager>();
	scriptSystem = gcnew<ScriptSystem>();
	lightSystem = gcnew<LightSystem>();
	forcedPass = "";
	pWorld = (World*)world;

	HMODULE hRenderer = LoadLibraryA("Renderer.dll");
	if (!hRenderer) {
		Terminate("Couldnt load Renderer.dll");
		return;
	}
	typedef IRenderer*(__cdecl *PFNCREATERENDERER)(IEngine *);
	PFNCREATERENDERER pfnCreateRenderer = (PFNCREATERENDERER)GetProcAddress(hRenderer, "CreateRenderer");
	if (!pfnCreateRenderer) {
		Terminate("Coulnt find CreateRenderer");
		return;
	}
	setRenderer(Ptr<IRenderer>(pfnCreateRenderer(this)));

	renderer->setVsync(pWorld->settings.vsync);
	if (!(renderer->createDeviceResources())) {
		Engine::Terminate(L"Failed to create device resources");
	}

	HMODULE hSoundSys = LoadLibraryA("SoundSystem.dll");
	if (!hSoundSys) {
		Terminate("Couldnt load SoundSystem.dll");
		return;
	}
	typedef ISoundSystem*(__cdecl *PFNCREATESOUNDSYSTEM)(IEngine *);
	PFNCREATESOUNDSYSTEM pfnCreateSoundSystem = (PFNCREATESOUNDSYSTEM)GetProcAddress(hSoundSys, "CreateSoundSystem");
	if (!pfnCreateSoundSystem) {
		Terminate("Coulnt find CreateSoundSystem");
		return;
	}
	setSoundSystem(Ptr<ISoundSystem>(pfnCreateSoundSystem(this)));

	HMODULE hEntitySystem = LoadLibraryA("EntitySystem.dll");
	typedef IRenderer*(__cdecl *PFNCREATEENTITYSYSTEM)(IEngine *);
	PFNCREATERENDERER pfnCreateEntitySystem = (PFNCREATEENTITYSYSTEM)GetProcAddress(hEntitySystem, "CreateEntitySystem");
	if (!pfnCreateEntitySystem) {
		Terminate("Coulnt find CreateEntitySystem");
		return;
	}
	pfnCreateEntitySystem(this);


	HMODULE hPhysicsSystem = LoadLibraryA("Physics.dll");
	typedef IPhysicsSystem*(__cdecl *PFNCREATEPHYSICSSYSTEM)(IEngine *);
	PFNCREATEPHYSICSSYSTEM pfnCreatePhysicsSystem = (PFNCREATEPHYSICSSYSTEM)GetProcAddress(hPhysicsSystem, "CreatePhysicsSystem");
	if (!pfnCreatePhysicsSystem) {
		Terminate("Coulnt find CreatePhysicsSystem");
		return;
	}
	physicsSystem = Ptr<IPhysicsSystem>(pfnCreatePhysicsSystem(this));
	
	full_alpha = loadTexture("engine/textures/full_alpha.dds", "FullAlpha", true, TextureSlotAlpha);
	no_texture = loadTexture("engine/textures/no_texture.dds", "White", true, TextureSlotAmbient);
	no_diffuse = loadTexture("engine/textures/no_texture.dds", "White", true, TextureSlotDiffuse);
	no_bump = loadTexture("engine/textures/no_bump.dds", "NoBump", true, TextureSlotNormal);
	no_misc = loadTexture("engine/textures/no_misc.dds", "NoMisc", true, TextureSlotMisc);
	no_ssr = loadTexture("engine/textures/no_ssr.dds", "NoSSR", true);
	letter_map = loadTexture("engine/textures/font.dds", "Letters", true);
	heightmap = loadTexture("engine/textures/heightmap.dds", "Heightmap", true);
	heightmap_normal = loadTexture("engine/textures/heightmap_normal.dds", "HeightmapNormal", true);
	lens_texture = loadTexture("engine/textures/lens_blur.dds", "LensFlareBlur", true);
	brdf_lut = loadTexture("engine/textures/brdf_lut.png", "BRDF_LUT", false);
	equi_lut = loadTexture("engine/textures/equi_lut.dds", "Equi_LUT", false);
	Ptr<ITexture> refl_map_tex = loadTexture("engine/textures/cubemap.dds", "ReflectionMap", true);
	active_cubemap = gcnew<CubeMap>();
	active_cubemap->local = false;
	active_cubemap->texture = refl_map_tex;
	refl_map_tex->generateMips();
	lens_texture->addRef();

	if (!lens_texture) {
		Engine::Terminate("FAIL");
	}

	no_texture->addRef();
	full_alpha->addRef();
	no_bump->addRef();
	no_ssr->addRef();
	no_diffuse->addRef();
	letter_map->addRef();
	heightmap->addRef();
	refl_map_tex->addRef();
	heightmap_normal->addRef();

	doSort = true;
	clear = true;
	depthClear = true;
	
	for (int i = 0; i < 4; i++) {
		std::thread(Engine::worker, this).detach();
	}
}

void Engine::queueMessage(const Message& msg) {
	queuedMessages.enqueue(msg);
}

void Engine::replyMessage(const Message& msg) {
	server->reply(msg);
}

IWindow* Engine::init(IWindow *pWindow, float width, float height, bool fullscreen, const char *name, IWorld *world, bool silent) {
	regConstants = "";
	for (int i = 0; i < 16; i++) {
		counters[i] = 0;
	}
	
	shadowBack = true;
	server = new Server(this);
	std::thread([this]() -> void {
		server->run(7142);
	}).detach();
	
	silentInit(world);
	
	updateShadow = true;
	pWorld = (World*)world;
	framesRendered = 0;
	realFramesRendered = 0;
	renderTime = 0;
	postFxTime = 0;
	colorGrading = 0;
	refs = 0;
	
	#ifdef _WIN32
	QueryPerformanceFrequency((LARGE_INTEGER*)&perfFreq);
	#else
	perfFreq = CLOCKS_PER_SEC;
	#endif
	startUp = getTicks();
	w = width;
	h = height;
	
	if (pWindow) {
		window = pWindow;
	} else window = new Window(this, name, (int)width, (int)height, fullscreen, silent);
	
	if (!renderer->createWindowResources(window)) {
		terminate("Failed to create window resources");
		return false;
	}
	if(fullscreen) renderer->goFullScreen();
	else renderer->goWindowed();

	for (int i = 0; i < MAX_INSTANCES; i++) {
		instanceConstants[i].instance_start.x = (unsigned int)i;
		pInstanceConstants[i] = renderer->createBuffer(eBT_Constant | eBT_Immutable, sizeof(InstanceConstants), &instanceConstants[i]);
	}
	
	pSceneConstants = renderer->createBuffer(eBT_Constant, sizeof(SceneConstants), 0);
	pEntityConstants = renderer->createBuffer(eBT_Constant | eBT_Dynamic, sizeof(EntityConstants), &pWorld->entityConstants);
	pAnimationConstants = renderer->createBuffer(eBT_Constant | eBT_Dynamic, sizeof(AnimationConstants), &pWorld->animationConstants);
	pLightConstants = renderer->createBuffer(eBT_Constant, sizeof(LightConstants), 0);
	pTextConstants = renderer->createBuffer(eBT_Constant, sizeof(TextConstants), 0);
	
	pVertexShader = renderer->loadShader("engine/shaders/Vertex.cso", VERTEX_SHADER);
	pPixelShader = renderer->loadShader("engine/shaders/Pixel.cso", FRAGMENT_SHADER);
	pTerrainVertex = renderer->loadShader("engine/shaders/TerrainVertex.cso", VERTEX_SHADER);
	pTerrainPixel = renderer->loadShader("engine/shaders/TerrainPixel.cso", FRAGMENT_SHADER);
	pHullShader = renderer->loadShader("engine/shaders/HullShader.cso", HULL_SHADER);
	pDomainShader = renderer->loadShader("engine/shaders/DomainShader.cso", DOMAIN_SHADER);
	
	pFrameShader = renderer->loadShader("engine/shaders/FrameVertex.cso", VERTEX_SHADER);
	pTextFrameShader = renderer->loadShader("engine/shaders/TextFrameVertex.cso", VERTEX_SHADER);
	pLightingMainShader = renderer->loadShader("engine/shaders/LightingMain.cso", FRAGMENT_SHADER);
	pLightingTransparentShader = renderer->loadShader("engine/shaders/LightingTransparent.cso", FRAGMENT_SHADER);
	pBakeMainShader = renderer->loadShader("engine/shaders/Bake.cso", FRAGMENT_SHADER);
	
	pBlurShader = renderer->loadShader("engine/shaders/Downsample.cso", FRAGMENT_SHADER);
	pDepthShader = renderer->loadShader("engine/shaders/DepthPixel.cso", FRAGMENT_SHADER);
	pSSRShader = renderer->loadShader("engine/shaders/SSRPixel.cso", FRAGMENT_SHADER);
	pSSRColorShader = renderer->loadShader("engine/shaders/SSRColor.cso", FRAGMENT_SHADER);
	
	pDOFShader = renderer->loadShader("engine/shaders/DepthOfField.cso", FRAGMENT_SHADER);
	pHorizontalBlurShader = renderer->loadShader("engine/shaders/HorizontalBlur.cso", FRAGMENT_SHADER);
	pVerticalBlurShader = renderer->loadShader("engine/shaders/VerticalBlur.cso", FRAGMENT_SHADER);
	pDOFShader = renderer->loadShader("engine/shaders/DepthOfField.cso", FRAGMENT_SHADER);
	pBloomPrepareShader = renderer->loadShader("engine/shaders/Bloom.cso", FRAGMENT_SHADER);
	pBloomShader = renderer->loadShader("engine/shaders/Tonemap.cso", FRAGMENT_SHADER);
	pNoModifyShader = renderer->loadShader("engine/shaders/NoModify.cso", FRAGMENT_SHADER);
	pTextNoModifyShader = renderer->loadShader("engine/shaders/TextNoModify.cso", FRAGMENT_SHADER);
	pFinalShader = renderer->loadShader("engine/shaders/Finalize.cso", FRAGMENT_SHADER);
	pFeedbackShader = renderer->loadShader("engine/shaders/Feedback.cso", FRAGMENT_SHADER);
	pKuwaharaShader = renderer->loadShader("engine/shaders/Kuwahara.cso", FRAGMENT_SHADER);
	pAOShader = renderer->loadShader("engine/shaders/SSAO.cso", FRAGMENT_SHADER);
	pLuminanceShader = renderer->loadShader("engine/shaders/Luminance.cso", FRAGMENT_SHADER);
	pLuminanceDownsampleShader = renderer->loadShader("engine/shaders/LuminanceDownsample.cso", FRAGMENT_SHADER);
	pIrradianceShader = renderer->loadShader("engine/shaders/Irradiance.cso", FRAGMENT_SHADER);
	
	auto rn = this->getClass("RenderQuad", 0, true);
	addObject(rn, true);
	
	for (int i = 0; i < NUM_CASCADES; i++) {
		pWorld->pLightProj[i] = shadowProjs + i;
		pWorld->pLightView[i] = shadowViews + i;
	}
	for (int i = 0; i < NUM_LIGHTS; i++) {
		pWorld->lightConstants.lights[i].position = Vec4(0.0f, 0.0f, 0.0f, 1.0f);
		pWorld->lightConstants.lights[i].color = Vec3(0.0f, 0.0f, 0.0f);
		pWorld->lightConstants.lights[i].intensity = 0.0f;
	}
	
	cout << "Engine initialized" << endl;
	
	return window;
}

void Engine::removePass(Ptr<Pass> pass) {
	for (auto it = passes.begin(); it != passes.end(); it++) {
		if (*it == pass) {
			passes.erase(it);
			break;
		}
	}
}
Ptr<Pass> Engine::createPass(float renderScale, Mat4 *view, Mat4 *proj, bool useScreenOutput) {
	Ptr<Pass> pass = gcnew<Pass>();
	int rw = (int)(w * pWorld->settings.renderQuality * renderScale);
	int rh = (int)(h * pWorld->settings.renderQuality * renderScale);
	int rrw = (int)(w * renderScale);
	int rrh = (int)(h * renderScale);
	
	int gbufferSizes[NUM_GBUFFERS];
	gbufferSizes[GBufferAlbedo] = PBR_DEPTH;
	gbufferSizes[GBufferNormal] = 16 | SIGNED;
#ifndef REAL_DEPTH_BUFFER
	gbufferSizes[GBufferDepth] = 16 | DEPTH_BUFFER;
#endif
	gbufferSizes[GBufferGeometry] = 32;
	gbufferSizes[GBufferMisc] = 8;
	
	pass->named["DOF"] = pass->pDOFPass = renderer->createFrameBuffer("DepthOfField", (int)rw, (int)rh, pFrameShader, pDOFShader, PBR_DEPTH, false);
	
	for (int i = 0; i < NUM_GBUFFERS; i++) {
		pass->named[std::string("GBuffer") + std::to_string(i)] = pass->pMainPass[i] = renderer->createFrameBuffer("Main", (int)(rw), (int)(rh), pVertexShader, pPixelShader, gbufferSizes[i], i==0);
		pass->pMainPass[i]->setVPW(view, proj, &pWorld->mainPass.world);
	}
	
	#ifdef POOR_REFRACTION
	pass->pDistortionAccumulation = renderer->createFrameBuffer("Distortion", (int)(rw), (int)(rh), pVertexShader, pPixelShader, gbufferSizes[GBufferNormal], false);
	#endif
	int res = 1024;
	float sq = res * pWorld->settings.shadowQuality * renderScale;
	if (sq < 1.0f) sq = 4.0f;
	pass->named["ShadowCascades"] = pass->pShadowCascades[0] = renderer->createFrameBuffer("ShadowCascade", (int)(sq), NUM_REAL_CASCADES * (int)(sq), pVertexShader, pDepthShader, -1, true);
	pass->pShadowCascades[0]->getViewport().Height = sq;
	for (int i = 0; i < NUM_REAL_CASCADES; i++) {
		pass->pShadowCascades[i] = renderer->createFrameBuffer("ShadowBuffer", pass->pShadowCascades[0]->getTexture(), pVertexShader, pDepthShader, 0.0f, i * sq);
		pass->pShadowCascades[i]->setVPW(&shadowViews[i], &shadowProjs[i], &pWorld->mainPass.world);
		pass->pShadowCascades[i]->getViewport().Height = sq;
	}
	
	float ssrq = pWorld->settings.ssrQuality;
	if (ssrq > 0.0f) {
		pass->named["SSR"] = pass->pSSRPass = renderer->createFrameBuffer("SSR", (int)(rw * ssrq), (int)(rh * ssrq), pFrameShader, pSSRShader, 16 | DOUBLE_DEPTH_BUFFER, false);
		pass->named["SSRColor"] = pass->pSSRColorPass = renderer->createFrameBuffer("SSRColor", (int)(rw), (int)(rh), pFrameShader, pSSRColorShader, PBR_DEPTH, false, 8);
	}

	pass->named["AO"] = pass->pAOPass = renderer->createFrameBuffer("AO", (int)(rw * 0.5f), (int)(rh * 0.5f), pFrameShader, pAOShader, 16 | DEPTH_BUFFER, false);

	pass->named["Luminance0"] = pass->pLuminance[0] = renderer->createFrameBuffer("Luminance", 64 , 64, pFrameShader, pLuminanceShader, 16 | DOUBLE_DEPTH_BUFFER, false);
	pass->named["Luminance1"] = pass->pLuminance[1] = renderer->createFrameBuffer("Luminance", 32, 32, pFrameShader, pLuminanceShader, 16 | DOUBLE_DEPTH_BUFFER, false);
	pass->named["Luminance2"] = pass->pLuminance[2] = renderer->createFrameBuffer("Luminance", 4, 4, pFrameShader, pLuminanceShader, 16 | DOUBLE_DEPTH_BUFFER, false);
	pass->named["Luminance3"] = pass->pLuminance[3] = renderer->createFrameBuffer("Luminance", 1, 1, pFrameShader, pLuminanceShader, 16 | DOUBLE_DEPTH_BUFFER, false);
	
	int postfxMainSizes[2];
	postfxMainSizes[0] = PBR_DEPTH;
	postfxMainSizes[1] = PBR_DEPTH;
	
	for (int i = 0; i < 2; i++) {
		pass->named[std::string("PostFX") + std::to_string(i)] = pass->pLightingPass[i] = renderer->createFrameBuffer("LightingMain", (int)rw, (int)rh, pFrameShader, pLightingMainShader, postfxMainSizes[i % 2], false);
	}
	
	for (int i = 0; i < 2; i++) {
		pass->named[std::string("Blur") + std::to_string(i)] = pass->pColorBlurPass[i] = renderer->createFrameBuffer("ColorBlur", (int)(rw * 0.125f * 0.5f), (int)(rh * 0.125f), pFrameShader, pBlurShader, PBR_DEPTH, false);
	}

	pass->named["Irradiance"] = pass->pIrradiancePass = renderer->createFrameBuffer("IrradiancePass", 128, 64, pFrameShader, pIrradianceShader, PBR_DEPTH, false);
	
	pass->named["Bake"] = pass->pBakePass = renderer->createFrameBuffer("BakeMain", (int)rw, (int)rh, pFrameShader, pBakeMainShader, PBR_DEPTH, false);
	pass->named["Bloom"] = pass->pBloomPass = renderer->createFrameBuffer("BloomPass", rw, rh, pFrameShader, pBloomShader, 8, false);
	#ifdef HAS_FEEDBACK_PASS
	pass->named["Feedback"] = pass->pFeedbackPass = renderer->createFrameBuffer("FeedbackPass", rw, rh, pFrameShader, pFeedbackShader, 8, false);
	pass->named["LastFrame"] = pass->pLastFrame = renderer->createFrameBuffer("LastFrame", rw, rh, pFrameShader, pNoModifyShader, 8, false);
	#endif
	if (useScreenOutput) {
		pass->named["Screen"] = pass->pScreen = renderer->createScreenOutputFrameBuffer(rrw, rrh, pFrameShader, pFinalShader);
	} else pass->pScreen = 0;
	passes.push_back(pass);
	return pass;
}

///TODO
void Engine::enableDebugging() {
	
}

///TODO
void Engine::disableDebugging() {
	
}

void Engine::setCamera(Ptr<ICamera> pCamera) {
	this->pCamera = pCamera;
}

Ptr<ICamera> Engine::getCamera() {
	return pCamera;
}

Ptr<ITexture> Engine::loadTexture(const char *f, const char *nm, bool generateMips, int slot, bool srgb) {
	return renderer->loadTexture(f, nm, generateMips, slot, srgb);
}

void Engine::loadRadiosityMap(const char *path, Vec4 resolution) {
	radiosity.texture = loadTexture(path, "Radiosity", false);
	radiosity.resolution = resolution;
}

void Engine::addObject(IObject3DPtr object, bool autoInit, bool cleanUp) {
	std::string name = object->getName();
	if (name.length()) {
		objectsByName[name] = object;
	}
	object->setEntityId(++objCounter);
	objects.push_back(object);
	objectsById[object->getEntityId()] = object;
	auto pDrawData = object->getDrawData();
	int start = 0;
	for (auto& it : pDrawData->getObjects()) {
		if (it->offset - start) {
			auto& tgroup = it->pTextures;
			ObjectPassInfo info(it);
			info.pObject = object.get();
			info.start = start;
			info.length = it->offset - start;
			it->start = start;
			if (info.length) {
				if (tgroup->isTransparent()) {
					adjustedObjects[UniquePassTransparent].push_back(info);
				} else {
					adjustedObjects[UniquePassMain].push_back(info);
				}
			}
			start = it->offset;
		}
	}
	if (autoInit) {
		initObject(object, cleanUp);
	}
}

void Engine::initObjects() {
	for (int i = 0, j = (int)objects.size(); i<j; i++) {
		initObject(objects[i]);
	}
}

int Engine::prepareForRender(IObject3DPtr obj, unsigned int flags, bool isShadow, int offset) {
	int cull_now = obj->getCulling();
	obj->update(pWorld);
	int render = (obj->getFlags() & flags);
	bool visible = obj->isVisible();
	if (render == 0) return 0;
	int instCount = obj->getInstancesCount();
	if (render && (visible || isShadow)) {
		Ptr<IMaterial>& txGroup = obj->getTextureGroup();
		if (txGroup && txGroup != activeTxGroup) txGroup->attach(this, offset);
		activeTxGroup = txGroup;
		if (isShadow) obj->setInstancesCount(obj->getMaxInstancesCount());
		if (cull_now != cullMode) {
			//GLFN(glCullFace,cull_now);
		}
		
		cullMode = cull_now;
		return instCount;
	}
	else return 0;
}

void Engine::onKeyDown(int keyCode) {
	if (pGame) {
		pGame->onKeyDown(keyCode);
		if (!keyStates[keyCode]) {
			pGame->onKeyPressed(keyCode);
			keyStates[keyCode] = 1;
		}
	}
}

void Engine::onKeyUp(int keyCode) {
	if (pGame) {
		pGame->onKeyUp(keyCode);
		keyStates[keyCode] = 0;
	}
}

void Engine::onMouseDown(int btn) {
	if (pGame) {
		pGame->onMouseDown(btn);
		if (!mouseStates[btn]) {
			pGame->onMousePressed(btn);
			mouseStates[btn] = 1;
		}
	}
}

void Engine::onMouseUp(int btn) {
	if (pGame) {
		pGame->onMouseUp(btn);
		mouseStates[btn] = 0;
	}
}

void Engine::onScroll(int dir) {
	if (pGame) {
		pGame->onScroll(dir);
	}
}

void Engine::onUpdate(double dt) {
	cpuTime.nextRound();
	gpuTime.nextRound();
	cpuTime.start();

	if (pGame) {
		pGame->onUpdate(pWorld->delta);
	}
	
	physicsSystem->update(dt);

	tris = 0;
	maps = 0;
	pWorld->delta = dt;
	TaskBase* task = 0;
	while (finishedTasks.try_dequeue(task)) {
		if (task) {
			task->finish();
			delete task;
		}
	}
	Message msg;
	while (queuedMessages.try_dequeue(msg)) {
		if (pGame) {
			pGame->onMessage(msg);
		}
	}
	soundSystem->onUpdate();
	if (activeBgm) {
		if (!activeBgm->isPlaying()) {
			activeBgm->stop();
			activeBgm->play();
		}
	}
	
	float3 cameraPos = pCamera->getPos();
	float3 cameraDir = pCamera->getDir();

	for (auto& it : this->objects) {
		for (auto& sound : it->getAttachedSounds()) {
			sound->update(it->getPos(), it->getRotation(), cameraPos, cameraDir, (float)pWorld->delta);
		}
		it->update(pWorld);
	}

	cameraPos = pCamera->getPos();
	cameraDir = pCamera->getDir();

	pWorld->mainPass.view = Math::lookAt(
		cameraPos,
		cameraPos + cameraDir,
		Vec3(0.0f, 0.0f, 1.0f)
	);


	pWorld->constants.cameraView = pWorld->mainPass.view;
	pWorld->constants.cameraPos = cameraPos;
	pWorld->constants.cameraDir = cameraDir;

	if (cubemapStep) {
		generateCubemap();
	}

	for (size_t i = 0, j = passes.size(); i < j; i++) {
		if (!passes[i]->active) continue;
		pWorld->constants.invProj = passes[i]->pMainPass[0]->getProj()->inverse();
		pWorld->constants.invView = passes[i]->pMainPass[0]->getView()->inverse();
		renderFrame(passes[i]);
	}

	gpuTime.start();
	window->SwapBuffers();
	gpuTime.stop();
	updateNo++;
	cpuTime.stop();
}

unsigned int Engine::getFrameNumber() const {
	return updateNo;
}

void Engine::renderFrame(Ptr<Pass> pass) {
	for (int i = 0; i < 64; i++) {
		pWorld->animationConstants.jointTransforms[i] = matrix(1.0f);
	}
	calcFrustums = true;
	activePass = pass;
	counters[0] = 0;
	totalDrawCalls = 0;
	culledOut = 0;
	
	unsigned long long start = getTicks();
	
	pWorld->constants.frame = (unsigned int)(realFramesRendered);
	
	static int offsets[] = { 0, 0, 0 };
	
	if (pWorld->settings.shadowQuality > 0.0f) {
		for (size_t i = 0; i < NUM_REAL_CASCADES; i++) {
			if (!(realFramesRendered & offsets[i])) {
				updateShadowMaps(pass->pShadowCascades[i]->getView());
				pWorld->constants.shadowProj[i] = *pass->pShadowCascades[i]->getProj();
				pWorld->constants.shadowView[i] = *pass->pShadowCascades[i]->getView();
			}
		}
	}

	gpuTime.start();
	renderer->setRasterizer(eRS_SolidFront);
	gpuTime.stop();

	for (size_t i = 0; i < NUM_REAL_CASCADES; i++) {
		if (!(realFramesRendered & offsets[i])) {
			clear = i == 0;
			depthClear = i == 0;
			calcFrustums = true;
			renderPass(&pass->pShadowCascades[i], 1, ENT_SHADOWPASS, true);
		}
	}

	if (pWorld->settings.wireFrame) {
		gpuTime.start();
		renderer->setRasterizer(eRS_Wireframe);
		gpuTime.stop();
	}

	calcFrustums = true;
	clear = true;
	depthClear = true;
	renderPass(pass->pMainPass, NUM_GBUFFERS, ENT_MAINPASS);

	gpuTime.start();
	if (pWorld->settings.wireFrame) {
		renderer->setRasterizer(eRS_SolidFront);
	}
	renderer->disableBlending();
	gpuTime.stop();

	depthClear = false;
	clear = false;

	if (active_cubemap->texture.get() != lastIrradiance) {
		renderPass(&pass->pIrradiancePass, 1, pIrradianceShader, {
			active_cubemap->texture,
			equi_lut
		});
		lastIrradiance = active_cubemap->texture.get();
	}

	renderPass(pass->pLightingPass, NUM_POSTFX_MAIN, POSTFX_LIGHTING);

	if (pWorld->settings.ssrQuality > 0.0f) {
		renderPass(&pass->pSSRPass, 1, pSSRShader, {
			pass->pMainPass[GBufferNormal]->getTexture(),
			pass->pMainPass[GBufferAlbedo]->getDepthTexture(),
			pass->pMainPass[GBufferGeometry]->getTexture()
		});
		
		renderPass(&pass->pSSRColorPass, 1, pSSRColorShader, {
			pass->pLightingPass[0]->getTexture(),
			pass->pSSRPass->getTexture()
		});

		pass->pSSRColorPass->getTexture()->generateMips();
	}

	depthClear = true;
	renderTransparent(pass);
	depthClear = false;
	clear = false;

	gpuTime.start();
	renderer->setRasterizer(eRS_SolidFront);
	renderer->disableBlending();
	gpuTime.stop();

	pass->pBlurryShadow = pass->pLightingPass[PostFXMainShadows]; // downSample(pLightingPass[PostFXMainShadows], 2);	
	
	renderPass(&pass->pBakePass, 1, pBakeMainShader, {
		pass->pLightingPass[0]->getTexture(),
		pWorld->settings.ssrQuality > 0.1f? pass->pSSRColorPass->getTexture() : no_ssr,
		pass->pMainPass[GBuffer::GBufferMisc]->getTexture(),
		active_cubemap->texture,
		pass->pMainPass[GBuffer::GBufferGeometry]->getTexture(),
		pass->pMainPass[GBuffer::GBufferNormal]->getTexture(),
		pWorld->settings.ssrQuality > 0.1f ? pass->pSSRPass->getTexture() : no_ssr
	});

	auto pNextView = pass->pBakePass->getTexture();
	auto pColorView = pNextView;
	
	renderPass(&pass->pColorBlurPass[0], 1, pHorizontalBlurShader, {
		pColorView
	});

	renderPass(&pass->pColorBlurPass[1], 1, pVerticalBlurShader, {
		pass->pColorBlurPass[0]->getTexture()
	});
	pNextView = pass->pColorBlurPass[1]->getTexture();
	
	globalQuality = 1.0f;
	renderPass(&pass->pLuminance[0], 1, pLuminanceShader, {
		pColorView,
		pass->pMainPass[GBufferAlbedo]->getTexture(),
		pass->pLuminance[1]->getTexture()
	});
	renderPass(&pass->pLuminance[1], 1, pLuminanceDownsampleShader, {
		pass->pLuminance[0]->getTexture()
	});
	renderPass(&pass->pLuminance[2], 1, pLuminanceDownsampleShader, {
		pass->pLuminance[1]->getTexture()
	});
	renderPass(&pass->pLuminance[3], 1, pLuminanceDownsampleShader, {
		pass->pLuminance[2]->getTexture()
	});
	globalQuality = pWorld->constants.realGlobalQuality;
	
	renderPass(&pass->pBloomPass, 1, pBloomShader, {
		pColorView,
		pNextView,
		pass->pLuminance[3]->getTexture(),
		lens_texture,
	});
	
	pNextView = pass->pBloomPass->getTexture();
	
	if (pWorld->settings.depthOfField) {
		
		renderPass(&pass->pDOFPass, 1, pDOFShader, {
			pNextView,
			pass->pMainPass[GBuffer::GBufferAlbedo]->getDepthTexture(),
			pass->pMainPass[GBuffer::GBufferGeometry]->getTexture(),
		});
		
		pNextView = pass->pDOFPass->getTexture();
	}
	
	#ifdef HAS_FEEDBACK_PASS
	renderPass(&pass->pFeedbackPass, 1, pFeedbackShader, {
		pNextView,
		pass->pLastFrame->getShaderRef()
	});
	
	renderPass(&pass->pLastFrame, 1, pNoModifyShader, {
		pass->pFeedbackPass->getShaderRef()
	});
	
	pNextView = pass->pLastFrame->getShaderRef();
	#endif
	
	if (pass->pScreen) {
		float q = globalQuality;
		globalQuality = 1.0f;
		gpuTime.start();
		renderer->enableBlending();
		gpuTime.stop();
		//#ifdef HAS_FEEDBACK_PASS
		if (forcedPass.length()) {
			pNextView = pass->named[forcedPass]->getTexture();
		}

		renderPass(&pass->pScreen, 1, pFinalShader, {
			pNextView,
			lut
		});

		//#endif
		renderText(pass->pScreen);

		gpuTime.start();
		renderer->disableBlending();
		gpuTime.stop();
		/*
		clear = true;
		depthClear = true;
		*/
		globalQuality = q;
		gpuTime.start();
		renderer->present();
		gpuTime.stop();
	}
	
	unsigned long long end = getTicks();
	renderTime += (end - start);
	realFramesRendered++;
	cullingEfficiency = 1.0f * culledOut / totalDrawCalls;
}

void Engine::gaussianBlur(Ptr<IFrameBuffer> tx, Ptr<IFrameBuffer> tmp, int power) {
	Viewport& original = tx->getViewport();
	tx->setViewport(tmp->getViewport());
	renderPass(&tmp, 1, pHorizontalBlurShader, {
		tx->getTexture()
	});
	renderPass(&tx, 1, pVerticalBlurShader, {
		tmp->getTexture()
	});
	tx->setViewport(original);
}

void Engine::renderText(Ptr<IFrameBuffer> pass) {
	Ptr<IDepthStencil> depthStencil = pass->getDepthStencil();
	
	Viewport vp = pass->getViewport();
	vp.Width *= globalQuality; vp.TopLeftX *= globalQuality;
	vp.Height *= globalQuality; vp.TopLeftY *= globalQuality;

	gpuTime.start();
	const float clearColor[] = { 0.0f, 0.0f, 0.0f , 1.000f };
	renderer->setViewports(1, &vp);
	renderer->setRenderTargets({ pass->getRenderTarget() }, depthStencil);
	gpuTime.stop();
	
	pWorld->constants.resolution = Math::Vec2((float)pass->getTexture()->getWidth() * globalQuality, (float)pass->getTexture()->getHeight() * globalQuality);
	pWorld->constants.globalQuality = globalQuality;
	pWorld->constants.time = (float)getGameTime();
	gpuTime.start();
	pSceneConstants->update(&pWorld->constants);

	renderer->setShader(pTextFrameShader);
	renderer->setShader(pTextNoModifyShader);
	
	renderer->setVertexConstantBuffers(0, pSceneConstants);
	renderer->setPixelConstantBuffers(0, pSceneConstants);
	renderer->setVertexConstantBuffers(2, pTextConstants);
	renderer->setPixelConstantBuffers(2, pTextConstants);

	renderer->setPixelShaderResources(0, { letter_map });
	gpuTime.stop();
	
	for (int i = 0; i < letters.size(); i += 256) {
		size_t j = 0;
		for (j = 0; j < 256 && (j+i)<(letters.size()); j++) {
			pWorld->texts.letters[j] = letters[j + i];
		}
		gpuTime.start();
		pTextConstants->update(&pWorld->texts);
		gpuTime.stop();
		renderObject(objects[0].get(), objects[0]->getDrawData(), true, true, false, (int)j);
	}
	letters.clear();
}

void Engine::drawText(float x, float y, const char *text, Vec3 color) {
	int N = 0;
	int R = 1;
	while (*text) {
		char c = *text;
		if (c == '\n') {
			R++;
			N = 0;
			text++;
			continue;
		}
		if (c == '$' && text[1] && text[2] && text[3] && text[4] && text[5] && text[6]) {
			char col[7] = { text[1], text[2], text[3], text[4], text[5], text[6], 0 };
			int ncol = 0;
			if (sscanf(col, "%x", &ncol) == 1) {
				float red = ((ncol >> 16) & 255) / 255.0f;
				float green = ((ncol >> 8) & 255) / 255.0f;
				float blue = (ncol & 255) / 255.0f;
				color = Vec3(red, green, blue);
				text += 7;
			}
			continue;
		}
		float fontSize = 0.7f;
		int ID = c - 31;
		int X = ID & 0xF;
		int Y = ID >> 4;
		Letter l;
		float Xscreen = (x + fontSize * 18.0f * N) / (pWorld->settings.width);
		float Yscreen = (y + fontSize * 26.0f * R) / (pWorld->settings.height);
		Yscreen = 1.0f - Yscreen;
		l.position = Vec2(Xscreen, Yscreen);
		l.id.x = X;
		l.id.y = Y;
		l.color = color;
		letters.push_back(l);
		text++;
		N++;
	}
}

void Engine::renderTransparent(Ptr<Pass> pass) {
	IFrameBuffer *fb = pass->pLightingPass[0].get();
	IFrameBuffer *cfb = pass->pMainPass[0].get();
	Ptr<IDepthStencil> depthStencil = pass->pMainPass[GBufferAlbedo]->getDepthStencil();
	
	Viewport vp = fb->getViewport();
	vp.Width *= globalQuality; vp.TopLeftX *= globalQuality;
	vp.Height *= globalQuality; vp.TopLeftY *= globalQuality;
	gpuTime.start();
	renderer->setViewports(1, &vp);
	gpuTime.stop();
	pVertexShader = cfb->getVertexShader();
	pPixelShader = pLightingTransparentShader;
	
	//pWorld->constants.cameraPos = Vec4(pCamera->getPos(), 0.0f);
	//pWorld->constants.cameraDir = Vec4(pCamera->getDir(), 0.0f);
	cfb->getVPW(&pWorld->constants.view, &pWorld->constants.proj, &pWorld->constants.world);
	pWorld->constants.resolution = Math::Vec2((float)fb->getTexture()->getWidth() * globalQuality, (float)fb->getTexture()->getHeight() * globalQuality);
	pWorld->constants.globalQuality = globalQuality;
	pWorld->constants.time = (float)getGameTime();

	gpuTime.start();
	pSceneConstants->update(&pWorld->constants);
	
	renderer->setShader(pVertexShader);
	renderer->setShader(pPixelShader);
	renderer->setVertexConstantBuffers(0, pSceneConstants);
	renderer->setPixelConstantBuffers(0, pSceneConstants);
	gpuTime.stop();
	
	const float clearColor[] = { 0.0f, 0.0f, 0.0f , 1.0f };
	static std::vector<Ptr<IRenderTarget>> renderTarget;
	renderTarget.reserve(8);
	renderTarget.clear();
	int S = 0;
	renderTarget.push_back(pass->pLightingPass[0]->getRenderTarget());
	//renderTarget.push_back(pass->pLightingPass[1]->getRenderTarget());
	renderTarget.push_back(pass->pMainPass[GBufferNormal]->getRenderTarget());
	renderTarget.push_back(pass->pMainPass[GBufferGeometry]->getRenderTarget());
#ifndef REAL_DEPTH_BUFFER
	renderTarget[S++] = pass->pMainPass[GBufferDepth]->getRenderTargetView().Get();
#endif
	#ifdef POOR_REFRACTION
	renderTarget[0] = pass->pLightingPass[2]->getRenderTargetView().Get();
	renderTarget[1] = pass->pDistortionAccumulation->getRenderTargetView().Get();
	context->OMSetRenderTargets(2, renderTarget, depthStencil);
	const float distortClear[] = { 0.0f, 0.0f, 1.0f, 1.0f };
	context->ClearRenderTargetView(pass->pDistortionAccumulation->getRenderTargetView().Get(), distortClear);
	float white[] = { 1.0f, 1.0f, 1.0f, 1.0f };
	context->ClearRenderTargetView(pass->pLightingPass[2]->getRenderTargetView().Get(), clearColor);
	float arr[] = { 1.0f, 1.0f, 1.0f, 1.0f };
	context->OMSetBlendState(pBlendState.Get(), arr, 0xFFFFFFFF);
	#else
	if (pWorld->settings.refractionQuality < 0.2f) {
		renderer->enableBlending();
	}
	renderer->setRenderTargets(renderTarget, depthStencil);
	#endif
	
	framesRendered++;
	
	std::vector<Light> lights; std::vector<DynamicLight> nearbyLights;
	if (pVertexShader && pPixelShader) {
		gpuTime.start();
		//context->setPixelShaderResources(S++, 1, pass->pMainPass[GBufferAlbedo]->getDepthTexture());
		renderer->setPixelShaderResources(5, pass->pShadowCascades[0]->getDepthTexture());
		#ifdef POOR_REFRACTION
		context->setPixelShaderResources(S++, 1, pass->pLightingPass[0]->getShaderRef().GetAddressOf());
		#else
		renderer->setPixelShaderResources(6, pass->pLightingPass[1]->getTexture());
		#endif
		//renderer->setPixelShaderResources(7, pass->pLightingPass[3]->getTexture());
		renderer->setPixelShaderResources(7, radiosity.texture);
		renderer->setPixelShaderResources(8, active_cubemap->texture);
		gpuTime.stop();
		
		renderAdjustedObjects(UniquePassTransparent, false);
	}
	#ifdef POOR_REFRACTION
	context->OMSetBlendState(0, 0, 0xFFFFFFFF);
	#else
	if (pWorld->settings.refractionQuality < 0.2f)
		renderer->disableBlending();
	#endif
}

void Engine::renderPass(Ptr<IFrameBuffer> *fbs, int len, unsigned int flags, bool together, std::vector<Ptr<ITexture>> views) {
	IFrameBuffer *fb = fbs[0].get();
	Ptr<IDepthStencil> depthStencil = fb->getDepthStencil();
	
	//if (depthClear || clear) {
	Viewport vp = fb->getViewport();
	vp.Width *= globalQuality; vp.TopLeftX *= globalQuality;
	vp.Height *= globalQuality; vp.TopLeftY *= globalQuality;
	gpuTime.start();
	renderer->setViewports(1, &vp);
	gpuTime.stop();
	//}
	
	pVertexShader = fb->getVertexShader();
	pPixelShader = fb->getPixelShader();
	
	if (flags & POSTFX) pVertexShader = pFrameShader;
	
	fb->getVPW(&pWorld->constants.view, &pWorld->constants.proj, &pWorld->constants.world);
	pWorld->constants.resolution = Vec2((float)fb->getTexture()->getWidth() * globalQuality, (float)fb->getTexture()->getHeight() * globalQuality);
	pWorld->constants.globalQuality = globalQuality;
	pWorld->constants.radiosityRes = radiosity.resolution;
	pWorld->constants.radiositySettings = radiosity.settings;
	pWorld->constants.cubemapPos = active_cubemap->position;
	pWorld->constants.cubemapPos.w = float(active_cubemap->local);
	pWorld->constants.cubemapBboxA = active_cubemap->position + active_cubemap->bbox1;
	pWorld->constants.cubemapBboxB = active_cubemap->position + active_cubemap->bbox2;
	pWorld->constants.time = (float)getGameTime();
	
	gpuTime.start();
	if (pVertexShader) {
		renderer->setShader(pVertexShader);
	}
	if (pPixelShader) {
		renderer->setShader(pPixelShader);
	}
	
	//context->HSsetShader(0, 0, 0);
	//context->DSsetShader(0, 0, 0);
	renderer->setVertexConstantBuffers(0, pSceneConstants);
	renderer->setPixelConstantBuffers(0, pSceneConstants);
	
	pSceneConstants->update(&pWorld->constants);
	gpuTime.stop();
	
	// Clear the render target and the z-buffer.
	const float clearColor[] = { 0.0f, 0.0f, 0.0f, 0.0f };//(const float*)&pWorld->constants.fogSettings;
	static std::vector<Ptr<IRenderTarget>> renderTarget;
	renderTarget.reserve(8);
	renderTarget.clear();
	bool hasRT = true;
	for ( int i = 0; i < len; i++ ) {
		renderTarget.push_back(fbs[i]->getRenderTarget());
		if (!renderTarget[i]) {
			hasRT = false;
			break;
		}
		if (clear) {
			gpuTime.start();
			renderer->clearRenderTarget(renderTarget[i], (const float*)clearColor);
			gpuTime.stop();
		}
	}
	if (depthClear && depthStencil) {
		gpuTime.start();
		renderer->clearDepthStencil(depthStencil);
		gpuTime.stop();
	}

	if (hasRT) {
		gpuTime.start();
		renderer->setRenderTargets(renderTarget, depthStencil);
		gpuTime.stop();
	} else {
		gpuTime.start();
		renderer->setRenderTargets({}, depthStencil);
		gpuTime.stop();
	}
	
	framesRendered++;
	
	std::vector<Light> lights; std::vector<Ptr<IDynamicLight>> nearbyLights;
	
	if (pVertexShader && pPixelShader) {
		if (flags&POSTFX) {
			renderer->assignSamplers();
			int i = 0;
			int offset = 0;
			int j = 0;
			switch (flags) {
				case POSTFX_LIGHTING: 
					nearbyLights = lightSystem->getNearbyLights(pCamera->getPos(), 250.0f, NUM_LIGHTS);
					pWorld->lightConstants.lightCount = (unsigned int)nearbyLights.size();
					for (int i = 0; i < NUM_LIGHTS; i++) {
						pWorld->lightConstants.lights[i].intensity = 0.0f;
					}
					if (nearbyLights.size()) {
						for (auto& it : nearbyLights) {
							lights.push_back(it->getFinalLight());
						}
						memcpy(&pWorld->lightConstants.lights, lights.data(), lights.size() * sizeof(Light));
					}
					gpuTime.start();
					renderer->setVertexConstantBuffers(2, pLightConstants);
					renderer->setPixelConstantBuffers(2, pLightConstants);
					pLightConstants->update(&pWorld->lightConstants);
					for (i = 0; i < NUM_GBUFFERS; i++) {
						renderer->setPixelShaderResources(i, activePass->pMainPass[i]->getTexture());
					}
#ifdef REAL_DEPTH_BUFFER
					renderer->setPixelShaderResources(i++, activePass->pMainPass[GBufferAlbedo]->getDepthTexture());
#endif
					renderer->setPixelShaderResources(i++, activePass->pShadowCascades[j]->getDepthTexture());
					renderer->setPixelShaderResources(i++, radiosity.texture);
					renderer->setPixelShaderResources(i++, active_cubemap->texture);
					renderer->setPixelShaderResources(i++, activePass->pAOPass->getTexture());
					renderer->setPixelShaderResources(i++, activePass->pIrradiancePass->getTexture());
					renderer->setPixelShaderResources(i++, brdf_lut);
					gpuTime.stop();
					i++;
					break;
				case POSTFX_BAKE:
					gpuTime.start();
					renderer->setPixelShaderResources(0, activePass->pLightingPass[PostFXMain::PostFXMainColor]->getTexture());
					renderer->setPixelShaderResources(1, activePass->pBlurryShadow->getTexture());
					if (pWorld->settings.ssrQuality > 0.0f) {
						renderer->setPixelShaderResources(2, activePass->pSSRColorPass->getTexture());
					} else {
						renderer->setPixelShaderResources(2, no_ssr);
					}
					renderer->setPixelShaderResources(3, activePass->pMainPass[GBuffer::GBufferMisc]->getTexture());
					renderer->setPixelShaderResources(4, active_cubemap->texture);
					#ifdef POOR_REFRACTION
					context->setPixelShaderResources(5, 1, activePass->pDistortionAccumulation->getShaderRef().GetAddressOf());
					context->setPixelShaderResources(6, 1, activePass->pLightingPass[2]->getShaderRef().GetAddressOf());
					#endif
					gpuTime.stop();
					break;
				default:
					gpuTime.start();
					for (i = 0; i < (int)views.size(); i++) {
						renderer->setPixelShaderResources(i, views[i]);
					}
					gpuTime.stop();
				break;
			}
			renderObject(objects[0].get(), objects[0]->getDrawData(), true, true);
			return;
		}
		renderAdjustedObjects(UniquePassMain, together);
	}
}

void Engine::renderAdjustedObjects(UniquePass passId, bool noTextures) {
	Vec3 cameraPos = pCamera->getPos();
	std::deque< ObjectPassInfo >& objects = adjustedObjects[passId];

	if (doSort && (realFramesRendered & 0x3F) == 0) {
		std::sort(objects.begin(), objects.end(), [=](const ObjectPassInfo& a, const ObjectPassInfo& b) -> bool {
			Vec3 aPos = a.buffer->relativePos;
			aPos += a.pObject->getPos();
			Vec3 bPos = b.buffer->relativePos;
			bPos += b.pObject->getPos();
			
			float dstA = aPos.dist(cameraPos);
			float dstB = bPos.dist(cameraPos);
			if (a.pObject->isAnimated() && !b.pObject->isAnimated()) return true;
			if (passId == UniquePassTransparent) {
				if (abs(aPos.z - bPos.z) < 0.001f) {
					return dstA > dstB;
				}
				return aPos.z < bPos.z;
			}
			if (a.pObject == b.pObject) return false;
			if (dstA < dstB) {
				return true;
			}
			return false;
		});
	}
	
	IObject3D* last = 0;
	EntityConstants entityConstants;
	InstanceData& ec = entityConstants.instances[0];
	std::set<IObject3D*> removal;
	float gameTime = (float)getGameTime();

	struct RenderInfo {
		std::vector<InstanceData> instances;
		IDrawData *pDrawData;
		IObject3D *pObj;
		int length;
		int start;
		char type;
		RenderInfo() : length(0), start(0), pDrawData(0), type(0), pObj(0) { instances.reserve(MAX_INSTANCES); }
	};

	static std::unordered_map<const IndexBuffer*, RenderInfo> renderInfo;
	renderInfo.reserve(objects.size());
	renderInfo.clear();

	static std::vector<InstanceData> renderECs;
	renderECs.reserve(objects.size());
	renderECs.clear();

	bool forceDiffuse = false;
	bool enableInstancing = true;

	Ptr<IShader> lastShader = pVertexShader;
	gpuTime.start();
	renderer->setShader(pPixelShader);
	renderer->setShader(pVertexShader);

	renderer->setVertexConstantBuffers(1, pEntityConstants);
	renderer->setVertexConstantBuffers(2, pAnimationConstants);
	renderer->setPixelConstantBuffers(1, pEntityConstants);

	renderer->assignSamplers();
	renderer->setTopology(eTL_TriangleList);
	gpuTime.stop();

	IObject3D *lastAnim = 0;

	int SortCtr = 0;

	for (auto& it_ : objects) {
		if (!it_.length) continue;
		IObject3D* pObj = it_.pObject;
		if (!pObj) continue;
		float3 scale = pObj->getScale();
		float3 relPos = (it_.buffer->relativePos*scale);
		relPos = mul(relPos, XMMatrixTranspose(XMMatrixRotationQuaternion(pObj->getRotation().raw())));
		Vec3 position = pObj->getPos() + relPos;
		float lifeTime = pObj->getLifeTime();
		if (lifeTime > 0.0f && gameTime > lifeTime) {
			removal.insert(pObj);
			continue;
		}
		Mat4 projView = (pWorld->constants.proj * pWorld->constants.view).transpose();
		float mxscale = scale.x > scale.y ? scale.x : scale.y;
		if (scale.z > mxscale) mxscale = scale.z;
		if ((passId == UniquePass::UniquePassTransparent || pWorld->settings.solidCulling) && !sphereCulling(position, it_.buffer->radius * 2.0f * mxscale, projView)) {
			culledOut++;
			continue;
		}
		auto pDrawData = pObj->getDrawData();
		renderer->resetMaterials();
		//Material::resetActive();
		
		bool willRenderNow = passId == UniquePassTransparent || !enableInstancing || pObj->isAnimated();
		
		if (pObj != last) {
			UINT stride = sizeof(Vertex);
			UINT offset = 0;
			
			if (willRenderNow) {
				gpuTime.start();
				renderer->setDrawData(pDrawData);
				gpuTime.stop();
			}

			forceDiffuse = false;
			if (pObj == selectedObject) {
				ec.diffuse = Vec4(1.0f, 0.2f, 0.2f, 1.0f);
				forceDiffuse = true;
			}
			
			last = pObj;
			
			if (pDrawData->needsCleanUp()) {
				size_t relSize = sizeof(Vertex) * pDrawData->getVertices().capacity() + sizeof(Index_t) * pDrawData->getIndices().capacity();
				if (pDrawData->getVertices().size()) {
					pDrawData->getVertices().clear();
				}
				if (pDrawData->getIndices().size()) {
					pDrawData->getIndices().clear();
				}
				pDrawData->needsCleanUp() = false;
			}
		}
		
		ec.model = pObj->getTransform(it_.buffer);
		ec.order = (uint)(++SortCtr);
		uintptr_t iDD = (uintptr_t)(pDrawData.get());
		uintptr_t iIB = (uintptr_t)(it_.buffer.get());
		memcpy(&ec.helper.x, &iDD, sizeof(iDD));
		memcpy(&ec.helper.z, &iIB, sizeof(iIB));
		
		unsigned int start = 0;
		auto& tgroup = it_.buffer->pTextures;
		bool refractive = false;

		ec.nodeId = (it_.buffer->id & 0x3FFFF) | (pObj->getDrawData()->getBufferId() << 18);
		
		if (!noTextures) {
			int count = 0;
			ec.ambient = tgroup->ambient();
			if (!forceDiffuse)
				ec.diffuse = tgroup->diffuse();
			if (pObj->usesPigment())
				ec.diffuse = pObj->getPigment();
			ec.pbrInfo = tgroup->pbr1();
			ec.pbrExtra = tgroup->pbr2();
			ec.diffuse.w = tgroup->alpha();
			ec.txScale = tgroup->scale();
			refractive = tgroup->isRefractive();
			gpuTime.start();
			if (tgroup) {
				count = tgroup->attach(this, 0);
			}
			gpuTime.stop();
		}
		
		if (willRenderNow) {
			gpuTime.start();
			pEntityConstants->copyData(&ec, sizeof(ec), 0);
			maps++;
			if (!refractive && passId == UniquePassTransparent) {
				renderer->enableBlending();
			} else {
				renderer->disableBlending();
			}
			gpuTime.stop();
		} else {
			renderECs.push_back(ec);
		}
		
		if (willRenderNow) {
			if (pObj->isAnimated() && pObj != lastAnim) {
				lastAnim = pObj;
				IAnimated *pAnimated = dynamic_cast<IAnimated*>(pObj);
				if (pAnimated ) {
					gpuTime.start();
					auto& vec = pAnimated->getJointTransforms();
					//MessageBoxA(0, "got joint transforms", 0, 0);
					pAnimationConstants->copyData(vec.data(), sizeof(matrix) * vec.size(), 0);
					maps++;
					gpuTime.stop();
				} else cout << "[Engine] Failed to cast IObject3D to IAnimated!\n";
			}
			gpuTime.start();
			renderer->setVertexConstantBuffers(3, pInstanceConstants[0]);
			renderer->setPixelConstantBuffers(3, pInstanceConstants[0]);
			renderer->drawIndexedInstanced(it_.length, 1, it_.start);
			tris += it_.length / 3;
			gpuTime.stop();
			totalDrawCalls++;
			if (refractive) {
				if (pWorld->settings.refractionQuality > 0.5f) {
					gpuTime.start();
					renderer->copyResource(activePass->pLightingPass[1]->getTexture(), activePass->pLightingPass[0]->getTexture());
					gpuTime.stop();
				}
			}
		}
	}

	if (passId == UniquePassMain && enableInstancing) {
		gpuTime.start();
		//if (!noTextures) {
		//	float arr[] = { 0.5f, 0.5f, 0.5f, 0.5f };
		//	context->OMSetBlendState(pBlendState.Get(), arr, 0xFFFFFFFF);
		//} else
		renderer->disableBlending();
		gpuTime.stop();
		IDrawData *lastDrawData = 0;
		IObject3D *pLO = 0;

		std::sort(renderECs.begin(), renderECs.end(), [=](const InstanceData& a, const InstanceData& b) -> bool {
			int a_ibid = a.nodeId & 0x3FFFF;
			int a_ddid = (a.nodeId >> 18);
			
			int b_ibid = b.nodeId & 0x3FFFF;
			int b_ddid = (b.nodeId >> 18);

			return a.nodeId < b.nodeId;// || a.order < b.order;
		});

		int Ictr = 0;
		uintptr_t ActiveDDID = -1; 

		size_t LoadedStart = 0;
		size_t LoadedEnd = 0;

		for (size_t i = 0, j = renderECs.size(); i < j;) {

			int same = 1;
			for (size_t k = i + 1; k < j; k++) {
				if (renderECs[k].nodeId != renderECs[i].nodeId) break;
				same++;
			}

			if (same >= MAX_INSTANCES) same = MAX_INSTANCES;

			int DDID = renderECs[i].nodeId >> 18;

			if (!LoadedEnd || i > LoadedEnd || (i + same) > LoadedEnd) {
				int left = j - i;
				if (left >= MAX_INSTANCES) left = MAX_INSTANCES;
				gpuTime.start();
				pEntityConstants->copyData(renderECs.data() + i, sizeof(InstanceData) * left, 0);
				gpuTime.stop();
				LoadedStart = i;
				LoadedEnd = i + left;
				maps++;
				Ictr = 0;
			}

			uintptr_t iDD = 0;
			uintptr_t iIB = 0;
			memcpy(&iDD, &renderECs[i].helper.x, sizeof(iDD));
			memcpy(&iIB, &renderECs[i].helper.z, sizeof(iIB));

			IndexBuffer *pIB = (IndexBuffer*)iIB;
			IDrawData *pDD = (IDrawData*)iDD;

			if (iDD != ActiveDDID) {
				renderer->setRawDrawData(pDD);
				ActiveDDID = iDD;
			}

			if (!noTextures) {
				pIB->pTextures->attach(this, 0);
			}

			gpuTime.start();
			renderer->setVertexConstantBuffers(3, pInstanceConstants[Ictr]);
			renderer->setPixelConstantBuffers(3, pInstanceConstants[Ictr]);
			renderer->drawIndexedInstanced(pIB->offset - pIB->start, (unsigned int)same, pIB->start);
			gpuTime.stop();
			tris += (pIB->offset - pIB->start) * same / 3;
			totalDrawCalls++;

			i += same;
			Ictr += same;
		}
	}
	
	if (removal.size()) {
		for (auto& it : removal) {
			removeObject(it);
		}
	}
	
	updateObjects = false;
}

bool Engine::renderObject(IObject3D* pObj, Ptr<IDrawData> pDrawData, bool together, bool noTextures, bool transparent, int instances) {
	gpuTime.start();
	renderer->setDrawData(pDrawData);
	renderer->setTopology(eTL_TriangleList);
	gpuTime.stop();
	
	if (pDrawData->needsCleanUp()) {
		size_t relSize = sizeof(Vertex) * pDrawData->getVertices().capacity() + sizeof(Index_t) * pDrawData->getIndices().capacity();
		if (pDrawData->getVertices().size()) {
			pDrawData->getVertices().clear();
		}
		if (pDrawData->getIndices().size()) {
			pDrawData->getIndices().clear();
		}
		cout << " [gc] Released " << toHuman((float)relSize) << " of memory as object is in VRAM already" << endl;
		pDrawData->needsCleanUp() = false;
	}
	
	gpuTime.start();
	renderer->setVertexConstantBuffers(3, pInstanceConstants[0]);
	renderer->drawIndexedInstanced(pDrawData->getTotalIndices(), instances, 0);
	gpuTime.stop();
	tris += pDrawData->getTotalIndices() * instances / 3;
	totalDrawCalls++;
	return true;
}

bool Engine::sphereCulling(Vec3 pos, float radius, Mat4 viewFrustum) {
	static Vec4 planes[6];
	if (calcFrustums) {
		
		// Left clipping plane
		planes[0].x = viewFrustum._14 + viewFrustum._11;
		planes[0].y = viewFrustum._24 + viewFrustum._21;
		planes[0].z = viewFrustum._34 + viewFrustum._31;
		planes[0].w = viewFrustum._44 + viewFrustum._41;
		// Right clipping plane
		planes[1].x = viewFrustum._14 - viewFrustum._11;
		planes[1].y = viewFrustum._24 - viewFrustum._21;
		planes[1].z = viewFrustum._34 - viewFrustum._31;
		planes[1].w = viewFrustum._44 - viewFrustum._41;
		// Top clipping plane
		planes[2].x = viewFrustum._14 - viewFrustum._12;
		planes[2].y = viewFrustum._24 - viewFrustum._22;
		planes[2].z = viewFrustum._34 - viewFrustum._32;
		planes[2].w = viewFrustum._44 - viewFrustum._42;
		// Bottom clipping plane
		planes[3].x = viewFrustum._14 + viewFrustum._12;
		planes[3].y = viewFrustum._24 + viewFrustum._22;
		planes[3].z = viewFrustum._34 + viewFrustum._32;
		planes[3].w = viewFrustum._44 + viewFrustum._42;
		// Near clipping plane
		planes[4].x = viewFrustum._13;
		planes[4].y = viewFrustum._23;
		planes[4].z = viewFrustum._33;
		planes[4].w = viewFrustum._43;
		// Far clipping plane
		planes[5].x = viewFrustum._14 - viewFrustum._13;
		planes[5].y = viewFrustum._24 - viewFrustum._23;
		planes[5].z = viewFrustum._34 - viewFrustum._33;
		planes[5].w = viewFrustum._44 - viewFrustum._43;
		calcFrustums = false;
	}
	
	return sphereCulling(Vec4(pos, 1.0f), radius, planes);
}

bool Engine::sphereCulling(Vec4 pos, float radius, Vec4 *planes) {
	bool res = true;
	for (int i = 0; i < 6; i++)
	{
		if (pos.dot(planes[i]) <= -radius) {
			res = false;
		}
	}
	return res;
}

void Engine::initObject(IObject3DPtr object, bool cleanUp) {
	int sz = 0;
	IObject3DPtr *objs = object->getObjects(&sz);
	if (objs) {
		for (int i = 0; i < sz; i++) {
			initObject(objs[i]);
		}
		return;
	}
	
	Ptr<IDrawData> pData = object->getDrawData();
	
	if (pData->isBound()) return;
	
	pData->getVertices().shrink_to_fit();
	pData->getIndices().shrink_to_fit();

	if (pData->getVertices().size() == 0) {
		pData->isBound() = true;
		pData->needsCleanUp() = cleanUp;
		return;
	}
	
	size_t vertSz = 0, indSz = 0;
	unsigned int i = pData->getBufferId();
	Vertex *verts = object->getVertices(vertSz);
	std::vector<Index_t>& indices = pData->getIndices();
	indSz = indices.size();
	
	Ptr<IBuffer> vertexBuffer = renderer->createBuffer(eBT_Vertex | eBT_Immutable, (unsigned int)(vertSz * sizeof(Vertex)), verts);
	Ptr<IBuffer> indexBuffer = renderer->createBuffer(eBT_Index | eBT_Immutable, (unsigned int)(sizeof(pData->getIndices()[0]) * indSz), indices.data());
	
	pData->setVertexBuffer(vertexBuffer);
	pData->setIndexBuffer(indexBuffer);
	
	pData->isBound() = true;
	pData->needsCleanUp() = cleanUp;
}

IObject3D* Engine::raytraceObject(RayTraceParams& params) {
	return physicsSystem->raytraceObject(params);
}

int Engine::updateShadowMaps(void *extra) {
	float *mults = pWorld->settings.shadowCascades;
	static float cascades[NUM_CASCADES] = { 0.1f, 0.15f, 0.20f, 0.25f };
	static bool computed = false;
	
	float lambda = 1.0f;
	
	float minDistance = 0.0f;
	float maxDistance = 1.0f;
	
	float nearClip = Z_NEAR;
	float farClip = Z_FAR;
	float clipRange = farClip - nearClip;
	
	float minZ = nearClip + minDistance * clipRange;
	float maxZ = nearClip + maxDistance * clipRange;
	
	float range = maxZ - minZ;
	float ratio = maxZ / minZ;
	
	int j = 0;
	
	void *val = (void*)extra;
	
	for (unsigned int i = 0; i < NUM_CASCADES; ++i)
	{
		if (val && (void*)(shadowViews + i) == val) {
			j = i;
		}
		
		if (!computed) {
			float p = (i + 2.5f) / (NUM_CASCADES + 2.0f);
			float log = minZ * std::pow(ratio, p);
			float uniform = minZ + range * p;
			float d = lambda * (log - uniform) + uniform;
			cascades[i] = mults[i] * (d - nearClip) / clipRange;
			printf("ShadowCascades[%d] = %f -> %f...\n", i, cascades[i], cascades[i] * Z_FAR);
		}
		
	}
	computed = true;
	
	float prevSplitDistance = j == 0 ? minDistance : cascades[j - 1];
	float splitDistance = cascades[j];
	
	Vec3 frustumCorners[8] =
	{
		Vec3(-1.0f, 1.0f, 0.0f),
		Vec3(1.0f, 1.0f, -1.0f),
		Vec3(1.0f, -1.0f, 0.0f),
		Vec3(-1.0f, -1.0f, 0.0f),
		Vec3(-1.0f, 1.0f, 1.0f),
		Vec3(1.0f, 1.0f, 1.0f),
		Vec3(1.0f, -1.0f, 1.0f),
		Vec3(-1.0f, -1.0f, 1.0f)
	};
	
	Mat4 cameraView = pWorld->mainPass.view;
	Mat4 cameraViewProj = (Math::perspective(Math::radians(pWorld->settings.renderFov), 1280.0f / 720.0f, j == 0 ? Z_NEAR : prevSplitDistance * Z_FAR, splitDistance*Z_FAR) * cameraView).transpose();
	
	Mat4 invViewProj = cameraViewProj.inverse();
	for (unsigned int i = 0; i < 8; ++i)
	{
		frustumCorners[i] = TransformFloat3(frustumCorners[i], invViewProj);
	}
	
	Vec3 frustumCenter = Vec3(0.0f);
	for (unsigned int i = 0; i < 8; i++)
	frustumCenter += frustumCorners[i];
	frustumCenter /= 8.0f;
	
	float radius = (frustumCorners[0] - frustumCorners[6]).length() / 2.0f;
	float ss = 1024.0f * pWorld->settings.shadowQuality * pWorld->constants.realGlobalQuality;
	
	float texelsPerUnit = ss / (radius * 2.0f);
	Vec3 lightDir = Vec3(pWorld->constants.sunDir.x, pWorld->constants.sunDir.y, pWorld->constants.sunDir.z); // (-0.3, -0.3, -0.3);
	Mat4 lookAt = Math::lookAt(Vec3(0.0f, 0.0f, 0.0f), -lightDir, Vec3(0.0f, 0.0f, 1.0f)) * texelsPerUnit;
	Mat4 lookAtInv = lookAt.inverse();
	/*frustumCenter = TransformFloat3(frustumCenter, lookAt);
	frustumCenter.x = floor(frustumCenter.x);
	frustumCenter.y = floor(frustumCenter.y);
	frustumCenter = TransformFloat3(frustumCenter, lookAtInv);*/
	
	Vec3 eye(frustumCenter - (lightDir * radius * 2.0f));
	Mat4 viewMatrix = Math::lookAt(eye, frustumCenter, Vec3(0.0f, 0.0f, 1.0f));
	Mat4 projMatrix = Math::ortho(-radius, radius, -radius, radius, -6.0f * radius, radius * 6.0f);
	//printf("radius: %f\n", radius);
	shadowProjs[j] = projMatrix;
	shadowViews[j] = viewMatrix;
	
	return j;
}

void Engine::destroy() {
	float unit = 1.0f / perfFreq;
	float renderTimeUnit = renderTime * unit;
	
	unsigned long long uptime = getTicks() - startUp;
	
	cout << endl << "Destroying engine" << endl;
	cout << " Uptime: " << (uptime * unit) << " s" << endl;
	
	cout << " Frames rendered: " << framesRendered << " (" << realFramesRendered << ")" << endl
	<< " Render ticks total: " << renderTime << endl
	<< " Render time total: " << renderTimeUnit << endl
	<< " CPU time total: " << getTotalCPUTime() << endl
	<< " GPU time total: " << getTotalGPUTime() << endl
	<< " Avg render time: " << (renderTimeUnit / realFramesRendered) << " (" << (realFramesRendered / renderTimeUnit) << " fps)" << endl << endl;
	

	delete window;
}

size_t Engine::getMemoryUsage(bool verbose) const {
	return 0;
}

bool Engine::removeObject(IObject3D* obj) {
	if (this == nullptr || !obj) return false;
	bool found = false;
	unsigned int entityId = obj->getEntityId();
	for (auto it = objects.begin(); it != objects.end();) {
		if (it->get() == obj) {
			//printf("first loop found and removing\n");
			(*it)->setVisible(false);
			for (auto& light : (*it)->getAttachedLights()) {
				lightSystem->removeLight(light);
			}
			auto phys = (*it)->getPhysics();
			if (phys) phys->release();
			it = objects.erase(it);
			if (obj == selectedObject) selectedObject = 0;
			found = true;
			break;
		}
		it++;
	}
	for (int i = 0; i < UniquePassLength; i++) {
		//printf("trying pass %d\n", i);
		for (auto it = adjustedObjects[i].begin(); it != adjustedObjects[i].end();) {
			if (it->pObject == obj) {
				//printf("found subobject\n");
				it = adjustedObjects[i].erase(it);
				continue;
			}
			it++;
		}
	}
	//printf("passes complete, lets try name");
	std::string name = obj->getName();
	//printf("name: %s\n", name.c_str());
	if (name.length()>0) {
		//printf("obj had name, removing\n");
		auto it = objectsByName.find(name);
		if (it != objectsByName.end() && it->second.get() == obj) objectsByName.erase(it);
	}
	auto it = objectsById.find(entityId);
	if (it != objectsById.end()) {
		objectsById.erase(it);
	}
	//printf("removed\n");
	return found;
}

std::deque<ObjectPassInfo>& Engine::getAdjustedObjects(UniquePass passId) {
	return adjustedObjects[passId];
}

Ptr<ITexture> Engine::getDefaultBump() { return no_bump; }

void Engine::selectObject(IObject3D *obj) { selectedObject = obj; }

Ptr<ILightSystem> Engine::getLightSystem() { return lightSystem; }

Vec3 Engine::getCameraDir() const {
	return pCamera->getDir();
}

void Engine::shutdown(const char *msg) {
	Terminate(msg);
}

Ptr<ITexture> Engine::getDefaultDiffuse() { return no_diffuse; }

Engine* Engine::getInstance() {
	if (destroyed) return 0;
	if (!instance) instance = new Engine();
	return instance;
}

void Engine::registerClass(const char *className, ClassFactory fn) {
	std::string name = className;
	classRegistry[name] = fn;
}

Ptr<ISoundSystem> Engine::getSoundSystem() { return soundSystem; }

IWorld* Engine::getWorld() {
	return (IWorld*)pWorld;
}

Ptr<ITexture> Engine::getDefaultAlpha() { return full_alpha; }

double Engine::getGameTime() {
	return (getTicks() / double(perfFreq));
}

Ptr<IPhysics> Engine::createPhysics() {
	return physicsSystem->createPhysics();
}

void Engine::setLUT(const char *path) {
	lut = loadTexture(path, "LUT", false);
	lut->addRef();
}

template<class T> void Engine::registerConstant(const char *name, T val) {
	regConstants += "#define " + std::string(name) + " " + std::to_string(val)+"\r\n";
}

void Engine::addTask(TaskBase* task) {
	tasks.enqueue(task);
}

IGame* Engine::getGame() {
	return pGame;
}

void Engine::renderPass(Ptr<IFrameBuffer> *fbs, int len, Ptr<IShader> pixelShader, std::vector<Ptr<ITexture>> views) {
	fbs[0]->setPixelShader(pixelShader);
	renderPass(fbs, len, POSTFX_OTHER, false, views);
}

ClassFactory Engine::getClass(const char *className) {
	std::string name = className;
	if (classRegistry.find(name) != classRegistry.end()) return classRegistry[name];
	return nullptr;
}

const std::vector<IObject3DPtr>::iterator Engine::getObjectIterator(IObject3D *obj) {
	for (decltype(objects)::iterator it = objects.begin(); it != objects.end(); it++) {
		if ((*it).get() == obj) {
			return it;
		}
	}
	return objects.end();
}

Vec3 Engine::getCameraPos() const {
	return pCamera->getPos();
}

IObject3D* Engine::getSelectedObject() { return selectedObject; }

Ptr<CubeMap> Engine::getActiveReflectionMap() {
	return active_cubemap;
}

void Engine::addReflectionMap(Ptr<CubeMap> cubemap) {
	cubemaps.push_back(cubemap);
}

std::vector<Ptr<CubeMap>>& Engine::getReflectionMaps() {
	return cubemaps;
}

Ptr<CubeMap> Engine::getClosestReflectionMap(float3 pos) {
	size_t idx = -1;
	float dst = 100000.0f;
	for (size_t i = 0, j = cubemaps.size(); i < j; i++) {
		float dstNow = pos.dist(cubemaps[i]->position);
		if (dstNow < dst) {
			dst = dstNow;
			idx = i;
		}
	}
	if (idx != -1) return cubemaps[idx];
	return nullptr;
}

void Engine::setReflectionMap(Ptr<CubeMap> cubemap) {
	active_cubemap = cubemap;
}

void Engine::setReflectionMap(Ptr<ITexture> pTexture) {
	if (!active_cubemap) return;
	active_cubemap->texture = pTexture;
}

void Engine::setReflectionMapPosition(float3 pos) {
	if (!active_cubemap) return;
	active_cubemap->position = pos;
}

void Engine::setReflectionMapBbox(float3 bboxA, float3 bboxB) {
	if (!active_cubemap) return;
	active_cubemap->bbox1 = bboxA;
	active_cubemap->bbox2 = bboxB;
}

std::vector<IObject3DPtr>& Engine::getObjects() { return objects; }

void Engine::setActiveBGM(Ptr<ISound> bgm) {
	activeBgm = bgm;
}

Ptr<ISound> Engine::getActiveBGM() {
	return activeBgm;
}

size_t Engine::getMemoryUsage() const { return getMemoryUsage(false); }

Ptr<ITexture> Engine::getDefaultMisc() { return no_misc; }

Ptr<ITexture> Engine::getDefaultTexture() { return no_texture; }

RadiosityMap& Engine::getRadiosityMap() {
	return radiosity;
}

void Engine::setRadiosityMap(RadiosityMap rm) {
	radiosity = rm;
}

int Engine::getVersion() {
	return LAMEGINE_SDK_VERSION;
}

bool Engine::checkCompatibility(int ver) {
	return (LAMEGINE_SDK_VERSION & 0xFFFFF000) == (ver & 0xFFFFF000);
}

void Engine::getVersionAsString(char *out, int v) const {
#define MAJ(v)  (((v & 0xFFFF0000)>>16))
#define MID1(v) (((v & 0x0000F000)>>12))
#define MID2(v) (((v & 0x00000F00)>>8))
#define MIN(v)  ((v & 0xFF))
	sprintf(out, "%d.%d.%d.%d",
		MAJ(v), MID1(v), MID2(v), MIN(v)
	);
}

Ptr<IDrawDataManager> Engine::getDrawDataManager() { return drawDataManager; }

void Engine::forceShadowsUpdate() {
	updateShadow = true;
}

unsigned long long Engine::getTicks() {
	#ifdef _WIN32
	LARGE_INTEGER large;
	QueryPerformanceCounter(&large);
	return large.QuadPart;
	#else
	return clock();
	#endif
}

Ptr<IObject3D> Engine::getClass(const char *className, void *params, bool init) {
	auto obj = getClass(className);
	if (!obj) return nullptr;
	auto pObj = obj(params);
	pObj->create(params);
	if(init)
		pObj->init(this);
	return pObj;
}

Ptr<ScriptSystem> Engine::getScriptSystem() { return scriptSystem; }

void Engine::setGame(IGame *gameObject) {
	pGame = gameObject;
	pGame->init(this);
}

void Engine::setGlobalQuality(float q) {
	globalQuality = q;
	pWorld->constants.realGlobalQuality = q;
}
float Engine::getGlobalQuality() {
	return pWorld->constants.realGlobalQuality;
}

void Engine::setTimeScale(float ts) {
	physicsSystem->setTimeScale(ts);
}

float Engine::getTimeScale() const {
	return physicsSystem->getTimeScale();
}

void Engine::setDistortion(float distortion) {
	pWorld->constants.distortionStrength = distortion;
}

float Engine::getDistortion() const {
	return pWorld->constants.distortionStrength;
}


double Engine::getCPUTime() const {
	unsigned long long t = cpuTime.getTime() - gpuTime.getTime();
	return t / double(perfFreq);
}
double Engine::getGPUTime() const {
	return gpuTime.getTime() / double(perfFreq);
}
double Engine::getTotalCPUTime() const {
	unsigned long long t = cpuTime.getTotalTime() - gpuTime.getTotalTime();
	return t / double(perfFreq);
}
double Engine::getTotalGPUTime() const {
	return gpuTime.getTotalTime() / double(perfFreq);
}

void Engine::generateCubemap() {
	if (cubemapStep == 0) {
		cubemapStep = 1;
		return;
	}
	auto pEngine = this;
	auto pRenderer = renderer;
	auto cam = pCamera;
	Mat4 cubemapProj = Math::perspective(Math::radians(90.0f), 1.0f, Z_NEAR, Z_FAR);
	Mat4 proj = Math::perspective(Math::radians(pWorld->settings.renderFov), 1280.0f / 720.0f, Z_NEAR, Z_FAR);

	cubemapQuality = PBR_DEPTH;
	if (cubemapRes == 0 || !cubemapArray[0]) {
		cubemapRes = pWorld->settings.cubemapQuality;

		cubemapArray[0] = pRenderer->createFrameBuffer("Cubemap0", cubemapRes, cubemapRes, nullShader, nullShader, cubemapQuality, false);
		cubemapArray[1] = pRenderer->createFrameBuffer("Cubemap1", cubemapRes, cubemapRes, nullShader, nullShader, cubemapQuality, false);
		cubemapArray[2] = pRenderer->createFrameBuffer("Cubemap2", cubemapRes, cubemapRes, nullShader, nullShader, cubemapQuality, false);
		cubemapArray[3] = pRenderer->createFrameBuffer("Cubemap3", cubemapRes, cubemapRes, nullShader, nullShader, cubemapQuality, false);
		cubemapArray[4] = pRenderer->createFrameBuffer("Cubemap4", cubemapRes, cubemapRes, nullShader, nullShader, cubemapQuality, false);
		cubemapArray[5] = pRenderer->createFrameBuffer("Cubemap5", cubemapRes, cubemapRes, nullShader, nullShader, cubemapQuality, false);
		//cubemapArray[6] = pRenderer->createFrameBuffer("Cubemap6", 2 * cubemapRes, cubemapRes, nullShader, nullShader, cubemapQuality, false);

		toSphere = pRenderer->loadShader("engine/shaders/ToSphere.cso", FRAGMENT_SHADER);
	}
	if (cubemapStep) {
		pEngine->setGlobalQuality(1.0f);
		if (cubemapStep == 7) {
			cubemapStep = 0;

			Ptr<IFrameBuffer> lastCubemap = pRenderer->createFrameBuffer("CubemapFinal", 2 * cubemapRes, cubemapRes, nullShader, nullShader, cubemapQuality, false, 8);
			pEngine->renderPass(&lastCubemap, 1, toSphere, {
				cubemapArray[0]->getTexture(),
				cubemapArray[1]->getTexture(),
				cubemapArray[2]->getTexture(),
				cubemapArray[3]->getTexture(),
				cubemapArray[4]->getTexture(),
				cubemapArray[5]->getTexture()
			});
			pWorld->mainPass.proj = proj;
			Ptr<CubeMap> nfo = gcnew<CubeMap>();
			lastCubemap->getTexture()->generateMips();
			nfo->texture = lastCubemap->getTexture();
			nfo->position = cam->getPos();
			nfo->local = true;
			nfo->bbox1 = float3(-16.0f, -16.0f, -2.0f);
			nfo->bbox2 = float3(16.0f, 16.0f, 14.0f);
			pEngine->addReflectionMap(nfo);
			pEngine->setReflectionMap(nfo);
			passes[0]->pBakePass = originalPreCubemapPass;
		} else {
			Vec3 dirs[] = {
				Vec3(0.0f, 1.0f, 0.0f),
				Vec3(0.0f, -1.0f, 0.0f),
				Vec3(1.0f, 0.0f, 0.0f),
				Vec3(-1.0f, 0.0f, 0.0f),
				Vec3(0.0001f, 0.0f, -1.0f),
				Vec3(0.0001f, 0.0f, 1.0f)
			};
			pWorld->mainPass.view = Math::lookAt(
				cam->getPos(),
				cam->getPos() + (dirs[cubemapStep - 1] * 0.1f),
				Vec3(0.0f, 0.0f, 1.0f)
			);
			pWorld->mainPass.proj = cubemapProj;
			if (cubemapStep == 1) {
				originalPreCubemapPass = passes[0]->pBakePass;
			}
			passes[0]->pBakePass = cubemapArray[cubemapStep - 1];
			cubemapStep++;
		}
	}
}