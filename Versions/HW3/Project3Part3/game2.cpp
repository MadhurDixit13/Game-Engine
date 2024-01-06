#include <SFML/Graphics.hpp>
#include <SFML/Window.hpp>
#include <SFML/System.hpp>
//standard sfml headers

#include <SFML/System/Mutex.hpp>
//using for shared resource management

#include <thread>
// c++ thread class header

#include <iostream>
#include <map>
#include <cmath>
// standard c++ header

#include <zmq.hpp>
#include "json.hpp"

using json = nlohmann::json;

using namespace sf;

//globally declared mutex
sf::Mutex mutex;

//cite - https://www.sfml-dev.org/tutorials/2.5/system-thread.php
//cite - https://www.geeksforgeeks.org/multithreading-in-cpp/

//custom shape class inheriting from sfml's default shape class
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
        return 5; // Pentagon has 5 points
    }

    virtual sf::Vector2f getPoint(size_t index) const {
        static const float pi = 3.141592654f;
        float angle = index * 2 * pi / 5; // Angle between points

        // Calculate the position of the points on the pentagon
        float x = m_size * std::cos(angle);
        float y = m_size * std::sin(angle);

        return sf::Vector2f(m_size + x, m_size + y); // Offset by m_size for positioning
    }

private:
    float m_size;
};
// class Movement{

// };
// class GameObject{
// 	public:
// 		GameObject(){}
// 		void set_position(sf)
// 	protected:

// }
class Spawner {
public:
    Spawner(float x, float y) : position(x, y) {}
    Vector2f getPosition() const { return position; }
	   // Implement a function to respawn the character at a specific spawn point
    void respawnCharacter(Shape& character, const sf::Vector2f& Spawner) {
        // Reset the character's position to the spawn point
        character.setPosition(Spawner.x, Spawner.y);
    }
protected:
    Vector2f position;
};


class DeathZone {
public:
    DeathZone(const sf::Shape& shape) : bounds(shape) {
        // Set the position and size of the death zone
        // You can also set its visual representation, e.g., color
    }
    // Implement a function to check for character collisions
    bool isCharacterColliding(const sf::FloatRect& characterBounds) const {
        return bounds.getGlobalBounds().intersects(characterBounds);
    }
protected:
    const sf::Shape& bounds;
};

class Lava : public DeathZone, public Spawner {
public:
    Lava(float windowWidth, float windowHeight, float lavaHeight) 
        : DeathZone(lavaShape), Spawner(0, windowHeight - lavaHeight), spawnY(windowHeight - lavaHeight) {
        lavaShape.setSize(Vector2f(windowWidth, lavaHeight));
        lavaShape.setPosition(0, windowHeight - lavaHeight);
        lavaShape.setFillColor(Color::Red); // Adjust the color as needed
		// std::cout<<"Bye"<<std::endl;
		std::cout<<windowHeight<<std::endl;
    }
	sf::Vector2f get_position(){
		return lavaShape.getPosition();
	}
	void set_position(Vector2f pos){
			lavaShape.setPosition(pos);
	}
    sf::RectangleShape getLava(){
		// std::cout<<"bye";
		return lavaShape;
	}
	void lava_respawn(){
		this->respawnCharacter(lavaShape, Vector2f(0, spawnY));
	}

private:
    RectangleShape lavaShape;
	float spawnY;
};



class SideBoundary {
public:
    SideBoundary(FloatRect bounds) : boundary(bounds) {}
    FloatRect getBounds() const { return boundary; }
private:
    FloatRect boundary;
};


class Platform{
public:
	Platform(sf::Color color, sf::Color outlineColor, Vector2f pos, Vector2f size) : platform(size){
		platform.setFillColor(color);
		platform.setOutlineThickness(5);
		platform.setOutlineColor(outlineColor);
		platform.setPosition(pos);
		// platform.setPosition(10.f,600.f);	

	}
	sf::FloatRect getBounds(){
		return platform.getGlobalBounds();
	}
	Vector2f get_position(){
        return platform.getPosition();
    }
	void set_position(Vector2f pos){
		platform.setPosition(pos);
	}

	sf::RectangleShape getPlatform(){
		return platform;
	}
	void platform_set_texture(sf::Texture& texture){
		platform.setTexture(&texture);
	}

protected:
	sf::RectangleShape platform;	
	sf::FloatRect platformBorder = platform.getGlobalBounds();	
};

//Timeline Class
class Timeline {
public:
    Timeline() : gameSpeed(1.f), paused(false), pauseTime(0.f) {}

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

