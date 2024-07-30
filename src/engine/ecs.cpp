#include "ecs.hpp"
namespace SPRF {
template <> Transform* Entity::get_component<Transform>() {
    return &m_transform;
}
} // namespace SPRF