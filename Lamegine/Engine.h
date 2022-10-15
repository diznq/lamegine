#pragma once
#ifndef __ENGINE_H__
#define __ENGINE_H__
#include "Interfaces.h"
#include "BlockingConcurrentQueue.h"
#include <memory>
#include <algorithm>
#include <time.h>
#include <queue>
#include <condition_variable>

class World;
class ScriptSystem;
class DeviceResources;
class Shader;
class Server;

#define REAL_DEPTH_BUFFER

enum GBuffer {
	GBufferAlbedo = 0,
	GBufferNormal,
	GBufferGeometry,
	GBufferMisc,
#ifndef REAL_DEPTH_BUFFER
	GBufferDepth,
#endif
	NUM_GBUFFERS
};

enum PostFXMain {
	PostFXMainColor = 0,
	PostFXMainShadows,
	NUM_POSTFX_MAIN
};

//#define RENDER_SHADOW_ONCE


#define INIT_UNIFORM(id) uniforms[shaderIdx][id]=GLFN(glGetUniformLocation,shaderProg->program, #id);

#define REG_CONSTANT(name) registerConstant(#name, name)

enum UniquePass {
	UniquePassMain,
	UniquePassTransparent,
	UniquePassLength
};

struct ObjectPassInfo {
	IObject3D* pObject;
	Ptr<IndexBuffer> buffer;
	int start;
	int length;
	ObjectPassInfo(Ptr<IndexBuffer> b) : pObject(0), start(0), length(0), buffer(b) {}
};

struct Pass {
	bool active=true;
	Ptr<IFrameBuffer>
		pScreen,
		pLastFrame,
		pDOFPass,
		pAOPass,
		pBloomPrepare,
		pBloomPass,
		pMainPass[NUM_GBUFFERS],
		pShadowCascades[NUM_REAL_CASCADES + 1],
		pSSRPass,
		pSSRColorPass,
		pIrradiancePass,
		pLightingPass[2],
		pDistortionAccumulation,
		pBakePass,
		pBlurPass[2],
		pColorBlurPass[2],
		pBlurryShadow,
		pFeedbackPass,
		pLuminance[4];
	std::map<std::string, Ptr<IFrameBuffer>> named;
};

class Engine : public IEngine {
protected:
	friend Server;
	Server *server;
	class Timer {
		unsigned long long totalTicks = 0;
		unsigned long long ticks = 0;
		unsigned long long startTime = 0;
	public:
		void nextRound() {
			totalTicks += ticks;
			ticks = 0;
		}
		void start() {
			startTime = getTicks();
		}
		void stop() {
			ticks += getTicks() - startTime;
		}
		unsigned long long getTime() const {
			return ticks;
		}
		unsigned long long getTotalTime() const {
			return totalTicks;
		}
	};
public:
	Engine();

	void silentInit(IWorld *world = 0);
	~Engine();
	void destroy();

	static void Terminate(const wchar_t* message);
	static void Terminate(const char *message0);
	static Engine* getInstance();
	static unsigned long long getTicks();
	static void worker(Engine* self);

	virtual void terminate(const char *msg) { Terminate(msg); }
	virtual void terminate(const wchar_t *msg) { Terminate(msg); }

	virtual IWindow* init(IWindow *pWindow, float w, float h, bool fullscreen = false, const char *nm = "Lamegine", IWorld *world = 0, bool silent = false);

	virtual void setSoundSystem(Ptr<ISoundSystem> pSS) {
		soundSystem = pSS;
	}
	virtual void setRenderer(Ptr<IRenderer> pRenderer) {
		renderer = pRenderer;
	}
	virtual Ptr<IPhysicsSystem> getPhysicsSystem() const {
		return physicsSystem;
	}
	virtual Ptr<IRenderer> getRenderer() const {
		return renderer;
	}

	virtual void setCamera(Ptr<ICamera> pCamera);
	virtual Ptr<ICamera> getCamera();

	virtual int getVersion();
	virtual bool checkCompatibility(int ver);

	virtual void shutdown(const char *msg);
	
	virtual void addTask(TaskBase* task);

	virtual void setGame(IGame *gameObject);
	virtual IGame* getGame();
	virtual IWorld* getWorld();
	virtual double getGameTime();
	virtual unsigned int getFrameNumber() const;
	virtual Ptr<IPhysics> createPhysics();


	virtual Vec3 getCameraPos() const;

	virtual Vec3 getCameraDir() const;

	virtual void registerClass(const char *className, ClassFactory fn);

	virtual ClassFactory getClass(const char *className);

	virtual Ptr<IObject3D> getClass(const char *className, void *params, bool init = true);

	size_t getMemoryUsage(bool verbose) const;
	size_t getMemoryUsage() const;

	Ptr<ScriptSystem> getScriptSystem();

	virtual Ptr<ILightSystem> getLightSystem();
	virtual Ptr<IDrawDataManager> getDrawDataManager();
	virtual Ptr<ISoundSystem> getSoundSystem();
	
