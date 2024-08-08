/** @file simulation.hpp
 *
 * This header file defines classes and functions for managing the physics
 * simulation in a multiplayer game. The PlayerBody class handles the physical
 * interactions of players, translating inputs into actions. The Simulation
 * class manages the overall simulation, including running the simulation loop,
 * updating game states, and handling collision detection. The near_callback
 * function handles near collisions, creating contact joints to simulate
 * physical interactions.
 *
 */

#ifndef _SPRF_NETWORKING_SIMULATION_HPP_
#define _SPRF_NETWORKING_SIMULATION_HPP_

#include "map.hpp"
#include "packet.hpp"
#include "player_body_base.hpp"
#include "player_stats.hpp"
#include "raylib-cpp.hpp"
#include "server_params.hpp"
#include <cassert>
#include <chrono>
#include <enet/enet.h>
#include <mutex>
#include <ode/ode.h>
#include <string>
#include <thread>
#include <unordered_map>

#define MAX_CONTACTS 32

namespace SPRF {

/**
 * @brief Represents the physical body of a player in the simulation.
 *
 * This class extends PlayerBodyBase and includes additional logic to handle
 * player movement, jumping, and collision with the ground. It is responsible
 * for interpreting player inputs and applying the corresponding forces and
 * velocities to the player's physical body.
 */
class PlayerBody : public PlayerBodyBase {
  private:
    /** @brief Tracks if the last input was a jump */
    bool m_last_was_jump = false;
    /** @brief Tracks if the player has jumped */
    bool m_jumped = false;
    /** @brief Tracks if the player is grounded */
    bool m_is_grounded = false;
    /** @brief Tracks if the player can jump */
    bool m_can_jump = false;
    /** @brief Counter to track time spent on the ground */
    float m_ground_counter = 0;

    /**
     * @brief Calculates the movement direction based on player inputs.
     *
     * Uses the forward and left directions, modified by the player's rotation,
     * to compute the movement direction.
     *
     * @return raylib::Vector3 The normalized movement direction.
     */
    raylib::Vector3 move_direction() {
        raylib::Vector3 forward = Vector3RotateByAxisAngle(
            raylib::Vector3(0, 0, 1.0f), raylib::Vector3(0, 1.0f, 0),
            this->rotation().y);

        raylib::Vector3 left = Vector3RotateByAxisAngle(
            raylib::Vector3(1.0f, 0, 0), raylib::Vector3(0, 1.0f, 0),
            this->rotation().y);

        return (forward * (this->m_forward - this->m_backward) +
                left * (this->m_left - this->m_right))
            .Normalize();
    }

    /**
     * @brief Updates the grounded state of the player.
     *
     * Checks if the player is grounded and updates the jump-related states
     * accordingly.
     *
     * When we land on the ground for the first time after jumping, we wait
     * `bunny_hop_forgiveness` seconds before we actually say we are grounded.
     * This makes bunnyhopping possible. NOTE: This has a weird (but makes
     * sense) behavior where, because we aren't adding drag while in the air, we
     * "skid" on the ground after landing for `bunny_hop_forgiveness` seconds.
     *
     * This sets the variables `m_is_grounded` and `m_can_jump`.
     *
     */
    bool update_grounded() {
        bool new_grounded = grounded();
        if (!new_grounded) {
            m_ground_counter = 0;
        } else if ((!m_is_grounded) && (new_grounded)) {
            m_ground_counter += dt();
            if (m_ground_counter < m_sim_params.bunny_hop_forgiveness) {
                m_can_jump = true;
                return new_grounded;
            }
        }
        m_is_grounded = new_grounded;
        m_can_jump = m_is_grounded;
        return new_grounded;
    }

    /**
     * @brief Checks if the player can jump and applies the jump force if
     * possible.
     *
     * Sets the variable `m_jump`, which will be `true` if the player jumped
     * this tick, or `false` if it didn't.
     *
     * @return bool True if the player jumped, false otherwise.
     */
    bool check_jump() {
        m_jumped = false;
        if (m_can_jump) {
            if (m_jump) {
                if (!m_last_was_jump) {
                    m_last_was_jump = true;
                    m_jumped = true;
                    add_force(raylib::Vector3(0, 1, 0) *
                              m_sim_params.jump_force);
                }
            } else {
                m_last_was_jump = false;
            }
        } else {
            m_last_was_jump = false;
        }
        return false;
    }

  public:
    using PlayerBodyBase::PlayerBodyBase;

