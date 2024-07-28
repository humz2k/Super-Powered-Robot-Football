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
    bool dev_console_active = false;
    GameInfo() {}

    template <class T>
    void draw_debug_var(std::string name, T var, int x, int y,
                        Color color = RED) {
        DrawTextEx(m_font, (name + ": " + std::to_string(var)),
                   raylib::Vector2(x, y), 10, 1, color);
    }

    void load_debug_font() {
        m_font = raylib::Font(
            "/Users/humzaqureshi/GitHub/Super-Powered-Robot-Football/src/"
            "JetBrainsMono-Regular.ttf");
    }

    void draw_debug() {
        draw_debug_var("visible_meshes", visible_meshes, 50, 50);
        draw_debug_var("hidden_meshes", hidden_meshes, 50, 60);
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