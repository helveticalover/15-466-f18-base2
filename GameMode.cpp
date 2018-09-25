#include "GameMode.hpp"

#include "MenuMode.hpp"
#include "Load.hpp"
#include "MeshBuffer.hpp"
#include "Scene.hpp"
#include "Sound.hpp"
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
#include <regex>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/string_cast.hpp>

Load< MeshBuffer > meshes(LoadTagDefault, [](){
	return new MeshBuffer(data_path("wolf-sheep-models.pnc"));
});

Load< GLuint > meshes_for_vertex_color_program(LoadTagDefault, [](){
	return new GLuint(meshes->make_vao_for_program(vertex_color_program->program));
});

Load< Sound::Sample > snarl_sample(LoadTagDefault, [](){
    return new Sound::Sample(data_path("wolf.wav"));
});

Load< Sound::Sample > whine_sample(LoadTagDefault, [](){
    return new Sound::Sample(data_path("whine.wav"));
});

Load< Sound::Sample > shot_sample(LoadTagDefault, [](){
    return new Sound::Sample(data_path("shot.wav"));
});

Load< Sound::Sample > sheep_sample(LoadTagDefault, [](){
    return new Sound::Sample(data_path("sheep.wav"));
});

Load< Sound::Sample > pig_sample(LoadTagDefault, [](){
    return new Sound::Sample(data_path("pig.wav"));
});

Load< Sound::Sample > cow_sample(LoadTagDefault, [](){
    return new Sound::Sample(data_path("cow.wav"));
});

Scene::Transform *player_transform = nullptr;   // parent transform for player mesh and camera
Scene::Transform *wolf_transform = nullptr;
Scene::Transform *farmer_transform = nullptr;
Scene::Object *wolfplayer_object = nullptr;
Scene::Object *hitmarker = nullptr;

std::regex sheepr ("(Sheep.)(.*)");
std::regex pigr ("(Pig.)(.*)");
std::regex cowr ("(Cow.)(.*)");
std::vector< Scene::Transform * > decoy_transforms;

Game::AnimalMesh wolf;
Game::AnimalMesh sheep;
Game::AnimalMesh pig;
Game::AnimalMesh cow;

Scene::Camera *camera = nullptr;