    raylib::Vector3 get_forward() {
        return Vector3RotateByAxisAngle(raylib::Vector3(0, 0, 1.0f),
                                        raylib::Vector3(0, 1.0f, 0),
                                        this->rotation().y);
    }

    raylib::Vector3 get_left() {
        return Vector3RotateByAxisAngle(raylib::Vector3(1.0f, 0, 0),
                                        raylib::Vector3(0, 1.0f, 0),
                                        this->rotation().y);
    }

    /**
     * @brief Handles the player inputs and updates the player's physical state.
     *
     * This method interprets the player inputs, updates the grounded state,
     * applies forces for movement and jumping, and ensures the player's
     * velocity stays within defined limits.
     *
     */
    void handle_inputs() {
        std::lock_guard<std::mutex> guard(m_player_mutex);
        update_grounded();

        // auto direction = move_direction();

        raylib::Vector3 forward = get_forward();

        raylib::Vector3 left = get_left();

        raylib::Vector3 direction = raylib::Vector3(0, 0, 0);

        raylib::Vector3 xz_v_delta = raylib::Vector3(0, 0, 0);

        if ((m_forward && m_backward) || ((!m_forward) && (!m_backward))) {
            xz_v_delta -= xz_velocity().Project(forward);
        } else {
            direction += forward * (m_forward - m_backward);
        }

        if ((m_left && m_right) || ((!m_left) && (!m_right))) {
            xz_v_delta -= xz_velocity().Project(left);
        } else {
            direction += left * (m_left - m_right);
        }

        direction = direction.Normalize();

        check_jump();
        if (m_is_grounded) {
            add_force(direction * m_sim_params.ground_acceleration);
            xz_velocity(xz_velocity() +
                        xz_v_delta * (m_sim_params.ground_drag));
            clamp_xz_velocity(m_sim_params.max_ground_velocity);
        } else {
            raylib::Vector3 proj_vel = Vector3Project(velocity(), direction);
            if ((proj_vel.Length() < m_sim_params.max_air_velocity) ||
                (direction.DotProduct(proj_vel) <= 0.0f)) {
                add_force(direction * m_sim_params.air_acceleration);
            }
            xz_velocity(xz_velocity() + xz_v_delta * (m_sim_params.air_drag));
            clamp_xz_velocity(m_sim_params.max_all_velocity);
        }
    }
};

/**
 * @brief Callback function for handling near collisions.
 *
 * This function is called whenever two geoms are near each other and a
 * potential collision needs to be resolved. It creates contact joints between
 * the colliding geoms to simulate physical interactions.
 *
 * @param data Pointer to user data (in this case, the Simulation object).
 * @param o1 The first geometry ID.
 * @param o2 The second geometry ID.
 */
static void near_callback(void* data, dGeomID o1, dGeomID o2);

class Simulation {
  private:
    /** @brief Mutex to protect simulation state */
    std::mutex simulation_mutex;
    /** @brief Simulation tick rate */
    enet_uint32 m_tickrate;
    /** @brief Time per tick in nanoseconds */
    std::chrono::nanoseconds m_time_per_tick;
    /** @brief Current simulation tick */
    enet_uint32 m_tick = 0;
    /** @brief Flag to indicate if the simulation should quit */
    bool m_should_quit = false;
    /** @brief Ground friction coefficient */
    float m_ground_friction = 0.0;
    /** @brief Time step per tick */
    float m_dt;

    /** @brief How much velocity objects inside each other will separate at
     * (default infinity but that seems dumb) */
    float m_correcting_velocity = 0.9;

    /** @brief dont do work if object is stationary */
    bool m_auto_disable = false;

    /** @brief The ODE world */
    dWorldID m_world;
    /** @brief The ODE contact group */
    dJointGroupID m_contact_group;
    /** @brief The ODE collision space */
    dSpaceID m_space;
    /** @brief The ground geometry */
    dGeomID m_ground_geom;

    /** @brief Thread running the simulation loop */
    std::thread m_simulation_thread;

    /** @brief Simulation parameters */
    SimulationParameters sim_params;

    /** @brief Map of player IDs to PlayerBody objects */
    std::unordered_map<enet_uint32, PlayerBody*> m_players;

  public:
    /**
     * @brief Checks if the simulation should quit.
     *
     * Locks `simulation_mutex`.
     *
     * @return bool True if the simulation should quit, false otherwise.
     */
    bool should_quit() {
        std::lock_guard<std::mutex> guard(simulation_mutex);
        return m_should_quit;
    }