	float getRealTime(sf::Clock& realTimeClock){
		//real or global time
		return realTimeClock.getElapsedTime().asSeconds();
	}

    float getGameTime(sf::Clock& realTimeClock) {
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
};

class MovingPlatform : public Timeline{
public:
	MovingPlatform(sf::Color color, sf::Color outlineColor, Vector2f size, Vector2f pos, Vector2f bound, Vector2f speed) :movingplatform(size), movementSpeed(speed), movingPlatformPosition(pos), boundary(bound){
		movingplatform.setFillColor(color);
		movingplatform.setOutlineThickness(5);
		movingplatform.setOutlineColor(outlineColor);
        movingplatform.setPosition(pos);
	}

	sf::RectangleShape getMovingPlatform(){
		return movingplatform;
	}
	sf::FloatRect getBounds(){
		return movingplatform.getGlobalBounds();
	}
	Vector2f getPlatformVelocity(){
		return platformVelocity;
	}
	void set_position(float x, float y){
		movingplatform.setPosition(x, y);
	}
	void platformMove(unsigned WINDOW_WIDTH){
		mutex.lock();
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
		mutex.unlock();
	}
	void movingPlatform_set_texture(sf::Texture& texture){
		movingplatform.setTexture(&texture);
	}
protected:
	Vector2f movementSpeed;
	Vector2f platformVelocity;
	Vector2f movingPlatformPosition;
    Vector2f boundary;
	Vector2f prevMovingPlatformPosition;
	sf::RectangleShape movingplatform;
	sf::FloatRect movingPlatformBorder = movingplatform.getGlobalBounds();
};

class Character : public PentagonShape, public Spawner{
public: 
	Character() : jumpSpeed(1000.f), moveX(false), moveY(false), isJumping(false), movementSpeed(150.f), character(50), Spawner(50.f, 545.f){	
	}

	void initialize(){
		character.setFillColor(sf::Color(255,0,0));
		character.setPosition(50.f, 545.f);//50, 545
	}

	void set_characterPlatformVelocity(float x, float y){
	characterPlatformVelocity.x = x;
	characterPlatformVelocity.y = y;
	}
	PentagonShape getCharacter(){
		return character;
	}

	Vector2f get_characterPlatformVelocity(){
		return characterPlatformVelocity;
	}

	Vector2f get_characterPosition(){
		return character.getPosition();
	}

	void set_characterPosition(float x, float y){
		character.setPosition(x, y);
	}

	FloatRect get_characterBorder(){
		return character.getGlobalBounds();
	}

	Vector2f get_characterVelocity(){
		return velocity;
	}

