#include <zmq.hpp>
#include <iostream>
#include <map>
#include <string>
#include <thread>
#include <SFML/System.hpp>
#include <SFML/Graphics.hpp>
#include <future>

using namespace sf;

struct CharacterState {
    float characterX;
    float characterY;
};

struct GameState {
    std::map<int, Vector2f> characterPositions;
    std::map<int, std::chrono::time_point<std::chrono::steady_clock>> clientTimestamps; 
    std::map<int, float> prevTimestamps;
};

class GameObject {
public:
    GameObject(sf::Shape *object) : shape(object) {}

    sf::Vector2f get_position() {
        return shape->getPosition();
	}
	sf::FloatRect getBoundsRectangleShape(RectangleShape *object){
		return object->getGlobalBounds();
	}
	void set_position_rectangle(RectangleShape *object, Vector2f pos){
		object->setPosition(pos);
	}
protected:
    sf::Shape *shape;
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
    NetworkManager() : context(1), req_socket(context, ZMQ_REP), pub_socket(context, ZMQ_PUB), disconnect_socket(context, ZMQ_REQ) {
        req_socket.bind("tcp://*:5556");
        pub_socket.bind("tcp://*:5555");
        disconnect_socket.bind("tcp://*:5557");
        std::cout << "Connection established" << std::endl;
    }
    void handleClientRequests(GameServer& server, GameState& gameState, CharacterState& characterState) {
    while (true) {
        zmq::message_t response;
        req_socket.recv(response, zmq::recv_flags::none);
        // req_socket.setsockopt(ZMQ_RCVTIMEO, 3000);
        std::string responseStr(static_cast<char*>(response.data()), response.size());
        // std::cout << responseStr << std::endl;
        int clientId;
        if (responseStr == "Joined") {
            server.update_clientID();
            clientId = server.get_clientId();
            gameState.prevTimestamps[clientId]=0.f;
            std::string reply = std::to_string(clientId);
            req_socket.send(zmq::const_buffer(reply.c_str(), reply.size()), zmq::send_flags::none);
        }
        else{
            float clientTimeStamp;
            sscanf(responseStr.c_str(), "%d,%f,%f,%f", &clientId,
                   &characterState.characterX,
                   &characterState.characterY,
                   &clientTimeStamp);
            gameState.characterPositions[clientId] = Vector2f(characterState.characterX, characterState.characterY);
            gameState.clientTimestamps[clientId] = std::chrono::steady_clock::now();
            std::string serializedData = "";
            for (auto i : gameState.characterPositions) {
                serializedData = serializedData + std::to_string(i.first) + "," + std::to_string(i.second.x) + "," + std::to_string(i.second.y) + ",";
            }
            std::string reply = "Coordinates received";
            req_socket.send(zmq::const_buffer(reply.c_str(), reply.size()), zmq::send_flags::none);
        }
        }        
    }
  void checkForDisconnectedClients(GameState& gameState) {
    const std::chrono::seconds disconnectTimeout(5); // Adjust the timeout as needed

    while (true) {
        auto now = std::chrono::steady_clock::now();
        for (auto it = gameState.clientTimestamps.begin(); it != gameState.clientTimestamps.end();) {
            int clientId = it->first;
            auto timeElapsed = now - gameState.clientTimestamps[clientId];
            if (timeElapsed >= disconnectTimeout) {
                std::cout << "Client " << clientId << " disconnected." << std::endl;
                // Remove the client from GameState
                it = gameState.clientTimestamps.erase(it);
                gameState.characterPositions[clientId] = Vector2f(-100.f, -100.f);
            } else {
                ++it;
            }
        }
        std::this_thread::sleep_for(std::chrono::seconds(1)); // Check once per second
    }
    }
    void dataPublish(zmq::const_buffer data){
        pub_socket.send(data, zmq::send_flags::none);
    }
private:
    zmq::context_t context;
    zmq::socket_t req_socket;
    zmq::socket_t pub_socket;
    zmq::socket_t disconnect_socket;
    std::map<int, float> prevtimes;
};

class Platform : public GameObject{
public:
	Platform(sf::Color color, sf::Color outlineColor, Vector2f pos, Vector2f size) : platform(size), GameObject(&platform){
		platform.setFillColor(color);
		platform.setOutlineThickness(5);
		platform.setOutlineColor(outlineColor);
		platform.setPosition(pos);	
	}
    sf::RectangleShape* getPlatform(){
		return &platform;
	}
	void platform_set_texture(sf::Texture& texture){
		platform.setTexture(&texture);
	}
protected:
	sf::RectangleShape platform;	
	sf::FloatRect platformBorder = platform.getGlobalBounds();	
};

