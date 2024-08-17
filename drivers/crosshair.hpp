#ifndef _SPRF_CROSSHAIR_HPP_
#define _SPRF_CROSSHAIR_HPP_

#include "engine/engine.hpp"

namespace SPRF {

/*class CrosshairCommands : public DevConsoleCommand{
    private:

};*/

class Crosshair : public Component {
  private:
    float m_display_width;
    float m_display_height;

  public:
    void init() {
        m_display_height = GetDisplayHeight();
        m_display_width = GetDisplayWidth();
        if (!KEY_EXISTS(game_settings.float_values, "crosshair_x_size")) {
            game_settings.float_values["crosshair_x_size"] = 0.75;
        }
        if (!KEY_EXISTS(game_settings.float_values, "crosshair_y_size")) {
            game_settings.float_values["crosshair_y_size"] = 0.75;
        }
        if (!KEY_EXISTS(game_settings.float_values, "crosshair_thickness")) {
            game_settings.float_values["crosshair_thickness"] = 0.25;
        }
        if (!KEY_EXISTS(game_settings.color_values, "crosshair_color")) {
            game_settings.color_values["crosshair_color"] =
                raylib::Color::Green();
        }
    }

    void update() {
        m_display_height = GetDisplayHeight();
        m_display_width = GetDisplayWidth();
    }

    void draw2D() {
        if (!game_info.dev_console_active) {
            float x_size =
                (game_settings.float_values["crosshair_x_size"] / 100.0f) *
                m_display_width;
            float y_size =
                (game_settings.float_values["crosshair_y_size"] / 100.0f) *
                m_display_width;
            float thickness =
                (game_settings.float_values["crosshair_thickness"] / 100.0f) *
                m_display_width;
            raylib::Color color = game_settings.color_values["crosshair_color"];
            DrawLineEx(raylib::Vector2(m_display_width * 0.5,
                                       m_display_height * 0.5 - y_size),
                       raylib::Vector2(m_display_width * 0.5,
                                       m_display_height * 0.5 + y_size),
                       thickness, color);
            DrawLineEx(raylib::Vector2(m_display_width * 0.5 - x_size,
                                       m_display_height * 0.5),
                       raylib::Vector2(m_display_width * 0.5 + x_size,
                                       m_display_height * 0.5),
                       thickness, color);
        }
    }
};

} // namespace SPRF

#endif // _SPRF_CROSSHAIR_HPP_