#pragma once

#include "GL.hpp"

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <random>

#include "Connection.hpp"
#include "Scene.hpp"
#include "Sound.hpp"

struct Game {
    enum PlayerType { WOLFPLAYER, FARMER, SPECTATOR };
    PlayerType player = SPECTATOR;

    struct {
        bool go_left = false;
        bool go_right = false;
        bool go_up = false;
        bool go_down = false;
    } wolf_controls;

    struct {
        float x_velocity = 0.0f;
        float y_velocity = 0.0f;
        glm::vec2 position = glm::vec2(0.0f, 0.0f);
        bool face_left = true;
        float last_cry = 0.0f;
        uint8_t disguise = 0;
        uint8_t num_eaten = 0;
    } wolf_state;

    struct {
        bool go_left = false;
        bool go_right = false;
        bool go_up = false;
        bool go_down = false;
    } farmer_controls;

    struct {
        float x_velocity = 0.0f;
        float y_velocity = 0.0f;
        glm::vec2 position = glm::vec2(0.0f, 0.0f);
        glm::vec2 last_hit = glm::vec2(0.0f, 0.0f);
        uint8_t strikes = 0;
    } farmer_state;

    static constexpr const float CryInterval = 20.0f;
    static constexpr const float MaxVelocity = 0.05f;
    static constexpr const float Acceleration = 0.75f;
    static constexpr const float Deceleration = 0.75f;
    static constexpr const float PenLimit = 7.25f;
    static constexpr const float EatOffset = 1.5f;
    static constexpr const float EatRange = 1.0f;

    struct AnimalMesh {
        GLuint mesh_start;
        GLuint mesh_count;
        GLuint dmesh_start;
        GLuint dmesh_count;
        GLuint hmesh_start;
        GLuint hmesh_count;
        glm::vec3 mesh_scale;
		std::shared_ptr< Sound::Sample > cry;
        float shoot_x;
        float shoot_y;
        float y_offset = 0.0f;
    };
    std::vector< AnimalMesh > animal_meshes;

    struct Decoy {
        bool face_left = true;
        glm::vec2 position;
        glm::vec2 target;
        uint8_t animal;
    };
    std::vector< Decoy > decoy_animals;
    uint32_t num_decoys = 35;
    int target = -1;

	bool update(float time);

	static constexpr const float FrameWidth = 10.0f;
	static constexpr const float FrameHeight = 8.0f;
	static constexpr const float PaddleWidth = 2.0f;
	static constexpr const float PaddleHeight = 0.4f;
	static constexpr const float BallRadius = 0.5f;
};
