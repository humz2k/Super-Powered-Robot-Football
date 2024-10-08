#ifndef _SPRF_MODEL_HPP_
#define _SPRF_MODEL_HPP_

#include "base.hpp"
#include "ecs.hpp"
//#include "raylib-cpp.hpp"
#include "renderer.hpp"
#include <memory>

namespace SPRF {

class Model : public Component {
  private:
    RenderModel* m_model;
    bool m_enabled = true;

  public:
    Model(RenderModel* model) : m_model(model) {}

    void draw3D(mat4x4 parent_transform) {
        if (m_enabled)
            m_model->add_instance(parent_transform);
    }

    void enable() { m_enabled = true; }

    void disable() { m_enabled = false; }

    RenderModel* render_model() { return m_model; }
};

} // namespace SPRF

#endif // _SPRF_MODEL_HPP_