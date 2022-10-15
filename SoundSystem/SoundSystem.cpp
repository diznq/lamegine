#include "SoundSystem.h"
#include <thread>
#include <future>


#define USE_OGG

#ifdef USE_OGG
#include <ogg/ogg.h>
#include <vorbis/vorbisfile.h>

#pragma comment(lib, "libogg_static")
#pragma comment(lib, "libvorbis_static")
#pragma comment(lib, "libvorbisfile_static")
#pragma comment(lib, "DirectXTK")
#pragma comment(lib, "DirectXTKAudioWin8")
#endif

SoundSystem::SoundSystem() {
	audioEngine = std::make_unique<DirectX::AudioEngine>(AudioEngine_EnvironmentalReverb | AudioEngine_ReverbUseFilters);
	audioEngine->SetReverb(Reverb_Generic); 
}

SoundSystem::~SoundSystem() {
	audioEngine->Suspend();
}

void SoundSystem::onUpdate() {
	if (!audioEngine->Update()) {
		std::cout << "Failed to update sound system" << std::endl;
	}
	if (audioEngine->IsCriticalError()) {
		std::cout << "Failed to update sound system (critical error)" << std::endl;
	}
}

Ptr<ISound> SoundSystem::getSound(const char *path, bool is3d) {
	wchar_t p[256];
	memset(p, 0, sizeof(p));
	for (int i = 0;; i++) {
		p[i] = path[i];
		if (path[i] == 0) break;
	}
	return getSound(p, is3d);
}

Ptr<ISound> SoundSystem::getSound(const wchar_t *path, bool is3d) {
	auto obj = gcnew<Sound>();
	std::wstring wPath = path;
	if (resources.find(wPath) == resources.end()) {
		#ifdef USE_OGG
		if (std::wstring(path).find(L".ogg") != std::string::npos) {
			clock_t start = clock();
			//printf("Decoding OGG\n");
			OggVorbis_File ogg;
			vorbis_info *pInfo;
			FILE *f = _wfopen(path, L"rb");
			ov_open(f, &ogg, 0, 0);
			std::vector<unsigned char> data;
			long bytes = 0;
			int bitStream;
			unsigned char *buffer = new unsigned char[32 * 1024*1024];
			pInfo = ov_info(&ogg, 0);
			do {
				bytes = ov_read(&ogg, (char*)buffer, 32 * 1024 * 1024, 0, 2, 1, &bitStream);
				data.insert(data.end(), buffer, buffer + bytes);
			} while (bytes > 0);
			delete[] buffer;
			std::unique_ptr<uint8_t[]> copy(new unsigned char[data.size() + sizeof(WAVEFORMATEX)]{});
			auto audioStart = (short*)(copy.get() + sizeof(WAVEFORMATEX));
			memcpy(copy.get() + sizeof(WAVEFORMATEX), data.data(), data.size());
			WAVEFORMATEX* wfx = (WAVEFORMATEX*)copy.get();
			wfx->cbSize = 0;
			wfx->nChannels = pInfo->channels;
			wfx->nSamplesPerSec = pInfo->rate;
			wfx->nAvgBytesPerSec = pInfo->rate * 2 * wfx->nChannels;
			wfx->wBitsPerSample = 16;
			wfx->wFormatTag = WAVE_FORMAT_PCM;
			wfx->nBlockAlign = (wfx->nChannels * wfx->wBitsPerSample) / 8;
			
			/*
			printf("wfx->cbSize: %d\n", wfx->cbSize);
			printf("wfx->nChannels: %d\n", wfx->nChannels);
			printf("wfx->nSamplesPerSec: %d\n", wfx->nSamplesPerSec);
			printf("wfx->nAvgBytesPerSec: %d\n", wfx->nAvgBytesPerSec);
			printf("wfx->wBitsPerSample: %d\n", (int)wfx->wBitsPerSample);
			printf("wfx->wFormatTag: %d\n", (int)wfx->wFormatTag);
			printf("wfx->nBlockAlign: %d\n", wfx->nBlockAlign);
			*/
			
			resources[path] = gcnew<DirectX::SoundEffect>(audioEngine.get(), copy, wfx, (unsigned char*)audioStart, data.size());
			ov_clear(&ogg);
			fclose(f);
			
			//printf("Loading OGG sound took %.2f seconds\n", (clock() - start)/(1.0f * CLOCKS_PER_SEC));
		} else {
			#endif
			resources[path] = gcnew<DirectX::SoundEffect>(audioEngine.get(), path);
			#ifdef USE_OGG
		}
		#endif
	}
	obj->instance = resources[path]->CreateInstance(
		(is3d?DirectX::SoundEffectInstance_Use3D:DirectX::SoundEffectInstance_Default) | DirectX::SoundEffectInstance_ReverbUseFilters
	);
	obj->is3dSound = is3d;
	return obj;
}

float3 Sound::getRelativePos() {
	return relativePos;
}

bool Sound::is3D() { return is3dSound; }

bool Sound::isPlaying() {
	return instance->GetState() == PLAYING;
}

void Sound::play() {
	if (isPlaying()) stop();
	instance->Play();
}

void Sound::update(float3 pos, float4 rotation, float3 listenerPos, float3 listenerDir, float dt) {
	if (is3dSound && instance->GetState() == DirectX::SoundState::PLAYING) {
		float3 relPos = relativePos * matrix(XMMatrixRotationQuaternion(rotation.raw()));
		emitter.Update(pos + relPos, float3(0.0f, 0.0f, 1.0f), dt);
		listener.Update(listenerPos, float3(0.0f, 0.0f, 1.0f), dt);
		emitter.SetOrientationFromQuaternion(rotation);
		listener.SetOrientation(listenerDir, float3(0.0f, 0.0f, 1.0f));
		
		instance->Apply3D(listener, emitter);
	}
}

void Sound::stop() {
	instance->Stop();
}

void Sound::setVolume(float vol) {
	instance->SetVolume(vol);
}

void Sound::setPitch(float pitch) {
	instance->SetPitch(pitch);
}

void Sound::resume() {
	instance->Resume();
}

void Sound::setRelativePos(float3 pos) {
	relativePos = pos;
}

void Sound::pause() {
	instance->Pause();
}

void Sound::setPan(float pan) {
	instance->SetPan(pan);
}

DirectX::AudioStatistics SoundSystem::getStatistics() {
	return audioEngine->GetStatistics();
}

void SoundSystem::setReverb(int id) {
	audioEngine->SetReverb((DirectX::AUDIO_ENGINE_REVERB)id);
}


SerializeData Sound::serialize() {
	SerializeData data;
	return data;
}

void Sound::deserialize(const SerializeData& params) {

}

extern "C" {
	__declspec(dllexport) ISoundSystem* __cdecl CreateSoundSystem(IEngine *pEngine) {
		return new SoundSystem();
	}
}