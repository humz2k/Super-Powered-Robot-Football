#ifndef _SPRF_PLAYER_BODY_BASE_HPP_
#define _SPRF_PLAYER_BODY_BASE_HPP_

#include "packet.hpp"
#include "player_stats.hpp"
#include "raylib-cpp.hpp"
#include "sim_params.hpp"
#include <cassert>
#include <enet/enet.h>
#include <mutex>
#include <ode/ode.h>
#include <string>
#include <thread>
#include <unordered_map>

namespace SPRF {

class PlayerBodyBase {
  protected:
    SimulationParameters& sim_params;

  private:
    std::mutex* simulation_mutex;
    enet_uint32 m_id;
    dWorldID m_world;
    dSpaceID m_space;
    float m_dt;
    float m_radius;
    float m_height;
    float m_foot_radius;
    float m_total_mass;
    float m_foot_offset;
    dBodyID m_body;
    dGeomID m_geom;
    dGeomID m_foot_geom;
    dMass m_mass;

    raylib::Vector3 m_rotation = raylib::Vector3(0, 0, 0);

  protected:
    std::mutex m_player_mutex;
    bool m_forward = false;
    bool m_backward = false;
    bool m_left = false;
    bool m_right = false;
    bool m_jump = false;

  public:
    PlayerBodyBase(SimulationParameters& sim_params_,
                   std::mutex* simulation_mutex_, enet_uint32 id,
                   dWorldID world, dSpaceID space, float dt,
                   raylib::Vector3 initial_position = raylib::Vector3(0, 5, 0),
                   float radius = PLAYER_RADIUS, float height = PLAYER_HEIGHT,
                   float foot_radius = PLAYER_FOOT_RADIUS,
                   float foot_offset = PLAYER_FOOT_OFFSET)
        : sim_params(sim_params_), simulation_mutex(simulation_mutex_),
          m_id(id), m_world(world), m_space(space), m_dt(dt), m_radius(radius),
          m_height(height), m_foot_radius(foot_radius),
          m_total_mass(sim_params.mass), m_foot_offset(foot_offset) {
        TraceLog(LOG_INFO, "Creating player %u in world", m_id);
        m_body = dBodyCreate(m_world);
        m_geom = dCreateCapsule(m_space, m_radius, m_height);
        m_foot_geom = dCreateSphere(m_space, m_foot_radius);
        dGeomDisable(m_foot_geom);
        dMassSetCapsuleTotal(&m_mass, m_total_mass, 3, m_radius, m_height);
        dBodySetMass(m_body, &m_mass);
        dGeomSetBody(m_geom, m_body);
        dGeomSetBody(m_foot_geom, m_body);
        dGeomSetOffsetPosition(m_foot_geom, 0, 0, m_foot_offset);
        dBodySetPosition(m_body, initial_position.x, initial_position.y,
                         initial_position.z);
        dMatrix3 rotation;
        dRFromAxisAndAngle(rotation, 1.0, 0, 0, M_PI / 2.0f);
        dBodySetRotation(m_body, rotation);
        dBodySetMaxAngularSpeed(m_body, 0);
        dBodySetLinearVel(m_body, 0, 0, 0);
        dBodySetData(m_body, (void*)0);
    }

    virtual ~PlayerBodyBase() {}

    float dt() { return m_dt; }

    void add_force(raylib::Vector3 force) {
        // std::lock_guard<std::mutex> guard(*simulation_mutex);
        dBodyAddForce(m_body, force.x, force.y, force.z);
    }

    void set_velocity(raylib::Vector3 velocity) {
        dBodySetLinearVel(m_body, velocity.x, velocity.y, velocity.z);
    }

    void clamp_xz_velocity(float max) {
        auto vel = velocity();
        float y = vel.y;
        vel.y = 0;
        if (vel.LengthSqr() > max * max) {
            vel = vel.Normalize() * max;
        }
        vel.y = y;
        set_velocity(vel);
    }

    raylib::Vector3 velocity() {
        const float* v = dBodyGetLinearVel(m_body);
        return raylib::Vector3(v[0], v[1], v[2]);
    }

    raylib::Vector3 xz_velocity() {
        auto out = velocity();
        out.y = 0;
        return out;
    }

    void set_xz_velocity(raylib::Vector3 vel) {
        vel.y = velocity().y;
        set_velocity(vel);
    }

    void add_acceleration(raylib::Vector3 acceleration) {
        set_velocity(acceleration * m_dt + velocity());
    }

    bool grounded(dGeomID ground_geom) {
        // std::lock_guard<std::mutex> guard(*simulation_mutex);
        dContactGeom contact;
        return dCollide(ground_geom, m_foot_geom, 1, &contact,
                        sizeof(dContactGeom));
    }

    enet_uint32 id() { return m_id; }

    void enable() {
        std::lock_guard<std::mutex> guard(*simulation_mutex);
        dBodyEnable(m_body);
    }

    void disable() {
        std::lock_guard<std::mutex> guard(*simulation_mutex);
        dBodyDisable(m_body);
    }

    raylib::Vector3 position() {
        const float* pos = dBodyGetPosition(m_body);
        return raylib::Vector3(pos[0], pos[1], pos[2]);
    }

    raylib::Vector3 rotation() { return m_rotation; }

    void update_inputs(ClientPacket packet) {
        std::lock_guard<std::mutex> guard(m_player_mutex);
        m_forward |= packet.forward;
        m_backward |= packet.backward;
        m_left |= packet.left;
        m_right |= packet.right;
        m_jump |= packet.jump;
        m_rotation = packet.rotation;
        // dMatrix3 rotation;
        // dRFromAxisAndAngle(rotation, 0, 1.0, 0, m_rotation.y);
        // dBodySetRotation(m_body, rotation);
    }

    void reset_inputs() {
        std::lock_guard<std::mutex> guard(m_player_mutex);
        m_forward = false;
        m_backward = false;
        m_left = false;
        m_right = false;
        m_jump = false;
    }

    virtual void handle_inputs(dGeomID ground) {}

    // void handle_inputs_(){
    //     std::lock_guard<std::mutex> guard(m_player_mutex);
    //     this->handle_inputs();
    // }
};

} // namespace SPRF

#endif // _SPRF_PLAYER_BODY_BASE_HPP_