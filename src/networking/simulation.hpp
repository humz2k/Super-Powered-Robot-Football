#ifndef _SPRF_NETWORKING_SIMULATION_HPP_
#define _SPRF_NETWORKING_SIMULATION_HPP_

#include "packet.hpp"
#include "player_body_base.hpp"
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

#define MAX_CONTACTS 5

namespace SPRF {

class PlayerBody : public PlayerBodyBase {
  private:
    bool m_last_was_jump = false;
    bool m_jumped = false;
    bool m_is_grounded = false;
    bool m_can_jump = false;
    float m_ground_counter = 0;
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

    void update_grounded(dGeomID ground) {
        bool new_grounded = grounded(ground);
        if (!new_grounded) {
            m_ground_counter = 0;
        } else if ((!m_is_grounded) && (new_grounded)) {
            m_ground_counter += dt();
            if (m_ground_counter < sim_params.bunny_hop_forgiveness) {
                TraceLog(LOG_INFO, "bunny hop forgiveness = %g",
                         m_ground_counter);
                m_can_jump = true;
                return;
            }
        }
        m_is_grounded = new_grounded;
        m_can_jump = m_is_grounded;
    }

    bool check_jump() {
        m_jumped = false;
        if (m_can_jump) {
            if (m_jump) {
                if (!m_last_was_jump) {
                    m_last_was_jump = true;
                    m_jumped = true;
                    add_force(raylib::Vector3(0, 1, 0) * sim_params.jump_force);
                }
            } else {
                m_last_was_jump = false;
            }
        } else {
            m_last_was_jump = false;
        }
        return false;
    }

    void update_drag(float drag) {
        if (m_is_grounded) {
            set_xz_velocity(xz_velocity() * drag);
        }
    }

  public:
    using PlayerBodyBase::PlayerBodyBase;

    void handle_inputs(dGeomID ground) {
        std::lock_guard<std::mutex> guard(m_player_mutex);
        update_grounded(ground);

        auto direction = move_direction();

        check_jump();
        if (m_is_grounded) {
            add_force(direction * sim_params.ground_acceleration);
            update_drag(sim_params.ground_drag);
            clamp_xz_velocity(sim_params.max_ground_velocity);
        } else {
            raylib::Vector3 proj_vel = Vector3Project(velocity(), direction);
            float proj_vel_mag = proj_vel.Length();
            bool is_away = direction.DotProduct(proj_vel) <= 0.0f;
            if ((proj_vel_mag < sim_params.max_air_velocity) || is_away) {
                if (!is_away) {
                    TraceLog(LOG_INFO, "is not away!");
                    add_force(direction * sim_params.air_acceleration);
                } else {
                    TraceLog(LOG_INFO, "is away!");
                    add_force(direction * sim_params.air_strafe_acceleration);
                }
            }
        }
        clamp_xz_velocity(sim_params.max_all_velocity);
    }
};

static void near_callback(void* data, dGeomID o1, dGeomID o2);

class Simulation {
  private:
    std::mutex simulation_mutex;
    enet_uint32 m_tickrate;
    long long m_time_per_tick;
    enet_uint32 m_tick = 0;
    bool m_should_quit = false;
    // float m_gravity = -5.0;
    float m_ground_friction = 0.0;
    float m_dt;

    /** @brief How much velocity objects inside each other will separate at
     * (default infinity but that seems dumb) */
    float m_correcting_velocity = 0.9;

    /** @brief dont do work if object is stationary */
    bool m_auto_disable = false;

    dWorldID m_world;
    dJointGroupID m_contact_group;
    dSpaceID m_space;
    dGeomID m_ground_geom;

    std::thread m_simulation_thread;

    SimulationParameters sim_params;

    std::unordered_map<enet_uint32, PlayerBody*> m_players;

  public:
    bool should_quit() {
        std::lock_guard<std::mutex> guard(simulation_mutex);
        return m_should_quit;
    }

    void quit() {
        std::lock_guard<std::mutex> guard(simulation_mutex);
        m_should_quit = true;
    }

    enet_uint32 tick() {
        std::lock_guard<std::mutex> guard(simulation_mutex);
        return m_tick;
    }

    void run() {
        while (!should_quit()) {
            step();
            std::this_thread::sleep_for(
                std::chrono::nanoseconds(m_time_per_tick));
        }
    }

    void launch() {
        m_simulation_thread = std::thread(&SPRF::Simulation::run, this);
    }

    void join() { m_simulation_thread.join(); }

    Simulation(enet_uint32 tickrate)
        : m_tickrate(tickrate), m_time_per_tick(1000000000L / m_tickrate),
          m_dt(1.0f / (float)m_tickrate) {
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
    }

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

    PlayerBody* create_player(enet_uint32 id) {
        std::lock_guard<std::mutex> guard(simulation_mutex);
        m_players[id] = new PlayerBody(sim_params, &simulation_mutex, id,
                                       m_world, m_space, m_dt);
        return m_players[id];
    }

    dGeomID ground_geom() { return m_ground_geom; }

    float ground_friction() { return m_ground_friction; }

    dWorldID world() { return m_world; }

    dJointGroupID contact_group() { return m_contact_group; }

    void step() {
        std::lock_guard<std::mutex> guard(simulation_mutex);
        for (auto& i : m_players) {
            i.second->handle_inputs(m_ground_geom);
            i.second->reset_inputs();
        }
        dSpaceCollide(m_space, this, near_callback);
        dWorldQuickStep(m_world, m_dt);
        dJointGroupEmpty(m_contact_group);
        m_tick++;
    }

    void update(enet_uint32* tick, std::vector<PlayerState>& states) {
        std::lock_guard<std::mutex> guard(simulation_mutex);
        *tick = m_tick;
        for (auto& i : states) {
            auto pos = m_players[i.id]->position();
            i.position[0] = pos.x;
            i.position[1] = pos.y;
            i.position[2] = pos.z;

            auto rot = m_players[i.id]->rotation();
            i.rotation[0] = rot.x;
            i.rotation[1] = rot.y;
            i.rotation[2] = rot.z;

            auto vel = m_players[i.id]->velocity();
            i.velocity[0] = vel.x;
            i.velocity[1] = vel.y;
            i.velocity[2] = vel.z;
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