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

#include "networking/map.hpp"
#include "networking/packet.hpp"
#include "networking/server_params.hpp"
#include "player_body.hpp"
#include "player_stats.hpp"
#include "raylib-cpp.hpp"
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

class Ball {
  private:
    SimulationParameters& m_sim_params;
    std::mutex* m_simulation_mutex;
    dWorldID m_world;
    dSpaceID m_space;
    float m_dt;
    float m_radius;
    dMass m_mass;
    dBodyID m_body;
    dGeomID m_geom;
    std::vector<dGeomID> m_geom_masks;

  public:
    Ball(SimulationParameters& sim_params, std::mutex* simulation_mutex,
         dWorldID world, dSpaceID space, float dt,
         raylib::Vector3 initial_position = raylib::Vector3(3, 3, 3))
        : m_sim_params(sim_params), m_simulation_mutex(simulation_mutex),
          m_world(world), m_space(space), m_dt(dt),
          m_radius(m_sim_params.ball_radius) {
        m_body = dBodyCreate(m_world);
        m_geom = dCreateSphere(m_space, m_radius);
        dMassSetSphereTotal(&m_mass, sim_params.ball_mass, m_radius);
        dBodySetMass(m_body, &m_mass);
        dGeomSetBody(m_geom, m_body);
        dBodySetPosition(m_body, initial_position.x, initial_position.y,
                         initial_position.z);
        m_geom_masks.push_back(m_geom);
    }

    raylib::Vector3 position() {
        const float* pos = dBodyGetPosition(m_body);
        return raylib::Vector3(pos[0], pos[1], pos[2]);
    }

    raylib::Vector3 rotation() {
        dQuaternion q;
        dQfromR(q, dBodyGetRotation(m_body));
        return QuaternionToEuler(raylib::Quaternion(q[0], q[1], q[2], q[3]));
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

    raylib::Vector3 position(raylib::Vector3 pos) {
        dBodySetPosition(m_body, pos.x, pos.y, pos.z);
        return position();
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

    dGeomID geom() { return m_geom; }

    bool grounded() {
        auto ray = RaycastQuery(m_space, position(), raylib::Vector3(0, -1, 0),
                                m_radius * 1.05, m_geom_masks);
        return ray.hit;
    }

    void update() {
        if (grounded()) {
            xz_velocity(xz_velocity() * m_sim_params.ball_damping *
                        (m_dt / 0.01));
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
    ScriptingManager& m_scripting;
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
    /** @brief Time step per tick */
    float m_dt;

    /** @brief How much velocity objects inside each other will separate at
     * (default infinity but that seems dumb) */
    // float m_correcting_velocity = 0.9;

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
    SimulationParameters m_sim_params;

    /** @brief Map of player IDs to PlayerBody objects */
    std::unordered_map<enet_uint32, PlayerBody*> m_players;

    Ball* m_ball = NULL;

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

    const SimulationParameters& params() { return m_sim_params; }

    /**
     * @brief Construct a new Simulation object.
     *
     * Initializes the simulation with the given tick rate and optional server
     * configuration file.
     *
     * @param tickrate The simulation tick rate.
     * @param server_config The path to the server configuration file.
     */
    Simulation(ScriptingManager& scripting, enet_uint32 tickrate,
               std::string server_config = "")
        : m_scripting(scripting), m_tickrate(tickrate),
          m_time_per_tick(1000000000L / m_tickrate),
          m_dt(1.0f / (float)m_tickrate), m_sim_params(server_config) {
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

        TraceLog(LOG_INFO, "Setting gravity = %g", m_sim_params.gravity);
        dWorldSetGravity(m_world, 0, m_sim_params.gravity, 0);

        TraceLog(LOG_INFO, "Setting ERP %g and CFM %g", m_sim_params.erp,
                 m_sim_params.cfm);
        dWorldSetERP(m_world, m_sim_params.erp);
        dWorldSetCFM(m_world, m_sim_params.cfm);

        TraceLog(LOG_INFO, "Setting auto disable flag %d", m_auto_disable);
        dWorldSetAutoDisableFlag(m_world, m_auto_disable);

        simple_map()->load(m_world, m_space);

        m_ball =
            new Ball(m_sim_params, &simulation_mutex, m_world, m_space, m_dt);

        register_scripts();
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
        delete m_ball;
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
        m_players[id] = new PlayerBody(m_sim_params, &simulation_mutex, id,
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

    Ball* ball() { return m_ball; }

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
        m_ball->update();
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
    void update(enet_uint32* tick, std::vector<player_state_data>& states,
                ball_state_data& ball_state) {
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
        ball_state.position(m_ball->position());
        ball_state.rotation(m_ball->rotation());
    }

    // scripting stuff

    void set_ball_position(raylib::Vector3 pos) {
        std::lock_guard<std::mutex> guard(simulation_mutex);
        m_ball->position(pos);
    }

    void set_ball_velocity(raylib::Vector3 pos) {
        std::lock_guard<std::mutex> guard(simulation_mutex);
        m_ball->velocity(pos);
    }

    void register_scripts(){
        m_scripting.register_function(
            [this](lua_State* L) {
                float x = luaL_checknumber(L, 1);
                float y = luaL_checknumber(L, 2);
                float z = luaL_checknumber(L, 3);
                TraceLog(LOG_INFO, "LUA: setting ball position %g %g %g", x, y, z);
                this->set_ball_position(raylib::Vector3(x, y, z));
                return 0;
            },
            "set_ball_position");

        m_scripting.register_function(
            [this](lua_State* L) {
                float x = luaL_checknumber(L, 1);
                float y = luaL_checknumber(L, 2);
                float z = luaL_checknumber(L, 3);
                TraceLog(LOG_INFO, "LUA: setting ball velocity %g %g %g", x, y, z);
                this->set_ball_velocity(raylib::Vector3(x, y, z));
                return 0;
            },
            "set_ball_velocity");
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
        if ((o1 == sim->ball()->geom()) || (o2 == sim->ball()->geom())) {
            contact[i].surface.mu = sim->params().ball_friction;
            contact[i].surface.mu2 = 0;
            contact[i].surface.bounce = sim->params().ball_bounce;
            contact[i].surface.bounce_vel = 0.05;
            contact[i].surface.soft_cfm = 0.01;
        } else {
            contact[i].surface.mu = sim->params().ground_friction;
            contact[i].surface.mu2 = 0;
            contact[i].surface.bounce = 0.01;
            contact[i].surface.bounce_vel = 0.1;
            contact[i].surface.soft_cfm = 0.01;
        }
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