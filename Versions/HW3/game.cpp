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
    GameObject(sf::Shape *object) : shape(object) {}

    sf::Vector2f get_position() {
        return shape->getPosition();
	}
	sf::FloatRect getBoundsRectangleShape(RectangleShape *object){
		return object->getGlobalBounds();
	}
	sf::FloatRect getBoundsPentagonShape(PentagonShape *object){
		return object->getGlobalBounds();
	}
	void set_position_pentagon(PentagonShape *object, Vector2f pos){
		object->setPosition(pos);
	}
	void set_position_rectangle(RectangleShape *object, Vector2f pos){
		object->setPosition(pos);
	}
protected:
    sf::Shape *shape;
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

class Lava : public DeathZone, public Spawner, public GameObject {
public:
    Lava(float windowWidth, float windowHeight, float lavaHeight) 
        : DeathZone(lavaShape), Spawner(0, windowHeight - lavaHeight), spawnY(windowHeight - lavaHeight), GameObject(&lavaShape) {
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

class SideBoundary {
public:
    SideBoundary(FloatRect bounds) : boundary(bounds) {}
    FloatRect getBounds() const { return boundary; }
private:
    FloatRect boundary;
};

class Platform:public GameObject{
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

class MovingPlatform : public Timeline, public GameObject{
public:
	MovingPlatform(sf::Color color, sf::Color outlineColor, Vector2f size, Vector2f pos, Vector2f bound, Vector2f speed) :movingplatform(size), GameObject(&movingplatform),movementSpeed(speed), movingPlatformPosition(pos), boundary(bound){
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

class Character : public PentagonShape, public Spawner, public GameObject{
public: 
	Character() : jumpSpeed(1000.f), moveX(false), moveY(false), isJumping(false), movementSpeed(150.f), character(50), Spawner(50.f, 545.f), GameObject(&character){	
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
		req_socket.send(zmq::const_buffer(reply.c_str(), reply.size()), zmq::send_flags::none);
	}
	void receiveMessage(){
		zmq::message_t response;
		req_socket.recv(response, zmq::recv_flags::none);
		std::string responseStr(static_cast<char*>(response.data()), response.size());
		clientID = std::stoi(responseStr);
	}
    void sendGameData(float characterX, float characterY, float timestamp) {
        std::string serializedData = std::to_string(clientID) + ","
                         + std::to_string(characterX) + ","
                         + std::to_string(characterY) + ","
						 + std::to_string(timestamp);
		req_socket.send(zmq::const_buffer(serializedData.c_str(), serializedData.size()), zmq::send_flags::none);
		zmq::message_t response;
        req_socket.recv(response, zmq::recv_flags::none);
    }
    std::map<int, sf::Vector2f> receiveGameData(std::vector<Platform>& platforms,std::vector<MovingPlatform>& movingPlatforms) {
        zmq::message_t message;
        sub_socket.recv(message, zmq::recv_flags::none);
        std::string receivedData(static_cast<char*>(message.data()), message.size()); 
		std::vector<std::string> components;
		std::istringstream dataStream(receivedData);
		std::string component;
		while (std::getline(dataStream, component, ',')) {
    		components.push_back(component);
		}
		const int componentsPerEntry = 3;
		std::vector <PentagonShape> characters;
		std::map <int, Vector2f> characterPositions;
		for(int i=0; i<2;i++)
		{
			sf::RectangleShape* movingPlatformShape = movingPlatforms[i].getMovingPlatform();
			movingPlatforms[i].set_position_rectangle(movingPlatformShape,Vector2f(std::stof(components[(i*2)]), std::stof(components[(i*2)+1])));
		}
		for(int i=2; i<4;i++)
		{
			sf::RectangleShape* platformShape = platforms[i].getPlatform();
 			platforms[i].set_position_rectangle(platformShape,Vector2f(std::stof(components[(i*2)]), std::stof(components[(i*2)+1])));
		}
    	for (size_t i = 8; i < components.size(); i += componentsPerEntry) {
        try {
            int id = std::stoi(components[i]);
            float x = std::stof(components[i + 1]);
            float y = std::stof(components[i + 2]);
            // Update the characterPositions map
            characterPositions[id] = sf::Vector2f(x, y);
        } catch (const std::exception& e) {
        }
		} 
        return characterPositions;
    }
private:
    int clientID;
    zmq::context_t context;
    zmq::socket_t req_socket;
    zmq::socket_t sub_socket;	
};

struct CharacterState {
    float characterX;
    float characterY; 
};

struct GameState{
	std::map<int, Vector2f> characterPositions;
	float movingPlatformPositionX;
    float movingPlatformPositionY;
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
    Game() : window(sf::VideoMode(WINDOW_WIDTH, WINDOW_HEIGHT), "Platforms"), gravity(20.f), clientID(0), WINDOW_WIDTH(1600), WINDOW_HEIGHT(1600), networkManager(clientID), lava(1800.f, 1600.f, 20.0f), characterSpawner(50.f, 545.f){
        window.setVerticalSyncEnabled(true);
	}	
	void run() {
		PentagonShape *characterShape = character.getCharacter();
		RectangleShape *lavaShape = lava.getLava();
		std::string reply = "Joined";
		realTimeClock.restart().asSeconds();
	    networkManager.sendMessage(reply);
	    networkManager.receiveMessage();
		std::vector<Platform> platforms;
		platforms.push_back(Platform(sf::Color(165,42,42), sf::Color(0,255,0), sf::Vector2f(10.f,600.f),sf::Vector2f(400.f, 100.f)));
		platforms.push_back(Platform(sf::Color(165,42,42), sf::Color(0,0,255), sf::Vector2f(1800.f,500.f), sf::Vector2f(200.f, 100.f)));
		std::vector<MovingPlatform> movingPlatforms;
		movingPlatforms.push_back(MovingPlatform(sf::Color(165,42,42), sf::Color(255,0,0),sf::Vector2f(500.f, 100.f), Vector2f(480.f, 500.f), Vector2f(480.f, 500.f), Vector2f(150.f, 0.f)));
		movingPlatforms.push_back(MovingPlatform(sf::Color(165,42,42), sf::Color(200,255,0),sf::Vector2f(500.f, 100.f), Vector2f(2150.f, 200.f), Vector2f(2150.f, 200.f), Vector2f(0.f, 150.f)));
		character.initialize();
		Vector2f platformVelocity;
		Vector2f prevMovingPlatformPosition;
		sf::Texture texture;
		sf::FloatRect platformBorder[platforms.size()];
		for(int i=0; i<platforms.size(); i++){
			sf::RectangleShape* platformShape = platforms[i].getPlatform();
			platformBorder[i] = platforms[i].getBoundsRectangleShape(platformShape);
		}
		sf::FloatRect movingPlatformBorder[movingPlatforms.size()];
		for(int i=0; i<movingPlatforms.size(); i++){
			sf::RectangleShape* movingPlatformShape = movingPlatforms[i].getMovingPlatform();
			movingPlatformBorder[i] = movingPlatforms[i].getBoundsRectangleShape(movingPlatformShape);
		}
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
					window.close();
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
		character.getBoundsPentagonShape(characterShape);
		character.set_characterVelocityY(character.get_characterVelocity().y+gravity*timeline.getDeltaJumpTime());
		character.set_moveX(false);
		character.set_moveY(true);
		for(int i = 0; i < platforms.size(); i++)
		{
			sf::RectangleShape* platformShape = platforms[i].getPlatform();
			if (character.getBoundsPentagonShape(characterShape).intersects(platforms[i].getBoundsRectangleShape(platformShape))) {
    		// Collision detected with the platform
    		character.set_characterVelocityY(0.f); // Stop falling due to gravity
    		character.set_isJumping(false);
    		// Adjust the character's position to be just above the platform
    		character.set_position_pentagon(characterShape,Vector2f(character.get_position().x, platforms[i].getBoundsRectangleShape(platformShape).top - character.getBoundsPentagonShape(characterShape).height - 1.f));
		}
		}
		// Collision detection with the moving platform
		for(int i = 0; i < movingPlatforms.size(); i++)
		{
			sf::RectangleShape* movingPlatformShape = movingPlatforms[i].getMovingPlatform();
			if (character.getBoundsPentagonShape(characterShape).intersects(movingPlatforms[i].getBoundsRectangleShape(movingPlatformShape))) {
    		// Collision detected with the moving platform
    		character.set_characterVelocityY(0.f); // Stop falling due to gravity
    		character.set_isJumping(false);
    		character.set_moveX(true);
    		character.set_position_pentagon(characterShape,Vector2f(character.get_position().x, movingPlatforms[i].getBoundsRectangleShape(movingPlatformShape).top - character.getBoundsPentagonShape(characterShape).height - 1.f));
    		character.set_characterPlatformVelocity(0.f, 0.f);
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
		if(character.get_moveX())
		{
			character.set_position_pentagon(characterShape,Vector2f(character.get_position().x + character.get_characterVelocity().x + character.get_characterPlatformVelocity().x, character.get_position().y));
		}
		if(character.get_moveY())
		{
			character.set_position_pentagon(characterShape,Vector2f(character.get_position().x, character.get_position().y + character.get_characterVelocity().y));
		}
		if(character.get_position().x<0)
		{
			character.set_position_pentagon(characterShape,Vector2f(0.f, character.get_position().y));
		}	
		if(lava.isCharacterColliding(character.getBoundsPentagonShape(characterShape)))
		{
			character.character_respawn();
			gameView.setCenter(WINDOW_WIDTH / 2, WINDOW_HEIGHT / 2);
        	window.setView(gameView);
			lava.lava_respawn();
		}
		if (character.get_position().x < WINDOW_WIDTH/2) {
                sf::Vector2f currentCenter = gameView.getCenter();
				if(currentCenter.x!=WINDOW_WIDTH / 2){
				while(currentCenter.x==WINDOW_WIDTH / 2)
				{
                gameView.setCenter(currentCenter.x - (WINDOW_WIDTH/2)-character.get_position().x, currentCenter.y);
				sf::RectangleShape *lavaShape = lava.getLava();
				lava.set_position_rectangle(lavaShape,Vector2f(lava.get_position().x - 1000.f-character.get_position().x, lava.get_position().y ));
				}
			}
                window.setView(gameView);
            } 
			else if (character.get_position().x >= WINDOW_WIDTH/2) {
				sf::Vector2f currentCenter = gameView.getCenter();
				gameView.setCenter(character.get_position().x, currentCenter.y);
				sf::RectangleShape *lavaShape = lava.getLava();
				lava.set_position_rectangle(lavaShape,Vector2f(character.get_position().x - (WINDOW_WIDTH/2), lava.get_position().y ));
                window.setView(gameView);
        }
		window.clear();
		for (int i = 0; i < platforms.size(); i++) {
            window.draw(*platforms[i].getPlatform());
        }
		window.draw(*lava.getLava());
		//network code
		characterState.characterX = character.get_position().x;
		characterState.characterY = character.get_position().y;
		networkManager.sendGameData(characterState.characterX, characterState.characterY, timeline.getGameTime(realTimeClock));
	
		std::map<int, sf::Vector2f> characterPositions;
		characterPositions = networkManager.receiveGameData(std::ref(platforms),std::ref(movingPlatforms));

		for(int i = 0; i < movingPlatforms.size(); i++)
		{
			window.draw(*movingPlatforms[i].getMovingPlatform());
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
	Character character;
	int clientID;
    float gravity;
    unsigned WINDOW_WIDTH;
    unsigned WINDOW_HEIGHT;
    sf::RenderWindow window;
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