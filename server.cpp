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

	//TODO: optimize-- vec3 -> vec2 (with less precision), rotation -> left/right, final destination of moving animals
	while (1) {
	    bool update_wolf = false;
//	    bool update_farmer = false;

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
			} else if (evt == Connection::OnClose) {
			    if (wolf == c) wolf = nullptr;
			    else if (farmer == c) farmer = nullptr;
			} else {
				assert(evt == Connection::OnRecv);

				if (c->recv_buffer[0] == 'h') {
					c->recv_buffer.erase(c->recv_buffer.begin(), c->recv_buffer.begin() + 1);
					std::cout << c << ": Got hello." << std::endl;
				}

				else if (c->recv_buffer[0] == 'w') {	// ---------------------- WOLF
					switch(c->recv_buffer[1]) {
						case 'p':	// ------------------------------- wolf position
							if (c->recv_buffer.size() < 2 + sizeof(glm::vec3)) {
								return; //wait for more data
							} else {
								memcpy(&globalState.wolf_state.position, c->recv_buffer.data() + 2, sizeof(glm::vec3));
								c->recv_buffer.erase(c->recv_buffer.begin(), c->recv_buffer.begin() + 2 + sizeof(glm::vec3));
                                std::cout << glm::to_string(globalState.wolf_state.position) << std::endl;
                                update_wolf = true;
							}
							break;
						case 'r':	// ------------------------------ wolf rotation
							if (c->recv_buffer.size() < 2 + sizeof(glm::quat)) {
								return; //wait for more data
							} else {
								memcpy(&globalState.wolf_state.rotation, c->recv_buffer.data() + 2, sizeof(glm::quat));
								c->recv_buffer.erase(c->recv_buffer.begin(), c->recv_buffer.begin() + 2 + sizeof(glm::quat));
								update_wolf = true;
							}
							break;
						default:
							std::cerr << "Unknown wolf message from client" << std::endl;
							break;
					}
				}
			}
		}, 0.01);

		// -------------------------- UPDATE CLIENT GAME STATES

		// farmer position, animals
		if (wolf) {

		}

        // wolf position, wolf disguise, animals
		if (farmer && update_wolf) {
            farmer->send_raw("wp", 2);
            farmer->send_raw(&globalState.wolf_state.position, sizeof(glm::vec3));
            farmer->send_raw("wr", 2);
            farmer->send_raw(&globalState.wolf_state.rotation, sizeof(glm::quat));
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
