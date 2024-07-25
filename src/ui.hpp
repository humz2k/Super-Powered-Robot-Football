#ifndef _SPRF_UI_HPP
#define _SPRF_UI_HPP

#include "ecs.hpp"
#include "raylib-cpp.hpp"

#include <sstream>
#include <unordered_map>

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

    void set_transparency(int transparency) { m_color.a = transparency; }
};

class UIText : public UIElement {
  protected:
    std::string m_text;

  private:
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

class UITextInputBox : public UIText {
  private:
    raylib::Color m_background_passive_color;
    raylib::Color m_background_selected_color;
    raylib::Color m_background_hover_color;
    raylib::Color m_text_color;
    bool m_selected = false;
    UIWindow m_background;

  public:
    UITextInputBox(raylib::Font& font, raylib::Vector2 top_left,
                   raylib::Vector2 bottom_right,
                   raylib::Color background_passive_color = DARKGRAY,
                   raylib::Color background_selected_color = GREEN,
                   raylib::Color background_hover_color = BLACK,
                   raylib::Color text_color = WHITE)
        : UIText(font, top_left, bottom_right.y - top_left.y, "", text_color),
          m_background_passive_color(background_passive_color),
          m_background_selected_color(background_selected_color),
          m_background_hover_color(background_hover_color),
          m_text_color(text_color),
          m_background(top_left, bottom_right, background_passive_color) {}

    void set_selected(bool selected) {
        m_selected = selected;
        if (m_selected) {
            m_background.color(m_background_selected_color);
        } else {
            m_background.color(m_background_passive_color);
        }
    }

    virtual void on_submit(std::string input) {}

    void update(raylib::Vector2 offset = raylib::Vector2(0, 0)) {
        if (IsMouseButtonPressed(0)) {
            set_selected(mouse_over(offset));
        }

        m_background.color(m_background_passive_color);
        if (mouse_over(offset)) {
            m_background.color(m_background_hover_color);
        }

        if (!m_selected)
            return;

        m_background.color(m_background_selected_color);

        std::string current_string = this->m_text;

        if (IsKeyPressed(KEY_BACKSPACE)) {
            if (current_string.size() > 0) {
                current_string =
                    current_string.substr(0, current_string.size() - 1);
            }
        }

        int key = GetCharPressed();
        if ((key >= 32) && (key <= 125) && (key != KEY_GRAVE)) {
            current_string.append(1, (char)key);
        }

        if (IsKeyPressed(KEY_ENTER)) {
            this->on_submit(current_string);
            current_string = "";
        }

        update_text(current_string);
    }

    void draw(raylib::Vector2 offset = raylib::Vector2(0, 0)) {
        m_background.draw(offset);
        UIText::draw(offset);
    }

    bool mouse_over(raylib::Vector2 offset = raylib::Vector2(0, 0)) {
        return m_background.mouse_over(offset);
    }
};

class UIManager : public Component {
  public:
};

} // namespace SPRF

#endif // _SPRF_UI_HPP