#ifndef _SPRF_NETWORKING_SIMULATION_HPP_
#define _SPRF_NETWORKING_SIMULATION_HPP_

#include "packet.hpp"
#include "raylib-cpp.hpp"
#include "player_stats.hpp"
#include <cassert>
#include <enet/enet.h>
#include <mutex>
#include <ode/ode.h>
#include <string>
#include <thread>
#include <unordered_map>

#define MAX_CONTACTS 5

namespace SPRF {

class PlayerBodyBase {
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

    raylib::Vector3 m_rotation = raylib::Vector3(0,0,0);
  protected:
    std::mutex m_player_mutex;
    bool m_forward = false;
    bool m_backward = false;
    bool m_left = false;
    bool m_right = false;

  public:
    PlayerBodyBase(std::mutex* simulation_mutex_, enet_uint32 id, dWorldID world, dSpaceID space, float dt,
               raylib::Vector3 initial_position = raylib::Vector3(0, 5, 0),
               float radius = PLAYER_RADIUS, float height = PLAYER_HEIGHT, float foot_radius = PLAYER_FOOT_RADIUS,
               float total_mass = PLAYER_MASS, float foot_offset = PLAYER_FOOT_OFFSET)
        : simulation_mutex(simulation_mutex_), m_id(id), m_world(world), m_space(space), m_dt(dt), m_radius(radius),
          m_height(height), m_foot_radius(foot_radius),
          m_total_mass(total_mass), m_foot_offset(foot_offset) {
        TraceLog(LOG_INFO,"Creating player %u in world",m_id);
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

    virtual ~PlayerBodyBase(){

    }

    void add_force(raylib::Vector3 force){
        //std::lock_guard<std::mutex> guard(*simulation_mutex);
        dBodyAddForce(m_body,force.x,force.y,force.z);
    }

    void set_velocity(raylib::Vector3 velocity){
        dBodySetLinearVel(m_body,velocity.x,velocity.y,velocity.z);
    }

    void clamp_xz_velocity(float max){
        auto vel = velocity();
        float y = vel.y;
        vel.y = 0;
        if (vel.LengthSqr() > max*max){
            vel = vel.Normalize() * max;
        }
        vel.y = y;
        set_velocity(vel);
    }

    raylib::Vector3 velocity(){
        const float* v = dBodyGetLinearVel(m_body);
        return raylib::Vector3(v[0],v[1],v[2]);
    }

    void add_acceleration(raylib::Vector3 acceleration){
        set_velocity(acceleration * m_dt + velocity());
    }

    bool grounded(dGeomID ground_geom){
        //std::lock_guard<std::mutex> guard(*simulation_mutex);
        dContactGeom contact;
        return dCollide(ground_geom, m_foot_geom, 1, &contact, sizeof(dContactGeom));
    }

    enet_uint32 id(){
        return m_id;
    }

    void enable() { std::lock_guard<std::mutex> guard(*simulation_mutex); dBodyEnable(m_body); }

    void disable() { std::lock_guard<std::mutex> guard(*simulation_mutex); dBodyDisable(m_body); }

    raylib::Vector3 position(){
        const float* pos = dBodyGetPosition(m_body);
        return raylib::Vector3(pos[0],pos[1],pos[2]);
    }

    raylib::Vector3 rotation(){
        return m_rotation;
    }

    void update_inputs(ClientPacket packet){
        std::lock_guard<std::mutex> guard(m_player_mutex);
        m_forward |= packet.forward;
        m_backward |= packet.backward;
        m_left |= packet.left;
        m_right |= packet.right;
        m_rotation = packet.rotation;
    }

    void reset_inputs(){
        std::lock_guard<std::mutex> guard(m_player_mutex);
        m_forward = false;
        m_backward = false;
        m_left = false;
        m_right = false;
    }

    virtual void handle_inputs(){}

    //void handle_inputs_(){
    //    std::lock_guard<std::mutex> guard(m_player_mutex);
    //    this->handle_inputs();
    //}
};


class PlayerBody : public PlayerBodyBase {
  public:
    using PlayerBodyBase::PlayerBodyBase;

