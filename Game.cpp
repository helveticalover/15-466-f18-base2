#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <iostream>

#include "Game.hpp"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/string_cast.hpp>

bool Game::update(float time) {

    // ----------------- WOLF MOVEMENT ----------------------
    if (player == WOLF) {
        if (wolf_controls.go_left) {
            wolf_state.x_velocity -= time * acceleration;
            wolf_state.rotation = glm::quat();
        }
        if (wolf_controls.go_up) {
            wolf_state.y_velocity += time * acceleration;
        }
        if (wolf_controls.go_right) {
            wolf_state.x_velocity += time * acceleration;
            wolf_state.rotation = glm::quat(glm::vec3(0.0f, 0.0f, glm::radians(180.0f)));
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
        glm::vec3 mv = wolf_state.x_velocity * glm::vec3(1.0f, 0.0f, 0.0f) +
                       wolf_state.y_velocity * glm::vec3(0.0f, 1.0f, 0.0f);

        if (mv != glm::vec3(0.0f, 0.0f, 0.0f)) {
            wolf_state.position += mv;

            std::cout << glm::to_string(wolf_state.position) << std::endl;
            return true;
        }

        return false;
    }

    return false;

    // -------------------------- BASE CODE ------------------------------

//	ball += ball_velocity * time;
//	if (ball.x >= 0.5f * FrameWidth - BallRadius) {
//		ball_velocity.x = -std::abs(ball_velocity.x);
//	}
//	if (ball.x <=-0.5f * FrameWidth + BallRadius) {
//		ball_velocity.x = std::abs(ball_velocity.x);
//	}
//	if (ball.y >= 0.5f * FrameHeight - BallRadius) {
//		ball_velocity.y = -std::abs(ball_velocity.y);
//	}
//	if (ball.y <=-0.5f * FrameHeight + BallRadius) {
//		ball_velocity.y = std::abs(ball_velocity.y);
//	}
//
//	auto do_point = [this](glm::vec2 const &pt) {
//		glm::vec2 to = ball - pt;
//		float len2 = glm::dot(to, to);
//		if (len2 > BallRadius * BallRadius) return;
//		//if point is inside ball, make ball velocity outward-going:
//		float d = glm::dot(ball_velocity, to);
//		ball_velocity += ((std::abs(d) - d) / len2) * to;
//	};
//
//	do_point(glm::vec2(paddle.x - 0.5f * PaddleWidth, paddle.y));
//	do_point(glm::vec2(paddle.x + 0.5f * PaddleWidth, paddle.y));
//
//	auto do_edge = [&](glm::vec2 const &a, glm::vec2 const &b) {
//		float along = glm::dot(ball-a, b-a);
//		float max = glm::dot(b-a,b-a);
//		if (along <= 0.0f || along >= max) return;
//		do_point(glm::mix(a,b,along/max));
//	};
//
//	do_edge(glm::vec2(paddle.x + 0.5f * PaddleWidth, paddle.y), glm::vec2(paddle.x - 0.5f * PaddleWidth, paddle.y));

}
