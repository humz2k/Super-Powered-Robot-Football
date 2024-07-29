/** @file player_body_base.hpp
 *
 * This header file defines the PlayerBodyBase class, which manages the physical
 * body of a player in a multiplayer game simulation. The class handles player
 * physics, such as movement, jumping, and collision detection, ensuring that
 * players interact correctly within the simulation environment. It provides
 * methods to update the player's state based on inputs, apply forces and
 * velocities, and check for ground contact. The class serves as a foundation
 * for more specialized player body implementations, allowing for flexible and
 * extensible game development.
 *
 */

#ifndef _SPRF_PLAYER_BODY_BASE_HPP_
#define _SPRF_PLAYER_BODY_BASE_HPP_

#include "packet.hpp"
#include "player_stats.hpp"
#include "raylib-cpp.hpp"
#include "server_params.hpp"
#include <cassert>
#include <enet/enet.h>
#include <mutex>
#include <ode/ode.h>
#include <string>
#include <thread>
#include <unordered_map>

namespace SPRF {
/**
 * @brief Base class for managing the physical body of a player in the
 * simulation.
 *
 * This class provides the fundamental mechanics for handling player physics in
 * the game, including movement, jumping, collision detection, and state
 * updates. It serves as a foundation for more specialized player body
 * implementations.
 */
class PlayerBodyBase {
  protected:
    /** @brief Reference to the simulation parameters */
    SimulationParameters& m_sim_params;

  private:
    /** @brief Mutex to protect simulation state */
    std::mutex* m_simulation_mutex;
    /** @brief Unique identifier for the player */
    enet_uint32 m_id;
    /** @brief The ODE world */
    dWorldID m_world;
    /** @brief The ODE collision space */
    dSpaceID m_space;
    /** @brief Time step per tick */
    float m_dt;
    /** @brief Radius of the player capsule */
    float m_radius;
    /** @brief Height of the player capsule */
    float m_height;
    /** @brief Radius of the player's foot collision sphere */
    float m_foot_radius;
    /** @brief Total mass of the player */
    float m_total_mass;
    /** @brief Offset for the foot collision sphere (i.e., where it is on the
     * player body) */
    float m_foot_offset;
    /** @brief The ODE body representing the player */
    dBodyID m_body;
    /** @brief The ODE geometry representing the player's main body */
    dGeomID m_geom;
    /** @brief The ODE geometry representing the player's foot */
    dGeomID m_foot_geom;
    /** @brief The mass properties of the player's body */
    dMass m_mass;

    /** @brief Player's rotation */
    raylib::Vector3 m_rotation = raylib::Vector3(0, 0, 0);

  protected:
    /** @brief Mutex to protect player state */
    std::mutex m_player_mutex;
    /** @brief Flag indicating forward movement */
    bool m_forward = false;
    /** @brief Flag indicating backward movement */
    bool m_backward = false;
    /** @brief Flag indicating left movement */
    bool m_left = false;
    /** @brief Flag indicating right movement */
    bool m_right = false;
    /** @brief Flag indicating jump action */
    bool m_jump = false;

