#pragma once
#include "raylib.h"
#include <unordered_map>
#include <string>
#include <iostream>

class AudioManager {
private:
    std::unordered_map<std::string, Sound> mSounds;
    std::unordered_map<std::string, Music> mMusic;

    Music* mCurrentMusic = nullptr;

    float mMasterVolume = 1.0f;
    float mMusicVolume = 0.5f;
    float mSFXVolume = 0.6f;

    static AudioManager* sInstance;
    AudioManager() {}

public:
    static AudioManager* getInstance();

    void init();
    void shutdown();
    void cleanup();

    void loadSound(const std::string& name, const std::string& path);
    void loadMusic(const std::string& name, const std::string& path);

    void playSound(const std::string& name, float pitch = 1.0f);
    void playSoundRandomPitch(const std::string& name, float minPitch = 0.6f, float maxPitch = 0.9f);

    void playMusic(const std::string& name, bool loop = true);
    void updateMusic();
    void stopMusic();

    bool isMusicPlaying() const;

    void setMasterVolume(float vol);
    void setMusicVolume(float vol);
    void setSFXVolume(float vol);
};