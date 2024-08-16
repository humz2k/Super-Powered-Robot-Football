#ifndef _SPRF_CONSOLE_HPP_
#define _SPRF_CONSOLE_HPP_

#include "ecs.hpp"
#include "log_manager.hpp"
#include "raylib-cpp.hpp"
#include "ui.hpp"

#include <sstream>
#include <unordered_map>

namespace SPRF {

class DevConsole;

class DevConsoleCommand {
  private:
    DevConsole& m_dev_console;

  public:
    DevConsoleCommand(DevConsole& dev_console) : m_dev_console(dev_console) {}
    DevConsole& dev_console() { return m_dev_console; }
    virtual ~DevConsoleCommand() {}
    virtual void handle(std::vector<std::string>& args) {}
};

struct CommandAlias {
    std::string command = "NULL";
    std::vector<std::string> args;
    CommandAlias(){};
    CommandAlias(std::string command_, std::vector<std::string> args_)
        : command(command_), args(args_) {}
};

class DevConsole : public Component, public UITextInputBox {
  private:
    int m_transparency = 230;
    int m_max_recursion_depth = 10;
    UIWindow m_background;
    UIWindow m_foreground;
    std::vector<UIText> m_text_boxes;
    int m_console_start = 0;
    raylib::Font m_font;
    bool m_enabled = false;
    raylib::Vector2 m_offset = raylib::Vector2(0, 0);
    bool m_clicked = false;
    float m_scroll_speed;
    std::unordered_map<std::string, std::shared_ptr<DevConsoleCommand>>
        m_commands;
    std::unordered_map<std::string, CommandAlias> m_aliases;
    // UITextInputBox m_input_box;

    void incr_console_start() {
        if ((log_manager.log_stack.size() - m_console_start) >
            (m_text_boxes.size())) {
            m_console_start++;
        }
    }

    void decr_console_start() {
        if (m_console_start > 0) {
            m_console_start--;
        }
    }

  public:
    DevConsole(int n_lines = 50, float scroll_speed = 2)
        : UITextInputBox(&m_font, raylib::Vector2(0.12, 0.86),
                         raylib::Vector2(0.88, 0.88)),
          m_background(raylib::Vector2(0.1, 0.1), raylib::Vector2(0.9, 0.9),
                       raylib::Color(23, 27, 33, m_transparency)),
          m_foreground(raylib::Vector2(0.12, 0.12), raylib::Vector2(0.88, 0.86),
                       raylib::Color(30, 35, 43, m_transparency)),
          m_font("assets/"
                 "JetBrainsMono-Regular.ttf",
                 128),
          m_scroll_speed(scroll_speed) {
        float total_height = 0.86 - 0.12;
        float text_height = total_height / ((float)n_lines);
        float start = 0.12;
        for (int i = 0; i < n_lines; i++) {
            m_text_boxes.push_back(
                UIText(&m_font,
                       raylib::Vector2(0.12, start + ((float)i) * text_height),
                       text_height * 0.95, "test"));
        }
        m_console_start = log_manager.log_stack.size() - m_text_boxes.size();
        game_info.dev_console_active = m_enabled;
    }

    bool enabled() { return m_enabled; }

    bool alias_exists(std::string command) {
        return m_aliases.find(command) != m_aliases.end();
    }

    bool command_exists(std::string command) {
        return m_commands.find(command) != m_commands.end();
    }

    CommandAlias
    evaluate_alias(std::string command,
                   std::vector<std::string> args = std::vector<std::string>(),
                   int depth = 0) {
        if (!alias_exists(command)) {
            return CommandAlias();
        }
        auto& alias = m_aliases[command];
        std::vector<std::string> final_args;
        final_args.reserve(alias.args.size() + args.size());
        for (auto& i : alias.args) {
            final_args.push_back(i);
        }
        for (auto& i : args) {
            final_args.push_back(i);
        }
        if (!alias_exists(alias.command)) {
            return CommandAlias(alias.command, final_args);
        }
        if (depth > m_max_recursion_depth) {
            TraceLog(LOG_CONSOLE, "Error - exceeded max recursion depth (%d)",
                     m_max_recursion_depth);
            return CommandAlias();
        }
        return evaluate_alias(alias.command, final_args);
    }

