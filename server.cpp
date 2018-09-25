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

	std::vector< Connection *> spectators;

	while (1) {
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

				while (c->recv_buffer.size()) {
                    if (c->recv_buffer[0] == 'h') {
                            c->recv_buffer.erase(c->recv_buffer.begin(), c->recv_buffer.begin() + 1);
                            std::cout << c << ": Got hello." << std::endl;
                            break;
                    } else if (c->recv_buffer[0] == 'w') { // Wolf state
                        if (c->recv_buffer.size() < 1 + sizeof(glm::vec2) + sizeof(bool)) break;

                        memcpy(&globalState.wolf_state.position, c->recv_buffer.data() + 1, sizeof(glm::vec2));
                        c->recv_buffer.erase(c->recv_buffer.begin(), c->recv_buffer.begin() + 1 + sizeof(glm::vec2));

                        memcpy(&globalState.wolf_state.face_left, c->recv_buffer.data(), sizeof(bool));
                        c->recv_buffer.erase(c->recv_buffer.begin(), c->recv_buffer.begin() + sizeof(bool));

                        if (farmer) {
                            farmer->send_raw("w", 1);
                            farmer->send_raw(&globalState.wolf_state.position, sizeof(glm::vec2));
                            farmer->send_raw(&globalState.wolf_state.face_left, sizeof(bool));
                        }
                    } else if (c->recv_buffer[0] == 'f') { // Farmer state
                        if (c->recv_buffer.size() < 1 + sizeof(glm::vec2)) break;

                        memcpy(&globalState.farmer_state.position, c->recv_buffer.data() + 1, sizeof(glm::vec2));
                        c->recv_buffer.erase(c->recv_buffer.begin(), c->recv_buffer.begin() + 1 + sizeof(glm::vec2));

                        if (wolf) {
                            wolf->send_raw("f", 1);
                            wolf->send_raw(&globalState.farmer_state.position, sizeof(glm::vec2));
                        }
                    } else if (c->recv_buffer[0] == 'd') { // Change disguise
                        if (c->recv_buffer.size() < 1 + sizeof(uint8_t)) return;

                        memcpy(&globalState.wolf_state.disguise, c->recv_buffer.data() + 1, sizeof(uint8_t));
                        c->recv_buffer.erase(c->recv_buffer.begin(), c->recv_buffer.begin() + 1 + sizeof(uint8_t));

                        if (farmer) {
                            farmer->send_raw("d", 1);
                            farmer->send_raw(&globalState.wolf_state.disguise, sizeof(uint8_t));
                        }
                    } else if (c->recv_buffer[0] == 'k') {
                        c->recv_buffer.erase(c->recv_buffer.begin(), c->recv_buffer.begin() + 1);
                        if (wolf) {
                            wolf->send_raw("k", 1);
                        }
                    } else if (c->recv_buffer[0] == 'm') {
                        c->recv_buffer.erase(c->recv_buffer.begin(), c->recv_buffer.begin() + 1);
                        if (wolf) {
                            wolf->send_raw("m", 1);
                            wolf->send_raw(&globalState.farmer_state.position, sizeof(glm::vec2));
                        }
                    }
				}

			}
		}, 0.01);

		// -------------------------- UPDATE CLIENT STATES




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
