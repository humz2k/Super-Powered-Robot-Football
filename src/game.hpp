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

class Game : public Logger {
  private:
    raylib::Window m_window;
    raylib::Rectangle m_window_rect;
    raylib::RenderTexture2D m_render_view;
    raylib::Rectangle m_render_rect;
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

    Game(int window_width, int window_height, std::string window_name,
         int render_width, int render_height, int fps_max = 200)
        : Logger("GAME"), m_window(window_width, window_height, window_name),
          m_window_rect(m_window.GetSize()),
          m_render_view(render_width, render_height),
          m_render_rect(render_rect()), m_fps_max(fps_max) {
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
        m_render_view.GetTexture().Draw(m_render_rect, m_window_rect);
        EndDrawing();
    }
};

} // namespace SPRF

#endif // _SPRF_GAME_HPP_