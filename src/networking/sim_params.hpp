#ifndef _SPRF_SIM_PARAMS_HPP_
#define _SPRF_SIM_PARAMS_HPP_

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
};

} // namespace SPRF

#endif // _SPRF_SIM_PARAMS_HPP_