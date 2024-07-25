#ifndef _SPRF_UI_HPP
#define _SPRF_UI_HPP

#include "ecs.hpp"
#include "raylib-cpp.hpp"

namespace SPRF {

class UIElement {
  public:
    virtual ~UIElement() {}

    float display_width() {
        if (IsWindowFullscreen()) {
            return GetRenderWidth();
        }
        return GetScreenWidth();
    }

    float display_height() {
        if (IsWindowFullscreen()) {
            return GetRenderHeight();
        }
        return GetScreenHeight();
    }

    raylib::Vector2 display_size() {
        return raylib::Vector2(display_width(), display_height());
    }

    raylib::Vector2 relative_to_actual(raylib::Vector2 coord) {
        return display_size() * coord;
    }

    virtual void draw(raylib::Vector2 offset = raylib::Vector2(0, 0)) {}
};

class UIWindow : public UIElement {
  private:
    raylib::Vector2 m_top_left;
    raylib::Vector2 m_bottom_right;
    raylib::Color m_color;

  public:
    UIWindow(raylib::Vector2 top_left, raylib::Vector2 bottom_right,
             raylib::Color color)
        : m_top_left(top_left), m_bottom_right(bottom_right), m_color(color) {}

    raylib::Rectangle rect() {
        raylib::Vector2 top_left = relative_to_actual(m_top_left);
        raylib::Vector2 bottom_right = relative_to_actual(m_bottom_right);
        raylib::Vector2 size = bottom_right - top_left;
        return raylib::Rectangle(top_left, size);
    }

    raylib::Color color() const { return m_color; }

    raylib::Color color(raylib::Color new_color) {
        m_color = new_color;
        return m_color;
    }

    void draw(raylib::Vector2 offset = raylib::Vector2(0, 0)) {
        auto r = rect();
        r.SetPosition(r.GetPosition() + offset);
        r.Draw(m_color);
    }

    bool mouse_over(raylib::Vector2 offset = raylib::Vector2(0, 0)) {
        auto r = rect();
        r.SetPosition(r.GetPosition() + offset);
        return r.CheckCollision(GetMousePosition());
    }
};

class UIText : public UIElement {
  private:
    std::string m_text;
    raylib::Vector2 m_pos;
    float m_height;
    raylib::Color m_color;
    raylib::Font& m_font;

  public:
    UIText(raylib::Font& font, raylib::Vector2 pos, float height,
           std::string text = "", raylib::Color color = GREEN)
        : m_text(text), m_pos(pos), m_height(height), m_color(color),
          m_font(font) {}

    void update_text(std::string text) { m_text = text; }

    void draw(raylib::Vector2 offset = raylib::Vector2(0, 0)) {
        raylib::Vector2 coord = relative_to_actual(m_pos) + offset;
        float height = m_height * display_height();
        raylib::Vector2 first_size =
            MeasureTextEx(m_font, m_text.c_str(), 20, 1);
        DrawTextEx(m_font, m_text, coord, 20 * (height / first_size.y), 1,
                   m_color);
    }

    void update_color(raylib::Color color) { m_color = color; }
};

class UIConsole : public Component {
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
    UIConsole(int n_lines = 50, float scroll_speed = 2)
        : m_background(raylib::Vector2(0.1, 0.1), raylib::Vector2(0.9, 0.9),
                       raylib::Color(23, 27, 33, 230)),
          m_foreground(raylib::Vector2(0.12, 0.12), raylib::Vector2(0.88, 0.83),
                       raylib::Color(30, 35, 43, 240)),
          m_font("/Users/humzaqureshi/GitHub/Super-Powered-Robot-Football/src/"
                 "JetBrainsMono-Regular.ttf"),
          m_scroll_speed(scroll_speed) {
        float total_height = 0.83 - 0.12;
        float text_height = total_height / ((float)n_lines);
        float start = 0.12;
        for (int i = 0; i < n_lines; i++) {
            m_text_boxes.push_back(UIText(
                m_font, raylib::Vector2(0.12, start + ((float)i) * text_height),
                text_height * 0.95, "test"));
        }
    }

    void update() {
        if (IsKeyPressed(KEY_GRAVE))
            m_enabled = !m_enabled;
        if (!m_enabled)
            return;
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
    }
};

class UIManager : public Component {
  public:
};

} // namespace SPRF

#endif // _SPRF_UI_HPP