    void handle_inputs(){
        std::lock_guard<std::mutex> guard(m_player_mutex);
        raylib::Vector3 forward = Vector3RotateByAxisAngle(
            raylib::Vector3(0,0,1.0f), raylib::Vector3(0,1.0f,0),
            this->rotation().y);
        raylib::Vector3 left = Vector3RotateByAxisAngle(
            raylib::Vector3(1.0f,0,0), raylib::Vector3(0,1.0f,0),
            this->rotation().y);
        auto direction = (forward * (this->m_forward - this->m_backward) + left * (this->m_left - this->m_right)).Normalize();
        add_acceleration(direction * 20.0f);
        clamp_xz_velocity(10);
        //auto force = direction * 10.0f;//raylib::Vector3(m_left-m_right,0,m_forward - m_backward) * 10.0f;
        //add_force(force);

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
    float m_gravity = -1.0;
    float m_ground_friction = 10.0;
    float m_dt;

    /** @brief Error correction parameters */
    float m_erp = 0.2;
    /** @brief Error correction parameters */
    float m_cfm = 1e-5;
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

    std::unordered_map<enet_uint32,PlayerBody*> m_players;

  public:
    bool should_quit(){
        std::lock_guard<std::mutex> guard(simulation_mutex);
        return m_should_quit;
    }

    void quit(){
        std::lock_guard<std::mutex> guard(simulation_mutex);
        m_should_quit = true;
    }

    enet_uint32 tick(){
        std::lock_guard<std::mutex> guard(simulation_mutex);
        return m_tick;
    }

    void run(){
        while (!should_quit()){
            step();
            std::this_thread::sleep_for(std::chrono::nanoseconds(m_time_per_tick));
        }
    }

    void launch(){
        m_simulation_thread = std::thread(&SPRF::Simulation::run, this);
    }

    void join(){
        m_simulation_thread.join();
    }

    Simulation(enet_uint32 tickrate) : m_tickrate(tickrate), m_time_per_tick(1000000000L/m_tickrate), m_dt(1.0f/(float)m_tickrate) {
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

        TraceLog(LOG_INFO, "Setting gravity = %g", m_gravity);
        dWorldSetGravity(m_world, 0, m_gravity, 0);

        TraceLog(LOG_INFO, "Setting ERP %g and CFM %g", m_erp, m_cfm);
        dWorldSetERP(m_world, m_erp);
        dWorldSetCFM(m_world, m_cfm);

        TraceLog(LOG_INFO, "Setting auto disable flag %d", m_auto_disable);
        dWorldSetAutoDisableFlag(m_world, m_auto_disable);
    }

    ~Simulation() {
        for (auto& i : m_players){
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

    PlayerBody* create_player(enet_uint32 id){
        std::lock_guard<std::mutex> guard(simulation_mutex);
        m_players[id] = new PlayerBody(&simulation_mutex,id,m_world,m_space,m_dt);
        return m_players[id];
    }

    dGeomID ground_geom(){
        return m_ground_geom;
    }

    float ground_friction(){
        return m_ground_friction;
    }

    dWorldID world(){
        return m_world;
    }

    dJointGroupID contact_group(){
        return m_contact_group;
    }

    void step(){
        std::lock_guard<std::mutex> guard(simulation_mutex);
        for (auto& i : m_players){
            i.second->handle_inputs();
            i.second->reset_inputs();
        }
        dSpaceCollide(m_space,this,near_callback);
        dWorldQuickStep(m_world,m_dt);
        dJointGroupEmpty(m_contact_group);
        m_tick++;
    }

    void update(enet_uint32* tick, std::vector<PlayerState>& states){
        std::lock_guard<std::mutex> guard(simulation_mutex);
        *tick = m_tick;
        for (auto& i : states){
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
                dJointID c =
                    dJointCreateContact(sim->world(), sim->contact_group(), contact + i);
                dJointAttach(c, b1, b2);
            }
        }
    }

}

#endif // _SPRF_NETWORKING_SIMULATION_HPP_