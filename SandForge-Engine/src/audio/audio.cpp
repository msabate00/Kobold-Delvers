#define MINIAUDIO_IMPLEMENTATION
#include "audio.h" 
#include "app/app.h"
#include "app/defs.h"
#include "core/engine.h"
#include <core/utils.h>
#include <algorithm>

Audio::Audio(App* app, bool start_enabled) : Module(app, start_enabled) {};
Audio::~Audio() = default;


bool Audio::Awake() {

	bool ret = (ma_engine_init(NULL, &ma_eng) == MA_SUCCESS);
	
    LoadAudiosInMemory();
	
	return ret;

}
bool Audio::Start() {

	return true;
}
bool Audio::PreUpdate() { return true; }

bool Audio::Update(float dt) {
	return true;
}
bool Audio::PostUpdate() { return true; }

bool Audio::CleanUp() {

    UnloadAll();
    ma_engine_uninit(&ma_eng);
    ticket = 0;
	return true;

}

AudioInstance Audio::Play(const std::string& key, float vol, bool loop)
{
    auto* s = FindSfx(key); if (!s) return {};
    int idx = PickVoice(*s);
    if (idx < 0) return {};
    Voice& v = s->voices[(size_t)idx];


    
    (void)ma_sound_stop(&v.snd);
    (void)ma_sound_seek_to_pcm_frame(&v.snd, 0);
    ma_sound_set_volume(&v.snd, vol);
    ma_sound_set_looping(&v.snd, loop ? MA_TRUE : MA_FALSE);
    v.loop = loop;
    v.ticket = ++ticket;
    (void)ma_sound_start(&v.snd);

    v.generation = static_cast<uint16_t>(v.generation + 1u);
    if (v.generation == 0) v.generation = 1;


    return AudioInstance{ static_cast<uint16_t>(idx), v.generation };
}

Voice* Audio::GetVoice(const std::string& key, AudioInstance id)
{
    return const_cast<Voice*>(
        std::as_const(*this).GetVoice(key, id)
        );
}

const Voice* Audio::GetVoice(const std::string& key, AudioInstance id) const
{
    const SFX* s = FindSfxConst(key);
    if (!s) return nullptr; 
    if (!id.valid() || id.index >= s->voices.size()) return nullptr; 
    const Voice& v = s->voices[id.index]; 
    if (v.generation != id.generation) return nullptr; 
    return &v;
}

int Audio::PickVoice(SFX& s)
{
    int best = -1;
    uint64_t bestTicket = UINT64_MAX;

    for (int i = 0; i < (int)s.voices.size(); ++i) {
        if (!ma_sound_is_playing(&s.voices[i].snd)) return i;
    }

    for (int i = 0; i < (int)s.voices.size(); ++i) {
        if (s.voices[i].ticket < bestTicket) { bestTicket = s.voices[i].ticket; best = i; }
    }
    return best;
}

AudioInstance Audio::Play(const std::string& key, int x, int y, float vol, bool loop)
{
    auto inst = Play(key, vol, loop);
    if (!inst.valid()) return inst;
    SetVoicePosition(key, inst, x, y);
    return inst;
}

void Audio::Stop(const std::string& key, AudioInstance id)
{
    Voice* v = GetVoice(key, id); if (!v) return;
    (void)ma_sound_stop(&v->snd);
    (void)ma_sound_seek_to_pcm_frame(&v->snd, 0);
}

void Audio::StopAll(const std::string& key)
{
    SFX* s = FindSfx(key); if (!s) return;
    for (auto& v : s->voices) {
        (void)ma_sound_stop(&v.snd);
        (void)ma_sound_seek_to_pcm_frame(&v.snd, 0);
    }
}

void Audio::Pause(const std::string& key, AudioInstance id)
{
    Voice* v = GetVoice(key, id); if (!v) return;
    (void)ma_sound_stop(&v->snd);
}

void Audio::Resume(const std::string& key, AudioInstance id)
{
    Voice* v = GetVoice(key, id); if (!v) return;
    (void)ma_sound_start(&v->snd);
}

bool Audio::IsPlaying(const std::string& key, AudioInstance id)
{
    const Voice* v = GetVoice(key, id); if (!v) return false;
    return ma_sound_is_playing(const_cast<ma_sound*>(&v->snd)) == MA_TRUE;
}