	void set_characterVelocityX(float x){
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

struct GameState{
	std::map<int, Vector2f> characterPositions;
	std::vector<Vector2f> movingPlatformPosition;
	std::vector<Vector2f> platformPositions;
	// float movingPlatformPositionX;
    // float movingPlatformPositionY;
};

class NetworkManager {
public:
    NetworkManager(int clientID) : clientID(clientID) {
        context = zmq::context_t(1);
        req_socket = zmq::socket_t(context, ZMQ_REQ);
        req_socket.connect("tcp://localhost:5556");
        sub_socket = zmq::socket_t(context, ZMQ_SUB);
        int conflate = 1;
        sub_socket.setsockopt(ZMQ_CONFLATE, &conflate, sizeof(conflate));
        sub_socket.connect("tcp://localhost:5555");
        sub_socket.setsockopt(ZMQ_SUBSCRIBE, "", 0);
    }

	void sendMessage(std::string reply){
		performance = performanceTime.restart().asSeconds();
		req_socket.send(zmq::const_buffer(reply.c_str(), reply.size()), zmq::send_flags::none);
	}

	void receiveMessage(){
		zmq::message_t response;
		req_socket.recv(response, zmq::recv_flags::none);
		std::string responseStr(static_cast<char*>(response.data()), response.size());
		clientID = std::stoi(responseStr);
	}

    void sendGameData(float characterX, float characterY) {
		performance = performanceTime.restart().asSeconds();
        std::string serializedData = std::to_string(clientID) + ","
                         + std::to_string(characterX) + ","
                         + std::to_string(characterY);
		req_socket.send(zmq::const_buffer(serializedData.c_str(), serializedData.size()), zmq::send_flags::none);
		zmq::message_t response;
        req_socket.recv(response, zmq::recv_flags::none);
        // std::string responseStr(static_cast<char*>(response.data()), response.size());
    }
	bool disconnect() {
		try{
        std::string disconnectMessage = std::to_string(clientID) + "," + "Disconnect";
        req_socket.send(zmq::const_buffer(disconnectMessage.c_str(), disconnectMessage.size()), zmq::send_flags::none);
        zmq::message_t response;
           while (true) {
        if (req_socket.recv(response, zmq::recv_flags::none)) {
            // Response received
            break;
        }
        // You can add a sleep or yield here to avoid busy waiting
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
		
    }
        // Additional cleanup or handling here
        req_socket.close();
        sub_socket.close();
		}
		catch (const zmq::error_t& e) {
    // Handle the ZeroMQ exception
    std::cerr << "ZeroMQ Error: " << e.what() << std::endl;
		}
		return true;
    }

    void receiveGameData(std::vector<Platform>& platforms, std::vector<MovingPlatform>& movingPlatforms, GameState& gameState) {
    zmq::message_t message;
    sub_socket.recv(message, zmq::recv_flags::none);
	std::cout<<performance;
	// std::cout<<message;
    std::string receivedData(static_cast<char*>(message.data()), message.size());

    // Parse the JSON data received from the server
    json gameStateJson = json::parse(receivedData);

    // Update the character positions
    json characterPositionsJson = gameStateJson["characterPositions"];
	std::map<int, sf::Vector2f> characterPositions;
	for (json::iterator it = characterPositionsJson.begin(); it != characterPositionsJson.end(); ++it) {
    int clientId = std::stoi(it.key());
    float x = it.value()["x"];
    float y = it.value()["y"];
    characterPositions[clientId] = sf::Vector2f(x, y);
	}

	json movingPlatformPositionsJson = gameStateJson["movingPlatformPosition"];
	std::vector<sf::Vector2f> movingPlatformPositions;
	for (const auto& positionJson : movingPlatformPositionsJson) {
    float x = positionJson["x"];
    float y = positionJson["y"];
    movingPlatformPositions.push_back(sf::Vector2f(x, y));
	}

// Accessing platform positions.
	json platformPositionsJson = gameStateJson["platformPositions"];
	std::vector<sf::Vector2f> platformPositions;
	for (const auto& positionJson : platformPositionsJson) {
    float x = positionJson["x"];
    float y = positionJson["y"];
    platformPositions.push_back(sf::Vector2f(x, y));
	}
    // Update the local game state
    // GameState gameState;
    gameState.characterPositions = characterPositions;
	gameState.movingPlatformPosition = movingPlatformPositions;
	gameState.platformPositions = platformPositions;

    // Update the positions of moving platforms and platforms
    for (int i = 0; i < 2; i++) {
        movingPlatforms[i].set_position(gameState.movingPlatformPosition[i].x,gameState.movingPlatformPosition[i].y);
    }

    for (int i = 0; i < 2; i++) {
        platforms[i].set_position(gameState.platformPositions[i]);
    }

    // return gameState;
}

private:
    int clientID;
    zmq::context_t context;
    zmq::socket_t req_socket;
    zmq::socket_t sub_socket;
	Clock performanceTime;
	float performance;
};

struct CharacterState {
    // int clientId;
    float characterX;
    float characterY; 
};



//loading and applying textures to platforms through multithreading

void thread1(sf::Texture& texture, std::vector<Platform>& platforms, std::vector<MovingPlatform>& movingPlatforms, int platformsize, int movingplatformsize) {
    mutex.lock();
    if (!texture.loadFromFile("brick1.png")) {
    	//asset link - bricks-texture- https://gamedeveloperstudio.itch.io/sma
        std::cerr << "Failed to load texture" << std::endl;
    }
     for (int i=0; i<platformsize; i++) {
        platforms[i].platform_set_texture(texture);
    }
	for (int i=0; i<movingplatformsize; i++) {
        movingPlatforms[i].movingPlatform_set_texture(texture);
    }
    mutex.unlock();
	}	

class Game{
public:
    Game() : window(sf::VideoMode(WINDOW_WIDTH, WINDOW_HEIGHT), "Platforms"), gravity(20.f), clientID(0), WINDOW_WIDTH(1600), WINDOW_HEIGHT(1600), networkManager(clientID), lava(1800.f, 1600.f, 20.0f), characterSpawner(50.f, 545.f) {
        window.setVerticalSyncEnabled(true);
    }
	
	void run() {
		std::string reply = "Joined";
	    networkManager.sendMessage(reply);
	    networkManager.receiveMessage();
		std::vector<Platform> platforms;
		platforms.push_back(Platform(sf::Color(165,42,42), sf::Color(0,255,0), sf::Vector2f(10.f,600.f),sf::Vector2f(400.f, 100.f)));
		platforms.push_back(Platform(sf::Color(165,42,42), sf::Color(0,0,255), sf::Vector2f(1800.f,500.f), sf::Vector2f(200.f, 100.f)));
		// platforms.push_back(Platform(sf::Color(165,42,42), sf::Color(0,0,255), sf::Vector2f(200.f, 200.f)));

		// Platform platform(sf::Color(165,42,42), sf::Color(0,255,0));
		std::vector<MovingPlatform> movingPlatforms;
		movingPlatforms.push_back(MovingPlatform(sf::Color(165,42,42), sf::Color(255,0,0),sf::Vector2f(500.f, 100.f), Vector2f(480.f, 500.f), Vector2f(480.f, 500.f), Vector2f(150.f, 0.f)));
		movingPlatforms.push_back(MovingPlatform(sf::Color(165,42,42), sf::Color(200,255,0),sf::Vector2f(500.f, 100.f), Vector2f(2150.f, 200.f), Vector2f(2150.f, 200.f), Vector2f(0.f, 150.f)));
		Character character;
		character.initialize();
		Vector2f platformVelocity;
		Vector2f prevMovingPlatformPosition;
		sf::Texture texture;
		// sf::RectangleShape platform(sf::Vector2f(400, 100));
		// sf::RectangleShape movingplatform(sf::Vector2f(500, 100));
		sf::FloatRect platformBorder[platforms.size()];
		for(int i=0; i<platforms.size(); i++){
			platformBorder[i] = platforms[i].getBounds();
		}
		sf::FloatRect movingPlatformBorder[movingPlatforms.size()];
		for(int i=0; i<movingPlatforms.size(); i++){
			movingPlatformBorder[i] = movingPlatforms[i].getBounds();
		}
		// sf::FloatRect movingPlatformBorder = movingPlatform.getBounds();
		float gameTime= timeline.getGameTime(realTimeClock); //game time
		sf::View gameView;
        gameView.setSize(WINDOW_WIDTH, WINDOW_HEIGHT);
        gameView.setCenter(WINDOW_WIDTH / 2, WINDOW_HEIGHT / 2);
        window.setView(gameView);
		
		std::thread t1(&thread1, std::ref(texture), std::ref(platforms), std::ref(movingPlatforms), platforms.size(), movingPlatforms.size());
		t1.join();

    	static bool pKeyPressed = false; 
    	static bool qKeyPressed = false;
        // Initialize the network manager here
		while(window.isOpen())
		{
		sf::Event event;
		if (window.hasFocus()) {
		while(window.pollEvent(event))
		{
			if(event.type == sf::Event::Closed)
			{
				bool disconnected = networkManager.disconnect();
				if(disconnected){
					window.close();
				}
			}
			else if (event.type == sf::Event::Resized && sf::Keyboard::isKeyPressed(sf::Keyboard::E))
    		{
        		sf::FloatRect visibleArea(0, 0, event.size.width, event.size.height);
        		window.setView(sf::View(visibleArea));
    		}
    		//timeline pause and play
    		else if (event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::P && !pKeyPressed)
    		{
        		pKeyPressed = true; // Set the flag to true
        		timeline.togglePause(dt_clock);
    		}
    		else if (event.type == sf::Event::KeyReleased && event.key.code == sf::Keyboard::P)
    		{
        		pKeyPressed = false; // Reset the flag when 'P' key is released
    		}
    		
		}
	
		if(sf::Keyboard::isKeyPressed(sf::Keyboard::Q))
		{
    			qKeyPressed = true;
    			if(timeline.getGameSpeed() == 1.f)
    			{
    				std::cout<<"gameSpeed"<<timeline.getGameSpeed()<<std::endl;
    				timeline.setGameSpeed(1.5f);
    			}
    			else if(timeline.getGameSpeed()  == 1.5f)
    			{
    				timeline.setGameSpeed(0.5f);
    			}
    			else if(timeline.getGameSpeed()  == 0.5f)
    			{
    				timeline.setGameSpeed(1.f);
    			}
		}
		}
		if (!timeline.isPaused()) 
		{
        // Only update dt if the game is not paused
		timeline.updateDeltaTime(dt_clock);
        timeline.setDeltaJumpTime(timeline.getDeltaTime()/timeline.getGameSpeed());
       
    	}	

		std::cout << std::fixed; 
		gravity = 20.f;
		

		character.set_characterPlatformVelocity(0.f, 0.f);

		character.get_characterBorder();
    	// platformBorder = platform.getBounds();
    	// movingPlatformBorder = movingPlatform.getBounds();

		character.set_characterVelocityY(character.get_characterVelocity().y+gravity*timeline.getDeltaJumpTime());
		character.set_moveX(false);
		character.set_moveY(true);
		
		for(int i = 0; i < platforms.size(); i++)
		{
			if (character.get_characterBorder().intersects(platforms[i].getBounds())) {
    		// Collision detected with the platform
    		character.set_characterVelocityY(0.f); // Stop falling due to gravity
    		character.set_isJumping(false);
    		// Adjust the character's position to be just above the platform
    		character.set_characterPosition(character.get_characterPosition().x, platforms[i].getBounds().top - character.get_characterBorder().height - 1.f);
		}
		}
		// Collision detection with the platform
	

		// Collision detection with the moving platform
		for(int i = 0; i < movingPlatforms.size(); i++)
		{
			if (character.get_characterBorder().intersects(movingPlatforms[i].getBounds())) {
    		// Collision detected with the moving platform
    		character.set_characterVelocityY(0.f); // Stop falling due to gravity
    		character.set_isJumping(false);
    		character.set_moveX(true);
    		// Adjust the character's position to be just above the moving platform
    		character.set_characterPosition(character.get_characterPosition().x, movingPlatforms[i].getBounds().top - character.get_characterBorder().height - 1.f);
    		// Update the characterPlatformVelocity to match the platform's velocity
    		character.set_characterPlatformVelocity(0.f, 0.f);
			// std::cout<<"velocity:"<<mpVelocity << std::endl;
			std::cout<<movingPlatforms[i].getPlatformVelocity().x * timeline.getDeltaTime()<<std::endl;
		}
		}
			

		//movement
		if (window.hasFocus()) {
		if (sf::Keyboard::isKeyPressed(sf::Keyboard::A))
		{
    		character.set_characterVelocityX(-character.get_movementSpeed()*timeline.getDeltaTime());
			character.set_moveX(true);
		}	
		if (sf::Keyboard::isKeyPressed(sf::Keyboard::D))
		{
    	    character.set_characterVelocityX(character.get_movementSpeed()*timeline.getDeltaTime());
    	    character.set_moveX(true);
		}
		if (sf::Keyboard::isKeyPressed(sf::Keyboard::Space) && !character.get_isJumping())
		{
    		// Space key is pressed, and the character is not already jumping
    		character.set_characterVelocityY(character.get_characterVelocity().y - (character.get_jumpSpeed() * timeline.getDeltaJumpTime()));  // Apply upward velocity for jumping
    		character.set_isJumping(true); // Set jumping flag to true	
		}
		}
		
		//Thread 2 logic
		for(int i = 0; i < movingPlatforms.size(); i++){
			std::thread t2(&MovingPlatform::platformMove, &movingPlatforms[i], WINDOW_WIDTH);
    		t2.detach();
		}
    	
		// t2.detach(); 
		// movingPlatform.platformMove(WINDOW_WIDTH);

		if(character.get_moveX())
		{
			character.set_characterPosition(character.get_characterPosition().x + character.get_characterVelocity().x + character.get_characterPlatformVelocity().x, character.get_characterPosition().y);
		}
		if(character.get_moveY())
		{
			character.set_characterPosition(character.get_characterPosition().x, character.get_characterPosition().y + character.get_characterVelocity().y);
		}

		if(character.get_characterPosition().x<0)
		{
			character.set_characterPosition(0.f, character.get_characterPosition().y);
		}
		// if(character.get_characterPosition().x  > WINDOW_WIDTH)
		// {
		// 	character.set_characterPosition(WINDOW_WIDTH, character.get_characterPosition().y);
		// }	
		if(lava.isCharacterColliding(character.get_characterBorder()))
		{
			character.character_respawn();
			gameView.setCenter(WINDOW_WIDTH / 2, WINDOW_HEIGHT / 2);
        	window.setView(gameView);
			lava.lava_respawn();
		}
		

		if (character.get_characterPosition().x < WINDOW_WIDTH/2) {
                // Character hit the left side boundary
                // Move all objects in the scene to the right to create the side-scrolling effect
                sf::Vector2f currentCenter = gameView.getCenter();
				if(currentCenter.x!=WINDOW_WIDTH / 2){
				while(currentCenter.x==WINDOW_WIDTH / 2)
				{
                gameView.setCenter(currentCenter.x - (WINDOW_WIDTH/2)-character.get_characterPosition().x, currentCenter.y);
				lava.set_position(Vector2f(lava.get_position().x - 1000.f-character.get_characterPosition().x, lava.get_position().y ));
				}
			}
                window.setView(gameView);
            } 
			else if (character.get_characterPosition().x >= WINDOW_WIDTH/2) {
                // Character hit the right side boundary
                // Move all objects in the scene to the left to create the side-scrolling effect
				sf::Vector2f currentCenter = gameView.getCenter();
				gameView.setCenter(character.get_characterPosition().x, currentCenter.y);
				// window.setView(gameView);
                // sf::Vector2f currentCenter = gameView.getCenter();
                // gameView.setCenter((WINDOW_WIDTH / 2) + character.get_characterPosition().x-1000.f, currentCenter.y);
				lava.set_position(Vector2f(character.get_characterPosition().x - (WINDOW_WIDTH/2), lava.get_position().y ));
                window.setView(gameView);
        }
		window.clear();
		for (int i = 0; i < platforms.size(); i++) {
            window.draw(platforms[i].getPlatform());
        }
		
		window.draw(lava.getLava());
		//network code
		characterState.characterX = character.get_characterPosition().x;
		characterState.characterY = character.get_characterPosition().y;
		networkManager.sendGameData(characterState.characterX, characterState.characterY);
	
		std::map<int, sf::Vector2f> characterPositions;
		networkManager.receiveGameData(std::ref(platforms),std::ref(movingPlatforms), std::ref(gameState));
		characterPositions = gameState.characterPositions;
	
		// for(int i = 0; i < movingPlatforms.size(); i++){
		// 	movingPlatforms[i].set_position(mpVelocity, 400.f);
		// }

		// movingPlatforms[0].set_position(mpVelocity, 400.f);
		// movingPlatforms[1].set_position(2200.f, mpVelocity);
		for(int i = 0; i < movingPlatforms.size(); i++)
		{
			window.draw(movingPlatforms[i].getMovingPlatform());
		}
		int count = 0;
		for (auto i : characterPositions) 
        {
				PentagonShape c[30];
				c[count].setPosition(sf::Vector2f(i.second.x, i.second.y ));
				c[count].setFillColor(sf::Color(255,0,0));
				window.draw(c[count]);
				count++;

        }
		window.display();
		// gameState.characterPositions.clear();
		
	}
    }
protected:
	CharacterState characterState;
	GameState gameState;
	NetworkManager networkManager;
	Clock dt_clock; //global time
	sf::Clock realTimeClock; //real-time clock
    Timeline timeline;
	Lava lava;
	Spawner characterSpawner;
	int clientID;
    float gravity;
    unsigned WINDOW_WIDTH;
    unsigned WINDOW_HEIGHT;
	// unsigned WORLD_WIDTH;
    sf::RenderWindow window;
	// float mpVelocity;
	const float SIDE_SCROLL_SPEED = 100.0f; // Adjust the scroll speed as needed
    const float CHARACTER_BOUNDARY_X = 0.0f;
};


int main()
{
	Game game;
	game.run();
	return 0;
}

//cite sfml-dev.org/documentation/2.5.1/classsf_1_1Event.php#details
//cite https://www.sfml-dev.org/tutorials/2.5/window-inputs.php
//cite sprite-animation- https://github.com/SFML/SFML/wiki/Easy-Animations-With-Spritesheets
//cite https://github.com/SFML/SFML/wiki/Easy-Animations-With-Spritesheets
//cite https://www.sfml-dev.org/tutorials/2.5/graphics-transform.php
//cite https://en.sfml-dev.org/forums/index.php?topic=27820.0
//cite https://www.youtube.com/watch?v=PGMdLgk4FD0
//cite https://www.youtube.com/watch?v=NhXXC37Xoyo
//cite https://www.sfml-dev.org/tutorials/2.2/graphics-view.php#showing-more-when-the-window-is-resized

//assets

//sprite assets https://zegley.itch.io/2d-platformermetroidvania-asset-pack/download/eyJleHBpcmVzIjoxNjkzMzY3NjU4LCJpZCI6MTg2MjY2MX0%3d.VaWUTqmFI%2f8YDMQLxJMepPX6BT4%3d
//https://pixelfrog-assets.itch.io/pixel-adventure-2