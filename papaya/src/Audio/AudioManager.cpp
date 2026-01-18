#include "AudioManager.hpp"

// Inicjalizacja statycznego wskaünika
AudioManager* AudioManager::sInstance = nullptr;

AudioManager* AudioManager::getInstance() {
    if (!sInstance) sInstance = new AudioManager();
    return sInstance;
}

void AudioManager::init() {
    InitAudioDevice();
    if (!IsAudioDeviceReady()) {
        std::cerr << "[AUDIO] Blad: Nie udalo sie zainicjowac urzadzenia audio!" << std::endl;
    }
    else {
        std::cout << "[AUDIO] System audio gotowy." << std::endl;
    }
}

void AudioManager::shutdown() {
    stopMusic();
    cleanup();
    CloseAudioDevice();
    std::cout << "[AUDIO] Zamknieto system audio." << std::endl;
}

void AudioManager::cleanup() {
    for (auto& pair : mSounds) {
        UnloadSound(pair.second);
    }
    mSounds.clear();

    for (auto& pair : mMusic) {
        UnloadMusicStream(pair.second);
    }
    mMusic.clear();
    mCurrentMusic = nullptr;
}

void AudioManager::loadSound(const std::string& name, const std::string& path) {
    if (mSounds.find(name) != mSounds.end()) return;

    if (!FileExists(path.c_str())) {
        std::cerr << "[AUDIO ERROR] PLIK NIE ISTNIEJE: " << path << std::endl;
        return;
    }

    Sound snd = LoadSound(path.c_str());
    if (snd.stream.buffer != 0) {
        mSounds[name] = snd;
        std::cout << "[AUDIO] Wczytano poprawnie: " << name << " (" << path << ")" << std::endl;
    }
    else {
        std::cerr << "[AUDIO ERROR] Nie udalo sie zaladowac: " << path << std::endl;
    }
}

void AudioManager::loadMusic(const std::string& name, const std::string& path) {
    if (mMusic.find(name) != mMusic.end()) return;

    Music mus = LoadMusicStream(path.c_str());
    if (mus.stream.buffer != 0) {
        mMusic[name] = mus;
        std::cout << "[AUDIO] Wczytano muzyke: " << name << std::endl;
    }
    else {
        std::cerr << "[AUDIO] BLAD: Nie znaleziono pliku muzyki: " << path << std::endl;
    }
}

void AudioManager::playSound(const std::string& name, float pitch) {
    if (mSounds.find(name) != mSounds.end()) {
        Sound& s = mSounds[name];
        SetSoundPitch(s, pitch);
        SetSoundVolume(s, mSFXVolume * mMasterVolume);
        PlaySound(s);
    }
}

void AudioManager::playSoundRandomPitch(const std::string& name, float minPitch, float maxPitch) {
    float pitch = minPitch + (float)GetRandomValue(0, 100) / 100.0f * (maxPitch - minPitch);
    playSound(name, pitch);
}

void AudioManager::playMusic(const std::string& name, bool loop) {
    if (mMusic.find(name) != mMusic.end()) {
        if (mCurrentMusic) StopMusicStream(*mCurrentMusic);

        mCurrentMusic = &mMusic[name];
        mCurrentMusic->looping = loop;
        SetMusicVolume(*mCurrentMusic, mMusicVolume * mMasterVolume);
        PlayMusicStream(*mCurrentMusic);
    }
}

void AudioManager::updateMusic() {
    if (mCurrentMusic) {
        UpdateMusicStream(*mCurrentMusic);
    }
}

void AudioManager::stopMusic() {
    if (mCurrentMusic) {
        StopMusicStream(*mCurrentMusic);
        mCurrentMusic = nullptr;
    }
}

void AudioManager::setMasterVolume(float vol) { mMasterVolume = vol; }
void AudioManager::setMusicVolume(float vol) { mMusicVolume = vol; }
void AudioManager::setSFXVolume(float vol) { mSFXVolume = vol; }

bool AudioManager::isMusicPlaying() const {
    if (!mCurrentMusic) return false;
    return IsMusicStreamPlaying(*mCurrentMusic);
}