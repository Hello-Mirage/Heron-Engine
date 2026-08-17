#pragma once

#include <string>
#include <unordered_map>
#include <cstdint>

// Include miniaudio types (without the implementation)
#include "miniaudio.h"

namespace Heron {

    // A lightweight audio system wrapping miniaudio.
    // Tracks one active sound per entity (by entity ID).
    class AudioSystem {
    public:
        static void Init();
        static void Shutdown();

        // Play an audio file for a given entity.
        // If the entity is already playing, it will stop the previous track first.
        static void PlayTrack(uint32_t entityId, const std::string& filepath);

        // Returns true when the sound associated with the entity has finished playing.
        static bool IsTrackFinished(uint32_t entityId);

        // Stops and releases the sound for the given entity.
        static void StopTrack(uint32_t entityId);

    private:
        static ma_engine* s_Engine;
        static std::unordered_map<uint32_t, ma_sound*> s_Sounds;
    };

} // namespace Heron
