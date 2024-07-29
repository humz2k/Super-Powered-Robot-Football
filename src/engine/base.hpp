#ifndef _SPRF_BASE_HPP_
#define _SPRF_BASE_HPP_

#include "raylib-cpp.hpp"
#include <cassert>
#include <iostream>
#include <memory>
#include <string>
#include <typeindex>
#include <typeinfo>
#include <unordered_map>

#ifndef M_PI
    #define M_PI 3.14159265358979323846
#endif

namespace SPRF {

static int GetDisplayWidth() {
    if (IsWindowFullscreen()) {
        return GetRenderWidth();
    }
    return GetScreenWidth();
}

static int GetDisplayHeight() {
    if (IsWindowFullscreen()) {
        return GetRenderHeight();
    }
    return GetScreenHeight();
}

class GameInfo {
  private:
    raylib::Font m_font;

  public:
    int visible_meshes = 0;
    int hidden_meshes = 0;
    float frame_time = 0;
    raylib::Vector3 position;
    raylib::Vector3 rotation;
    raylib::Vector3 velocity;
    int ping;
    bool dev_console_active = false;
    GameInfo() {}

    template <class T>
    void draw_debug_var(std::string name, T var, int x, int y,
                        Color color = RED) {
        DrawTextEx(m_font, (name + ": " + std::to_string(var)),
                   raylib::Vector2(x, y), 20, 1, color);
    }

    void draw_debug_var(std::string name, std::string var, int x, int y,
                        Color color = RED) {
        DrawTextEx(m_font, (name + ": " + var), raylib::Vector2(x, y), 20, 1,
                   color);
    }

    void draw_debug_var(std::string name, raylib::Vector3 var, int x, int y,
                        Color color = RED) {
        DrawTextEx(m_font, (name + ": " + var.ToString()),
                   raylib::Vector2(x, y), 20, 1, color);
    }

    void load_debug_font() {
        m_font = raylib::Font(
            "/Users/humzaqureshi/GitHub/Super-Powered-Robot-Football/src/"
            "JetBrainsMono-Regular.ttf");
    }

    void draw_debug() {
        draw_debug_var("pos", position, 0, 0);
        draw_debug_var("vel", velocity, 0, 20);
        draw_debug_var("xz_vel_mag",
                       raylib::Vector3(velocity.x, 0, velocity.z).Length(), 0,
                       40);
        draw_debug_var("rot", rotation, 0, 60);
        draw_debug_var("ping", ping, 0, 80);
        draw_debug_var("visible_meshes", visible_meshes, 0, 100);
        draw_debug_var("hidden_meshes", hidden_meshes, 0, 120);
    }

    void unload_debug_font() { m_font.Unload(); }
};

extern GameInfo game_info;

class Logger {
  private:
    std::string m_log_name;

  public:
    Logger() : m_log_name("GAME") {}
    Logger(std::string log_name) : m_log_name(log_name) {}

    template <typename... Args> void log(int log_level, Args... args) {
        // typeid(*this).name();
        char msg[512];
        snprintf(msg, sizeof(msg), args...);
        TraceLog(log_level, "%s: %s", m_log_name.c_str(), msg);
    }
};

} // namespace SPRF

#endif // _SPRF_BASE_HPP_