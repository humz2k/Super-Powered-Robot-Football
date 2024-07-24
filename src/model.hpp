#ifndef _SPRF_MODEL_HPP_
#define _SPRF_MODEL_HPP_

#include "ecs.hpp"
#include "raylib-cpp.hpp"
#include "renderer.hpp"
#include <memory>

namespace SPRF {

class Model : public Component {
  private:
    std::shared_ptr<RenderModel> m_model;

  public:
    Model(std::shared_ptr<RenderModel> model) : m_model(model) {}

    void draw3D(raylib::Matrix parent_transform) {
        m_model->add_instance(parent_transform);
    }
};

} // namespace SPRF

#endif // _SPRF_MODEL_HPP_