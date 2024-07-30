#ifndef _SPRF_LOADING_SCREEN_HPP_
#define _SPRF_LOADING_SCREEN_HPP_

#include "base.hpp"
#include "raylib-cpp.hpp"
#include <string>

namespace SPRF {

static void DrawOutlinedText(const char* text, int posX, int posY, int fontSize,
                             Color color, int outlineSize = 1,
                             Color outlineColor = BLACK) {
    rlDrawText(text, posX - outlineSize, posY - outlineSize, fontSize,
               outlineColor);
    rlDrawText(text, posX + outlineSize, posY - outlineSize, fontSize,
               outlineColor);
    rlDrawText(text, posX - outlineSize, posY + outlineSize, fontSize,
               outlineColor);
    rlDrawText(text, posX + outlineSize, posY + outlineSize, fontSize,
               outlineColor);
    rlDrawText(text, posX, posY, fontSize, color);
}

class LoadingScreen {
  private:
    raylib::Texture2D m_loading_texture;

    void draw_image() {
        auto dest_rect =
            raylib::Rectangle(0, 0, GetDisplayWidth(), GetDisplayHeight());
        auto source_rect = raylib::Rectangle(
            0, 0, m_loading_texture.width,
            m_loading_texture.width *
                (((float)GetDisplayHeight()) / ((float)GetDisplayWidth())));
        m_loading_texture.Draw(source_rect, dest_rect);
    }

    void draw_bar(float percent, std::string hint = "") {
        float display_width = GetDisplayWidth();
        float actual_x = display_width * 0.95;
        float x_start = display_width - actual_x;
        float width = actual_x * percent;
        float display_height = GetDisplayHeight();
        float actual_y = display_height * 0.05;
        float y_start = display_height - 2 * actual_y;
        float height = actual_y;
        if (hint != "") {
            float text_height =
                MeasureTextEx(GetFontDefault(), hint.c_str(), 20, 1).y;
            DrawOutlinedText(hint.c_str(), x_start, y_start - text_height, 20,
                             WHITE);
        }
        Color shadow = BLACK;
        shadow.a = 150;
        DrawRectangle(x_start - display_height * 0.005,
                      y_start + display_height * 0.005, width, height, shadow);
        DrawRectangle(x_start, y_start, width, height, WHITE);
    }

  public:
    LoadingScreen(std::string loading_image = "src/loading_screen.png")
        : m_loading_texture(raylib::Image(loading_image)) {}

    ~LoadingScreen() {}

    void draw() {
        BeginDrawing();
        ClearBackground(BLACK);
        draw_image();
        EndDrawing();
    }

    void draw(float percent) {
        BeginDrawing();
        ClearBackground(BLACK);
        draw_image();
        draw_bar(percent);
        EndDrawing();
    }

    void draw(float percent, std::string hint) {
        BeginDrawing();
        ClearBackground(BLACK);
        draw_image();
        draw_bar(percent, hint);
        EndDrawing();
    }

    void draw_splash_screen(float time = 5,
                            std::string logo_path = "src/logo.png") {
        float start = GetTime();
        float end = start + time;
        raylib::Texture2D logo_texture =
            raylib::Texture2D(raylib::Image(logo_path));
        auto dest_rect =
            raylib::Rectangle((((float)GetDisplayWidth()) / 2.0f) -
                                  0.5f * (((float)GetDisplayWidth()) / 2.0f),
                              (((float)GetDisplayHeight()) / 2.0f) -
                                  0.5f * (((float)GetDisplayWidth()) / 2.0f),
                              ((float)GetDisplayWidth()) / 2.0f,
                              ((float)GetDisplayWidth()) / 2.0f);
        auto source_rect = raylib::Rectangle(0, 0, m_loading_texture.width,
                                             m_loading_texture.height);
        while (GetTime() < end) {
            float alpha = sinf(((GetTime() - start) / time) * M_PI);
            Color color = WHITE;
            color.a = alpha * 255.0f;
            BeginDrawing();
            ClearBackground(BLACK);
            logo_texture.Draw(source_rect, dest_rect, (Vector2){0, 0}, 0,
                              color);
            EndDrawing();
            if (IsMouseButtonPressed(0) || (GetKeyPressed() != 0)) {
                break;
            }
        }
    }
};

} // namespace SPRF

#endif // _SPRF_LOADING_SCREEN_HPP_