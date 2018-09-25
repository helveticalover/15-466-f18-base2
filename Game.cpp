#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <iostream>

#include "Game.hpp"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/string_cast.hpp>

bool Game::update(float time) {

    // ----------------- WOLF MOVEMENT ----------------------
    if (player == WOLFPLAYER) {
        if (wolf_controls.go_left) {
            wolf_state.x_velocity -= time * Acceleration;
            wolf_state.face_left = true;
        }
        if (wolf_controls.go_up) {
            wolf_state.y_velocity += time * Acceleration;
        }
        if (wolf_controls.go_right) {
            wolf_state.x_velocity += time * Acceleration;
            wolf_state.face_left = false;
        }
        if (wolf_controls.go_down) {
            wolf_state.y_velocity -= time * Acceleration;
        }

        // Decelerate to a stop
        if (!wolf_controls.go_left && !wolf_controls.go_right && wolf_state.x_velocity != 0.0f) {
            int sign = wolf_state.x_velocity < 0 ? -1 : 1;
            wolf_state.x_velocity -= sign * Deceleration * time;

            if (sign > 0) {
                wolf_state.x_velocity = glm::clamp(wolf_state.x_velocity, 0.0f, MaxVelocity);
            } else {
                wolf_state.x_velocity = glm::clamp(wolf_state.x_velocity, -MaxVelocity, 0.0f);
            }
        }
        if (!wolf_controls.go_up && !wolf_controls.go_down && wolf_state.y_velocity != 0.0f) {
            int sign = wolf_state.y_velocity < 0 ? -1 : 1;
            wolf_state.y_velocity -= sign * Deceleration * time;

            if (sign > 0) {
                wolf_state.y_velocity = glm::clamp(wolf_state.y_velocity, 0.0f, MaxVelocity);
            } else {
                wolf_state.y_velocity = glm::clamp(wolf_state.y_velocity, -MaxVelocity, 0.0f);
            }
        }

        wolf_state.x_velocity = glm::clamp(wolf_state.x_velocity, -MaxVelocity, MaxVelocity);
        wolf_state.y_velocity = glm::clamp(wolf_state.y_velocity, -MaxVelocity, MaxVelocity);
        glm::vec2 mv = wolf_state.x_velocity * glm::vec2(1.0f, 0.0f) +
                       wolf_state.y_velocity * glm::vec2(0.0f, 1.0f);

        if (mv != glm::vec2(0.0f, 0.0f)) {
            wolf_state.position += mv;

            wolf_state.position.x = glm::clamp(wolf_state.position.x, -PenLimit, PenLimit);
            wolf_state.position.y = glm::clamp(wolf_state.position.y, -PenLimit, PenLimit);
            return true;
        }
    }
    else if (player == FARMER) {
        if (farmer_controls.go_left) {
            farmer_state.x_velocity -= time * Acceleration;
        }
        if (farmer_controls.go_up) {
            farmer_state.y_velocity += time * Acceleration;
        }
        if (farmer_controls.go_right) {
            farmer_state.x_velocity += time * Acceleration;
        }
        if (farmer_controls.go_down) {
            farmer_state.y_velocity -= time * Acceleration;
        }

        // Decelerate to a stop
        if (!farmer_controls.go_left && !farmer_controls.go_right && farmer_state.x_velocity != 0.0f) {
            int sign = farmer_state.x_velocity < 0 ? -1 : 1;
            farmer_state.x_velocity -= sign * Deceleration * time;

            if (sign > 0) {
                farmer_state.x_velocity = glm::clamp(farmer_state.x_velocity, 0.0f, MaxVelocity);
            } else {
                farmer_state.x_velocity = glm::clamp(farmer_state.x_velocity, -MaxVelocity, 0.0f);
            }
        }
        if (!farmer_controls.go_up && !farmer_controls.go_down && farmer_state.y_velocity != 0.0f) {
            int sign = farmer_state.y_velocity < 0 ? -1 : 1;
            farmer_state.y_velocity -= sign * Deceleration * time;

            if (sign > 0) {
                farmer_state.y_velocity = glm::clamp(farmer_state.y_velocity, 0.0f, MaxVelocity);
            } else {
                farmer_state.y_velocity = glm::clamp(farmer_state.y_velocity, -MaxVelocity, 0.0f);
            }
        }

        farmer_state.x_velocity = glm::clamp(farmer_state.x_velocity, -MaxVelocity, MaxVelocity);
        farmer_state.y_velocity = glm::clamp(farmer_state.y_velocity, -MaxVelocity, MaxVelocity);
        glm::vec2 mv = farmer_state.x_velocity * glm::vec2(1.0f, 0.0f) +
                       farmer_state.y_velocity * glm::vec2(0.0f, 1.0f);

        if (mv != glm::vec2(0.0f, 0.0f)) {
            farmer_state.position += mv;

            farmer_state.position.x = glm::clamp(farmer_state.position.x, -PenLimit, PenLimit);
            farmer_state.position.y = glm::clamp(farmer_state.position.y, -PenLimit, PenLimit);
            return true;
        }
    }

    return false;
}
