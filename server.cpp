#include "Connection.hpp"
#include "Game.hpp"

#include <iostream>
#include <set>
#include <chrono>

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/string_cast.hpp>

int main(int argc, char **argv) {
	if (argc != 2) {
		std::cerr << "Usage:\n\t./server <port>" << std::endl;
		return 1;
	}
	
	Server server(argv[1]);

	Game globalState;
	Connection *wolf = nullptr;
	Connection *farmer = nullptr;

	glm::vec2 wolf_position;
	bool wolf_face_left;
	uint8_t wolf_disguise;
	glm::vec2 farmer_position;

	std::vector< Connection *> spectators;

	while (1) {
	    bool update_farmer = false;
	    bool update_wolf = false;

		server.poll([&](Connection *c, Connection::Event evt){

			if (evt == Connection::OnOpen) {    // ---------------------- ASSIGN ROLES
				if (!wolf) {
					wolf = c;
					wolf->send_raw("w", 1);
				} else if (!farmer) {
					farmer = c;
					farmer->send_raw("f", 1);
				} else {
					spectators.emplace_back(c);
					c->send_raw("s", 1);
				}
			}

			else if (evt == Connection::OnClose) {
			    if (wolf == c) wolf = nullptr;
			    else if (farmer == c) farmer = nullptr;
			}

			else {
				assert(evt == Connection::OnRecv);
				switch (c->recv_buffer[0]) {
				    case 'h':
                        c->recv_buffer.erase(c->recv_buffer.begin(), c->recv_buffer.begin() + 1);
                        std::cout << c << ": Got hello." << std::endl;
				        break;
				    case 'w':
                        if (c->recv_buffer.size() < 2) return;
                        switch(c->recv_buffer[1]) {
                            case 's':	// ------------------------------- wolf position
                                if (c->recv_buffer.size() < 2 + sizeof(glm::vec2) + sizeof(bool) + sizeof(uint8_t)) return;

                                memcpy(&globalState.wolf_state.position, c->recv_buffer.data() + 2, sizeof(glm::vec2));
                                c->recv_buffer.erase(c->recv_buffer.begin(), c->recv_buffer.begin() + 2 + sizeof(glm::vec2));

                                memcpy(&globalState.wolf_state.face_left, c->recv_buffer.data(), sizeof(bool));
                                c->recv_buffer.erase(c->recv_buffer.begin(), c->recv_buffer.begin() + sizeof(bool));

								memcpy(&globalState.wolf_state.disguise, c->recv_buffer.data(), sizeof(uint8_t));
								c->recv_buffer.erase(c->recv_buffer.begin(), c->recv_buffer.begin() + sizeof(uint8_t));

								break;
                            case 'f':
                                if (c->recv_buffer.size() < 2 + sizeof(glm::vec2)) return;

                                memcpy(&farmer_position, c->recv_buffer.data() + 2, sizeof(glm::vec2));
                                c->recv_buffer.erase(c->recv_buffer.begin(), c->recv_buffer.begin() + 2 + sizeof(glm::vec2));

                                update_wolf = farmer_position != globalState.farmer_state.position;
                                break;
                            default:
                                std::cerr << "Unknown wolf message from client" << std::endl;
                                break;
                        }
				        break;
				    case 'f':
                        if (c->recv_buffer.size() < 2) return;
                        switch(c->recv_buffer[1]) {
                            case 's':
                                if (c->recv_buffer.size() < 2 + sizeof(glm::vec2)) return;

                                memcpy(&globalState.farmer_state.position, c->recv_buffer.data() + 2, sizeof(glm::vec2));
                                c->recv_buffer.erase(c->recv_buffer.begin(), c->recv_buffer.begin() + 2 + sizeof(glm::vec2));

                                break;
                            case 'w':
                                if (c->recv_buffer.size() < 2 + sizeof(glm::vec2) + sizeof(bool) + sizeof(uint8_t)) return;

                                memcpy(&wolf_position, c->recv_buffer.data() + 2, sizeof(glm::vec2));
                                c->recv_buffer.erase(c->recv_buffer.begin(), c->recv_buffer.begin() + 2 + sizeof(glm::vec2));

                                memcpy(&wolf_face_left, c->recv_buffer.data(), sizeof(bool));
                                c->recv_buffer.erase(c->recv_buffer.begin(), c->recv_buffer.begin() + sizeof(bool));

								memcpy(&wolf_disguise, c->recv_buffer.data(), sizeof(uint8_t));
								c->recv_buffer.erase(c->recv_buffer.begin(), c->recv_buffer.begin() + sizeof(uint8_t));

                                update_farmer = wolf_position != globalState.wolf_state.position ||
                                		wolf_face_left != globalState.wolf_state.face_left ||
                                		wolf_disguise != globalState.wolf_state.disguise;
                                break;
                            default:
                                std::cerr << "Unknown wolf message from client" << std::endl;
                                break;
                        }
				        break;
				    default:
				        break;
				}

			}
		}, 0.01);

		// -------------------------- UPDATE CLIENT GAME STATES

		// farmer position, animals
		if (wolf && update_wolf) {
            wolf->send_raw("f", 1);
            wolf->send_raw(&globalState.farmer_state.position, sizeof(glm::vec2));
		}

        // wolf position, wolf disguise, animals
		if (farmer && update_farmer) {
            farmer->send_raw("w", 1);
            farmer->send_raw(&globalState.wolf_state.position, sizeof(glm::vec2));
            farmer->send_raw(&globalState.wolf_state.face_left, sizeof(bool));
            farmer->send_raw(&globalState.wolf_state.disguise, sizeof(uint8_t));
		}

		//wolf position, farmer position, animals
		if (spectators.size()) {

		}

		//every second or so, dump the current paddle position:
//		static auto then = std::chrono::steady_clock::now();
//		auto now = std::chrono::steady_clock::now();
//		if (now > then + std::chrono::seconds(1)) {
//			then = now;
//			std::cout << "Current wolf position: " << glm::to_string(globalState.wolf_state.position) << std::endl;
//			//std::cout << "Current paddle position: " << globalState.paddle.x << std::endl;
//		}
	}
}