Load< Scene > scene(LoadTagDefault, [](){
	Scene *ret = new Scene;

	//load transform hierarchy:
	ret->load(data_path("wolf-sheep.scene"), [](Scene &s, Scene::Transform *t, std::string const &m){
		if (t->name == "SheepDisguise" || t->name == "PigDisguise" || t->name == "CowDisguise" ||
		    t->name == "SheepHighlight" || t->name == "PigHighlight" || t->name == "CowHighlight" ||
		    t->name == "WolfHighlight")
			return;

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

		if (t->name == "HitMarker") {
			hitmarker = obj;
		}
	});

	for (Scene::Transform *t = ret->first_transform; t != nullptr; t = t->alloc_next) {
		if (t->name == "Wolf") {
			if (wolf_transform) throw std::runtime_error("Multiple 'Wolf' transforms in scene.");
            wolf_transform = t;
            wolf.mesh_scale = t->scale;
		} else if (t->name == "Crosshair") {
            if (farmer_transform) throw std::runtime_error("Multiple 'Crosshair' transforms in scene.");
            farmer_transform = t;
		} else if (t->name == "Sheep.") {
				sheep.mesh_scale = t->scale;
		} else if (t->name == "Pig.") {
			pig.mesh_scale = t->scale;
		} else if (t->name == "Cow.") {
			cow.mesh_scale = t->scale;
		}

        if ((std::regex_match(t->name, sheepr) || std::regex_match(t->name, pigr) || std::regex_match(t->name, cowr)) &&
            !(t->name == "SheepDisguise" || t->name == "PigDisguise" || t->name == "CowDisguise" ||
            t->name == "SheepHighlight" || t->name == "PigHighlight" || t->name == "CowHighlight" ||
            t->name == "WolfHighlight")) {
            decoy_transforms.emplace_back(t);
        }
	}
	if (!wolf_transform) throw std::runtime_error("No 'Wolf' transform in scene.");
    if (!farmer_transform) throw std::runtime_error("No 'Crosshair' transform in scene.");

	//look up animal meshes:
	MeshBuffer::Mesh const &wolf_mesh = meshes->lookup("Wolf");
	MeshBuffer::Mesh const &sheep_mesh = meshes->lookup("Sheep");
	MeshBuffer::Mesh const &pig_mesh = meshes->lookup("Pig");
	MeshBuffer::Mesh const &cow_mesh = meshes->lookup("Cow");
	MeshBuffer::Mesh const &dsheep_mesh = meshes->lookup("SheepDisguise");
	MeshBuffer::Mesh const &dpig_mesh = meshes->lookup("PigDisguise");
	MeshBuffer::Mesh const &dcow_mesh = meshes->lookup("CowDisguise");
	MeshBuffer::Mesh const &hwolf_mesh = meshes->lookup("WolfHighlight");
    MeshBuffer::Mesh const &hsheep_mesh = meshes->lookup("SheepHighlight");
    MeshBuffer::Mesh const &hpig_mesh = meshes->lookup("PigHighlight");
    MeshBuffer::Mesh const &hcow_mesh = meshes->lookup("CowHighlight");

	wolf.mesh_start = wolf_mesh.start;
	wolf.mesh_count = wolf_mesh.count;
    wolf.hmesh_start = hwolf_mesh.start;
    wolf.hmesh_count = hwolf_mesh.count;
	sheep.mesh_start = sheep_mesh.start;
	sheep.mesh_count = sheep_mesh.count;
	sheep.dmesh_start = dsheep_mesh.start;
	sheep.dmesh_count = dsheep_mesh.count;
    sheep.hmesh_start = hsheep_mesh.start;
    sheep.hmesh_count = hsheep_mesh.count;
	pig.mesh_start = pig_mesh.start;
	pig.mesh_count = pig_mesh.count;
	pig.dmesh_start = dpig_mesh.start;
	pig.dmesh_count = dpig_mesh.count;
    pig.hmesh_start = hpig_mesh.start;
    pig.hmesh_count = hpig_mesh.count;
	cow.mesh_start = cow_mesh.start;
	cow.mesh_count = cow_mesh.count;
	cow.dmesh_start = dcow_mesh.start;
	cow.dmesh_count = dcow_mesh.count;
    cow.hmesh_start = hcow_mesh.start;
    cow.hmesh_count = hcow_mesh.count;

	// For hit detection
	wolf.shoot_x = 0.5f;
	wolf.shoot_y = 0.3f;
	sheep.shoot_x = 0.4f;
	sheep.shoot_y = 0.2f;
	pig.shoot_x = 0.6f;
	pig.shoot_y = 0.4f;
	cow.shoot_x = 0.8f;
	cow.shoot_y = 0.4f;
	cow.y_offset = 0.0f;

	//look up the camera:
	for (Scene::Camera *c = ret->first_camera; c != nullptr; c = c->alloc_next) {
		if (c->transform->name == "Camera") {
			if (camera) throw std::runtime_error("Multiple 'Camera' objects in scene.");
			camera = c;
		}
	}
	if (!camera) throw std::runtime_error("No 'Camera' camera in scene.");

	player_transform = ret->new_transform();
	return ret;
});

