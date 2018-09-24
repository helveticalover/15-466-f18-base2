#pragma once

#include "GL.hpp"

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <map>

#include "Connection.hpp"

struct Game {
    enum PlayerType { WOLFPLAYER, FARMER, SPECTATOR };
    PlayerType player = SPECTATOR;

    struct {
        bool go_left = false;
        bool go_right = false;
        bool go_up = false;
        bool go_down = false;
    } wolf_controls;

    static constexpr const float max_velocity = 0.05f;
    static constexpr const float acceleration = 0.75f;
    static constexpr const float deceleration = 0.75f;
    const glm::quat left_rotation = glm::angleAxis(glm::radians(-90.0f), glm::vec3(0,0,1)) *
            glm::angleAxis(glm::radians(90.0f), glm::vec3(1,0,0));
    const glm::quat right_rotation = glm::angleAxis(glm::radians(90.0f), glm::vec3(0,0,1)) *
            glm::angleAxis(glm::radians(90.0f), glm::vec3(1,0,0));

    struct AnimalMesh {
        GLuint mesh_start;
        GLuint mesh_count;
        glm::vec3 mesh_scale;
    };
    std::vector< AnimalMesh > animal_meshes;

    struct {
        float x_velocity = 0.0f;
        float y_velocity = 0.0f;
        glm::vec2 position = glm::vec2(0.0f, 0.0f);
        bool face_left = true;
        uint8_t disguise = 0;
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
