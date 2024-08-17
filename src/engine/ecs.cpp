#include "ecs.hpp"
namespace SPRF {

int id_counter = 0;

template <> Transform* Entity::get_component<Transform>() {
    return &m_transform;
}
} // namespace SPRF