#ifndef _SPRF_MOUSELOOK_HPP_
#define _SPRF_MOUSELOOK_HPP_

#include "engine/engine.hpp"

namespace SPRF {

class MouseLook : public Component {
  private:
    bool mouse_locked = false;

  public:
    void init() {}

    void update() {
        if (IsKeyPressed(KEY_Q)) {
            if (mouse_locked) {
                EnableCursor();
                mouse_locked = false;
            } else {
                DisableCursor();
                mouse_locked = true;
            }
        }
        if (game_info.dev_console_active) {
            if (mouse_locked) {
                EnableCursor();
                mouse_locked = false;
            }
            return;
        }
        if (!mouse_locked) {
            return;
        }

        auto mouse_delta =
            GetRawMouseDelta() *
            vec2(game_settings.float_values["m_yaw"],
                            game_settings.float_values["m_pitch"]) *
            game_settings.float_values["m_sensitivity"];
        this->entity()->get_component<Transform>()->rotation.x += mouse_delta.y;
        this->entity()->get_component<Transform>()->rotation.y -= mouse_delta.x;

        this->entity()->get_component<Transform>()->rotation.y =
            Wrap(this->entity()->get_component<Transform>()->rotation.y, 0,
                 2 * M_PI);
        this->entity()->get_component<Transform>()->rotation.x =
            Clamp(this->entity()->get_component<Transform>()->rotation.x,
                  -M_PI * 0.5f + 0.5, M_PI * 0.5f - 0.5);
    }
};

} // namespace SPRF

#endif // _SPRF_MOUSELOOK_HPP_