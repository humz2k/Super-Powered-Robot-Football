#ifndef _SPRF_RAYCAST_HPP_
#define _SPRF_RAYCAST_HPP_

#include "raylib-cpp.hpp"
#include <ode/ode.h>
#include <vector>

namespace SPRF {

// Performs raycasting on a space and returns the point of collision. Return
// false for no hit.
raylib::RayCollision
RaycastQuery(dSpaceID Space, raylib::Vector3 start, raylib::Vector3 direction,
             float length, std::vector<dGeomID> masks = std::vector<dGeomID>());

} // namespace SPRF

#endif // _SPRF_RAYCAST_HPP_