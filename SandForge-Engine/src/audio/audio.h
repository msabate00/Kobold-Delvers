#pragma once
#include "app/module.h"
#include <vector>
#include <cstdint>
#include <core/material.h>
#include <unordered_map>
#include <limits>
#include "../../third_party/miniaudio/miniaudio.h"


struct Vertex;

enum class AudioBus : uint8_t {
    Sfx = 0,
    Music
};
struct Voice {
    ma_sound snd{};
    bool inited = false;
    bool loop = false;
    float baseVolume = 1.0f;
    uint16 generation = 0; 
    uint64 ticket = 0;
};


struct SFX {
    std::vector<Voice> voices;
    AudioBus bus = AudioBus::Sfx;
    ma_uint32 sampleRate = 0;
    double lengthSec = 0.0;
};

struct AudioInstance {
    uint16 index = UINT16_MAX;
    uint16 generation = 0; 
    bool valid() const { return generation != 0; }
    explicit operator bool() const { return valid(); } 
    static constexpr AudioInstance null() { return {}; }
};


class Audio : public Module {
public:
    Audio(App* app, bool start_enabled = true);
    virtual ~Audio();

    bool Awake();
    bool Start();
    bool PreUpdate();
    bool Update(float dt);
    bool PostUpdate();

    bool CleanUp();

    bool Load(const std::string& key, const char* path, int voices = 8, AudioBus bus = AudioBus::Sfx);
    void Unload(const std::string& key);
    void UnloadAll();

    
    AudioInstance Play(const std::string& key, int x, int y, float vol = 1.0f, bool loop = false);
    AudioInstance PlayOneShot(const std::string& key, float volume = 1.0f) { return Play(key, volume, false); }

    void Stop(const std::string& key, AudioInstance id);
    void StopAll(const std::string& key);

    void Pause(const std::string& key, AudioInstance id);
    void Resume(const std::string& key, AudioInstance id);


    bool IsPlaying(const std::string& key, AudioInstance id);


    double ElapsedSeconds(const std::string& key, AudioInstance id) const;
    double DurationSeconds(const std::string& key) const;


    void SetLoop(const std::string& key, AudioInstance id, bool loop);
    void SetVolume(const std::string& key, AudioInstance id, float volume);
    void SetGlobalVolume(float volume);
    void SetMusicVolume(float volume);
    void SetSfxVolume(float volume);
    float MusicVolume() const { return musicVolume; }
    float SfxVolume() const { return sfxVolume; }
    void SetVoicePosition(const std::string& key, AudioInstance id, int x, int y);

    void PlayMusic(const std::string& key, float fadeSec = 0.35f);
    void StopMusic(float fadeSec = 0.25f);
    const std::string& CurrentMusicKey() const { return currentMusicKey; }

private:

    float BusVolume(AudioBus b) const;
    void ReapplyBusVolumes(AudioBus b);
    void LoadAudiosInMemory();
    AudioInstance Play(const std::string& key, float vol = 1.0f, bool loop = false);
    void StartMusicTransition(const std::string& nextKey, float fadeSec);
    void UpdateMusicTransition(float dt);

    SFX* FindSfx(const std::string& key) { auto it = sfx.find(key); return it == sfx.end() ? nullptr : &it->second; }
    const SFX* FindSfxConst(const std::string& key) const { auto it = sfx.find(key); return it == sfx.end() ? nullptr : &it->second; }

    Voice* GetVoice(const std::string& key, AudioInstance id);
    const Voice* GetVoice(const std::string& key, AudioInstance id) const;

    int PickVoice(SFX& s);



public:


private:
    ma_engine ma_eng{};
    uint64 ticket = 0;
    std::unordered_map<std::string, SFX> sfx;

    float musicVolume = 1.0f;
    float sfxVolume = 1.0f;

    std::string currentMusicKey;
    AudioInstance currentMusic = AudioInstance::null();
    std::string prevMusicKey;
    AudioInstance prevMusic = AudioInstance::null();
    float musicFadeTimer = 0.0f;
    float musicFadeDuration = 0.0f;
    bool musicTransitionActive = false;
};
