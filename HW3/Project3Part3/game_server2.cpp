#include <zmq.hpp>
#include <iostream>
#include <map>
#include <string>
#include <thread>
#include <SFML/System.hpp>
#include <SFML/Graphics.hpp>
#include "json.hpp"

using json = nlohmann::json;
using namespace sf;

struct CharacterState {
    float characterX;
    float characterY;
};

struct GameState {
    std::map<int, Vector2f> characterPositions;
    std::vector<Vector2f> movingPlatformPosition;
    std::vector<Vector2f> platformPositions;
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

class NetworkManager {
public:
    NetworkManager() : context(1), req_socket(context, ZMQ_REP), pub_socket(context, ZMQ_PUB) {
        req_socket.bind("tcp://*:5556");
        pub_socket.bind("tcp://*:5555");
        std::cout << "Connection established" << std::endl;
    }

    void handleClientRequests(GameServer& server, GameState& gameState, CharacterState& characterState) {
        while (true) {
            zmq::message_t response;
            req_socket.recv(response, zmq::recv_flags::none);

            std::string responseStr(static_cast<char*>(response.data()), response.size());
            int clientId;

            if (responseStr == "Joined") {
                server.update_clientID();
                clientId = server.get_clientId();
                std::string reply = std::to_string(clientId);
                req_socket.send(zmq::const_buffer(reply.c_str(), reply.size()), zmq::send_flags::none);
            } else if (responseStr.find("Disconnect") != std::string::npos) {
                sscanf(responseStr.c_str(), "%d,Disconnect", &clientId);
                gameState.characterPositions.erase(clientId);
                gameState.characterPositions[clientId] = Vector2f(-100.f, -100.f);
                std::string reply = "Goodbye" + std::to_string(clientId);
                req_socket.send(zmq::const_buffer(reply.c_str(), reply.size()), zmq::send_flags::none);
            } else {
                sscanf(responseStr.c_str(), "%d,%f,%f", &clientId, &characterState.characterX, &characterState.characterY);
                gameState.characterPositions[clientId] = Vector2f(characterState.characterX, characterState.characterY);
                std::string reply = "Coordinates received";
                req_socket.send(zmq::const_buffer(reply.c_str(), reply.size()), zmq::send_flags::none);
            }
        }
    }

    void dataPublish(GameState& gameState) {
        json gameStateJson;
        json characterPositionsJson;
        for (const auto& entry : gameState.characterPositions) {
            characterPositionsJson[std::to_string(entry.first)] = {entry.second.x, entry.second.y};
        }
        json movingPlatformPositionsJson;
        

        for (const sf::Vector2f& position : gameState.movingPlatformPosition) {
            json positionJson;
            positionJson["x"] = position.x;
            positionJson["y"] = position.y;
            movingPlatformPositionsJson.push_back(positionJson);
        }
        json platformPositionsJson;
        

        for (const sf::Vector2f& position : gameState.platformPositions) {
            json positionJson;
            positionJson["x"] = position.x;
            positionJson["y"] = position.y;
            platformPositionsJson.push_back(positionJson);
        }
        gameStateJson["characterPositions"] = characterPositionsJson;
        gameStateJson["movingPlatformPosition"] = movingPlatformPositionsJson;
        gameStateJson["platformPositions"] = platformPositionsJson;

        // Step 2: Convert JSON to string
        std::string jsonData = gameStateJson.dump();
        // for (const auto& entry : gameState.characterPositions) {
        //     serializedData[std::to_string(entry.first)] = {entry.second.x, entry.second.y};
        // }
        // std::string jsonData = serializedData.dump();
        pub_socket.send(zmq::const_buffer(jsonData.c_str(), jsonData.size()), zmq::send_flags::none);
    }

private:
    zmq::context_t context;
    zmq::socket_t req_socket;
    zmq::socket_t pub_socket;
};

class Platform {
public:
    Platform(sf::Color color, sf::Color outlineColor, Vector2f pos, Vector2f size) : platform(size) {
        platform.setFillColor(color);
        platform.setOutlineThickness(5);
        platform.setOutlineColor(outlineColor);
        platform.setPosition(pos);
    }

    Vector2f get_position() {
        return platform.getPosition();
    }

    sf::FloatRect getBounds() {
        return platform.getGlobalBounds();
    }

    sf::RectangleShape getPlatform() {
        return platform;
    }

