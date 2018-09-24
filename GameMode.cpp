#include "GameMode.hpp"

#include "MenuMode.hpp"
#include "Load.hpp"
#include "MeshBuffer.hpp"
#include "Scene.hpp"
#include "gl_errors.hpp" //helper for dumpping OpenGL error messages
#include "read_chunk.hpp" //helper for reading a vector of structures from a file
#include "data_path.hpp" //helper to get paths relative to executable
#include "compile_program.hpp" //helper to compile opengl shader programs
#include "draw_text.hpp" //helper to... um.. draw text
#include "vertex_color_program.hpp"

#include <glm/gtc/type_ptr.hpp>

#include <iostream>
#include <fstream>
#include <map>
#include <cstddef>
#include <random>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/string_cast.hpp>

Load< MeshBuffer > meshes(LoadTagDefault, [](){
	return new MeshBuffer(data_path("wolf-sheep-models.pnc"));
});

Load< GLuint > meshes_for_vertex_color_program(LoadTagDefault, [](){
	return new GLuint(meshes->make_vao_for_program(vertex_color_program->program));
});

Scene::Transform *wolf_transform = nullptr;
Scene::Transform *farmer_transform = nullptr;
Scene::Object *wolfplayer_object = nullptr;

Game::AnimalMesh wolf;
Game::AnimalMesh sheep;
Game::AnimalMesh pig;
Game::AnimalMesh cow;

Scene::Camera *camera = nullptr;

Load< Scene > scene(LoadTagDefault, [](){
	Scene *ret = new Scene;
	//load transform hierarchy:
	ret->load(data_path("wolf-sheep.scene"), [](Scene &s, Scene::Transform *t, std::string const &m){
		Scene::Object *obj = s.new_object(t);

		obj->program = vertex_color_program->program;
		obj->program_mvp_mat4  = vertex_color_program->object_to_clip_mat4;
		obj->program_mv_mat4x3 = vertex_color_program->object_to_light_mat4x3;
		obj->program_itmv_mat3 = vertex_color_program->normal_to_light_mat3;

		MeshBuffer::Mesh const &mesh = meshes->lookup(m);

		obj->vao = *meshes_for_vertex_color_program;
		obj->start = mesh.start;
		obj->count = mesh.count;

		if (t->name == "Wolf") {
		    wolfplayer_object = obj;
		}
	});

	for (Scene::Transform *t = ret->first_transform; t != nullptr; t = t->alloc_next) {
		if (t->name == "Wolf") {
			if (wolf_transform) throw std::runtime_error("Multiple 'Wolf' transforms in scene.");
            wolf_transform = t;
            wolf.mesh_scale = t->scale;
		} else if(t->name == "Crosshair") {
            if (farmer_transform) throw std::runtime_error("Multiple 'Crosshair' transforms in scene.");
            farmer_transform = t;
		} else if (t->name == "Sheep") {
			sheep.mesh_scale = t->scale;
		} else if (t->name == "Pig") {
			pig.mesh_scale = t->scale;
		} else if (t->name == "Cow") {
			cow.mesh_scale = t->scale;
		}
	}
	if (!wolf_transform) throw std::runtime_error("No 'Wolf' transform in scene.");
    if (!farmer_transform) throw std::runtime_error("No 'Crosshair' transform in scene.");

	//look up animal meshes:
	MeshBuffer::Mesh const &wolf_mesh = meshes->lookup("Wolf");
	MeshBuffer::Mesh const &sheep_mesh = meshes->lookup("Sheep");
	MeshBuffer::Mesh const &pig_mesh = meshes->lookup("Pig");
	MeshBuffer::Mesh const &cow_mesh = meshes->lookup("Cow");

	wolf.mesh_start = wolf_mesh.start;
	wolf.mesh_count = wolf_mesh.count;
	sheep.mesh_start = sheep_mesh.start;
	sheep.mesh_count = sheep_mesh.count;
	pig.mesh_start = pig_mesh.start;
	pig.mesh_count = pig_mesh.count;
	cow.mesh_start = cow_mesh.start;
	cow.mesh_count = cow_mesh.count;

	//look up the camera:
	for (Scene::Camera *c = ret->first_camera; c != nullptr; c = c->alloc_next) {
		if (c->transform->name == "Camera") {
			if (camera) throw std::runtime_error("Multiple 'Camera' objects in scene.");
			camera = c;
		}
	}
	if (!camera) throw std::runtime_error("No 'Camera' camera in scene.");
	return ret;
});

