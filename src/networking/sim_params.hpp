#ifndef _SPRF_SIM_PARAMS_HPP_
#define _SPRF_SIM_PARAMS_HPP_

namespace SPRF{

class SimulationParameters{
    public:
    float ground_acceleration = 100.0f;
    float air_acceleration = 5.0f;
    float jump_force = 160.0f;
    float ground_drag = 0.8f;
    float air_drag = 0.99f;
    float max_velocity = 10.0f;
    float mass = 1.0f;
    float gravity = -5.0;
    /** @brief Error correction parameters */
    float erp = 0.2;
    /** @brief Error correction parameters */
    float cfm = 1e-5;
};

} // namespace SPRF

#endif // _SPRF_SIM_PARAMS_HPP_