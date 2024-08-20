#ifndef _SPRF_ENGINE_HPP_
#define _SPRF_ENGINE_HPP_

#include "raylib-cpp.hpp"
#include <functional>
#include <iostream>
#include <memory>
#include <mini/ini.hpp>
#include <string>
#include <vector>

#include "camera.hpp"
#include "ecs.hpp"
#include "model.hpp"
#include "rss.hpp"
#include "shaders.hpp"

#include "base.hpp"

#include "console.hpp"
#include "loading_screen.hpp"
#include "scripting/scripting.hpp"
#include "soloud.h"
#include "soloud_wav.h"
#include "ui.hpp"

#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)

namespace SPRF {

extern bool game_should_quit;

void quit();

class DirectoryChanger {
  public:
    DirectoryChanger() {
        TraceLog(LOG_INFO, "working in %s", GetWorkingDirectory());
        TraceLog(LOG_INFO, "cd %s", GetApplicationDirectory());
        ChangeDirectory(GetApplicationDirectory());
        TraceLog(LOG_INFO, "now working in %s", GetWorkingDirectory());
    }
};

class Game : public Logger {
  private:
    raylib::Window m_window;
    DirectoryChanger m_dir_changer;
    raylib::RenderTexture2D m_render_view;
    int m_fps_max;
    std::shared_ptr<Scene> m_current_scene;

    std::function<void()> m_load_next;
    bool m_scene_to_load = false;

  public:
    LoadingScreen loading_screen;
    float deltaTime = 0;
    SoLoud::Soloud soloud;
    // ScriptingManager scripting;

    raylib::Rectangle render_rect() {
        raylib::Rectangle out(
            -vec2(m_render_view.GetTexture().GetSize()));
        out.SetWidth(-out.GetWidth());
        return out;
    }

    raylib::Rectangle window_rect() {
        if (IsWindowFullscreen()) {
            return raylib::Rectangle(0, 0, GetRenderWidth(), GetRenderHeight());
        }
        return m_window.GetSize();
    }

    bool fullscreen() { return IsWindowFullscreen(); }

    bool fullscreen(bool v) {
        if (IsWindowFullscreen()) {
            if (!v) {
                ToggleFullscreen();
            }
        } else {
            if (v) {
                ToggleFullscreen();
            }
        }
        return fullscreen();
    }

    Game() {}

    Game(int window_width, int window_height, std::string window_name,
         int render_width, int render_height, int fps_max = 200,
         bool start_fullscreen = false, float volume = 1.0)
        : Logger("GAME"), m_window(window_width, window_height, window_name),
          m_render_view(render_width, render_height), m_fps_max(fps_max) {
        log(LOG_INFO, "initializing soloud");
        soloud.init();
        soloud.setGlobalVolume(volume);
        fullscreen(start_fullscreen);
        // monitor size in inches
        int monitor = GetCurrentMonitor();
        game_info.monitor_size =
            vec2(GetMonitorPhysicalWidth(monitor),
                            GetMonitorPhysicalHeight(monitor)) *
            0.039;

        log(LOG_INFO, "Physical Monitor Size: %gx%g", game_info.monitor_size.x,
            game_info.monitor_size.y);
        SoLoud::Wav wav;
        log(LOG_INFO, "loading startup.wav");
        if (wav.load("assets/startup.wav") != SoLoud::SO_NO_ERROR) {
            log(LOG_ERROR, "loading startup.wav failed");
        }
        soloud.play(wav);
        loading_screen.draw_splash_screen();
        loading_screen.draw();
        log(LOG_INFO, "Launching game");
        SetTargetFPS(m_fps_max);
        m_current_scene = std::make_shared<Scene>();
        m_current_scene->init();
        SetTextureFilter(m_render_view.texture, TEXTURE_FILTER_TRILINEAR);
        game_info.load_debug_font();
    }