    void platform_set_texture(sf::Texture& texture) {
        platform.setTexture(&texture);
    }

protected:
    sf::RectangleShape platform;
    sf::FloatRect platformBorder = platform.getGlobalBounds();
};

class MovingPlatform {
public:
    MovingPlatform(sf::Color color, sf::Color outlineColor, Vector2f size, Vector2f pos, Vector2f bound, Vector2f speed)
        : movingplatform(size), movementSpeed(speed), movingPlatformPosition(pos), boundary(bound) {
        movingplatform.setFillColor(color);
        movingplatform.setOutlineThickness(5);
        movingplatform.setOutlineColor(outlineColor);
        movingplatform.setPosition(pos);
    }

    sf::RectangleShape getMovingPlatform() {
        return movingplatform;
    }

    sf::FloatRect getBounds() {
        return movingplatform.getGlobalBounds();
    }

    Vector2f getPlatformVelocity() {
        return platformVelocity;
    }

    void setPlatformVelocity(Vector2f vel) {
        platformVelocity = vel;
    }

    Vector2f getMovingPlatformPosition() {
        return movingplatform.getPosition();
    }

    void set_position(float x, float y) {
        movingplatform.setPosition(x, y);
    }

    void platformMovement(unsigned WINDOW_WIDTH) {
        if (movingplatform.getPosition().x <= boundary.x) {
            platformVelocity.x = movementSpeed.x;
        } else if (movingplatform.getPosition().x + movingPlatformBorder.width >= boundary.x + 1200.f) {
            platformVelocity.x = -movementSpeed.x;
        }

        if (movingplatform.getPosition().y <= boundary.y) {
            platformVelocity.y = movementSpeed.y;
        } else if (movingplatform.getPosition().y + movingPlatformBorder.height >= boundary.y + 500.f) {
            platformVelocity.y = -movementSpeed.y;
        }
    }

    void movingPlatform_set_texture(sf::Texture& texture) {
        movingplatform.setTexture(&texture);
    }

protected:
    Vector2f movementSpeed;
    Vector2f movingPlatformPosition;
    Vector2f boundary;
    Vector2f platformVelocity;
    Vector2f prevMovingPlatformPosition;
    sf::RectangleShape movingplatform;
    sf::FloatRect movingPlatformBorder = movingplatform.getGlobalBounds();
};

class Server {
public:
    Server() {}

    void run() {
        platforms.push_back(Platform(sf::Color(165, 42, 42), sf::Color(0, 255, 0), sf::Vector2f(10.f, 600.f), sf::Vector2f(400.f, 100.f)));
        platforms.push_back(Platform(sf::Color(165, 42, 42), sf::Color(0, 0, 255), sf::Vector2f(1800.f, 500.f), sf::Vector2f(200.f, 100.f)));
        movingPlatforms.push_back(
            MovingPlatform(sf::Color(165, 42, 42), sf::Color(255, 0, 0), sf::Vector2f(500.f, 100.f), Vector2f(480.f, 500.f), Vector2f(480.f, 500.f), Vector2f(150.f, 0.f)));
        movingPlatforms.push_back(
            MovingPlatform(sf::Color(165, 42, 42), sf::Color(200, 255, 0), sf::Vector2f(500.f, 100.f), Vector2f(2150.f, 200.f), Vector2f(2150.f, 200.f), Vector2f(0.f, 150.f)));

        std::thread requestThread(&NetworkManager::handleClientRequests, &network, std::ref(server), std::ref(gameState), std::ref(characterState));

        while (true) {
            float dt = dt_clock.restart().asSeconds();
            for (int i = 0; i < movingPlatforms.size(); i++) {
                movingPlatforms[i].platformMovement(WINDOW_WIDTH);
                movingPlatforms[i].set_position(movingPlatforms[i].getMovingPlatformPosition().x + movingPlatforms[i].getPlatformVelocity().x * dt,
                                               movingPlatforms[i].getMovingPlatformPosition().y + movingPlatforms[i].getPlatformVelocity().y * dt);
            }

            network.dataPublish(gameState);
        }

        requestThread.join();
    }

protected:
    CharacterState characterState;
    GameState gameState;
    NetworkManager network;
    GameServer server;
    std::vector<MovingPlatform> movingPlatforms;
    std::vector<Platform> platforms;
    Clock dt_clock;
    const unsigned WINDOW_WIDTH = 1800;
};

int main() {
    Server server;
    server.run();
    return 0;
}
