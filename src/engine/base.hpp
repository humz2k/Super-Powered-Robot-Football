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

#define DEFAULT_FOVY (59.0f)
#define CSGO_MAGIC_SENSE_MULTIPLIER (360.0f / 16363.6364f)

#ifndef M_PI
#define M_PI (3.14159265358979323846)
#endif

#ifndef M_PI_2
#define M_PI_2 (3.14159265358979323846 * 0.5)
#endif

#define KEY_EXISTS(map, key) ((map).find((key)) != (map).end())

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

static float randrange(float min = 0, float max = 1) {
    float unscale = ((float)rand()) / ((float)RAND_MAX);
    return unscale * (max - min) + min;
}

static inline raylib::Vector2 GetDisplaySize() {
    return raylib::Vector2(GetDisplayWidth(), GetDisplayHeight());
}

class GameInfo {
  private:
    raylib::Font m_font;

  public:
    int visible_meshes = 0;
    int hidden_meshes = 0;
    float frame_time = 0;
    raylib::Vector2 monitor_size; // inches
    raylib::Vector3 position;
    raylib::Vector3 rotation;
    raylib::Vector3 velocity;
    int ping;
    float recieve_delta;
    float send_delta;
    bool dev_console_active = false;
    raylib::Vector2 mouse_sense_ratio = raylib::Vector2(0.0165, 0.022);
    int packet_queue_size = 0;
    GameInfo() {}

    template <class T>
    void draw_debug_var(std::string name, T var, int x, int y,
                        Color color = RED) {
        std::string text = (name + ": " + std::to_string(var));
        raylib::DrawTextEx(m_font, text.c_str(), raylib::Vector2(x, y), 20, 1,
                           color);
    }

    void draw_debug_var(std::string name, std::string var, int x, int y,
                        Color color = RED) {
        std::string text = (name + ": " + var);
        raylib::DrawTextEx(m_font, text.c_str(), raylib::Vector2(x, y), 20, 1,
                           color);
    }

    void draw_debug_var(std::string name, raylib::Vector3 var, int x, int y,
                        Color color = RED) {
        std::string text = (name + ": " + var.ToString());
        raylib::DrawTextEx(m_font, text.c_str(), raylib::Vector2(x, y), 20, 1,
                           color);
    }

    void load_debug_font() {
        m_font = raylib::Font("assets/"
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
        draw_debug_var("send_delta", send_delta, 0, 100);
        draw_debug_var("recv_delta", recieve_delta, 0, 120);
        draw_debug_var("packet_queue_size", packet_queue_size, 0, 140);
        draw_debug_var("visible_meshes", visible_meshes, 0, 200);
        draw_debug_var("hidden_meshes", hidden_meshes, 0, 220);
    }

    void unload_debug_font() { m_font.Unload(); }
};

extern GameInfo game_info;

static inline raylib::Vector2 GetRawMouseDelta() {
    raylib::Vector2 sc = raylib::Vector2(GetMouseDelta()) *
                         DEG2RAD; // * game_info.monitor_size * 400 * 0.022;
    return sc;
}

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