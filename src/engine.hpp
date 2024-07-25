#ifndef _SPRF_ENGINE_HPP_
#define _SPRF_ENGINE_HPP_

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

#include "console.hpp"
#include "ui.hpp"

namespace SPRF {

extern bool game_should_quit;

void quit();

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
        SetTextureFilter(m_render_view.texture, TEXTURE_FILTER_BILINEAR);
    }

    ~Game() {
        log(LOG_INFO, "Closing game");
        m_current_scene->destroy();
        log(LOG_INFO, "Closed game");
    }

    bool running() { return (!m_window.ShouldClose()) && (!game_should_quit); }

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

class EchoCommand : public DevConsoleCommand {
    void handle(std::vector<std::string>& args) {
        std::string out = "";
        for (auto& i : args) {
            out += i + " ";
        }
        TraceLog(LOG_CONSOLE, out.c_str());
    }
};

class QuitCommand : public DevConsoleCommand {
    void handle(std::vector<std::string>& args) { quit(); }
};

class DefaultDevConsole : public DevConsole {
  public:
    DefaultDevConsole() {
        add_command<EchoCommand>("echo");
        add_command<QuitCommand>("quit");
    }
};

class DefaultScene : public Scene {
  private:
    DefaultDevConsole& m_dev_console;

  public:
    DefaultScene()
        : m_dev_console(
              this->create_entity()->add_component<DefaultDevConsole>()) {}

    DefaultDevConsole& dev_console() { return m_dev_console; }
};

} // namespace SPRF

#endif // _SPRF_ENGINE_HPP_