	template<class T> Ptr<T> getObjectByName(const char* name) {
		auto it = objectsByName.find(name);
		if (it != objectsByName.end()) return std::static_pointer_cast<T>(it->second);
		return nullptr;
	}

	virtual void onUpdate(double dt);
	void onKeyDown(int keyCode);
	void onKeyUp(int keyCode);
	void onMouseDown(int btn);
	void onMouseUp(int btn);
	void onScroll(int dir);

	virtual Ptr<ITexture> loadTexture(const char *file, const char *name, bool generateMips = true, int slot = -1, bool srgb = false);
	
	void initObjects();
	void initObject(IObject3DPtr object, bool cleanUp = true);
	virtual void addObject(IObject3DPtr object, bool autoInit=false, bool cleanUp=true);
	virtual std::vector<IObject3DPtr>& getObjects();
	virtual IObject3D* getSelectedObject();
	virtual void selectObject(IObject3D *obj);
	virtual bool removeObject(IObject3D *obj);
	virtual const std::vector<IObject3DPtr>::iterator getObjectIterator(IObject3D *obj);
	virtual IObject3D *raytraceObject(RayTraceParams& params);
	virtual void drawText(float x, float y, const char *text, Vec3 color = Vec3(1.0f, 1.0f, 1.0f));

	virtual void setLUT(const char *path);

	Ptr<Pass> createPass(float renderScale, Mat4 *view, Mat4 *proj, bool useScreenOutput = true);
	void removePass(Ptr<Pass> pass);
	virtual IWindow *getWindow() { return this->window; }
	virtual void setRadiosityMap(RadiosityMap rm);
	virtual RadiosityMap& getRadiosityMap();
	virtual void loadRadiosityMap(const char *path, Vec4 resolution);

	virtual Ptr<ITexture> getDefaultDiffuse();
	virtual Ptr<ITexture> getDefaultBump();
	virtual Ptr<ITexture> getDefaultAlpha();
	virtual Ptr<ITexture> getDefaultMisc();
	virtual Ptr<ITexture> getDefaultTexture();

	virtual void setActiveBGM(Ptr<ISound> bgm);

	virtual Ptr<ISound> getActiveBGM();

	virtual void addReflectionMap(Ptr<CubeMap> cubemap);
	virtual std::vector<Ptr<CubeMap>>& getReflectionMaps();
	virtual Ptr<CubeMap> getClosestReflectionMap(float3 pos);
	virtual void setReflectionMap(Ptr<CubeMap> cubemap);
	virtual void setReflectionMap(Ptr<ITexture> cubemap);
	virtual void setReflectionMapPosition(float3 centre);
	virtual void setReflectionMapBbox(float3 bboxA, float3 bboxB);
	virtual Ptr<CubeMap> getActiveReflectionMap();

	virtual void setGlobalQuality(float q);
	virtual float getGlobalQuality();

	virtual void setTimeScale(float timeScale);
	virtual float getTimeScale() const;


	virtual void setDistortion(float distortion);
	virtual float getDistortion() const;

	virtual void getVersionAsString(char *out, int ver) const;

	int getCulledCount() { return culledOut; }
	int getDrawCallsCount() { return totalDrawCalls; }

	virtual void displayPass(std::string name) { forcedPass = name; }

	virtual double getCPUTime() const;
	virtual double getGPUTime() const;
	virtual double getTotalCPUTime() const;
	virtual double getTotalGPUTime() const;

	virtual unsigned long getTris() const { return tris; }


	virtual void queueMessage(const Message& msg);
	virtual void replyMessage(const Message& msg);


	virtual Ptr<IObject3D> getObject(unsigned int entityId) {
		auto it = objectsById.find(entityId);
		if (it != objectsById.end()) return it->second;
		return nullptr;
	}

	virtual int getMaps() const { return maps; }

	virtual void generateCubemap();

private:
	moodycamel::BlockingConcurrentQueue<TaskBase*> tasks;
	moodycamel::ConcurrentQueue<TaskBase*> finishedTasks;
	moodycamel::ConcurrentQueue<Message> queuedMessages;
	Ptr<ITexture> no_texture, no_diffuse, no_bump, full_alpha, no_misc, no_ssr, letter_map, brdf_lut, equi_lut;
	Ptr<ISound> activeBgm;
	std::string forcedPass;
	std::map<std::string, ClassFactory> classRegistry;
	bool sphereCulling(Vec3 pos, float radius, Mat4 viewFrustum);
	bool sphereCulling(Vec4 pos, float radius, Vec4* planes);
	void renderFrame(Ptr<Pass> pass);
	void renderTransparent(Ptr<Pass> pass);
	void renderAdjustedObjects(UniquePass passId, bool noTextures=false);
	void renderPass(Ptr<IFrameBuffer> *fbs, int len = 1, unsigned int flags = ENT_ALL, bool together = false, std::vector<std::shared_ptr<ITexture>> views = {});
public:
	void renderPass(Ptr<IFrameBuffer> *fbs, int len, Ptr<IShader> pixelShader, std::vector<std::shared_ptr<ITexture>> views);
private:
	void gaussianBlur(Ptr<IFrameBuffer> tx, Ptr<IFrameBuffer> tmp, int power=4);
	void renderText(Ptr<IFrameBuffer> pass);
	bool renderObject(IObject3D* obj, Ptr<IDrawData> pDrawData, bool together=false, bool noTextures=false, bool transparent=false, int instances=1);
	int prepareForRender(IObject3DPtr obj, unsigned int flags=0, bool isShadow=false, int offset=0);
	int updateShadowMaps(void* id);
	void enableDebugging();
	void disableDebugging();
	void forceShadowsUpdate();
	std::deque<ObjectPassInfo>& getAdjustedObjects(UniquePass passId);
	