GameMode::GameMode(Client &client_) : client(client_) {
	client.connection.send_raw("h", 1); //send a 'hello' to the server

	state.animal_meshes.emplace_back(wolf);
	state.animal_meshes.emplace_back(sheep);
	state.animal_meshes.emplace_back(pig);
	state.animal_meshes.emplace_back(cow);

	bool joined_team = false;
	while(!joined_team) {
        client.poll([&](Connection *c, Connection::Event evt) {

            if (evt == Connection::OnOpen || evt == Connection::OnClose) return;
            assert(evt == Connection::OnRecv);

            switch (c->recv_buffer[0]) {
                case 'w':
                    c->recv_buffer.erase(c->recv_buffer.begin(),
                                                        c->recv_buffer.begin() + 1);
                    std::cout << "JOINED AS WOLF" << std::endl;
                    state.player = Game::PlayerType::WOLFPLAYER;
                    joined_team = true;
                    break;
                case 'f':
                    c->recv_buffer.erase(c->recv_buffer.begin(),
                                                        c->recv_buffer.begin() + 1);
                    std::cout << "JOINED AS FARMER" << std::endl;
                    state.player = Game::PlayerType::FARMER;
                    joined_team = true;
                    break;
                case 's':
                    c->recv_buffer.erase(c->recv_buffer.begin(),
                                                        c->recv_buffer.begin() + 1);
                default:
                    std::cout << "JOINED AS SPECTATOR" << std::endl;
                    state.player = Game::PlayerType::SPECTATOR;
                    joined_team = true;
                    break;
            }
        });
	}
}

GameMode::~GameMode() {
}

bool GameMode::handle_event(SDL_Event const &evt, glm::uvec2 const &window_size) {
	//ignore any keys that are the result of automatic key repeat:
	if (evt.type == SDL_KEYDOWN && evt.key.repeat) {
		return false;
	}

	switch (state.player){
		case Game::PlayerType::WOLFPLAYER:
			if (evt.type == SDL_KEYDOWN || evt.type == SDL_KEYUP) {
				if (evt.key.keysym.scancode == SDL_SCANCODE_W) {
					state.wolf_controls.go_up = (evt.type == SDL_KEYDOWN);
					return true;
				} else if (evt.key.keysym.scancode == SDL_SCANCODE_S) {
					state.wolf_controls.go_down = (evt.type == SDL_KEYDOWN);
					return true;
				} else if (evt.key.keysym.scancode == SDL_SCANCODE_A) {
					state.wolf_controls.go_left = (evt.type == SDL_KEYDOWN);
					return true;
				} else if (evt.key.keysym.scancode == SDL_SCANCODE_D) {
					state.wolf_controls.go_right = (evt.type == SDL_KEYDOWN);
					return true;
				}
			}

			if (evt.type == SDL_KEYDOWN && evt.key.keysym.scancode == SDL_SCANCODE_SPACE) {
				++(state.wolf_state.disguise);
				if (state.wolf_state.disguise == state.animal_meshes.size())
					state.wolf_state.disguise = 1;
				return true;
			}
			break;
		case Game::PlayerType::FARMER:
            if (evt.type == SDL_KEYDOWN || evt.type == SDL_KEYUP) {
                if (evt.key.keysym.scancode == SDL_SCANCODE_W) {
                    state.farmer_controls.go_up = (evt.type == SDL_KEYDOWN);
                    return true;
                } else if (evt.key.keysym.scancode == SDL_SCANCODE_S) {
                    state.farmer_controls.go_down = (evt.type == SDL_KEYDOWN);
                    return true;
                } else if (evt.key.keysym.scancode == SDL_SCANCODE_A) {
                    state.farmer_controls.go_left = (evt.type == SDL_KEYDOWN);
                    return true;
                } else if (evt.key.keysym.scancode == SDL_SCANCODE_D) {
                    state.farmer_controls.go_right = (evt.type == SDL_KEYDOWN);
                    return true;
                }
            }
			break;
		case Game::PlayerType::SPECTATOR:
			break;
		default:
			std::cerr << "Unknown player type in GameMode::handle_event." << std::endl;
	}
	return false;
}

