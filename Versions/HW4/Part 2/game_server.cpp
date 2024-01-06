#include <zmq.hpp>
#include <iostream>
#include <map>
#include <string>
#include <thread>
#include <SFML/System.hpp>
#include <SFML/Graphics.hpp>
#include <condition_variable>
#include <future>
#include <cmath>
#include <queue>

using namespace sf;

struct CharacterState {
    float characterX;
    float characterY;
};

struct GameState {
    std::map<int, Vector2f> characterPositions;
    std::map<int, float> gameSpeed;
    std::map<int, std::chrono::time_point<std::chrono::steady_clock>> clientTimestamps; 
    std::map<int, float> prevTimestamps;
};

class PentagonShape : public sf::Shape {
public:
    PentagonShape(float size = 50.f) : m_size(size) {
        update();
    }
    void setSize(float size) {
        m_size = size;
        update();
    }
    float getSize() const {
        return m_size;
    }
    virtual size_t getPointCount() const {
        return 5; 
    }
    virtual sf::Vector2f getPoint(size_t index) const {
        static const float pi = 3.141592654f;
        float angle = index * 2 * pi / 5; // Angle between points
        float x = m_size * std::cos(angle);
        float y = m_size * std::sin(angle);
        return sf::Vector2f(m_size + x, m_size + y); // Offset by m_size for positioning
    }

private:
    float m_size;
};

class GameObject {
public:
    GameObject() {}

    sf::Vector2f get_position(RectangleShape *object) {
        return object->getPosition();
	}
    sf::Vector2f get_position_pentagon(PentagonShape *object) {
		return object->getPosition();
	}
	sf::FloatRect getBoundsRectangleShape(RectangleShape *object){
		return object->getGlobalBounds();
	}
    sf::FloatRect getBoundsPentagonShape(PentagonShape *object){
		return object->getGlobalBounds();
	}
    void set_position_pentagon(PentagonShape *object, Vector2f pos){
		// std::cout<<"Here"<<std::endl;
		// std::cout<<pos.x<<":"<<pos.y<<std::endl;
		object->setPosition(pos);
	}
	void set_position_rectangle(RectangleShape *object, Vector2f pos){
		object->setPosition(pos);
	}
protected:
    
};

class DeathZone {
public:
    DeathZone(const sf::Shape& shape) : bounds(shape) {
    }
    bool isCharacterColliding(const sf::FloatRect& characterBounds) const {
        return bounds.getGlobalBounds().intersects(characterBounds);
    }
protected:
    const sf::Shape& bounds;
};

class Spawner {
public:
    Spawner(float x, float y) : position(x, y) {}
    Vector2f getPosition() const { return position; }
    void respawnCharacter(Shape& character, const sf::Vector2f& Spawner) {
        // Reset the character's position to the spawn point
        character.setPosition(Spawner.x, Spawner.y);
    }
protected:
    Vector2f position;
};

class Lava : public DeathZone, public Spawner, public GameObject {
public:
    Lava(float windowWidth, float windowHeight, float lavaHeight) 
        : DeathZone(lavaShape), Spawner(0, windowHeight - lavaHeight), spawnY(windowHeight - lavaHeight), GameObject() {
        lavaShape.setSize(Vector2f(windowWidth, lavaHeight));
        lavaShape.setPosition(0, windowHeight - lavaHeight);
        lavaShape.setFillColor(Color::Red); // Adjust the color as needed
    }
    sf::RectangleShape* getLava(){
		return &lavaShape;
	}
	void set_position(Vector2f pos){
		lavaShape.setPosition(pos);
	}
	void lava_respawn(){
		this->respawnCharacter(lavaShape, Vector2f(0, spawnY));
	}
private:
    RectangleShape lavaShape;
	float spawnY;
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



class Timeline {
public:
    Timeline() : gameSpeed(1.f), paused(false), pauseTime(0.f) {
	}

