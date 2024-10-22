#ifndef _SPRF_CONSOLE_HPP_
#define _SPRF_CONSOLE_HPP_

#include "ecs.hpp"
#include "imgui/imgui.h"
#include "imgui/rlImGui.h"
#include "log_manager.hpp"
//#include "raylib-cpp.hpp"
#include "ui.hpp"

#include <fstream>
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
    vec2 m_offset = vec2(0, 0);
    bool m_clicked = false;
    float m_scroll_speed;
    std::unordered_map<std::string, std::shared_ptr<DevConsoleCommand>>
        m_commands;
    std::unordered_map<std::string, CommandAlias> m_aliases;
    std::vector<std::string> m_inputs;
    std::unordered_map<KeyboardKey, std::string> m_binds;
    int m_input_pointer = 0;
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
        : UITextInputBox(&m_font, vec2(0.12, 0.86),
                         vec2(0.88, 0.88)),
          m_background(vec2(0.1, 0.1), vec2(0.9, 0.9),
                       Color(23, 27, 33, m_transparency)),
          m_foreground(vec2(0.12, 0.12), vec2(0.88, 0.86),
                       Color(30, 35, 43, m_transparency)),
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
                       vec2(0.12, start + ((float)i) * text_height),
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

    void run_command(std::string command,
                     std::vector<std::string> args = std::vector<std::string>(),
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

    inline void ltrim(std::string& s) {
        s.erase(s.begin(),
                std::find_if(s.begin(), s.end(), [](unsigned char ch) {
                    return !std::isspace(ch);
                }));
    }

    // trim from end (in place)
    inline void rtrim(std::string& s) {
        s.erase(std::find_if(s.rbegin(), s.rend(),
                             [](unsigned char ch) { return !std::isspace(ch); })
                    .base(),
                s.end());
    }

    void submit(std::string input, bool silent = false) {
        if (!silent) {
            TraceLog(LOG_INFO, input.c_str());
        }
        ltrim(input);
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

    void exec(std::string filename) {
        std::ifstream file(filename);
        std::string line;

        if (file.is_open()) {
            while (getline(file, line)) {
                if (line.size() > 0)
                    submit(line, true);
            }
            file.close();
        } else {
            TraceLog(LOG_ERROR, "couldn't open file %s", filename.c_str());
        }
    }

    void on_submit(std::string input) {
        if (input != "") {
            m_inputs.push_back(input);
            m_input_pointer = m_inputs.size();
        }
        submit(input);
    }

    template <class T, typename... Args>
    void add_command(std::string name, Args... args) {
        m_commands[name] = std::make_shared<T>(*this, args...);
        // return m_commands[name];
    }

    void add_bind(KeyboardKey key, std::string command) {
        m_binds[key] = command;
    }

    void update_binds() {
        for (auto& i : m_binds) {
            if (i.first == KEY_NULL) {
                if (GetMouseWheelMove() != 0.0f) {
                    run_command(i.second);
                }
                continue;
            }
            if (IsKeyDown(i.first)) {
                run_command(i.second);
            }
        }
    }

    void update() {
        if (IsKeyPressed(KEY_GRAVE)) {
            m_enabled = !m_enabled;
            if (m_enabled) {
                set_selected(true);
            }
        }
        game_info.dev_console_active = m_enabled;
        if (!m_enabled) {
            update_binds();
            return;
        }
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
            m_input_pointer++;
            if ((m_input_pointer >= 0) && (m_input_pointer < m_inputs.size())) {
                update_text(m_inputs[m_input_pointer]);
            } else {
                m_input_pointer = m_inputs.size();
                update_text("");
            }
        }
        if (IsKeyPressed(KEY_UP)) {
            m_input_pointer--;
            if ((m_input_pointer >= 0) && (m_input_pointer < m_inputs.size())) {
                update_text(m_inputs[m_input_pointer]);
            } else {
                m_input_pointer = 0;
            }
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
            Color out_color;
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
                } else if ((msg.source == "GAME") || (msg.source == "LUA")) {
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

    void draw_editor() {
        ImGui::Text("DevConsole");
        if (ImGui::TreeNode("commands")) {
            for (auto& i : m_commands) {
                ImGui::Text("%s", i.first.c_str());
            }
            ImGui::TreePop();
        }
        if (ImGui::TreeNode("aliases")) {
            ImGui::BeginTable("aliases_table", 2);
            ImGui::TableNextColumn();
            ImGui::Text("alias");
            ImGui::TableNextColumn();
            ImGui::Text("command");
            ImGui::TableNextRow();
            for (auto& i : m_aliases) {
                std::string alias_command = i.second.command;
                for (auto& i : i.second.args) {
                    alias_command += " ";
                    alias_command += i;
                }
                ImGui::TableNextColumn();
                ImGui::Text("%s", i.first.c_str());
                ImGui::TableNextColumn();
                ImGui::Text("%s", alias_command.c_str());
                ImGui::TableNextRow();
            }
            ImGui::EndTable();
            ImGui::TreePop();
        }
        if (ImGui::TreeNode("binds")) {
            ImGui::BeginTable("binds_table", 2);
            ImGui::TableNextColumn();
            ImGui::Text("bind");
            ImGui::TableNextColumn();
            ImGui::Text("command");
            ImGui::TableNextRow();
            for (auto& i : m_binds) {
                std::string key_name = "";
                auto key = i.first;
                if (key == KEY_NULL) {
                    key_name = "mwheel";
                } else if (key == KEY_LEFT) {
                    key_name = "left_arrow";
                } else if (key == KEY_RIGHT) {
                    key_name = "right_arrow";
                } else if (key == KEY_UP) {
                    key_name = "up_arrow";
                } else if (key == KEY_DOWN) {
                    key_name = "down_arrow";
                } else if (key == KEY_SPACE) {
                    key_name = "space";
                } else {
                    key_name = std::string(1, std::tolower(((char)key)));
                }
                ImGui::TableNextColumn();
                ImGui::Text("%s", key_name.c_str());
                ImGui::TableNextColumn();
                ImGui::Text("%s", i.second.c_str());
                ImGui::TableNextRow();
            }
            ImGui::EndTable();
            ImGui::TreePop();
        }
    }
};

} // namespace SPRF

#endif // _SPRF_CONSOLE_HPP_