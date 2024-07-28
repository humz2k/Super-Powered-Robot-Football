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
        game_info.load_debug_font();
    }

    ~Game() {
        log(LOG_INFO, "Closing game");
        game_info.unload_debug_font();
        m_current_scene->destroy();
        log(LOG_INFO, "Closed game");
    }

    bool running() { return (!m_window.ShouldClose()) && (!game_should_quit); }

    template <class T, typename... Args>
    std::shared_ptr<T> load_scene(Args... args) {
        log(LOG_INFO, "Loading scene %s", typeid(T).name());
        m_current_scene->destroy();
        auto out = std::make_shared<T>(this, args...);
        m_current_scene = out;
        m_current_scene->init();
        return out;
    }

    void draw() {
        BeginDrawing();
        m_current_scene->draw(m_render_view);
        m_render_view.GetTexture().Draw(render_rect(), window_rect());
        m_current_scene->draw2D();
        std::string fps_string = std::to_string(GetFPS()) + " fps";
        DrawText(fps_string.c_str(),
                 GetDisplayWidth() - MeasureText(fps_string.c_str(), 10), 0, 10,
                 GREEN);
        game_info.draw_debug();
        EndDrawing();
        game_info.frame_time = GetFrameTime();
        if (m_current_scene->should_close()) {
            m_current_scene->on_close();
        }
    }
};

// extern Game* game;

class EchoCommand : public DevConsoleCommand {
  public:
    using DevConsoleCommand::DevConsoleCommand;
    void handle(std::vector<std::string>& args) {
        std::string out = "";
        for (auto& i : args) {
            out += i + " ";
        }
        TraceLog(LOG_CONSOLE, out.c_str());
    }
};

class QuitCommand : public DevConsoleCommand {
  public:
    using DevConsoleCommand::DevConsoleCommand;
    void handle(std::vector<std::string>& args) { quit(); }
};

class FnCommand : public DevConsoleCommand {
  public:
    using DevConsoleCommand::DevConsoleCommand;
    void handle(std::vector<std::string>& args) {
        if (args.size() < 3) {
            TraceLog(LOG_CONSOLE, "Error - not enough arguments");
            return;
        }
        auto& variable = args[0];

        auto& term = args[args.size() - 1];

        std::vector<std::string> out;
        std::string command = args[1];
        if (command == variable) {
            command = term;
        }
        for (int i = 2; i < args.size() - 1; i++) {
            if (args[i] != variable) {
                out.push_back(args[i]);
            } else {
                out.push_back(term);
            }
        }
        std::string running = command + " ";
        for (auto& i : out) {
            running += i + " ";
        }
        TraceLog(LOG_CONSOLE, "%s", running.c_str());
        dev_console().run_command(command, out);
    }
};

class DefaultDevConsole : public DevConsole {
  public:
    DefaultDevConsole() {
        add_command<EchoCommand>("echo");
        add_command<QuitCommand>("quit");
        add_command<FnCommand>("fn");
    }
};

class DefaultScene : public Scene {
  private:
    DefaultDevConsole* m_dev_console;
    Game* m_game;

  public:
    DefaultScene(Game* game)
        : m_dev_console(
              this->create_entity()->add_component<DefaultDevConsole>()),
          m_game(game) {}

    DefaultDevConsole* dev_console() { return m_dev_console; }

    Game* game() { return m_game; }
};

} // namespace SPRF

#endif // _SPRF_ENGINE_HPP_