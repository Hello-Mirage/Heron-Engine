// Define the miniaudio implementation only once in this translation unit.
#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio.h"

#include "AudioSystem.h"
#include "Heron/Core/Log.h"

#include <algorithm>

namespace Heron {

    ma_engine*                              AudioSystem::s_Engine = nullptr;
    std::unordered_map<uint32_t, ma_sound*> AudioSystem::s_Sounds;

    void AudioSystem::Init() {
        s_Engine = new ma_engine();
        ma_result result = ma_engine_init(nullptr, s_Engine);
        if (result != MA_SUCCESS) {
            HRN_CORE_ERROR("AudioSystem: Failed to initialize miniaudio engine! Code: {0}", (int)result);
            delete s_Engine;
            s_Engine = nullptr;
        } else {
            HRN_CORE_INFO("AudioSystem: Initialized successfully.");
        }
    }

    void AudioSystem::Shutdown() {
        for (auto& [id, sound] : s_Sounds) {
            if (sound) {
                ma_sound_stop(sound);
                ma_sound_uninit(sound);
                delete sound;
            }
        }
        s_Sounds.clear();

        if (s_Engine) {
            ma_engine_uninit(s_Engine);
            delete s_Engine;
            s_Engine = nullptr;
        }
        HRN_CORE_INFO("AudioSystem: Shut down.");
    }

    void AudioSystem::PlayTrack(uint32_t entityId, const std::string& filepath) {
        if (!s_Engine) return;

        // Stop and release any existing sound for this entity
        StopTrack(entityId);

        ma_sound* sound = new ma_sound();
        ma_result result = ma_sound_init_from_file(s_Engine, filepath.c_str(), 0, nullptr, nullptr, sound);
        if (result != MA_SUCCESS) {
            HRN_CORE_ERROR("AudioSystem: Failed to load '{0}'. Code: {1}", filepath, (int)result);
            delete sound;
            return;
        }

        ma_sound_start(sound);
        s_Sounds[entityId] = sound;
        HRN_CORE_INFO("AudioSystem: Playing '{0}' for entity {1}", filepath, entityId);
    }

    bool AudioSystem::IsTrackFinished(uint32_t entityId) {
        auto it = s_Sounds.find(entityId);
        if (it == s_Sounds.end() || !it->second) return true;
        // ma_sound_at_end returns MA_TRUE when the sound has reached end of stream
        return ma_sound_at_end(it->second) == MA_TRUE;
    }

    void AudioSystem::StopTrack(uint32_t entityId) {
        auto it = s_Sounds.find(entityId);
        if (it != s_Sounds.end() && it->second) {
            ma_sound_stop(it->second);
            ma_sound_uninit(it->second);
            delete it->second;
            s_Sounds.erase(it);
        }
    }

} // namespace Heron