GameMode::GameMode(Client &client_) : client(client_) {
	client.connection.send_raw("h", 1); //send a 'hello' to the server

	state.animal_meshes.emplace_back(wolf);
	state.animal_meshes.emplace_back(sheep);
	state.animal_meshes.emplace_back(pig);
	state.animal_meshes.emplace_back(cow);

    std::regex sheepr ("(Sheep.)(.*)");
    std::regex pigr ("(Pig.)(.*)");
    std::regex cowr ("(Cow.)(.*)");

    for (uint32_t i = 0; i < state.num_decoys; ++i) {
        Game::Decoy d;
        d.position = glm::vec2(decoy_transforms[i]->position.x, decoy_transforms[i]->position.y);
        d.target = d.position;

//        float r = rand() / RAND_MAX;
//        d.face_left = r <= 0.5f;

        if (std::regex_match(decoy_transforms[i]->name, sheepr)) {
            d.animal = 1;
        } else if (std::regex_match(decoy_transforms[i]->name, pigr)) {
            d.animal = 2;
        } else if (std::regex_match(decoy_transforms[i]->name, cowr)) {
            d.animal = 3;
        } else {
            std::cerr << "Non-animal in decoy animals list" << std::endl;
        }

        state.decoy_animals.emplace_back(d);
    }
    std::cout << state.decoy_animals.size() << std::endl;

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

					player_transform->position = wolf_transform->position;
					player_transform->position.z = 0.0f;
					wolf_transform->position.x = 0.0f;
					wolf_transform->position.y = 0.0f;
					wolf_transform->set_parent(player_transform);

					player_transform->position = camera->transform->position;
					player_transform->position.z = 0.0f;
					camera->transform->position.x = 0.0f;
					camera->transform->position.y = 0.0f;
					camera->transform->set_parent(player_transform);

                    player_transform->scale.x = -1.0f;
                    player_transform->scale.y = -1.0f;

                    joined_team = true;
                    break;
                case 'f':
                    c->recv_buffer.erase(c->recv_buffer.begin(),
                                                        c->recv_buffer.begin() + 1);
                    std::cout << "JOINED AS FARMER" << std::endl;
                    state.player = Game::PlayerType::FARMER;

                    player_transform->position = farmer_transform->position;
                    player_transform->position.z = 0.0f;
                    farmer_transform->position.x = 0.0f;
                    farmer_transform->position.y = 0.0f;
                    farmer_transform->set_parent(player_transform);

                    player_transform->position = camera->transform->position;
					player_transform->position.z = 0.0f;
					camera->transform->position.x = 0.0f;
                    camera->transform->set_parent(player_transform);

                    player_transform->scale.x = -1.0f;
                    player_transform->scale.y = -1.0f;

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

    auto animal_cry = [&](int index, glm::mat4x3 animal_to_world) {
        switch(index) {
            case 0:
                snarl_sample->play(animal_to_world * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f), 2.0f, Sound::Once);
                break;
            case 1:
                sheep_sample->play(animal_to_world * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f), 2.0f, Sound::Once);
                break;
            case 2:
                pig_sample->play(animal_to_world * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f), 2.0f, Sound::Once);
                break;
            case 3:
                cow_sample->play(animal_to_world * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f), 2.0f, Sound::Once);
                break;
        }
    };

	auto try_shoot = [&]() -> int {
	    if (state.player != Game::PlayerType::FARMER) return -1;

		state.farmer_state.last_hit = state.farmer_state.position;
		glm::vec2 hit_center = state.farmer_state.position;

		float x_dist = state.animal_meshes[state.wolf_state.disguise].shoot_x;
		float y_dist = state.animal_meshes[state.wolf_state.disguise].shoot_y;
		float y_offset = state.animal_meshes[state.wolf_state.disguise].y_offset;

		if (glm::abs(state.wolf_state.position.x - hit_center.x) <= x_dist &&
			glm::abs(state.wolf_state.position.y + y_offset - hit_center.y) <= y_dist) {

            hitmarker->transform->position.x = state.wolf_state.position.x;
            hitmarker->transform->position.y = state.wolf_state.position.y;

			return state.num_decoys;
		}

		for (uint32_t i = 0; i < state.num_decoys; ++i) {
			x_dist = state.animal_meshes[state.decoy_animals[i].animal].shoot_x;
			y_dist = state.animal_meshes[state.decoy_animals[i].animal].shoot_y;
			y_offset = state.animal_meshes[state.decoy_animals[i].animal].y_offset;

			if (glm::abs(state.decoy_animals[i].position.x - hit_center.x) <= x_dist &&
				glm::abs(state.decoy_animals[i].position.y + y_offset - hit_center.y) <= y_dist) {

				hitmarker->transform->position.x = state.decoy_animals[i].position.x;
				hitmarker->transform->position.y = state.decoy_animals[i].position.y;

				return i;
			}
		}

        hitmarker->transform->position.x = state.farmer_state.position.x;
        hitmarker->transform->position.y = state.farmer_state.position.y;

		return -1;
	};

	auto try_eat = [&]() -> int {
		float x_min = state.wolf_state.position.x +
				(state.wolf_state.face_left ? -state.EatOffset : state.EatOffset) - state.EatRange;
		float x_max = state.wolf_state.position.x +
					  (state.wolf_state.face_left ? -state.EatOffset : state.EatOffset) + state.EatRange;
		float y_min = state.wolf_state.position.y - state.EatRange;
		float y_max = state.wolf_state.position.y + state.EatRange;

		for (uint32_t i = 0; i < state.num_decoys; ++i) {
			if (x_min <= state.decoy_animals[i].position.x && state.decoy_animals[i].position.x <= x_max &&
				y_min <= state.decoy_animals[i].position.y && state.decoy_animals[i].position.y <= y_max) {
				return (int)i;
			}
		}
		return -1;
	};

	state.target = -1;
	switch (state.player){
		case Game::PlayerType::WOLFPLAYER:

		    state.target = try_eat();

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

                Game::AnimalMesh mesh_info = state.animal_meshes[state.wolf_state.disguise];
                wolfplayer_object->start = mesh_info.dmesh_start;
                wolfplayer_object->count = mesh_info.dmesh_count;
                wolfplayer_object->transform->scale = mesh_info.mesh_scale;

                if (client.connection) {
                    client.connection.send_raw("d", 1);
                    client.connection.send_raw(&state.wolf_state.disguise, sizeof(uint8_t));
                }
				return true;
			}

			if (evt.type == SDL_KEYDOWN && evt.key.keysym.scancode == SDL_SCANCODE_LSHIFT) {
			    if (state.target >= 0) {
					glm::mat4x3 wolf_to_world = wolf_transform->make_local_to_world();
					snarl_sample->play(wolf_to_world * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f), 10.0f, Sound::Once);

					glm::mat4x3 animal_to_world = decoy_transforms[state.target]->make_local_to_world();
					animal_cry(state.decoy_animals[state.target].animal, animal_to_world);

					++state.wolf_state.num_eaten;

					std::cout << "Ate animal " + decoy_transforms[state.target]->name << std::endl;
					client.connection.send_raw("e", 1);
					client.connection.send_raw(&state.target, sizeof(int));
			    }

			    return true;
			}
			break;
		case Game::PlayerType::FARMER:

			state.target = try_shoot();

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

            if (evt.type == SDL_KEYDOWN && evt.key.keysym.scancode == SDL_SCANCODE_SPACE) {

                glm::mat4x3 farmer_to_world = camera->transform->make_local_to_world();
                shot_sample->play(farmer_to_world * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f), 5.0f, Sound::Once);

                if (state.target == (int)state.num_decoys) {
					glm::mat4x3 wolf_to_world = wolf_transform->make_local_to_world();
					whine_sample->play(wolf_to_world * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f), 2.0f, Sound::Once);
                    client.connection.send_raw("k", 1);
                } else if (state.target >= 0) {
					glm::mat4x3 animal_to_world = decoy_transforms[state.target]->make_local_to_world();
					uint8_t animal_index = state.decoy_animals[state.target].animal;
					animal_cry(animal_index, animal_to_world);

                    ++state.farmer_state.strikes;

					std::cout << "Shot animal " + decoy_transforms[state.target]->name << std::endl;
					client.connection.send_raw("j", 1);
					client.connection.send_raw(&state.target, sizeof(int));
                } else {
                    client.connection.send_raw("m", 1);
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

	auto animal_cry = [&](int index, glm::mat4x3 animal_to_world) {
		switch(index) {
			case 0:
				snarl_sample->play(animal_to_world * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f), 2.0f, Sound::Once);
				break;
			case 1:
				sheep_sample->play(animal_to_world * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f), 2.0f, Sound::Once);
				break;
			case 2:
				pig_sample->play(animal_to_world * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f), 2.0f, Sound::Once);
				break;
			case 3:
				cow_sample->play(animal_to_world * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f), 2.0f, Sound::Once);
				break;
		}
	};

    auto make_noise = [&]() {
        glm::mat4x3 wolf_to_world = wolf_transform->make_local_to_world();
		animal_cry(state.wolf_state.disguise, wolf_to_world);
    };

    auto try_make_noise = [&]() -> bool {
        if (state.player != Game::PlayerType::WOLFPLAYER) return false;

        state.wolf_state.last_cry += elapsed;
        if (state.wolf_state.last_cry < state.CryInterval) return false;

        state.wolf_state.last_cry = 0.0f;
        make_noise();
        return true;
    };

	bool update = state.update(elapsed);
	bool cry = try_make_noise();

	if (client.connection) {
		//send own game state and predicted game state to server:
        switch(state.player) {
        	case Game::PlayerType::WOLFPLAYER:		// WOLF
        		if (update) {	// ------------------- send actual wolf state
					client.connection.send_raw("w", 1);
					client.connection.send_raw(&state.wolf_state.position, sizeof(glm::vec2));
					client.connection.send_raw(&state.wolf_state.face_left, sizeof(bool));
        		}

        		if (cry) client.connection.send_raw("c", 1);
				break;
        	case Game::PlayerType::FARMER:			// FARMER
        	    if (update) {	// ------------------- send actual farmer state
                    client.connection.send_raw("f", 1);
                    client.connection.send_raw(&state.farmer_state.position, sizeof(glm::vec2));
        	    }
                break;
        	default:
        		std::cerr << "Unknown player type in GameMode::update." << std::endl;
        }
	}

	// Update game state if receive from server
	client.poll([&](Connection *c, Connection::Event event){
		if (event == Connection::OnOpen) {
		} else if (event == Connection::OnClose) {
			std::cerr << "Lost connection to server." << std::endl;
			exit(0);
		} else { assert(event == Connection::OnRecv);
			switch (state.player) {
				case Game::PlayerType::WOLFPLAYER:		    // WOLF
				    while (c->recv_buffer.size()) {
				        if (c->recv_buffer[0] == 'f') {
                            if (c->recv_buffer.size() < 1 + sizeof(glm::vec2)) break;

                            memcpy(&state.farmer_state.position, c->recv_buffer.data() + 1, sizeof(glm::vec2));
                            c->recv_buffer.erase(c->recv_buffer.begin(), c->recv_buffer.begin() + 1 + sizeof(glm::vec2));

				        } else if (c->recv_buffer[0] == 'a') {
				            if (c->recv_buffer.size() < 1 + sizeof(uint32_t) + sizeof(glm::vec2)) break;

				            uint32_t decoy_index;
                            memcpy(&decoy_index, c->recv_buffer.data() + 1, sizeof(uint32_t));
                            c->recv_buffer.erase(c->recv_buffer.begin(), c->recv_buffer.begin() + 1 + sizeof(uint32_t));

                            glm::vec2 goal;
                            memcpy(&goal, c->recv_buffer.data(), sizeof(glm::vec2));
                            c->recv_buffer.erase(c->recv_buffer.begin(), c->recv_buffer.begin() + sizeof(glm::vec2));

                            state.decoy_animals[decoy_index].target = goal;

				        } else if (c->recv_buffer[0] == 'k') {
				            c->recv_buffer.erase(c->recv_buffer.begin(), c->recv_buffer.begin() + 1);

                            hitmarker->transform->position.x = state.wolf_state.position.x;
                            hitmarker->transform->position.y = state.wolf_state.position.y;

                            glm::mat4x3 farmer_to_world = farmer_transform->make_local_to_world();
                            shot_sample->play(farmer_to_world * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f), 5.0f, Sound::Once);
                            glm::mat4x3 wolf_to_world = camera->transform->make_local_to_world();
                            whine_sample->play(wolf_to_world * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f), 2.0f, Sound::Once);

				            std::cout << "You've been killed!" << std::endl;

				        } else if (c->recv_buffer[0] == 'm') {
							if (c->recv_buffer.size() < 1 + sizeof(glm::vec2)) break;

							memcpy(&state.farmer_state.last_hit, c->recv_buffer.data() + 1, sizeof(glm::vec2));
							c->recv_buffer.erase(c->recv_buffer.begin(), c->recv_buffer.begin() + 1 + sizeof(glm::vec2));

							hitmarker->transform->position.x = state.farmer_state.last_hit.x;
							hitmarker->transform->position.y = state.farmer_state.last_hit.y;

                            glm::mat4x3 farmer_to_world = farmer_transform->make_local_to_world();
                            shot_sample->play(farmer_to_world * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f), 5.0f, Sound::Once);
				        } else if (c->recv_buffer[0] == 'j') {
							if (c->recv_buffer.size() < 1 + sizeof(int)) return;

							int animal_killed;
							memcpy(&animal_killed, c->recv_buffer.data() + 1, sizeof(int));
							c->recv_buffer.erase(c->recv_buffer.begin(), c->recv_buffer.begin() + 1 + sizeof(int));

							glm::mat4x3 farmer_to_world = farmer_transform->make_local_to_world();
							shot_sample->play(farmer_to_world * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f), 5.0f, Sound::Once);

							glm::mat4x3 animal_to_world = decoy_transforms[animal_killed]->make_local_to_world();
							uint8_t animal_index = state.decoy_animals[animal_killed].animal;
							animal_cry(animal_index, animal_to_world);
				        }
				    }
					break;
				case Game::PlayerType::FARMER:			    // FARMER
				    while (c->recv_buffer.size()) {
                        if (c->recv_buffer[0] == 'w') {     // update local wolf state
                            if (c->recv_buffer.size() < 1 + sizeof(glm::vec2) + sizeof(bool)) break;

                            memcpy(&state.wolf_state.position, c->recv_buffer.data() + 1, sizeof(glm::vec2));
                            c->recv_buffer.erase(c->recv_buffer.begin(),
                                                 c->recv_buffer.begin() + 1 + sizeof(glm::vec2));
                            memcpy(&state.wolf_state.face_left, c->recv_buffer.data(), sizeof(bool));
                            c->recv_buffer.erase(c->recv_buffer.begin(), c->recv_buffer.begin() + sizeof(bool));

                        } else if (c->recv_buffer[0] == 'a') {
                            if (c->recv_buffer.size() < 1 + sizeof(uint32_t) + sizeof(glm::vec2)) break;

                            uint32_t decoy_index;
                            memcpy(&decoy_index, c->recv_buffer.data() + 1, sizeof(uint32_t));
                            c->recv_buffer.erase(c->recv_buffer.begin(), c->recv_buffer.begin() + 1 + sizeof(uint32_t));

                            glm::vec2 goal;
                            memcpy(&goal, c->recv_buffer.data(), sizeof(glm::vec2));
                            c->recv_buffer.erase(c->recv_buffer.begin(), c->recv_buffer.begin() + sizeof(glm::vec2));

                            state.decoy_animals[decoy_index].target = goal;

                        } else if (c->recv_buffer[0] == 'd') {  // disguise
                            if (c->recv_buffer.size() < 1 + sizeof(glm::uint8_t)) break;

                            memcpy(&state.wolf_state.disguise, c->recv_buffer.data() + 1, sizeof(uint8_t));
                            c->recv_buffer.erase(c->recv_buffer.begin(), c->recv_buffer.begin() + 1 + sizeof(uint8_t));

                            Game::AnimalMesh mesh_info = state.animal_meshes[state.wolf_state.disguise];
                            wolfplayer_object->start = mesh_info.mesh_start;
                            wolfplayer_object->count = mesh_info.mesh_count;
                            wolfplayer_object->transform->scale = mesh_info.mesh_scale;

                        } else if (c->recv_buffer[0] == 'c') {
                            c->recv_buffer.erase(c->recv_buffer.begin(), c->recv_buffer.begin() + 1);
                            make_noise();

                        } else if (c->recv_buffer[0] == 'e') {
							if (c->recv_buffer.size() < 1 + sizeof(int)) return;

							int animal_killed;
							memcpy(&animal_killed, c->recv_buffer.data() + 1, sizeof(int));
							c->recv_buffer.erase(c->recv_buffer.begin(), c->recv_buffer.begin() + 1 + sizeof(int));

							glm::mat4x3 wolf_to_world = wolf_transform->make_local_to_world();
							snarl_sample->play(wolf_to_world * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f), 10.0f, Sound::Once);

							glm::mat4x3 animal_to_world = decoy_transforms[animal_killed]->make_local_to_world();
							uint8_t animal_index = state.decoy_animals[animal_killed].animal;
							animal_cry(animal_index, animal_to_world);
                        }
				    }
					break;
				default:
					break;
			}
		}
	});

	//copy game state to scene positions:
	switch(state.player) {
	    case Game::PlayerType::WOLFPLAYER:
	        player_transform->position.x = state.wolf_state.position.x;
	        player_transform->position.y = state.wolf_state.position.y;
	        wolf_transform->scale.z = state.wolf_state.face_left ? wolf_transform->scale.x : -1.0f * wolf_transform->scale.x;

            farmer_transform->position.x = state.farmer_state.position.x;
            farmer_transform->position.y = state.farmer_state.position.y;
	        break;
	    case Game::PlayerType::FARMER:
            player_transform->position.x = state.farmer_state.position.x;
            player_transform->position.y = state.farmer_state.position.y;

            wolf_transform->position.x = state.wolf_state.position.x;
            wolf_transform->position.y = state.wolf_state.position.y;
            wolf_transform->scale.z = state.wolf_state.face_left ? wolf_transform->scale.x : -1.0f * wolf_transform->scale.x;
	        break;
	    default:
	        break;
	}

	//transform references not working
	for (uint32_t i = 0; i < state.decoy_animals.size(); ++i) {
		Scene::Transform *t = decoy_transforms[i];
		Game::Decoy decoy_info = state.decoy_animals[i];

		t->position.x = decoy_info.position.x;
		t->position.y = decoy_info.position.y;
		t->scale.z = decoy_info.face_left ? t->scale.x : -1.0f * t->scale.x;
	}

	// Draw text
//    float height = 1.0f;
//    GLint viewport[4];
//    glGetIntegerv(GL_VIEWPORT, viewport);
////    float aspect = viewport[2] / float(viewport[3]);
//    std::string HUD;
//
//    switch(state.player) {
//	    case Game::PlayerType::WOLFPLAYER:
//	        HUD = "ANIMALS EATEN " + state.wolf_state.num_eaten;
//            draw_text(HUD, glm::vec2(0.0f, 0.0f), height, glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));
//	        break;
//	    case Game::PlayerType::FARMER:
//            HUD = "STRIKES " + state.farmer_state.strikes;
//            draw_text(HUD, glm::vec2(0.0f, 0.0f), height, glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));
//	        break;
//        default:
//            break;
//	}
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