    /**
     * @brief Sets the flag to quit the simulation.
     *
     * Locks `simulation_mutex`.
     */
    void quit() {
        std::lock_guard<std::mutex> guard(simulation_mutex);
        m_should_quit = true;
    }

    /**
     * @brief Gets the current simulation tick.
     *
     * Locks `simulation_mutex`.
     *
     * @return enet_uint32 The current simulation tick.
     */
    enet_uint32 tick() {
        std::lock_guard<std::mutex> guard(simulation_mutex);
        return m_tick;
    }

    /**
     * @brief Runs the simulation loop.
     *
     * WARNING: No idea if this is smart...
     *
     * This method continuously steps the simulation and sleeps for the
     * remainder of the tick duration.
     */
    void run() {
        while (!should_quit()) {
            auto start = std::chrono::high_resolution_clock::now();
            step();
            auto finish = std::chrono::high_resolution_clock::now();
            std::this_thread::sleep_for(
                m_time_per_tick -
                std::chrono::duration_cast<std::chrono::nanoseconds>(finish -
                                                                     start));
        }
    }

    /**
     * @brief Launches the simulation in a separate thread.
     */
    void launch() {
        m_simulation_thread = std::thread(&SPRF::Simulation::run, this);
    }

    /**
     * @brief Waits for the simulation thread to finish.
     */
    void join() { m_simulation_thread.join(); }

    /**
     * @brief Construct a new Simulation object.
     *
     * Initializes the simulation with the given tick rate and optional server
     * configuration file.
     *
     * @param tickrate The simulation tick rate.
     * @param server_config The path to the server configuration file.
     */
    Simulation(enet_uint32 tickrate, std::string server_config = "")
        : m_tickrate(tickrate), m_time_per_tick(1000000000L / m_tickrate),
          m_dt(1.0f / (float)m_tickrate), sim_params(server_config) {
        TraceLog(LOG_INFO, "Initializing ODE");
        dInitODE();

        TraceLog(LOG_INFO, "Creating world");
        m_world = dWorldCreate();

        TraceLog(LOG_INFO, "Creating collision space");
        m_space = dSimpleSpaceCreate(0);

        TraceLog(LOG_INFO, "Creating contact group");
        m_contact_group = dJointGroupCreate(0);

        TraceLog(LOG_INFO, "Ground Plane %g %g %g %g", 0, 1, 0, 0);
        m_ground_geom = dCreatePlane(m_space, 0, 1, 0, 0);

        TraceLog(LOG_INFO, "Setting gravity = %g", sim_params.gravity);
        dWorldSetGravity(m_world, 0, sim_params.gravity, 0);

        TraceLog(LOG_INFO, "Setting ERP %g and CFM %g", sim_params.erp,
                 sim_params.cfm);
        dWorldSetERP(m_world, sim_params.erp);
        dWorldSetCFM(m_world, sim_params.cfm);

        TraceLog(LOG_INFO, "Setting auto disable flag %d", m_auto_disable);
        dWorldSetAutoDisableFlag(m_world, m_auto_disable);

        simple_map()->load(m_world, m_space);
    }

    /**
     * @brief Destructor for the Simulation class.
     *
     * Cleans up the simulation resources, including deleting player bodies and
     * destroying ODE objects.
     */
    ~Simulation() {
        for (auto& i : m_players) {
            delete i.second;
        }
        TraceLog(LOG_INFO, "Destroying contact group");
        dJointGroupDestroy(m_contact_group);
        TraceLog(LOG_INFO, "Destroying space");
        dSpaceDestroy(m_space);
        TraceLog(LOG_INFO, "Destroying world");
        dWorldDestroy(m_world);

        TraceLog(LOG_INFO, "Closing ODE");
        dCloseODE();
        TraceLog(LOG_INFO, "Closed ODE");
    }

    /**
     * @brief Creates a new player in the simulation.
     *
     * Locks `simulation_mutex`.
     *
     * @param id The unique identifier for the player.
     * @return PlayerBody* Pointer to the created PlayerBody object.
     */
    PlayerBody* create_player(enet_uint32 id) {
        std::lock_guard<std::mutex> guard(simulation_mutex);
        m_players[id] = new PlayerBody(sim_params, &simulation_mutex, id,
                                       m_world, m_space, m_dt);
        return m_players[id];
    }

    /**
     * @brief Gets the ground geometry ID.
     *
     * @return dGeomID The ground geometry ID.
     */
    dGeomID ground_geom() { return m_ground_geom; }

    /**
     * @brief Gets the ground friction coefficient.
     *
     * @return float The ground friction coefficient.
     */
    float ground_friction() { return m_ground_friction; }

