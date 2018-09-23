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

Scene::Transform *paddle_transform = nullptr;
Scene::Transform *ball_transform = nullptr;

Scene::Transform *wolf_transform = nullptr;

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
	});

	//look up paddle and ball transforms:
	for (Scene::Transform *t = ret->first_transform; t != nullptr; t = t->alloc_next) {
//		if (t->name == "Paddle") {
//			if (paddle_transform) throw std::runtime_error("Multiple 'Paddle' transforms in scene.");
//			paddle_transform = t;
//		}
//		if (t->name == "Ball") {
//			if (ball_transform) throw std::runtime_error("Multiple 'Ball' transforms in scene.");
//			ball_transform = t;
//		}

		if (t->name == "Wolf") {
			if (wolf_transform) throw std::runtime_error("Multiple 'Wolf' transforms in scene.");

//			Scene::Transform *n = ret->new_transform();
//			n->position = t->position;
//			t->position = glm::vec3(0.0f, 0.0f, 0.0f);
//			t->set_parent(n);
//			wolf_transform = n;
            wolf_transform = t;

			std::cout << glm::to_string(t->position) << std::endl;
			std::cout << glm::to_string(t->rotation) << std::endl;
		}
	}
//	if (!paddle_transform) throw std::runtime_error("No 'Paddle' transform in scene.");
//	if (!ball_transform) throw std::runtime_error("No 'Ball' transform in scene.");
	if (!wolf_transform) throw std::runtime_error("No 'Wolf' transform in scene.");

	paddle_transform = ret->new_transform();
	ball_transform = ret->new_transform();

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
                    state.player = Game::PlayerType::WOLF;
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
		case Game::PlayerType::WOLF:
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
			break;
		case Game::PlayerType::FARMER:
			break;
		case Game::PlayerType::SPECTATOR:
			break;
		default:
			std::cerr << "Unknown player type in GameMode::handle_event." << std::endl;
	}


//	if (evt.type == SDL_MOUSEMOTION) {
//		state.paddle.x = (evt.motion.x - 0.5f * window_size.x) / (0.5f * window_size.x) * Game::FrameWidth;
//		state.paddle.x = std::max(state.paddle.x, -0.5f * Game::FrameWidth + 0.5f * Game::PaddleWidth);
//		state.paddle.x = std::min(state.paddle.x,  0.5f * Game::FrameWidth - 0.5f * Game::PaddleWidth);
//	}

	return false;
}

void GameMode::update(float elapsed) {
	bool update = state.update(elapsed);

	if (client.connection) {
		//send game state to server:
        switch(state.player) {
        	case Game::PlayerType::WOLF:
				client.connection.send_raw("wp", 2);
				client.connection.send_raw(&state.wolf_state.position, sizeof(glm::vec3));
				client.connection.send_raw("wr", 2);
				client.connection.send_raw(&state.wolf_state.rotation, sizeof(glm::quat));
				break;
        	case Game::PlayerType::FARMER:
        		break;
        	case Game::PlayerType::SPECTATOR:
        		break;
        	default:
        		std::cerr << "Unknown player type in GameMode::update." << std::endl;
        }
	}

	client.poll([&](Connection *c, Connection::Event event){
		if (event == Connection::OnOpen) {
			//probably won't get this.
		} else if (event == Connection::OnClose) {
			std::cerr << "Lost connection to server." << std::endl;
			exit(0);
		} else { assert(event == Connection::OnRecv);
            switch (state.player) {
                case Game::PlayerType::WOLF:
                    break;
                case Game::PlayerType::FARMER:
                    assert(c->recv_buffer[0] == 'w');
                    switch(c->recv_buffer[1]) {
                        case 'p':	// ------------------------------- wolf position
                            if (c->recv_buffer.size() < 2 + sizeof(glm::vec3)) {
                                return; //wait for more data
                            } else {
                                memcpy(&state.wolf_state.position, c->recv_buffer.data() + 2, sizeof(glm::vec3));
                                c->recv_buffer.erase(c->recv_buffer.begin(), c->recv_buffer.begin() + 2 + sizeof(glm::vec3));
                                std::cout << glm::to_string(state.wolf_state.position) << std::endl;
                            }
                            break;
                        case 'r':	// ------------------------------ wolf rotation
                            if (c->recv_buffer.size() < 2 + sizeof(glm::quat)) {
                                return; //wait for more data
                            } else {
                                memcpy(&state.wolf_state.rotation, c->recv_buffer.data() + 2, sizeof(glm::quat));
                                c->recv_buffer.erase(c->recv_buffer.begin(), c->recv_buffer.begin() + 2 + sizeof(glm::quat));
                            }
                            break;
                        default:
                            std::cerr << "Unknown wolf message from client" << std::endl;
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
	ball_transform->position.x = state.ball.x;
	ball_transform->position.y = state.ball.y;

	paddle_transform->position.x = state.paddle.x;
	paddle_transform->position.y = state.paddle.y;

	wolf_transform->position = state.wolf_state.position;
	wolf_transform->rotation = state.wolf_state.rotation;
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
