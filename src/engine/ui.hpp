#ifndef _SPRF_UI_HPP
#define _SPRF_UI_HPP

#include "base.hpp"
#include "ecs.hpp"
//#include "raylib-cpp.hpp"

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

    vec2 display_size() {
        return vec2(display_width(), display_height());
    }

    vec2 relative_to_actual(vec2 coord) {
        return display_size() * coord;
    }

    virtual void draw(vec2 offset = vec2(0, 0)) {}
};

class UIWindow : public UIElement {
  private:
    vec2 m_top_left;
    vec2 m_bottom_right;
    Color m_color;

  public:
    UIWindow(vec2 top_left, vec2 bottom_right,
             Color color)
        : m_top_left(top_left), m_bottom_right(bottom_right), m_color(color) {}

    raylib::Rectangle rect() {
        vec2 top_left = relative_to_actual(m_top_left);
        vec2 bottom_right = relative_to_actual(m_bottom_right);
        vec2 size = bottom_right - top_left;
        return raylib::Rectangle(top_left, size);
    }

    Color color() const { return m_color; }

    Color color(Color new_color) {
        m_color = new_color;
        return m_color;
    }

    void draw(vec2 offset = vec2(0, 0)) {
        auto r = rect();
        r.SetPosition(r.GetPosition() + offset);
        r.Draw(m_color);
    }

    bool mouse_over(vec2 offset = vec2(0, 0)) {
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
    vec2 m_pos;
    float m_height;
    Color m_color;
    raylib::Font* m_font;

  public:
    UIText(raylib::Font* font, vec2 pos, float height,
           std::string text = "", Color color = GREEN)
        : m_text(text), m_pos(pos), m_height(height), m_color(color),
          m_font(font) {}

    void update_text(std::string text) { m_text = text; }

    void draw(vec2 offset = vec2(0, 0)) {
        vec2 coord = relative_to_actual(m_pos) + offset;
        float height = m_height * display_height();
        vec2 first_size =
            MeasureTextEx(*m_font, m_text.c_str(), 20, 1);
        DrawTextEx(*m_font, m_text, coord, 20 * (height / first_size.y), 1,
                   m_color);
    }

    void update_color(Color color) { m_color = color; }
};

class UITextInputBox : public UIText {
  private:
    Color m_background_passive_color;
    Color m_background_selected_color;
    Color m_background_hover_color;
    Color m_text_color;
    bool m_selected = false;
    UIWindow m_background;

  public:
    UITextInputBox(raylib::Font* font, vec2 top_left,
                   vec2 bottom_right,
                   Color background_passive_color = DARKGRAY,
                   Color background_selected_color = GREEN,
                   Color background_hover_color = BLACK,
                   Color text_color = WHITE)
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

    void update(vec2 offset = vec2(0, 0)) {
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

    void draw(vec2 offset = vec2(0, 0)) {
        m_background.draw(offset);
        UIText::draw(offset);
    }

    bool mouse_over(vec2 offset = vec2(0, 0)) {
        return m_background.mouse_over(offset);
    }
};

class UITextComponent : public Component {
  private:
    raylib::Font* m_font;
    UIText m_text;

  public:
    UITextComponent(raylib::Font* font, vec2 pos, float height,
                    std::string text = "", Color color = GREEN)
        : m_font(font), m_text(m_font, pos, height, text, color) {
        TraceLog(LOG_INFO, "setting height = %g", height);
    }

    void draw2D() { m_text.draw(); }
};

} // namespace SPRF

#endif // _SPRF_UI_HPP