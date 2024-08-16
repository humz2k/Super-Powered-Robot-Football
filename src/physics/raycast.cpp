#include "raycast.hpp"

#define MAX_CONTACTS 32

namespace SPRF {

struct ode_raycast {
    bool hit;
    dReal depth;
    dVector3 pos;
    dVector3 normal;
    std::vector<dGeomID> masks;
};

// Check ray collision against a space
static void RayCallback(void* Data, dGeomID Geometry1, dGeomID Geometry2) {
    ode_raycast* HitPosition = (ode_raycast*)Data;

    // Check collisions
    dContact Contacts[MAX_CONTACTS];
    int Count = dCollide(Geometry1, Geometry2, MAX_CONTACTS, &Contacts[0].geom,
                         sizeof(dContact));
    for (int i = 0; i < Count; i++) {

        // Check depth against current closest hit
        if (Contacts[i].geom.depth < HitPosition->depth) {
            for (auto& i : HitPosition->masks) {
                if ((Geometry1 == i) || (Geometry2 == i))
                    continue;
            }
            dCopyVector3(HitPosition->pos, Contacts[i].geom.pos);
            dCopyVector3(HitPosition->normal, Contacts[i].geom.normal);
            HitPosition->depth = Contacts[i].geom.depth;
        }
    }
}

// Performs raycasting on a space and returns the point of collision. Return
// false for no hit.
raylib::RayCollision RaycastQuery(dSpaceID Space, raylib::Vector3 start,
                                  raylib::Vector3 direction, float length,
                                  std::vector<dGeomID> masks) {
    dVector3 Start = {start.x, start.y, start.z};
    dVector3 Direction = {direction.x, direction.y, direction.z};

    // Get length
    dReal Length = length;
    dReal InverseLength = dRecip(Length);

    // Normalize
    dScaleVector3(Direction, InverseLength);

    // Create ray
    dGeomID Ray = dCreateRay(0, Length);
    dGeomRaySet(Ray, Start[0], Start[1], Start[2], Direction[0], Direction[1],
                Direction[2]);

    // Check collisions
    ode_raycast HitPosition;
    HitPosition.depth = dInfinity;
    HitPosition.hit = false;
    dSpaceCollide2(Ray, (dGeomID)Space, &HitPosition, &RayCallback);

    // Cleanup
    dGeomDestroy(Ray);

    // Check for hit
    if (HitPosition.depth != dInfinity) {
        HitPosition.hit = true;
    }

    return raylib::RayCollision(
        HitPosition.hit, HitPosition.depth,
        raylib::Vector3(HitPosition.pos[0], HitPosition.pos[1],
                        HitPosition.pos[2]),
        raylib::Vector3(HitPosition.normal[0], HitPosition.normal[1],
                        HitPosition.normal[2]));
}

} // namespace SPRF