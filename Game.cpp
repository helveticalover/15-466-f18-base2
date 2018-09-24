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
            wolf_state.x_velocity -= time * acceleration;
            wolf_state.face_left = true;
        }
        if (wolf_controls.go_up) {
            wolf_state.y_velocity += time * acceleration;
        }
        if (wolf_controls.go_right) {
            wolf_state.x_velocity += time * acceleration;
            wolf_state.face_left = false;
        }
        if (wolf_controls.go_down) {
            wolf_state.y_velocity -= time * acceleration;
        }

        // Decelerate to a stop
        if (!wolf_controls.go_left && !wolf_controls.go_right && wolf_state.x_velocity != 0.0f) {
            int sign = wolf_state.x_velocity < 0 ? -1 : 1;
            wolf_state.x_velocity -= sign * deceleration * time;

            if (sign > 0) {
                wolf_state.x_velocity = glm::clamp(wolf_state.x_velocity, 0.0f, max_velocity);
            } else {
                wolf_state.x_velocity = glm::clamp(wolf_state.x_velocity, -max_velocity, 0.0f);
            }
        }
        if (!wolf_controls.go_up && !wolf_controls.go_down && wolf_state.y_velocity != 0.0f) {
            int sign = wolf_state.y_velocity < 0 ? -1 : 1;
            wolf_state.y_velocity -= sign * deceleration * time;

            if (sign > 0) {
                wolf_state.y_velocity = glm::clamp(wolf_state.y_velocity, 0.0f, max_velocity);
            } else {
                wolf_state.y_velocity = glm::clamp(wolf_state.y_velocity, -max_velocity, 0.0f);
            }
        }

        wolf_state.x_velocity = glm::clamp(wolf_state.x_velocity, -max_velocity, max_velocity);
        wolf_state.y_velocity = glm::clamp(wolf_state.y_velocity, -max_velocity, max_velocity);
        glm::vec2 mv = wolf_state.x_velocity * glm::vec2(1.0f, 0.0f) +
                       wolf_state.y_velocity * glm::vec2(0.0f, 1.0f);

        if (mv != glm::vec2(0.0f, 0.0f)) {
            wolf_state.position += mv;
            return true;
        }

        return false;
    }

    return false;
}
