#ifndef _SPRF_RENDERER_HPP_
#define _SPRF_RENDERER_HPP_

#include <memory>
#include "ecs.hpp"
#include "raylib-cpp.hpp"

namespace SPRF {

class Model : public Component {
  private:
    std::shared_ptr<raylib::Model> m_model;

  public:
    Model(std::shared_ptr<raylib::Model> model) : m_model(model) {}

    void draw3D(raylib::Matrix parent_transform) {
        raylib::Matrix transform =
            raylib::Matrix(m_model->GetTransform()) * parent_transform;
        for (int i = 0; i < m_model->meshCount; i++) {
            DrawMesh(m_model->meshes[i],
                     m_model->materials[m_model->meshMaterial[i]], transform);
        }
    }
};

} // namespace SPRF

#endif // _SPRF_RENDERER_HPP_