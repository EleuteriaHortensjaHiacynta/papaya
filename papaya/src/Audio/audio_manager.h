#pragma once
#include "raylib.h"
#include <unordered_map>
#include <string>

class AudioManager {
private:
	std::unordered_map<std::string, Sound> mSounds;
	std::unordered_map<std::string, Music> mMusic;

	Music* mCurrentMusic = nullptr;
	float mMasterVolume = 1.0f;
	float mMusicVolume = 0.7f;
	float mSFXVolume = 1.0f;

	// singleton
	static AudioManager* sInstance;
	AudioManager() {}

public:
	static AudioManager* getInstance() {
		if (!sInstance) sInstance = new AudioManager();
		return sInstance;
	}

	void init() {
		InitAudioDevice();
	}

	void shutdown() {
		CloseAudioDevice();
	}

	// ³adowanie
	void loadSound(const std::string& name, const std::string& path) {
		mSounds[name] = LoadSound(path.c_str());
	}

	void loadMusic(const std::string& name, const std::string& path) {
		mMusic[name] = LoadMusicStream(path.c_str());
	}

	// sfx
	void playSound(const std::string& name, float pitch = 1.0f) {
		if (mSounds.find(name) != mSounds.end()) {
			SetSoundPitch(mSounds[name], pitch);
			SetSoundVolume(mSounds[name], mSFXVolume * mMasterVolume);
			PlaySound(mSounds[name]);
		}
	}

	// losowy pitch dla ró¿norodnoœci dŸwiêku
	void playSoundRandomPitch(const std::string& name, float minPitch = 0.9f, float maxPitch = 1.1f) {
		float pitch = minPitch + (float)GetRandomValue(0, 100) / 100.0f * (maxPitch - minPitch);
		playSound(name, pitch);
	}

	// Muzyka
	void playMusic(const std::string& name, bool loop = true) {
		if (mMusic.find(name) != mMusic.end()) {
			if (mCurrentMusic) StopMusicStream(*mCurrentMusic);
			mCurrentMusic = &mMusic[name];
			mCurrentMusic->looping = loop;
			SetMusicVolume(*mCurrentMusic, mMusicVolume * mMasterVolume);
			PlayMusicStream(*mCurrentMusic);
		}
	}

	void updateMusic() {
		if (mCurrentMusic) {
			UpdateMusicStream(*mCurrentMusic);
		}
	}

	void stopMusic() {
		if (mCurrentMusic) {
			StopMusicStream(*mCurrentMusic);
			mCurrentMusic = nullptr;
		}
	}

	// volume (g³oœnoœæ)
	void setMasterVolume(float vol) { mMasterVolume = vol; }
	void setMusicVolume(float vol) { mMusicVolume = vol; }
	void setSFXVolume(float vol) { mSFXVolume = vol; }
};