class MovingPlatform : public GameObject{
public:
	MovingPlatform(sf::Color color, sf::Color outlineColor, Vector2f size, Vector2f pos, Vector2f bound, Vector2f speed) : movingplatform(size), GameObject(&movingplatform), movementSpeed(speed), movingPlatformPosition(pos), boundary(bound){
		movingplatform.setFillColor(color);
		movingplatform.setOutlineThickness(5);
		movingplatform.setOutlineColor(outlineColor);
        movingplatform.setPosition(pos);
	}
	sf::RectangleShape* getMovingPlatform(){
		return &movingplatform;
	}
	Vector2f getPlatformVelocity(){
		return platformVelocity;
	}
    sf::Vector2f get_position(){
        return movingplatform.getPosition();
    }
    void setPlatformVelocity(Vector2f vel){
        platformVelocity = vel;
    }
    void platformMovement(unsigned WINDOW_WIDTH){
            if (movingplatform.getPosition().x <= boundary.x) {
                platformVelocity.x = movementSpeed.x; 
            } 
            else if (movingplatform.getPosition().x + movingPlatformBorder.width >= boundary.x + 1200.f) {
                platformVelocity.x = -movementSpeed.x;
            }
            if(movingplatform.getPosition().y <= boundary.y)
            {
                platformVelocity.y = movementSpeed.y;
            }
            else if(movingplatform.getPosition().y + movingPlatformBorder.height >= boundary.y + 500.f){
                platformVelocity.y = -movementSpeed.y;
            }
        }
	void movingPlatform_set_texture(sf::Texture& texture){
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

class Server{
public:
    Server(){}
    void run(){
		platforms.push_back(Platform(sf::Color(165,42,42), sf::Color(0,255,0), sf::Vector2f(10.f,600.f),sf::Vector2f(400.f, 100.f)));
		platforms.push_back(Platform(sf::Color(165,42,42), sf::Color(0,0,255), sf::Vector2f(1800.f,500.f), sf::Vector2f(200.f, 100.f)));
		movingPlatforms.push_back(MovingPlatform(sf::Color(165,42,42), sf::Color(255,0,0),sf::Vector2f(500.f, 100.f), Vector2f(480.f, 500.f), Vector2f(480.f, 500.f), Vector2f(150.f, 0.f)));
		movingPlatforms.push_back(MovingPlatform(sf::Color(165,42,42), sf::Color(200,255,0),sf::Vector2f(500.f, 100.f), Vector2f(2150.f, 200.f), Vector2f(2150.f, 200.f), Vector2f(0.f, 150.f)));
        std::thread requestThread(&NetworkManager::handleClientRequests, &network, std::ref(server), std::ref(gameState), std::ref(characterState));
        std::thread disconnectThread([this]() {
        while (true) {
            network.checkForDisconnectedClients(std::ref(gameState));
            std::this_thread::sleep_for(std::chrono::seconds(5));
        }
        });
        while (true) {
        float dt = dt_clock.restart().asSeconds();
        for(int i=0; i<movingPlatforms.size(); i++){
            movingPlatforms[i].platformMovement(WINDOW_WIDTH);
            sf::RectangleShape* movingPlatformShape = movingPlatforms[i].getMovingPlatform();
            movingPlatforms[i].set_position_rectangle(movingPlatformShape,Vector2f(movingPlatforms[i].get_position().x + movingPlatforms[i].getPlatformVelocity().x * dt, movingPlatforms[i].get_position().y + movingPlatforms[i].getPlatformVelocity().y * dt));
        }
        std::string serializedData = "";
        for(int i=0; i<movingPlatforms.size(); i++){
            serializedData = serializedData+std::to_string(movingPlatforms[i].get_position().x) + "," + std::to_string(movingPlatforms[i].get_position().y) + ","  ;
        }
         for(int i=0; i<platforms.size(); i++){
            serializedData = serializedData+std::to_string(platforms[i].get_position().x) + "," + std::to_string(platforms[i].get_position().y) + ","  ;
        }
        for (auto i : gameState.characterPositions) {
            serializedData = serializedData + std::to_string(i.first) + "," + std::to_string(i.second.x) + "," + std::to_string(i.second.y) + "," ;
        }
        network.dataPublish(zmq::const_buffer(serializedData.c_str(), serializedData.size()));
        }
        requestThread.join();
        disconnectThread.join();
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
