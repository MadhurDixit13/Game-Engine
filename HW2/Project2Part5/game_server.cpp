#include <zmq.hpp>
#include <iostream>
#include <map>
#include <string>
#include <thread>
#include <SFML/System.hpp>
#include <future>

using namespace sf;

struct CharacterState {
    float characterX;
    float characterY;
};

struct GameState {
    std::map<int, Vector2f> characterPositions;
};

class GameServer {
public:
    GameServer() : clientID(0) {}

    int get_clientId() {
        return clientID;
    }
    void update_clientID() {
        clientID = clientID + 1;
    }

protected:
    int clientID;
};

void handleClientRequests(zmq::socket_t& req_socket, GameServer& server, GameState& gameState, CharacterState& characterState) {
    while (true) {
        zmq::message_t response;
        req_socket.recv(response, zmq::recv_flags::none);

        std::string responseStr(static_cast<char*>(response.data()), response.size());
        std::cout << responseStr << std::endl;
        int clientId;

        if (responseStr == "Joined") {
            server.update_clientID();
            clientId = server.get_clientId();

            std::string reply = std::to_string(clientId);
            std::cout << reply << std::endl;

            req_socket.send(zmq::const_buffer(reply.c_str(), reply.size()), zmq::send_flags::none);
        } else {
            sscanf(responseStr.c_str(), "%d,%f,%f", &clientId,
                   &characterState.characterX,
                   &characterState.characterY);
            gameState.characterPositions[clientId] = Vector2f(characterState.characterX, characterState.characterY);
            std::string serializedData = "";
            for (auto i : gameState.characterPositions) {
                serializedData = serializedData + std::to_string(i.first) + "," + std::to_string(i.second.x) + "," + std::to_string(i.second.y) + ",";
            }
            std::string reply = "Coordinates received";
            req_socket.send(zmq::const_buffer(reply.c_str(), reply.size()), zmq::send_flags::none);
            // pub_socket.send(zmq::const_buffer(serializedData.c_str(), serializedData.size()), zmq::send_flags::none);
        }
    }
}

int main() {
    CharacterState characterState;
    GameState gameState;
    GameServer server;

    float movingPlatformPositionX = 480.f;
    float platformVelocity = 150.f;
    float movementSpeed = 150.f;
    const unsigned WINDOW_WIDTH = 1800;
    float platformWidth = 500.f;
    zmq::context_t context(1);
    Clock dt_clock;
    zmq::socket_t req_socket(context, ZMQ_REP);
    req_socket.bind("tcp://*:5556");
    zmq::socket_t pub_socket(context, ZMQ_PUB);
    pub_socket.bind("tcp://*:5555");

    std::cout << "Connection established" << std::endl;

    // Start a thread to handle client requests
    std::thread requestThread(handleClientRequests, std::ref(req_socket), std::ref(server), std::ref(gameState), std::ref(characterState));


    while (true) {
        float dt = dt_clock.restart().asSeconds();

        if (movingPlatformPositionX <= 480) {
            platformVelocity = movementSpeed;
        } else if (movingPlatformPositionX + platformWidth >= WINDOW_WIDTH) {
            platformVelocity = -movementSpeed;
        }
        movingPlatformPositionX += platformVelocity * dt;

        // for (auto& character : gameState.characterPositions) {
        // int clientId = character.first;
        // Vector2f& charPosition = character.second;

        // // Update the character's X position to maintain the relative position to the moving platform
        // charPosition.x += platformVelocity * dt;
        // // charPosition.y can be updated as well if needed
        // }

        std::string serializedData = "";
        for (auto i : gameState.characterPositions) {
            serializedData = serializedData + std::to_string(i.first) + "," + std::to_string(i.second.x) + "," + std::to_string(i.second.y) + "," + std::to_string(movingPlatformPositionX) + ",";
        }
        pub_socket.send(zmq::const_buffer(serializedData.c_str(), serializedData.size()), zmq::send_flags::none);
    }
     
    // Wait for the request thread to finish (this may never happen in this example)
    requestThread.join();

    return 0;
}