	void start_realtime(){
		realTimeClock.restart().asSeconds();
	}
    void togglePause(sf::Clock& dt_clock) {
        if (paused){
            // Pause the game
			paused = false;
			dt_clock.restart();
			float currentPauseTime = pauseClock.getElapsedTime().asSeconds();
            pauseTime += currentPauseTime; //adding the current pause time to the total pause time.
            pauseClock.restart();
        } else {
            // Unpause the game
			paused = true;
			dt=0;
			dt_jump=0;
        	// Restart the pause clock when pausing
        	pauseClock.restart();
            
        }
    }
    void updateDeltaTime(sf::Clock& dt_clock) {
        if (!paused) {
            dt = dt_clock.restart().asSeconds() * gameSpeed;
        } else {
            dt = 0.f;
        }
    }
    float getDeltaTime(){
		//loop iteration time, frame-rate
        return dt;
    }
	void setDeltaTime(float dt_value){
		dt = dt_value;
	}
    void setGameSpeed(float speed) {
        gameSpeed = speed;
    }
    float getGameSpeed() {
        return gameSpeed;
    }
    bool isPaused() {
        return paused;
    }
	float getRealTime(){
		//real or global time
		return realTimeClock.getElapsedTime().asSeconds();
	}
    float getGameTime() {
		//game time
        return realTimeClock.getElapsedTime().asSeconds() - pauseTime;
    }
	float getDeltaJumpTime(){
		return dt_jump;
	}
	void setDeltaJumpTime(float dt_jump_updated){
		dt_jump = dt_jump_updated;
	}
protected:
    float dt;
	float dt_jump;
    float gameSpeed;
    bool paused;
    float pauseTime;
    sf::Clock pauseClock;
	sf::Clock realTimeClock;
};

class Character : public PentagonShape, public Spawner, public GameObject{
public: 
	Character() : jumpSpeed(1000.f), moveX(false), moveY(false), isJumping(false), movementSpeed(150.f), character(50), Spawner(50.f, 545.f), GameObject(){	
	}
	void initialize(){
		character.setFillColor(sf::Color(255,0,0));
		character.setPosition(50.f, 545.f);//50, 545
	}
	void set_characterPlatformVelocity(float x, float y){
	characterPlatformVelocity.x = x;
	characterPlatformVelocity.y = y;
	}
	PentagonShape* getCharacter(){
		return &character;
	}
	Vector2f get_characterPlatformVelocity(){
		return characterPlatformVelocity;
	}
	Vector2f get_characterVelocity(){
		return velocity;
	}
	void set_characterVelocityX(float x){
		std::cout<<x<<std::endl;
		// std::cout<<"Hi"<<std::endl;
		velocity.x = x;
	}
	void set_characterVelocityY(float y){
		velocity.y = y;
	}
	float get_movementSpeed(){
		return movementSpeed;
	}
	float get_jumpSpeed(){
		return jumpSpeed;
	}
	bool get_isJumping(){
		return isJumping;
	}
	void set_isJumping(bool value){
		isJumping = value;
	}
	void set_moveX(bool value){
		// std::cout<<value<<std::endl;
		moveX = value;
	}
	void set_moveY(bool value){
		moveY = value;
	}
	bool get_moveX(){
		return moveX;
	}
	bool get_moveY(){
		return moveY;
	}
	void character_draw(sf::RenderWindow& window){
		window.draw(character);
	}
	void character_respawn(){
		this->respawnCharacter(character, Vector2f(50.f, 545.f));
	}
protected:
	float jumpSpeed;
	bool moveX;
    bool moveY;
    bool isJumping;
	float movementSpeed;
	Vector2f velocity;
	Vector2f characterPlatformVelocity; 
	PentagonShape character;
};

class EventManager {
public:
    struct GameEvent {
        GameEvent(){}

        enum class Type {
            CharacterCollision,
            CharacterDeath,
            CharacterSpawn,
            UserInput,
            ChangeGameSpeed
            // Add more event types as needed
        } type;
        float timestamp;  // To store the time when the event occurred.
        int clientId;
        std::string name;
        GameState *gameState;
		bool operator<(const GameEvent& other) const {
            return timestamp > other.timestamp;  // Use > to make the priority queue behave like a min-heap.
        }
    };

