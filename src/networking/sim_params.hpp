#ifndef _SPRF_SIM_PARAMS_HPP_
#define _SPRF_SIM_PARAMS_HPP_

#include <string>
#include <mini/ini.hpp>
#include <cassert>

#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)

namespace SPRF {

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

    SimulationParameters(){}
    SimulationParameters(std::string filename){
        if (filename == "")return;

        TraceLog(LOG_INFO,"Reading file %s",filename.c_str());

        #define DUMB_HACK(field,token) if (field.has(TOSTRING(token))){ \
                token = std::stof(field[TOSTRING(token)]); \
                TraceLog(LOG_INFO,"Server Config: %s = %g",TOSTRING(token),token);\
                field[TOSTRING(token)] = std::to_string(token); \
            }

        mINI::INIFile file(filename);
        mINI::INIStructure ini;
        assert(file.read(ini));
        if (ini.has("physics")){
            auto& physics = ini["physics"];
            DUMB_HACK(physics,ground_acceleration)
            DUMB_HACK(physics,air_strafe_acceleration)
            DUMB_HACK(physics,air_acceleration)
            DUMB_HACK(physics,jump_force)
            DUMB_HACK(physics,ground_drag)
            DUMB_HACK(physics,air_drag)
            DUMB_HACK(physics,max_ground_velocity)
            DUMB_HACK(physics,max_air_velocity)
            DUMB_HACK(physics,max_all_velocity)
            DUMB_HACK(physics,mass)
            DUMB_HACK(physics,gravity)
            DUMB_HACK(physics,bunny_hop_forgiveness)
        }
        if (ini.has("error_correction")){
            auto& error_correction = ini["physics"];
            DUMB_HACK(error_correction,erp)
            DUMB_HACK(error_correction,cfm)
        }

        file.write(ini);

        #undef DUMB_HACK
    }
};

} // namespace SPRF

#endif // _SPRF_SIM_PARAMS_HPP_