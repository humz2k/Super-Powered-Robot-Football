#ifndef _SPRF_CONSOLE_HPP_
#define _SPRF_CONSOLE_HPP_

#include "ecs.hpp"
#include "log_manager.hpp"
#include "raylib-cpp.hpp"
#include "ui.hpp"

#include <sstream>
#include <unordered_map>

namespace SPRF {

class DevConsoleCommand {
  public:
    DevConsoleCommand() {}
    virtual ~DevConsoleCommand() {}
    virtual void handle(std::vector<std::string>& args) {}
};

class DevConsole : public Component, public UITextInputBox {
  private:
    UIWindow m_background;
    UIWindow m_foreground;
    std::vector<UIText> m_text_boxes;
    int m_console_start = 0;
    raylib::Font m_font;
    bool m_enabled = true;
    raylib::Vector2 m_offset = raylib::Vector2(0, 0);
    bool m_clicked = false;
    float m_scroll_speed;
    std::unordered_map<std::string, std::shared_ptr<DevConsoleCommand>>
        m_commands;
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
        : m_background(raylib::Vector2(0.1, 0.1), raylib::Vector2(0.9, 0.9),
                       raylib::Color(23, 27, 33, 230)),
          m_foreground(raylib::Vector2(0.12, 0.12), raylib::Vector2(0.88, 0.86),
                       raylib::Color(30, 35, 43, 240)),
          m_font("/Users/humzaqureshi/GitHub/Super-Powered-Robot-Football/src/"
                 "JetBrainsMono-Regular.ttf"),
          m_scroll_speed(scroll_speed),
          UITextInputBox(m_font, raylib::Vector2(0.12, 0.86),
                         raylib::Vector2(0.88, 0.88)) {
        float total_height = 0.86 - 0.12;
        float text_height = total_height / ((float)n_lines);
        float start = 0.12;
        for (int i = 0; i < n_lines; i++) {
            m_text_boxes.push_back(UIText(
                m_font, raylib::Vector2(0.12, start + ((float)i) * text_height),
                text_height * 0.95, "test"));
        }
        m_console_start = log_manager.log_stack.size() - m_text_boxes.size();
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
        if (m_commands.find(command) == m_commands.end()) {
            TraceLog(LOG_CONSOLE, "Error - unknown command '%s'",
                     command.c_str());
        } else {
            m_commands[command]->handle(args);
        }
        m_console_start = log_manager.log_stack.size() - m_text_boxes.size();
    }

    template <class T, typename... Args>
    void add_command(std::string name, Args... args) {
        m_commands[name] = std::make_shared<T>(args...);
    }

    void update() {
        if (IsKeyPressed(KEY_GRAVE))
            m_enabled = !m_enabled;
        if (!m_enabled)
            return;
        UITextInputBox::update(m_offset);
        if ((m_background.mouse_over(m_offset)) && (IsMouseButtonDown(0)) &&
            (!m_foreground.mouse_over(m_offset))) {
            m_clicked = true;
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
            text_box.update_color(out_color);
            text_box.draw(m_offset);
        }
        UITextInputBox::draw(m_offset);
    }
};

} // namespace SPRF

#endif // _SPRF_CONSOLE_HPP_