    EventManager() {
    }
	void handleChangeGameSpeed(GameEvent& event){
        std::cout<<"Hi"<<std::endl;
	            if(event.gameState->gameSpeed[event.clientId] == 1.f)
    			{
    				// std::cout<<"gameSpeed"<<timeline.getGameSpeed()<<std::endl;
    				event.gameState->gameSpeed[event.clientId] = 1.5f;
    			}
    			else if(event.gameState->gameSpeed[event.clientId] == 1.5f)
    			{
    				event.gameState->gameSpeed[event.clientId] = 0.5f;
    			}
    			else if(event.gameState->gameSpeed[event.clientId] == 0.5f)
    			{
    				event.gameState->gameSpeed[event.clientId] = 1.f;
    			}
	}
    void changeGameSpeed(int clientId, std::string name, GameState *gameState){
        GameEvent event;
        event.type = GameEvent::Type::ChangeGameSpeed;
        event.clientId = clientId;
        event.name = name;
        event.gameState = gameState; 
        eventQueue.push(event);
    }
	void handleEvents() {
    while (!eventQueue.empty()) {
        GameEvent event = eventQueue.top();
        if (timeline.getGameTime() >= event.timestamp) {
           if (event.type == GameEvent::Type::ChangeGameSpeed) {
                handleChangeGameSpeed(event);
            } 
            eventQueue.pop();
        } else {
            break;
        }
    }
	}

protected:
    std::priority_queue<GameEvent> eventQueue; 
    Timeline timeline;
};

class NetworkManager {
public:
    NetworkManager() : context(1), req_socket(context, ZMQ_REP), pub_socket(context, ZMQ_PUB), event_socket(context, ZMQ_REP) {
        req_socket.bind("tcp://*:5556");
        pub_socket.bind("tcp://*:5555");
        event_socket.bind("tcp://*:5557");
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
            gameState.gameSpeed[clientId] = 1.f;
            std::cout<<gameState.gameSpeed[clientId]<<std::endl;
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
    //NEW CODE This is for sending and receiving event data
    void handleEventResponses(GameState *gameState){
        // std::cout<<"in"<<std::endl;
        while(true){
        int clientId;
        zmq::message_t response;
        event_socket.recv(response, zmq::recv_flags::none);
        std::string responseStr(static_cast<char*>(response.data()), response.size());
        // std::cout<<responseStr<<std::endl;
        if(responseStr == "Empty"){
            std::string reply = "Ok";
            event_socket.send(zmq::const_buffer(reply.c_str(), reply.size()), zmq::send_flags::none);   
        }
        else{
        std::vector<std::string> components;
		std::istringstream dataStream(responseStr);
		std::string component;
		while (std::getline(dataStream, component, ',')) {
    		components.push_back(component);
		}
        sscanf(responseStr.c_str(), "%d", &clientId);
        std::string name = components[1];
        std::cout<<name<<std::endl;

        EventManager event;
        event.changeGameSpeed(clientId, name, gameState);
        std::cout<<gameState->gameSpeed[clientId]<<std::endl;
        event.handleEvents();
        std::cout<<gameState->gameSpeed[clientId]<<std::endl;
        std::string reply = std::to_string(gameState->gameSpeed[clientId]);
        event_socket.send(zmq::const_buffer(reply.c_str(), reply.size()), zmq::send_flags::none);               
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
    //NEW SOCKET
    zmq::socket_t event_socket;
    std::map<int, float> prevtimes;
};

class Platform : public GameObject{
public:
	Platform(sf::Color color, sf::Color outlineColor, Vector2f pos, Vector2f size) : platform(size), GameObject(){
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
	MovingPlatform(sf::Color color, sf::Color outlineColor, Vector2f size, Vector2f pos, Vector2f bound, Vector2f speed) : movingplatform(size), GameObject(), movementSpeed(speed), movingPlatformPosition(pos), boundary(bound){
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
    // sf::Vector2f get_position(){
    //     return movingplatform.getPosition();
    // }
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
        //NEW CODE TO handleEventResponses.
        std::thread eventThread(&NetworkManager::handleEventResponses, &network,&gameState);
        std::thread disconnectThread([this]() {
        while (true) {
            network.checkForDisconnectedClients(std::ref(gameState));
            std::this_thread::sleep_for(std::chrono::seconds(5));
        }
        });
        while (true) {
        // eventManager.handleEvents();
        // std::cout<<gameState.gameSpeed[0]<<std::endl;
        float dt = dt_clock.restart().asSeconds();
        for(int i=0; i<movingPlatforms.size(); i++){
            movingPlatforms[i].platformMovement(WINDOW_WIDTH);
            sf::RectangleShape* movingPlatformShape = movingPlatforms[i].getMovingPlatform();
            movingPlatforms[i].set_position_rectangle(movingPlatformShape,Vector2f(movingPlatforms[i].get_position(movingPlatformShape).x + movingPlatforms[i].getPlatformVelocity().x * dt, movingPlatforms[i].get_position(movingPlatformShape).y + movingPlatforms[i].getPlatformVelocity().y * dt));
        }
        std::string serializedData = "";
        for(int i=0; i<movingPlatforms.size(); i++){
            sf::RectangleShape* movingPlatformShape = movingPlatforms[i].getMovingPlatform();
            serializedData = serializedData+std::to_string(movingPlatforms[i].get_position(movingPlatformShape).x) + "," + std::to_string(movingPlatforms[i].get_position(movingPlatformShape).y) + ","  ;
        }
         for(int i=0; i<platforms.size(); i++){
            sf::RectangleShape* platformShape = platforms[i].getPlatform();
            serializedData = serializedData+std::to_string(platforms[i].get_position(platformShape).x) + "," + std::to_string(platforms[i].get_position(platformShape).y) + ","  ;
        }
        for (auto i : gameState.characterPositions) {
            serializedData = serializedData + std::to_string(i.first) + "," + std::to_string(i.second.x) + "," + std::to_string(i.second.y) + "," ;
        }
        // std::cout<<serializedData<<std::endl;
        network.dataPublish(zmq::const_buffer(serializedData.c_str(), serializedData.size()));
        }
        requestThread.join();
        eventThread.join();
        disconnectThread.join();
    }
protected:
    CharacterState characterState;
    GameState gameState;
    NetworkManager network;
    GameServer server;
    EventManager eventManager;
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