void GameMode::update(float elapsed) {
	bool update = state.update(elapsed);

	if (client.connection) {
		//send own game state and predicted game state to server:
        switch(state.player) {
        	case Game::PlayerType::WOLFPLAYER:
        		if (update) {
					client.connection.send_raw("ws", 2);
					client.connection.send_raw(&state.wolf_state.position, sizeof(glm::vec2));
					client.connection.send_raw(&state.wolf_state.face_left, sizeof(bool));
					client.connection.send_raw(&state.wolf_state.disguise, sizeof(uint8_t));
        		}
                client.connection.send_raw("wf", 2);
                client.connection.send_raw(&state.farmer_state.position, sizeof(glm::vec2));
				break;
        	case Game::PlayerType::FARMER:
        	    if (update) {
                    client.connection.send_raw("fs", 2);
                    client.connection.send_raw(&state.farmer_state.position, sizeof(glm::vec2));
        	    }
                client.connection.send_raw("fw", 2);
                client.connection.send_raw(&state.wolf_state.position, sizeof(glm::vec2));
                client.connection.send_raw(&state.wolf_state.face_left, sizeof(bool));
                client.connection.send_raw(&state.wolf_state.disguise, sizeof(uint8_t));
                break;
        	default:
        		std::cerr << "Unknown player type in GameMode::update." << std::endl;
        }
	}

	client.poll([&](Connection *c, Connection::Event event){
		if (event == Connection::OnOpen) {
		} else if (event == Connection::OnClose) {
			std::cerr << "Lost connection to server." << std::endl;
			exit(0);
		} else { assert(event == Connection::OnRecv);
            switch (state.player) {
                case Game::PlayerType::WOLFPLAYER:
                    switch (c->recv_buffer[0]) {
                        case 'f':
                            if (c->recv_buffer.size() < 1 + sizeof(glm::vec2) + sizeof(bool)) return;

                            memcpy(&state.farmer_state.position, c->recv_buffer.data() + 1, sizeof(glm::vec2));
                            c->recv_buffer.erase(c->recv_buffer.begin(), c->recv_buffer.begin() + 1 + sizeof(glm::vec2));
                            break;
                    }
                    break;
                case Game::PlayerType::FARMER:
                    switch (c->recv_buffer[0]) {
                        case 'w':
                            if (c->recv_buffer.size() < 1 + sizeof(glm::vec2) + sizeof(bool)) return;

                            memcpy(&state.wolf_state.position, c->recv_buffer.data() + 1, sizeof(glm::vec2));
                            c->recv_buffer.erase(c->recv_buffer.begin(), c->recv_buffer.begin() + 1 + sizeof(glm::vec2));

                            memcpy(&state.wolf_state.face_left, c->recv_buffer.data(), sizeof(bool));
                            c->recv_buffer.erase(c->recv_buffer.begin(), c->recv_buffer.begin() + sizeof(bool));

                            memcpy(&state.wolf_state.disguise, c->recv_buffer.data(), sizeof(uint8_t));
                            c->recv_buffer.erase(c->recv_buffer.begin(), c->recv_buffer.begin() + sizeof(uint8_t));
                            break;
                    }
                    break;
                case Game::PlayerType::SPECTATOR:
                    break;
                default:
                    break;
            }
		}
	});

	//copy game state to scene positions:
	wolf_transform->position.x = state.wolf_state.position.x;
	wolf_transform->position.y = state.wolf_state.position.y;
	wolf_transform->rotation = state.wolf_state.face_left ? state.left_rotation : state.right_rotation;

	farmer_transform->position.x = state.farmer_state.position.x;
	farmer_transform->position.y = state.farmer_state.position.y;

	Game::AnimalMesh mesh_info = state.animal_meshes[state.wolf_state.disguise];
	wolfplayer_object->start = mesh_info.mesh_start;
	wolfplayer_object->count = mesh_info.mesh_count;
	wolfplayer_object->transform->scale = mesh_info.mesh_scale;
}

void GameMode::draw(glm::uvec2 const &drawable_size) {
	camera->aspect = drawable_size.x / float(drawable_size.y);

	glClearColor(0.25f, 0.0f, 0.5f, 0.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	//set up basic OpenGL state:
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);
	glBlendEquation(GL_FUNC_ADD);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	//set up light positions:
	glUseProgram(vertex_color_program->program);

	glUniform3fv(vertex_color_program->sun_color_vec3, 1, glm::value_ptr(glm::vec3(0.81f, 0.81f, 0.76f)));
	glUniform3fv(vertex_color_program->sun_direction_vec3, 1, glm::value_ptr(glm::normalize(glm::vec3(-0.2f, 0.2f, 1.0f))));
	glUniform3fv(vertex_color_program->sky_color_vec3, 1, glm::value_ptr(glm::vec3(0.2f, 0.2f, 0.3f)));
	glUniform3fv(vertex_color_program->sky_direction_vec3, 1, glm::value_ptr(glm::vec3(0.0f, 1.0f, 0.0f)));

	scene->draw(camera);

	GL_ERRORS();
}