    /**
     * @brief Gets the ODE world ID.
     *
     * @return dWorldID The ODE world ID.
     */
    dWorldID world() { return m_world; }

    /**
     * @brief Gets the ODE contact group ID.
     *
     * @return dJointGroupID The ODE contact group ID.
     */
    dJointGroupID contact_group() { return m_contact_group; }

    /**
     * @brief Steps the simulation.
     *
     * This method handles player inputs, performs collision detection, steps
     * the simulation, and updates the contact joints.
     *
     * Called in `run`. Locks `simulation_mutex`.
     */
    void step() {
        std::lock_guard<std::mutex> guard(simulation_mutex);
        for (auto& i : m_players) {
            i.second->handle_inputs();
            i.second->reset_inputs();
        }
        dSpaceCollide(m_space, this, near_callback);
        dWorldQuickStep(m_world, m_dt);
        dJointGroupEmpty(m_contact_group);
        m_tick++;
    }

    /**
     * @brief Updates the player states with the current simulation tick and
     * player positions, rotations, and velocities.
     *
     * Locks `simulation_mutex`.
     *
     * @param tick Pointer to the current simulation tick.
     * @param states Vector of player_state_data to be updated.
     */
    void update(enet_uint32* tick, std::vector<player_state_data>& states) {
        std::lock_guard<std::mutex> guard(simulation_mutex);
        *tick = m_tick;
        for (auto& i : states) {
            auto pos = m_players[i.id]->position();
            i.position_data[0] = pos.x;
            i.position_data[1] = pos.y;
            i.position_data[2] = pos.z;

            auto rot = m_players[i.id]->rotation();
            i.rotation_data[0] = rot.x;
            i.rotation_data[1] = rot.y;
            i.rotation_data[2] = rot.z;

            auto vel = m_players[i.id]->velocity();
            i.velocity_data[0] = vel.x;
            i.velocity_data[1] = vel.y;
            i.velocity_data[2] = vel.z;
        }
    }
};

static void near_callback(void* data, dGeomID o1, dGeomID o2) {

    Simulation* sim = (Simulation*)data;

    // Temporary index for each contact
    int i;

    // Get the dynamics body for each geom
    dBodyID b1 = dGeomGetBody(o1);
    dBodyID b2 = dGeomGetBody(o2);

    // Create an array of dContact objects to hold the contact joints
    dContact contact[MAX_CONTACTS];

    // Now we set the joint properties of each contact. Going into the full
    // details here would require a tutorial of its own. I'll just say that
    // the members of the dContact structure control the joint behaviour,
    // such as friction, velocity and bounciness. See section 7.3.7 of the
    // ODE manual and have fun experimenting to learn more.
    for (i = 0; i < MAX_CONTACTS; i++) {
        contact[i].surface.mode = dContactBounce | dContactSoftCFM;
        if ((o1 == sim->ground_geom()) || (o2 == sim->ground_geom()))
            contact[i].surface.mu = sim->ground_friction();
        else
            contact[i].surface.mu = 0;
        contact[i].surface.mu2 = 0;
        contact[i].surface.bounce = 0.01;
        contact[i].surface.bounce_vel = 0.1;
        contact[i].surface.soft_cfm = 0.01;
    }

    // Here we do the actual collision test by calling dCollide. It returns
    // the number of actual contact points or zero if there were none. As
    // well as the geom IDs, max number of contacts we also pass the address
    // of a dContactGeom as the fourth parameter. dContactGeom is a
    // substructure of a dContact object so we simply pass the address of
    // the first dContactGeom from our array of dContact objects and then
    // pass the offset to the next dContactGeom as the fifth paramater,
    // which is the size of a dContact structure. That made sense didn't it?
    int numc;
    if ((numc = dCollide(o1, o2, MAX_CONTACTS, &contact[0].geom,
                         sizeof(dContact)))) {
        // To add each contact point found to our joint group we call
        // dJointCreateContact which is just one of the many different joint
        // types available.
        for (i = 0; i < numc; i++) {
            // dJointCreateContact needs to know which world and joint group
            // to work with as well as the dContact object itself. It
            // returns a new dJointID which we then use with dJointAttach to
            // finally create the temporary contact joint between the two
            // geom bodies.
            dJointID c = dJointCreateContact(sim->world(), sim->contact_group(),
                                             contact + i);
            dJointAttach(c, b1, b2);
        }
    }
}

} // namespace SPRF

#endif // _SPRF_NETWORKING_SIMULATION_HPP_