  public:
    /**
     * @brief Constructs a new PlayerBodyBase object.
     *
     * Initializes the player body with the given parameters and sets up the ODE
     * physics properties.
     *
     * @param sim_params Reference to the simulation parameters.
     * @param simulation_mutex Pointer to the simulation mutex.
     * @param id Unique identifier for the player.
     * @param world The ODE world.
     * @param space The ODE collision space.
     * @param dt Time step per tick.
     * @param initial_position Initial position of the player.
     * @param radius Radius of the player capsule.
     * @param height Height of the player capsule.
     * @param foot_radius Radius of the player's foot collision sphere.
     * @param foot_offset Offset for the foot collision sphere.
     */
    PlayerBodyBase(SimulationParameters& sim_params,
                   std::mutex* simulation_mutex, enet_uint32 id, dWorldID world,
                   dSpaceID space, float dt,
                   raylib::Vector3 initial_position = raylib::Vector3(0, 5, 0),
                   float radius = PLAYER_RADIUS, float height = PLAYER_HEIGHT,
                   float foot_radius = PLAYER_FOOT_RADIUS,
                   float foot_offset = PLAYER_FOOT_OFFSET)
        : m_sim_params(sim_params), m_simulation_mutex(simulation_mutex),
          m_id(id), m_world(world), m_space(space), m_dt(dt), m_radius(radius),
          m_height(height), m_foot_radius(foot_radius),
          m_total_mass(m_sim_params.mass), m_foot_offset(foot_offset) {
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

    /**
     * @brief Virtual destructor for PlayerBodyBase.
     */
    virtual ~PlayerBodyBase() {}

    /**
     * @brief Gets the time step per tick.
     *
     * @return float The time step per tick.
     */
    float dt() { return m_dt; }

    /**
     * @brief Adds a force to the player body.
     *
     * @param force The force to be added.
     */
    void add_force(raylib::Vector3 force) {
        dBodyAddForce(m_body, force.x, force.y, force.z);
    }

    /**
     * @brief Sets the velocity of the player body.
     *
     * @param vel The velocity to be set.
     *
     * @return raylib::Vector3 The current velocity.
     */
    raylib::Vector3 velocity(raylib::Vector3 vel) {
        dBodySetLinearVel(m_body, vel.x, vel.y, vel.z);
        return vel;
    }

    /**
     * @brief Gets the current velocity of the player body.
     *
     * @return raylib::Vector3 The current velocity.
     */
    raylib::Vector3 velocity() {
        const float* v = dBodyGetLinearVel(m_body);
        return raylib::Vector3(v[0], v[1], v[2]);
    }

    /**
     * @brief Clamps the player's XZ velocity to a maximum magnitude.
     *
     * @param max The maximum magnitude.
     *
     * @return raylib::Vector3 The current velocity.
     */
    raylib::Vector3 clamp_xz_velocity(float max) {
        auto vel = velocity();
        float y = vel.y;
        vel.y = 0;
        if (vel.LengthSqr() > max * max) {
            vel = vel.Normalize() * max;
        }
        vel.y = y;
        return velocity(vel);
    }

    /**
     * @brief Gets the current XZ velocity of the player body.
     *
     * @return raylib::Vector3 The current XZ velocity.
     */
    raylib::Vector3 xz_velocity() {
        auto out = velocity();
        out.y = 0;
        return out;
    }

    /**
     * @brief Sets the XZ velocity of the player body.
     *
     * @param vel The XZ velocity to be set.
     *
     * @return raylib::Vector3 The current XZ velocity.
     */
    raylib::Vector3 xz_velocity(raylib::Vector3 vel) {
        auto tmp = vel;
        tmp.y = velocity().y;
        velocity(tmp);
        return vel;
    }

    /**
     * @brief Adds an acceleration to the player body.
     *
     * @param acceleration The acceleration to be added.
     */
    void add_acceleration(raylib::Vector3 acceleration) {
        velocity(acceleration * m_dt + velocity());
    }

    /**
     * @brief Checks if the player body is grounded.
     *
     * @param ground_geom The ground geometry ID.
     * @return bool True if the player body is grounded, false otherwise.
     */
    bool grounded(dGeomID ground_geom) {
        dContactGeom contact;
        return dCollide(ground_geom, m_foot_geom, 1, &contact,
                        sizeof(dContactGeom));
    }

    /**
     * @brief Gets the unique identifier of the player.
     *
     * @return enet_uint32 The unique identifier of the player.
     */
    enet_uint32 id() { return m_id; }

    /**
     * @brief Enables the player body in the simulation.
     *
     * Locks `simulation_mutex`.
     */
    void enable() {
        std::lock_guard<std::mutex> guard(*m_simulation_mutex);
        dBodyEnable(m_body);
    }

    /**
     * @brief Disables the player body in the simulation.
     *
     * Locks `simulation_mutex`.
     */
    void disable() {
        std::lock_guard<std::mutex> guard(*m_simulation_mutex);
        dBodyDisable(m_body);
    }

    /**
     * @brief Gets the current position of the player body.
     *
     * @return raylib::Vector3 The current position.
     */
    raylib::Vector3 position() {
        const float* pos = dBodyGetPosition(m_body);
        return raylib::Vector3(pos[0], pos[1], pos[2]);
    }

    /**
     * @brief Sets the current position of the player body.
     *
     * @param pos The position to be set.
     *
     * @return raylib::Vector3 The current position.
     */
    raylib::Vector3 position(raylib::Vector3 pos) {
        dBodySetPosition(m_body, pos.x, pos.y, pos.z);
        return pos;
    }

    /**
     * @brief Gets the current rotation of the player body.
     *
     * @return raylib::Vector3 The current rotation.
     */
    raylib::Vector3 rotation() { return m_rotation; }

    /**
     * @brief Sets the current rotation of the player body.
     *
     * @param rot The rotation to be set.
     *
     * @return raylib::Vector3 The current rotation.
     */
    raylib::Vector3 rotation(raylib::Vector3 rot) {
        m_rotation = rot;
        return m_rotation;
    }

    /**
     * @brief Updates the player inputs based on a received packet.
     *
     * Locks `m_player_mutex`.
     *
     * @param packet The received client packet.
     */
    void update_inputs(ClientPacket packet) {
        std::lock_guard<std::mutex> guard(m_player_mutex);
        m_forward |= packet.forward;
        m_backward |= packet.backward;
        m_left |= packet.left;
        m_right |= packet.right;
        m_jump |= packet.jump;
        m_rotation = packet.rotation;
    }

    /**
     * @brief Resets the player inputs.
     *
     * Locks `m_player_mutex`.
     */
    void reset_inputs() {
        std::lock_guard<std::mutex> guard(m_player_mutex);
        m_forward = false;
        m_backward = false;
        m_left = false;
        m_right = false;
        m_jump = false;
    }

    /**
     * @brief Virtual method to handle player inputs. To be overridden in
     * derived classes.
     *
     * @param ground The ground geometry ID.
     */
    virtual void handle_inputs(dGeomID ground) {}
};

} // namespace SPRF

#endif // _SPRF_PLAYER_BODY_BASE_HPP_