    void run_command(std::string command, std::vector<std::string> args,
                     int depth = 0) {
        if (command == "alias") {
            if (args.size() >= 2) {
                create_alias(
                    args[0], args[1],
                    std::vector<std::string>(args.begin() + 2, args.end()));
            } else {
                TraceLog(LOG_CONSOLE, "Error - not enough arguments");
            }
            return;
        }
        if (alias_exists(command)) {
            auto alias = evaluate_alias(command, args);
            run_command(alias.command, alias.args);
            return;
        }
        if (command_exists(command)) {
            m_commands[command]->handle(args);
        } else {
            TraceLog(LOG_CONSOLE, "Error - unknown command '%s'",
                     command.c_str());
        }
    }

    void create_alias(std::string alias, std::string command,
                      std::vector<std::string> arguments) {
        if (command_exists(alias)) {
            TraceLog(LOG_CONSOLE, "Error - command %s exists", alias.c_str());
            return;
        }
        m_aliases[alias] = CommandAlias(command, arguments);
    }

    void on_submit(std::string input) {
        TraceLog(LOG_INFO, input.c_str());
        std::istringstream iss(input);
        std::string s;
        std::vector<std::string> args;
        getline(iss, s, ' ');
        std::string command = s;

        while (getline(iss, s, ' ')) {
            if (s.size() > 0)
                args.push_back(s);
        }

        run_command(command, args);

        m_console_start = log_manager.log_stack.size() - m_text_boxes.size();
    }

    template <class T, typename... Args>
    void add_command(std::string name, Args... args) {
        m_commands[name] = std::make_shared<T>(*this, args...);
        // return m_commands[name];
    }

    void update() {
        if (IsKeyPressed(KEY_GRAVE)) {
            m_enabled = !m_enabled;
            if (m_enabled) {
                set_selected(true);
            }
        }
        game_info.dev_console_active = m_enabled;
        if (!m_enabled)
            return;
        UITextInputBox::update(m_offset);
        if (IsMouseButtonDown(0)) {
            if ((m_background.mouse_over(m_offset))) {
                m_clicked = true;
                m_transparency = 230;
            } else {
                m_transparency = 150;
            }
        }
        if (IsMouseButtonUp(0)) {
            m_clicked = false;
        }
        if (m_clicked) {
            m_offset += GetMouseDelta();
        }
        if (m_foreground.mouse_over(m_offset)) {
            int scroll_amount = (int)(GetMouseWheelMoveV().y * m_scroll_speed);
            int abs_scroll_amount = abs(scroll_amount);
            if (scroll_amount < 0) {
                for (int i = 0; i < abs_scroll_amount; i++) {
                    incr_console_start();
                }
            } else {
                for (int i = 0; i < abs_scroll_amount; i++) {
                    decr_console_start();
                }
            }
        }
        if (IsKeyPressed(KEY_DOWN)) {
            incr_console_start();
        }
        if (IsKeyPressed(KEY_UP)) {
            decr_console_start();
        }
    }

    void draw2D() {
        if (!m_enabled)
            return;
        m_background.set_transparency(m_transparency);
        m_foreground.set_transparency(m_transparency);
        m_background.draw(m_offset);
        m_foreground.draw(m_offset);
        assert(m_console_start >= 0);
        int n_lines = m_text_boxes.size() <
                              (log_manager.log_stack.size() - m_console_start)
                          ? m_text_boxes.size()
                          : (log_manager.log_stack.size() - m_console_start);
        for (int i = 0; i < n_lines; i++) {
            auto& text_box = m_text_boxes[i];
            LogMessage& msg = log_manager.log_stack[i + m_console_start];
            text_box.update_text(msg.message);
            raylib::Color out_color;
            switch (msg.type) {
            case LOG_ERROR:
            case LOG_WARNING:
            case LOG_CONSOLE:
                out_color = LIGHTGRAY;
                break;
            case LOG_DEBUG:
                out_color = RED;
                break;
            default:
                out_color = RAYWHITE;
                break;
            }
            if (msg.type == LOG_INFO) {
                if ((msg.source == "SHADER") || (msg.source == "TEXTURE") ||
                    (msg.source == "MODEL") || (msg.source == "IMAGE")) {
                    out_color = BLUE;
                } else if (msg.source == "GAME") {
                    out_color = GREEN;
                } else if ((msg.source == "GL") || (msg.source == "GLAD") ||
                           (msg.source == "RLGL") || (msg.source == "VAO") ||
                           (msg.source == "FBO")) {
                    out_color = DARKBLUE;
                }
            }
            out_color.a = m_transparency;
            text_box.update_color(out_color);
            text_box.draw(m_offset);
        }
        UITextInputBox::draw(m_offset);
    }
};

} // namespace SPRF

#endif // _SPRF_CONSOLE_HPP_