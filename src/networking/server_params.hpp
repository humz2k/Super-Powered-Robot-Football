/** @file server_params.hpp
 *
 * This header file defines classes for handling server configuration and
 * simulation parameters. These classes enable easy modification and tuning of
 * server settings and gameplay mechanics through INI files, facilitating quick
 * adjustments and fine-tuning without recompiling the code. The ServerConfig
 * class manages network-related settings, while the SimulationParameters class
 * handles physics and gameplay parameters.
 *
 */

#ifndef _SPRF_SIM_PARAMS_HPP_
#define _SPRF_SIM_PARAMS_HPP_

#include <cassert>
#include <mini/ini.hpp>
#include <string>

#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)

namespace SPRF {

/**
 * @brief Class representing the configuration parameters for the server.
 *
 * This class holds the configuration settings for the server, such as host,
 * port, peer count, channel count, bandwidth limits, and tick rate. The
 * configuration can be loaded from an INI file, which allows for easy
 * modification and extension of server settings without recompiling the code.
 */
class ServerConfig {
  public:
    /** @brief Default host address for the server */
    std::string host = "127.0.0.1";
    /** @brief Default port number for the server */
    enet_uint16 port = 9999;
    /** @brief Default number of peers (clients) that can connect to the server
     */
    size_t peer_count = 4;
    /** @brief Default number of channels for communication */
    size_t channel_count = 2;
    /** @brief Default incoming bandwidth limit (0 means unlimited) */
    size_t iband = 0;
    /** @brief Default outgoing bandwidth limit (0 means unlimited) */
    size_t oband = 0;
    /** @brief Default tick rate (number of simulation updates per second) */
    enet_uint32 tickrate = 64;

    /**
     * @brief Construct a new ServerConfig object.
     *
     * Loads the server configuration from an INI file if a filename is
     * provided.
     *
     * @param filename The path to the INI file containing the server
     * configuration. If `filename==""`, then default values are used.
     */
    ServerConfig(std::string filename = "") {
        if (filename == "")
            return;

        TraceLog(LOG_INFO, "Reading file %s", filename.c_str());

#define DUMB_HACK(field, token)                                                \
    if (field.has(TOSTRING(token))) {                                          \
        token = std::stoi(field[TOSTRING(token)]);                             \
        TraceLog(LOG_INFO, "Server Config: %s = %d", TOSTRING(token), token);  \
        field[TOSTRING(token)] = std::to_string(token);                        \
    }

        mINI::INIFile file(filename);
        mINI::INIStructure ini;
        assert(file.read(ini)); // Ensure the INI file is successfully read
        if (ini.has("server")) {
            auto& server = ini["server"];
            if (server.has("host")) {
                host = server["host"];
                server["host"] = host;
            }
            DUMB_HACK(server, port)
            DUMB_HACK(server, peer_count)
            DUMB_HACK(server, channel_count)
            DUMB_HACK(server, iband)
            DUMB_HACK(server, oband)
            DUMB_HACK(server, tickrate)
        }

        // file.write(ini); // Write any changes back to the INI file

#undef DUMB_HACK
    }
};

/**
 * @brief Class representing the simulation parameters.
 *
 * This class holds the parameters that control the physics and gameplay
 * mechanics in the simulation, such as acceleration, drag, velocity limits, and
 * gravity. The parameters can be loaded from an INI file, allowing for easy
 * tuning of gameplay without recompiling the code.
 */
class SimulationParameters {
  public:
    /** @brief Default ground acceleration */
    float ground_acceleration = 50.0f;
    /** @brief Default air acceleration */
    float air_acceleration = 20.0f;
    /** @brief Default jump force */
    float jump_force = 190.0f;
    /** @brief Default ground drag coefficient */
    float ground_drag = 0.85f;
    /** @brief Default air drag coefficient */
    float air_drag = 0.99f;
    /** @brief Default maximum ground velocity */
    float max_ground_velocity = 5.0f;
    /** @brief Default maximum air velocity */
    float max_air_velocity = 5.0f;
    /** @brief Default maximum overall velocity */
    float max_all_velocity = 15.0f;
    /** @brief Default mass of the player */
    float mass = 1.0f;
    /** @brief Default gravity applied to the player */
    float gravity = -5.0f;
    /** @brief Default bunny hop forgiveness time */
    float bunny_hop_forgiveness = 0.15f;

    float ground_friction = 0.5f;

    float ball_radius = 0.5f;
    float ball_mass = 0.5f;

    float ball_friction = 1.0f;
    float ball_damping = 0.99f;
    float ball_bounce = 0.9f;

    /** @brief Error reduction parameter */
    float erp = 0.2;
    /** @brief Constraint force mixing parameter */
    float cfm = 1e-5;

    /**
     * @brief Construct a new SimulationParameters object.
     *
     * Loads the simulation parameters from an INI file if a filename is
     * provided.
     *
     * @param filename The path to the INI file containing the simulation
     * parameters. If `filename==""`, then default values are used.
     */
    SimulationParameters(std::string filename = "") {
        if (filename == "")
            return;

        TraceLog(LOG_INFO, "Reading sim params file %s", filename.c_str());

#define DUMB_HACK(field, token)                                                \
    TraceLog(LOG_INFO, "Server Config: %s", TOSTRING(token));                  \
    if (field.has(TOSTRING(token))) {                                          \
        token = std::stof(field[TOSTRING(token)]);                             \
        TraceLog(LOG_INFO, "Server Config: %s = %g", TOSTRING(token), token);  \
        field[TOSTRING(token)] = std::to_string(token);                        \
    }

        mINI::INIFile file(filename);
        mINI::INIStructure ini;
        assert(file.read(ini));
        if (ini.has("physics")) {
            auto& physics = ini["physics"];
            DUMB_HACK(physics, ground_acceleration)
            DUMB_HACK(physics, air_acceleration)
            DUMB_HACK(physics, jump_force)
            DUMB_HACK(physics, ground_drag)
            DUMB_HACK(physics, air_drag)
            DUMB_HACK(physics, max_ground_velocity)
            DUMB_HACK(physics, max_air_velocity)
            DUMB_HACK(physics, max_all_velocity)
            DUMB_HACK(physics, mass)
            DUMB_HACK(physics, gravity)
            DUMB_HACK(physics, bunny_hop_forgiveness)
            DUMB_HACK(physics, ground_friction)
        }
        if (ini.has("error_correction")) {
            auto& error_correction = ini["error_correction"];
            DUMB_HACK(error_correction, erp)
            DUMB_HACK(error_correction, cfm)
        }

        // file.write(ini);

#undef DUMB_HACK
    }
};

} // namespace SPRF

#endif // _SPRF_SIM_PARAMS_HPP_