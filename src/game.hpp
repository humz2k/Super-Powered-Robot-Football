#ifndef _SPRF_GAME_HPP_
#define _SPRF_GAME_HPP_

#include "raylib-cpp.hpp"
#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include "camera.hpp"
#include "ecs.hpp"
#include "model.hpp"
#include "shaders.hpp"

#include "base.hpp"

namespace SPRF {

struct LogMessage {
    std::string message;
    std::string source;
    int type;
    LogMessage(std::string message_, std::string source_, int type_)
        : message(message_), source(source_), type(type_) {}
};

void CustomLog(int msgType, const char* text, va_list args);

class LogManager {
  public:
    std::vector<LogMessage> log_stack;

    LogManager() { SetTraceLogCallback(CustomLog); }
};

extern LogManager log_manager;

class Game : public Logger {
  private:
    raylib::Window m_window;
    raylib::RenderTexture2D m_render_view;
    int m_fps_max;
    std::shared_ptr<Scene> m_current_scene;

  public:
    float deltaTime = 0;

    raylib::Rectangle render_rect() {
        raylib::Rectangle out(
            -raylib::Vector2(m_render_view.GetTexture().GetSize()));
        out.SetWidth(-out.GetWidth());
        return out;
    }

    raylib::Rectangle window_rect() {
        if (IsWindowFullscreen()) {
            return raylib::Rectangle(0, 0, GetRenderWidth(), GetRenderHeight());
        }
        return m_window.GetSize();
    }

    Game(int window_width, int window_height, std::string window_name,
         int render_width, int render_height, int fps_max = 200)
        : Logger("GAME"), m_window(window_width, window_height, window_name),
          m_render_view(render_width, render_height), m_fps_max(fps_max) {
        log(LOG_INFO, "Launching game");
        SetTargetFPS(m_fps_max);
        m_current_scene = std::make_shared<Scene>();
        m_current_scene->init();
    }

    ~Game() {
        log(LOG_INFO, "Closing game");
        m_current_scene->destroy();
        log(LOG_INFO, "Closed game");
    }

    bool running() { return !m_window.ShouldClose(); }

    template <class T, typename... Args> void load_scene(Args... args) {
        log(LOG_INFO, "Loading scene %s", typeid(T).name());
        m_current_scene->destroy();
        m_current_scene = std::make_shared<T>(args...);
        m_current_scene->init();
    }

    void draw() {
        BeginDrawing();
        m_current_scene->draw(m_render_view);
        m_render_view.GetTexture().Draw(render_rect(), window_rect());
        m_current_scene->draw2D();
        DrawFPS(10, 10);
        EndDrawing();
    }
};

} // namespace SPRF

#endif // _SPRF_GAME_HPP_