	template<class T> static void registerConstant(const char *name, T val);

	bool doSort;
	float cullingEfficiency;

	Timer cpuTime, gpuTime;

	bool shadowBack = false;
	float globalQuality;
	IGame *pGame = 0;
	static std::string regConstants;
	Ptr<ITexture> lut, heightmap, heightmap_normal, lens_texture;
	Ptr<CubeMap> active_cubemap;
	Ptr<ICamera> pCamera;
	std::vector<Letter> letters;

	ITexture* lastIrradiance = 0;

	IObject3D* selectedObject = 0;

	unsigned long long renderTime, postFxTime;
	unsigned long long framesRendered, realFramesRendered;
	unsigned long updateNo = 0;
	static unsigned long long perfFreq;
	unsigned long long startUp;
	ITexture *colorGrading;
	Ptr<IMaterial> activeTxGroup;
	bool updateShadow;
	int cullMode;
	int drawCalls = 0;

	Ptr<ScriptSystem> scriptSystem;
	Ptr<ISoundSystem> soundSystem;

	Ptr<IShader>
		pVertexShader,
		pPixelShader,
		pTerrainVertex,
		pTerrainPixel,
		pFrameShader,
		pTextFrameShader,
		pLightingMainShader,
		pBakeMainShader,
		pDepthShader,
		pSSRShader,
		pSSRColorShader,
		pBlurShader,
		pHorizontalBlurShader,
		pVerticalBlurShader,
		pDOFShader,
		pAOShader,
		pBloomPrepareShader,
		pBloomShader,
		pIrradianceShader,
		pKuwaharaShader,
		pNoModifyShader,
		pTextNoModifyShader,
		pFeedbackShader,
		pFinalShader,
		pLightingTransparentShader,
		pLuminanceShader,
		pLuminanceDownsampleShader,
		pHullShader,
		pDomainShader;

	std::vector<IObject3DPtr> objects;
	std::vector<Ptr<CubeMap>> cubemaps;
	Ptr<IDrawDataManager> drawDataManager;
	Ptr<ILightSystem> lightSystem;
	std::deque< ObjectPassInfo > adjustedObjects[UniquePassLength];
	std::map<std::string, IObject3DPtr> objectsByName;
	std::map<unsigned int, IObject3DPtr> objectsById;

	RadiosityMap radiosity;

	int currentPass;
	float w, h;
	unsigned int texCounter;
	unsigned int objCounter;

	World *pWorld;
	IWindow *window;
	Mat4 shadowProjs[NUM_CASCADES];
	Mat4 shadowViews[NUM_CASCADES];

	std::vector<Ptr<Pass>> passes;
	Ptr<Pass> activePass, mainPass;

	unsigned int passesStart[PASSES_LENGTH];
	unsigned int passesEnd[PASSES_LENGTH];

	Ptr<DeviceResources> deviceResources;
	Ptr<IRenderer> renderer;
	
	Ptr<IBuffer> pSceneConstants, pEntityConstants, pLightConstants, pTextConstants, pAnimationConstants;
	Ptr<IBuffer> pInstanceConstants[MAX_INSTANCES];

	InstanceConstants instanceConstants[MAX_INSTANCES];

	int counters[16];
	int keyStates[256] = { 0 };
	int mouseStates[256] = { 0 };

	static Engine *instance;
	static bool destroyed;
	int refs;
	bool clear, depthClear;
	int culledOut, totalDrawCalls;
	bool calcFrustums;
	bool updateObjects;
	unsigned long tris = 0;
	int maps = 0;
	int cubemapStep = 0;

	Ptr<IShader> nullShader;
	int cubemapRes = 0;
	int cubemapQuality = 11;
	Ptr<IFrameBuffer> cubemapArray[6], originalPreCubemapPass;
	Ptr<IShader> toSphere;
	Ptr<IPhysicsSystem> physicsSystem = 0;

	Vec4 cubemapCentre, cubemapBboxA, cubemapBboxB;
};
#endif