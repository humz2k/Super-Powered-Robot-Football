#ifndef _SPRF_CAMERA_HPP_
#define _SPRF_CAMERA_HPP_

#include "ecs.hpp"
//#include "raylib-cpp.hpp"

namespace SPRF {

/**
 * @brief Component class representing a camera.
 */
class Camera : public Component {
  private:
    /** @brief Camera object */
    raylib::Camera3D m_camera;

    void update_camera() {
        auto transform = this->entity()->global_transform();
        auto feet = vec3(0, 0, 0).Transform(transform);
        auto head = vec3(0, 1, 0).Transform(transform);
        auto eyes = vec3(0, 0, 1).Transform(transform);
        m_camera.SetPosition(feet);
        m_camera.SetUp((head - feet).Normalize());
        m_camera.SetTarget(eyes);
    }

  public:
    /**
     * @brief Construct a new Camera object.
     */
    Camera(float fovy = DEFAULT_FOVY,
           CameraProjection projection = CAMERA_PERSPECTIVE)
        : m_camera(vec3(0, 0, 0), vec3(0, 0, 0),
                   vec3(0, 1, 0), fovy, projection) {}

    /**
     * @brief Set the camera as active in the scene.
     */
    void set_active() { this->entity()->scene()->set_active_camera(&m_camera); }

    bool active() {
        return (&m_camera) == this->entity()->scene()->get_active_camera();
    }

    void init() { update_camera(); }

    /**
     * @brief Update the camera's position and orientation.
     */
    void update() { update_camera(); }

    void draw_editor() {
        bool before = active();
        bool v = active();
        ImGui::Text("Camera");
        ImGui::Checkbox("enabled", &v);
        ImGui::Text("%g fov", m_camera.fovy);
        ImGui::Text("%.3f %.3f %.3f target", m_camera.target.x,
                    m_camera.target.y, m_camera.target.z);
        ImGui::Text("%.3f %.3f %.3f up", m_camera.up.x, m_camera.up.y,
                    m_camera.up.z);
        ImGui::Text("%.3f %.3f %.3f pos", m_camera.position.x,
                    m_camera.position.y, m_camera.position.z);

        if (v == before)
            return;
        if (v) {
            set_active();
        } else {
            this->entity()->scene()->set_active_camera();
        }
    }
};

} // namespace SPRF

#endif // _SPRF_CAMERA_HPP_