    ~Game() {
        log(LOG_INFO, "Closing game");
        game_info.unload_debug_font();
        m_current_scene->destroy();
        log(LOG_INFO, "deinitializing soloud");
        soloud.deinit();
        log(LOG_INFO, "Closed game");
        log(LOG_INFO, "mem usage: current = %g gb, peak = %g gb",
            1e-9 * (double)getCurrentRSS(), 1e-9 * (double)getPeakRSS());
    }

    void change_render_size(int render_width, int render_height) {
        m_render_view = raylib::RenderTexture2D(render_width, render_height);
    }

    void change_window_size(int window_width, int window_height) {
        SetWindowSize(window_width, window_height);
    }

    bool running() { return (!game_should_quit); }

    template <class T, typename... Args> void load_scene(Args... args) {
        m_load_next = [this, args...]() {
            loading_screen.draw();
            log(LOG_INFO, "Loading scene %s", typeid(T).name());
            m_current_scene->destroy();
            auto out = std::make_shared<T>(this, args...);
            m_current_scene = out;
            m_current_scene->init();
        };
        m_scene_to_load = true;
    }

    void draw() {
        BeginDrawing();
        m_current_scene->draw(m_render_view);
        m_render_view.GetTexture().Draw(render_rect(), window_rect());
        m_current_scene->draw2D();
        std::string fps_string = std::to_string(GetFPS()) + " fps";
        rlDrawText(fps_string.c_str(),
                   GetDisplayWidth() - MeasureText(fps_string.c_str(), 20), 0,
                   20, GREEN);
        game_info.draw_debug();
        EndDrawing();
        game_info.frame_time = GetFrameTime();
        if (m_scene_to_load) {
            m_current_scene->on_close();
            m_load_next();
            m_scene_to_load = false;
            log(LOG_INFO, "mem usage: current = %g gb, peak = %g gb",
                1e-9 * (double)getCurrentRSS(), 1e-9 * (double)getPeakRSS());
        } else if (m_current_scene->should_close()) {
            m_current_scene->on_close();
        }
    }

