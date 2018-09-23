#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include "Connection.hpp"

struct Game {
    enum PlayerType { WOLF, FARMER, SPECTATOR };
    PlayerType player = SPECTATOR;

    struct {
        bool go_left = false;
        bool go_right = false;
        bool go_up = false;
        bool go_down = false;
    } wolf_controls;

    static constexpr const float max_velocity = 0.1f;
    static constexpr const float acceleration = 0.75f;
    static constexpr const float deceleration = 0.75f;

    struct {
        float x_velocity = 0.0f;
        float y_velocity = 0.0f;
        glm::quat rotation = glm::quat();
        glm::vec3 position = glm::vec3(0.0f, 0.0f, 0.0f);
    } wolf_state;

	glm::vec2 paddle = glm::vec2(0.0f,-3.0f);
	glm::vec2 ball = glm::vec2(0.0f, 0.0f);
	glm::vec2 ball_velocity = glm::vec2(0.0f,-2.0f);

	bool update(float time);

	static constexpr const float FrameWidth = 10.0f;
	static constexpr const float FrameHeight = 8.0f;
	static constexpr const float PaddleWidth = 2.0f;
	static constexpr const float PaddleHeight = 0.4f;
	static constexpr const float BallRadius = 0.5f;
};
