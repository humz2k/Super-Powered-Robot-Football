#ifndef _SPRF_SOUND_HPP_
#define _SPRF_SOUND_HPP_

#include "engine.hpp"

namespace SPRF {

class SoundListener : public Component {
  private:
    Transform* m_transform;

  public:
    void init() { m_transform = this->entity()->get_component<Transform>(); }

    void update() {
        auto transform = this->entity()->global_transform();
        auto feet = vec3(0, 0, 0).Transform(transform);
        // auto head = vec3(0, 1, 0).Transform(transform);
        auto eyes = (vec3(0, 0, 1).Transform(
            this->entity()->global_rotation()));
        // TraceLog(LOG_INFO,"updating listener: %g %g %g | %g %g
        // %g",feet.x,feet.y,feet.z,eyes.x,eyes.y,eyes.z);
        game->soloud.set3dListenerParameters(feet.x, feet.y, feet.z, eyes.x,
                                             eyes.y, eyes.z, 0, 1, 0);
        game->soloud.update3dAudio();
    }
};

class SoundEmitter : public Component {
  private:
    std::unordered_map<std::string, SoLoud::Wav> m_sounds;
    std::unordered_map<std::string, SoLoud::handle> m_handles;
    float m_attenuation;
    SoLoud::AudioSource::ATTENUATION_MODELS m_audio_model;

  public:
    SoundEmitter(float attenuation = 0.1f,
                 SoLoud::AudioSource::ATTENUATION_MODELS audio_model =
                     SoLoud::AudioSource::INVERSE_DISTANCE)
        : m_attenuation(attenuation), m_audio_model(audio_model) {}

    void add_sound(std::string name, std::string file) {
        m_sounds[name].load(file.c_str());
    }

    void update() {
        auto transform = this->entity()->global_transform();
        auto feet = vec3(0, 0, 0).Transform(transform);
        for (auto& i : m_handles) {
            // TraceLog(LOG_INFO,"updating %s: %g %g
            // %g",i.first.c_str(),feet.x,feet.y,feet.z);
            game->soloud.set3dSourceParameters(i.second, feet.x, feet.y,
                                               feet.z);
        }
    }

    void fire_sound(std::string name) {
        if (!KEY_EXISTS(m_sounds, name)) {
            TraceLog(LOG_ERROR, "sound %s doesn't exist", name.c_str());
            return;
        }
        auto transform = this->entity()->global_transform();
        auto feet = vec3(0, 0, 0).Transform(transform);
        game->soloud.play3d(m_sounds[name], feet.x, feet.y, feet.z);
    }

    void play_sound(std::string name) {
        if (!KEY_EXISTS(m_sounds, name)) {
            TraceLog(LOG_ERROR, "sound %s doesn't exist", name.c_str());
            return;
        }
        auto transform = this->entity()->global_transform();
        auto feet = vec3(0, 0, 0).Transform(transform);
        m_handles[name] =
            game->soloud.play3d(m_sounds[name], feet.x, feet.y, feet.z);
        game->soloud.set3dSourceAttenuation(m_handles[name], m_audio_model,
                                            m_attenuation);
    }
};

} // namespace SPRF

#endif // _SPRF_SOUND_HPP_