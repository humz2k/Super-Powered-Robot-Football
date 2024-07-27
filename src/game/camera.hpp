#ifndef _SPRF_CAMERA_HPP_
#define _SPRF_CAMERA_HPP_

#include "ecs.hpp"
#include "raylib-cpp.hpp"

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
        auto feet = raylib::Vector3(0, 0, 0).Transform(transform);
        auto head = raylib::Vector3(0, 1, 0).Transform(transform);
        auto eyes = raylib::Vector3(0, 0, 1).Transform(transform);
        m_camera.SetPosition(feet);
        m_camera.SetUp((head - feet).Normalize());
        m_camera.SetTarget(eyes);
    }

  public:
    /**
     * @brief Construct a new Camera object.
     */
    Camera(float fovy = 45.0f, CameraProjection projection = CAMERA_PERSPECTIVE)
        : m_camera(raylib::Vector3(0, 0, 0), raylib::Vector3(0, 0, 0),
                   raylib::Vector3(0, 1, 0), fovy, projection) {}

    /**
     * @brief Set the camera as active in the scene.
     */
    void set_active() { this->entity()->scene()->set_active_camera(&m_camera); }

    void init() { update_camera(); }

    /**
     * @brief Update the camera's position and orientation.
     */
    void update() { update_camera(); }
};

} // namespace SPRF

#endif // _SPRF_CAMERA_HPP_