#pragma once
#include "../Lamegine/Interfaces.h"
#include <Audio.h>

class Sound : public ISound {
private:
	friend class SoundSystem;
	bool is3dSound;
	float3 relativePos;
	Ptr<DirectX::SoundEffectInstance> instance;
	DirectX::AudioEmitter emitter;
	DirectX::AudioListener listener;
public:
	Sound() {

	}
	virtual void setRelativePos(float3 pos);
	virtual float3 getRelativePos();
	virtual void update(float3 pos, float4 rotation, float3 listenerPos, float3 listenerDir, float dt);
	virtual void play();
	virtual void pause();
	virtual void stop();
	virtual void resume();
	virtual void setPan(float pan);
	virtual void setVolume(float vol);
	virtual void setPitch(float pitch);
	virtual bool isPlaying();
	virtual bool is3D();
	virtual SerializeData serialize();
	virtual void deserialize(const SerializeData& params);
};

class SoundSystem : public ISoundSystem {
private:
	std::unique_ptr<DirectX::AudioEngine> audioEngine;
	std::map<std::wstring, Ptr<DirectX::SoundEffect> > resources;
public:
	SoundSystem();
	virtual ~SoundSystem();
	virtual Ptr<ISound> getSound(const char *path, bool is3d=true);
	virtual Ptr<ISound> getSound(const wchar_t *path, bool is3d=true);
	virtual void onUpdate();
	virtual void setReverb(int id);
	DirectX::AudioStatistics getStatistics();
};