    vec2 render_size() {
        return m_render_view.GetTexture().GetSize();
    }
};

extern Game* game;

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

class FPSMaxCommand : public DevConsoleCommand {
  public:
    using DevConsoleCommand::DevConsoleCommand;
    void handle(std::vector<std::string>& args) {
        if (args.size() > 0) {
            int max_fps = std::stoi(args[0]);
            SetTargetFPS(max_fps);
        }
    }
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

class LambdaCommand : public DevConsoleCommand {
  public:
    using DevConsoleCommand::DevConsoleCommand;
    void handle(std::vector<std::string>& args) {
        if (args.size() == 0) {
            TraceLog(LOG_CONSOLE, "Error - not enough arguments");
            return;
        }
        int nargs = std::stoi(args[0]);
        if (args.size() < 2 * nargs + 2) {
            TraceLog(LOG_CONSOLE, "Error - not enough arguments");
            return;
        }
        std::vector<std::string> arg_names;
        for (int i = 1; i < nargs + 1; i++) {
            arg_names.push_back(args[i]);
        }
        std::unordered_map<std::string, std::string> arguments;
        int start = args.size() - nargs;
        for (int i = 0; i < nargs; i++) {
            arguments[arg_names[i]] = args[i + start];
        }

        std::vector<std::string> transformed_arguments;
        for (int i = nargs + 1; i < start; i++) {
            if (KEY_EXISTS(arguments, args[i])) {
                transformed_arguments.push_back(arguments[args[i]]);
            } else {
                transformed_arguments.push_back(args[i]);
            }
        }

        std::string command = transformed_arguments[0];
        std::vector<std::string> final_args;
        for (int i = 1; i < transformed_arguments.size(); i++) {
            final_args.push_back(transformed_arguments[i]);
        }
        std::string running = command + " ";
        for (auto& i : final_args) {
            running += i + " ";
        }
        TraceLog(LOG_CONSOLE, "%s", running.c_str());
        dev_console().run_command(command, final_args);
    }
};

class DoCommand : public DevConsoleCommand {
  public:
    using DevConsoleCommand::DevConsoleCommand;
    void handle(std::vector<std::string>& args) {
        std::string combined = "";
        for (auto& i : args) {
            combined += i + " ";
        }
        std::istringstream iss(combined);
        std::string s;

        while (getline(iss, s, ';')) {
            if (s.size() > 0) {
                dev_console().submit(s);
            }
        }
    }
};

class ConfigCommand : public DevConsoleCommand {
  public:
    using DevConsoleCommand::DevConsoleCommand;
    void handle(std::vector<std::string>& args) {
        if (args.size() < 2)
            return;
        if (args[0] == "float") {
            if (args.size() == 3) {
                game_settings.float_values[args[1]] = std::stof(args[2]);
            }
            TraceLog(LOG_CONSOLE, "%s = %g", args[1].c_str(),
                     game_settings.float_values[args[1]]);
            return;
        }
        if (args[0] == "color") {
            if (args.size() == 6) {
                game_settings.color_values[args[1]] =
                    raylib::Color(std::stoi(args[2]), std::stoi(args[3]),
                                  std::stoi(args[4]), std::stoi(args[5]));
            }
            TraceLog(LOG_CONSOLE, "%s = %d %d %d %d", args[1].c_str(),
                     game_settings.color_values[args[1]].r,
                     game_settings.color_values[args[1]].g,
                     game_settings.color_values[args[1]].b,
                     game_settings.color_values[args[1]].a);
            return;
        }
        if (args[0] == "int") {
            if (args.size() == 3) {
                game_settings.int_values[args[1]] = std::stoi(args[2]);
            }
            TraceLog(LOG_CONSOLE, "%s = %d", args[1].c_str(),
                     game_settings.int_values[args[1]]);
            return;
        }
    }
};

class BindCommand : public DevConsoleCommand {
  public:
    using DevConsoleCommand::DevConsoleCommand;
    void handle(std::vector<std::string>& args) {
        if (args.size() != 2)
            return;
        std::string& key_name = args[0];
        std::string& command = args[1];
        if (key_name == "mwheel") {
            dev_console().add_bind(KEY_NULL, command);
        } else if (key_name == "space") {
            dev_console().add_bind(KEY_SPACE, command);
        } else if (key_name == "left_arrow") {
            dev_console().add_bind(KEY_LEFT, command);
        } else if (key_name == "right_arrow") {
            dev_console().add_bind(KEY_RIGHT, command);
        } else if (key_name == "up_arrow") {
            dev_console().add_bind(KEY_UP, command);
        } else if (key_name == "down_arrow") {
            dev_console().add_bind(KEY_DOWN, command);
        } else if (key_name.size() == 1) {
            dev_console().add_bind((KeyboardKey)std::toupper(key_name[0]),
                                   command);
        } else {
            return;
        }
        TraceLog(LOG_CONSOLE, "bind %s %s", key_name.c_str(), command.c_str());
    }
};

class FullscreenCommand : public DevConsoleCommand {
  public:
    using DevConsoleCommand::DevConsoleCommand;
    void handle(std::vector<std::string>& args) {
        if (args.size() > 1) {
            return;
        } else if (args.size() == 0) {
            TraceLog(LOG_CONSOLE, "fullscreen %d", game->fullscreen());
            return;
        } else {
            if (args[0] == "1") {
                game->fullscreen(true);
            } else if (args[0] == "0") {
                game->fullscreen(false);
            }
            TraceLog(LOG_CONSOLE, "fullscreen %d", game->fullscreen());
            return;
        }
    }
};

class RenderSizeCommand : public DevConsoleCommand {
  public:
    using DevConsoleCommand::DevConsoleCommand;
    void handle(std::vector<std::string>& args) {
        if (args.size() == 0) {
            auto tmp = game->render_size();
            TraceLog(LOG_CONSOLE, "render_size %d %d", (int)tmp.x, (int)tmp.y);
            return;
        }
        if (args.size() != 2) {
            return;
        }
        int rx = std::stoi(args[0]);
        int ry = std::stoi(args[1]);
        if (!((rx > 0) && (ry > 0))) {
            return;
        }
        game->change_render_size(rx, ry);
        auto tmp = game->render_size();
        TraceLog(LOG_CONSOLE, "render_size %d %d", (int)tmp.x, (int)tmp.y);
    }
};

class WindowSizeCommand : public DevConsoleCommand {
  public:
    using DevConsoleCommand::DevConsoleCommand;
    void handle(std::vector<std::string>& args) {
        if (args.size() == 0) {
            TraceLog(LOG_CONSOLE, "window_size %d %d", GetDisplayWidth(),
                     GetDisplayHeight());
            return;
        }
        if (args.size() != 2) {
            return;
        }
        int wx = std::stoi(args[0]);
        int wy = std::stoi(args[1]);
        if (!((wx > 0) && (wy > 0))) {
            return;
        }
        game->change_window_size(wx, wy);
        TraceLog(LOG_CONSOLE, "window_size %d %d", GetDisplayWidth(),
                 GetDisplayHeight());
    }
};

class MemUsageCommand : public DevConsoleCommand {
  public:
    using DevConsoleCommand::DevConsoleCommand;
    void handle(std::vector<std::string>& args) {
        TraceLog(LOG_CONSOLE, "mem usage: current = %g gb, peak = %g gb",
                 1e-9 * (double)getCurrentRSS(), 1e-9 * (double)getPeakRSS());
    }
};

class VolumeCommand : public DevConsoleCommand {
  public:
    using DevConsoleCommand::DevConsoleCommand;
    void handle(std::vector<std::string>& args) {
        if (args.size() > 1)
            return;
        if (args.size() == 1) {
            game->soloud.setGlobalVolume(std::stof(args[0]));
        }
        TraceLog(LOG_CONSOLE, "volume %g", game->soloud.getGlobalVolume());
    }
};

class PassCommand : public DevConsoleCommand {
  public:
    using DevConsoleCommand::DevConsoleCommand;
    void handle(std::vector<std::string>& args) {}
};

class DefaultDevConsole : public DevConsole {
  public:
    DefaultDevConsole() {
        add_command<EchoCommand>("echo");
        add_command<QuitCommand>("quit");
        add_command<FnCommand>("fn");
        add_command<FPSMaxCommand>("fps_max");
        add_command<ConfigCommand>("config");
        add_command<BindCommand>("bind");
        add_command<FullscreenCommand>("fullscreen");
        add_command<WindowSizeCommand>("window_size");
        add_command<RenderSizeCommand>("render_size");
        add_command<MemUsageCommand>("mem_usage");
        add_command<VolumeCommand>("volume");
        add_command<DoCommand>("do");
        add_command<LambdaCommand>("lambda");
        add_command<PassCommand>("pass");
    }

    void init() { exec("autoexec.cfg"); }
};

class DefaultScene : public Scene {
  private:
    DefaultDevConsole* m_dev_console;
    Game* m_game;

  public:
    DefaultScene(Game* game)
        : m_dev_console(this->create_entity("dev_console")
                            ->add_component<DefaultDevConsole>()),
          m_game(game) {
            scripting.refresh();
        }

    DefaultDevConsole* dev_console() { return m_dev_console; }

    Game* game() { return m_game; }
};

class UpdateInput : public DevConsoleCommand {
  private:
    bool* m_var;

  public:
    UpdateInput(DevConsole& console, bool* var)
        : DevConsoleCommand(console), m_var(var){};
    void handle(std::vector<std::string>& args) { *m_var = true; }
};

} // namespace SPRF

#endif // _SPRF_ENGINE_HPP_