double Audio::ElapsedSeconds(const std::string& key, AudioInstance id) const
{
    const Voice* v = GetVoice(key, id); if (!v) return 0.0;
    ma_uint64 cur = 0; (void)ma_sound_get_cursor_in_pcm_frames(const_cast<ma_sound*>(&v->snd), &cur);
    const SFX* s = FindSfxConst(key); if (!s || s->sampleRate == 0) return 0.0;
    return (double)cur / (double)s->sampleRate;
}

double Audio::DurationSeconds(const std::string& key) const
{
    const SFX* s = FindSfxConst(key); if (!s) return 0.0;
    return s->lengthSec;
}

void Audio::SetLoop(const std::string& key, AudioInstance id, bool loop)
{
    Voice* v = GetVoice(key, id); if (!v) return;
    v->loop = loop; ma_sound_set_looping(&v->snd, loop ? MA_TRUE : MA_FALSE);
}

void Audio::SetVolume(const std::string& key, AudioInstance id, float volume)
{
    Voice* v = GetVoice(key, id); if (!v) return;
    ma_sound_set_volume(&v->snd, volume);
}

void Audio::SetGlobalVolume(float volume)
{
    ma_engine_set_volume(&ma_eng, volume);
}

void Audio::SetVoicePosition(const std::string& key, AudioInstance id, int x, int y)
{
    if (!id.valid()) return;
    auto* v = GetVoice(key, id);
    if (!v) return;

    const float camX = app->camera.pos.x;
    const float camW = std::fmax(1.0f, app->camera.size.x);

    const float t = (((float)x + 0.5f) - camX) / camW;
    float pan = t * 2.0f - 1.0f;
    pan = std::clamp(pan, -1.0f, 1.0f);

    ma_sound_set_pan(&v->snd, pan);
}



void Audio::LoadAudiosInMemory()
{
	Load("ignite", AUDIO_DIR  "/ignite.wav", 16);
	Load("paint", AUDIO_DIR  "/paint.wav", 8);
}

bool Audio::Load(const std::string& key, const char* path, int voices)
{
    if ( !path || voices <= 0) return false;

    auto itOld = sfx.find(key);
    if (itOld != sfx.end()) Unload(key);


    SFX s; s.voices.resize(static_cast<size_t>(voices));


    for (int i = 0; i < voices; ++i) {
        ma_result r = ma_sound_init_from_file(&ma_eng, path, 0, nullptr, nullptr, &s.voices[i].snd);
        if (r != MA_SUCCESS) {
            
            for (int j = 0; j < i; ++j) ma_sound_uninit(&s.voices[j].snd);
            return false;
        }
        s.voices[i].inited = true;
        s.voices[i].generation = 1;
        s.voices[i].ticket = 0;
        s.voices[i].loop = false;

        ma_sound_set_volume(&s.voices[i].snd, 1.0f);
        ma_sound_set_looping(&s.voices[i].snd, MA_FALSE);
    }


    
    if (!s.voices.empty()) {
        ma_uint64 lenFrames = 0;
        (void)ma_sound_get_length_in_pcm_frames(&s.voices[0].snd, &lenFrames);
        ma_data_source* ds = ma_sound_get_data_source(&s.voices[0].snd);
        ma_uint32 sr = 0;
        if (ds) {
            
            (void)ma_data_source_get_data_format(ds, nullptr, nullptr, &sr, nullptr, 0);
        }
        if (sr == 0) sr = ma_engine_get_sample_rate(&ma_eng);
        s.sampleRate = sr;
        s.lengthSec = (sr > 0) ? (double)lenFrames / (double)sr : 0.0;
    }


    sfx.emplace(key, std::move(s));
    return true;
}

void Audio::Unload(const std::string& key) {
    auto it = sfx.find(key);
    if (it == sfx.end()) return;
    for (auto& v : it->second.voices) if (v.inited) ma_sound_uninit(&v.snd);
    sfx.erase(it);
}


void Audio::UnloadAll() {
    for (auto& kv : sfx) {
        for (auto& v : kv.second.voices) if (v.inited) ma_sound_uninit(&v.snd);
    }
    sfx.clear();
}


