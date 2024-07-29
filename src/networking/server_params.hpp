#ifndef _SPRF_SIM_PARAMS_HPP_
#define _SPRF_SIM_PARAMS_HPP_

#include <cassert>
#include <mini/ini.hpp>
#include <string>

#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)

namespace SPRF {

class ServerConfig {
  public:
    std::string host = "127.0.0.1";
    enet_uint16 port = 9999;
    size_t peer_count = 4;
    size_t channel_count = 2;
    size_t iband = 0;
    size_t oband = 0;
    enet_uint32 tickrate = 64;
    ServerConfig(std::string filename) {
        if (filename == "")
            return;

        TraceLog(LOG_INFO, "Reading file %s", filename.c_str());

#define DUMB_HACK(field, token)                                                \
    if (field.has(TOSTRING(token))) {                                          \
        token = std::stoi(field[TOSTRING(token)]);                             \
        TraceLog(LOG_INFO, "Server Config: %s = %g", TOSTRING(token), token);  \
        field[TOSTRING(token)] = std::to_string(token);                        \
    }

        mINI::INIFile file(filename);
        mINI::INIStructure ini;
        assert(file.read(ini));
        if (ini.has("physics")) {
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

        file.write(ini);

#undef DUMB_HACK
    }
};

class SimulationParameters {
  public:
    float ground_acceleration = 50.0f;
    float air_strafe_acceleration = 5.0f;
    float air_acceleration = 20.0f;
    float jump_force = 190.0f;
    float ground_drag = 0.85f;
    float air_drag = 0.99f;
    float max_ground_velocity = 5.0f;
    float max_air_velocity = 5.0f;
    float max_all_velocity = 15.0f;
    float mass = 1.0f;
    float gravity = -5.0;
    float bunny_hop_forgiveness = 0.15;

    /** @brief Error correction parameters */
    float erp = 0.2;
    /** @brief Error correction parameters */
    float cfm = 1e-5;

    SimulationParameters() {}
    SimulationParameters(std::string filename) {
        if (filename == "")
            return;

        TraceLog(LOG_INFO, "Reading file %s", filename.c_str());

#define DUMB_HACK(field, token)                                                \
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
            DUMB_HACK(physics, air_strafe_acceleration)
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
        }
        if (ini.has("error_correction")) {
            auto& error_correction = ini["physics"];
            DUMB_HACK(error_correction, erp)
            DUMB_HACK(error_correction, cfm)
        }

        file.write(ini);

#undef DUMB_HACK
    }
};

} // namespace SPRF

#endif // _SPRF_SIM_PARAMS_HPP_