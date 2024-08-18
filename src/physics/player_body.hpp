#ifndef _SPRF_PLAYER_BODY_HPP_
#define _SPRF_PLAYER_BODY_HPP_

#include "player_body_base.hpp"

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
                if (!m_last_was_jump)
                    m_can_jump = true;
                return new_grounded;
            }
        }
        m_is_grounded = new_grounded;
        if (m_is_grounded && (!m_last_was_jump))
            m_can_jump = true;
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
        if (m_jump && m_can_jump && (!m_last_was_jump)) {
            m_jumped = true;
            add_force(raylib::Vector3(0, 1, 0) * m_sim_params.jump_force);
            m_can_jump = false;
        }
        m_last_was_jump = m_jump;
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

} // namespace SPRF

#endif // _SPRF_PLAYER_BODY_HPP_