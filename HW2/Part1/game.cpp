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
#include <iomanip>
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

class Character : public PentagonShape{
public: 
	Character() : jumpSpeed(1000.f), moveX(false), moveY(false), isJumping(false), movementSpeed(150.f), character(50){	
	}

	void initialize(){
		character.setFillColor(sf::Color(255,0,0));
		character.setPosition(50.f, 545.f);//50, 545
	}

	void set_characterPlatformVelocity(float x, float y){
	characterPlatformVelocity.x = x;
	characterPlatformVelocity.y = y;
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

struct CharacterState {
    // int clientId;
    float characterX;
    float characterY; 
};

struct GameState{
	std::map<int, Vector2f> characterPositions;
	float movingPlatformPositionX;
    float movingPlatformPositionY;
};

//loading and applying textures to platforms through multithreading
void thread1(sf::Texture& texture, sf::RectangleShape& platform, sf::RectangleShape& movingplatform) {
    mutex.lock();
    if (!texture.loadFromFile("brick1.png")) {
    	//asset link - bricks-texture- https://gamedeveloperstudio.itch.io/sma
        std::cerr << "Failed to load texture" << std::endl;
    }
    platform.setTexture(&texture);
    movingplatform.setTexture(&texture);
    mutex.unlock();
}

//Using seperate thread to handle the movement of the moving platform
void thread2(sf::RectangleShape& movingplatform, Vector2f& platformVelocity, float movementSpeed, float dt, const unsigned WINDOW_WIDTH, bool paused) {
    mutex.lock();

    if (movingplatform.getPosition().x <= 480) {
        if (!paused) {
            platformVelocity.x = movementSpeed;
        } else {
        	dt=0;
        	platformVelocity.x = movementSpeed;
            // platformVelocity.x = 0.f; // Don't update velocity when paused
        }
    } else if (movingplatform.getPosition().x + movingplatform.getGlobalBounds().width >= WINDOW_WIDTH) {
        if (!paused) {
            platformVelocity.x = -movementSpeed;
        } else {
        	dt=0;
            platformVelocity.x = -movementSpeed; // Don't update velocity when paused
        }
    }
    platformVelocity.y = 0.f;

    mutex.unlock();
}

int main()
{


	//network code
	int clientID = 0;
	CharacterState characterState;
	GameState gameState;
	zmq::context_t context(1);
    zmq::socket_t req_socket(context, ZMQ_REQ);
    req_socket.connect("tcp://localhost:5556");

    zmq::socket_t sub_socket(context, ZMQ_SUB);
	int conflate = 1;
    sub_socket.setsockopt(ZMQ_CONFLATE, &conflate, sizeof(conflate));
    sub_socket.connect("tcp://localhost:5555");
    sub_socket.setsockopt(ZMQ_SUBSCRIBE, "", 0);
	std::string reply = "Joined";
    req_socket.send(zmq::const_buffer(reply.c_str(), reply.size()), zmq::send_flags::none);

	zmq::message_t response;
	req_socket.recv(response, zmq::recv_flags::none);
	std::string responseStr(static_cast<char*>(response.data()), response.size());
	// std::cout<<responseStr<<std::endl;
	clientID = std::stoi(responseStr);
	
	
	// Network network;
	// CharacterState characterState;
	// network.server_sendMessage(characterState);
	//initializations
	float gravity = 20.f;
	const unsigned WINDOW_WIDTH = 1800;
	const unsigned WINDOW_HEIGHT = 1600;
	Vector2f platformVelocity;
	Vector2f prevMovingPlatformPosition;
	Clock dt_clock; //global time
	sf::Clock realTimeClock; //real-time clock

	sf::Texture texture;
	sf::RenderWindow window(sf::VideoMode(WINDOW_WIDTH, WINDOW_HEIGHT), "Platforms");
	sf::RectangleShape platform(sf::Vector2f(400, 100));

	sf::RectangleShape movingplatform(sf::Vector2f(500, 100));

	//Borders for collision detection
	sf::FloatRect platformBorder = platform.getGlobalBounds();
	sf::FloatRect movingPlatformBorder = movingplatform.getGlobalBounds();

	//timeline stuff
	Timeline timeline;
	Character character;
	character.initialize();
	float gameTime= timeline.getGameTime(realTimeClock); //game time
	//set frame rate limit
	// window.setFramerateLimit(120);

	window.setVerticalSyncEnabled(true);

	platform.setFillColor(sf::Color(165,42,42));
	platform.setOutlineThickness(5);
	platform.setOutlineColor(sf::Color(0,255,0));
	platform.setPosition(10.f,600.f);

	movingplatform.setFillColor(sf::Color(165,42,42));
	movingplatform.setOutlineThickness(5);
	movingplatform.setOutlineColor(sf::Color(0,255,0));	
	movingplatform.setPosition(480, 400);

	std::thread t1(&thread1, std::ref(texture), std::ref(platform), std::ref(movingplatform));
    t1.join();

    static bool pKeyPressed = false; 
    static bool qKeyPressed = false;

	while(window.isOpen())
	{
		// Clock loop_clock;
		// sf::Time loopIterationTime = loop_clock.getElapsedTime();
		// float loopIterationTimeElapsed = loopIterationTime.asSeconds();
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
    			// std::cout<<"\n P key pressed";
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
			// std::cout<<"\n Q key pressed";
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
    	platformBorder = platform.getGlobalBounds();
    	movingPlatformBorder = movingplatform.getGlobalBounds();

		// velocity.y += gravity*timeline.getDeltaJumpTime();
		character.set_characterVelocityY(character.get_characterVelocity().y+gravity*timeline.getDeltaJumpTime());
		character.set_moveX(false);
		character.set_moveY(true);
		
		// Collision detection with the platform
		if (character.get_characterBorder().intersects(platformBorder)) {
    		// Collision detected with the platform
    		character.set_characterVelocityY(0.f); // Stop falling due to gravity
    		character.set_isJumping(false);
    		// Adjust the character's position to be just above the platform
    		character.set_characterPosition(character.get_characterPosition().x, platformBorder.top - character.get_characterBorder().height - 1.f);
		}

		// Collision detection with the moving platform
		if (character.get_characterBorder().intersects(movingPlatformBorder)) {
    		// Collision detected with the moving platform
    		character.set_characterVelocityY(0.f); // Stop falling due to gravity
    		character.set_isJumping(false);
    		character.set_moveX(true);
    		// Adjust the character's position to be just above the moving platform
    		character.set_characterPosition(character.get_characterPosition().x, movingPlatformBorder.top - character.get_characterBorder().height - 1.f);
    		// Update the characterPlatformVelocity to match the platform's velocity
    		character.set_characterPlatformVelocity(platformVelocity.x * timeline.getDeltaTime(), 0.f);
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
    	std::thread t2(&thread2, std::ref(movingplatform), std::ref(platformVelocity), character.get_movementSpeed(), timeline.getDeltaTime(), std::ref(WINDOW_WIDTH), timeline.isPaused());
        t2.detach(); 

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
		if(character.get_characterPosition().x  > WINDOW_WIDTH)
		{
			character.set_characterPosition(WINDOW_WIDTH, character.get_characterPosition().y);
		}

		window.clear();
		window.draw(platform);
		window.draw(movingplatform);
		//network code
		// std::cout<<"Method called"<<std::endl;
		characterState.characterX = character.get_characterPosition().x;
		characterState.characterY = character.get_characterPosition().y;
		// std::cout<<clientID<<" "<<characterState.characterX<<" "<<characterState.characterY<<std::endl;
		std::string serializedData = std::to_string(clientID) + ","
                         + std::to_string(characterState.characterX) + ","
                         + std::to_string(characterState.characterY);
		req_socket.send(zmq::const_buffer(serializedData.c_str(), serializedData.size()), zmq::send_flags::none);
		zmq::message_t response;
        req_socket.recv(response, zmq::recv_flags::none);

        std::string responseStr(static_cast<char*>(response.data()), response.size());
        // std::cout<<responseStr<<std::endl;

		zmq::message_t message;
		sub_socket.recv(message, zmq::recv_flags::none);
		std::string receivedData(static_cast<char*>(message.data()), message.size()); 
		std::vector<std::string> components;
		std::istringstream dataStream(receivedData);
		std::string component;
		while (std::getline(dataStream, component, ',')) {
    		components.push_back(component);
		}
		// std::cout<<receivedData<<std::endl;
		// Ensure that there are at least three components (id, x, y) in each entry
		const int componentsPerEntry = 3;
		std::vector <PentagonShape> characters;
		std::map <int, Vector2f> characterPositions;
		if (components.size() % componentsPerEntry == 0) {
    	for (size_t i = 0; i < components.size(); i += componentsPerEntry) {
        try {
            int id = std::stoi(components[i]);
            float x = std::stof(components[i + 1]);
            float y = std::stof(components[i + 2]);
			std::cout<< id << ":" << x << ":" << y << std::endl;
            // Update the characterPositions map
            characterPositions[id] = sf::Vector2f(x, y);
			// PentagonShape character(50);
			// character.setPosition(sf::Vector2f(x, y));
			// character.setFillColor(sf::Color(255,0,0));
			// characters.push_back(character);
        } catch (const std::exception& e) {
			
        }
    	}
		} 
		int count = 0;
		for (auto i : characterPositions) 
        {
                // std::cout<<i.first << i.second.x << i.second.y << "  " <<std::endl; 
				PentagonShape c[30];
				c[count].setPosition(sf::Vector2f(i.second.x, i.second.y ));
				c[count].setFillColor(sf::Color(255,0,0));
				window.draw(c[count]);
				count++;
				// characters.push_back(c);

        }
		// for (const PentagonShape& c : characters) {
		// 	// std::cout << "Character position: " << c.getPosition().x<<std::endl;j
    	// 	// window.draw(c);
		// }
		movingplatform.move(platformVelocity.x*timeline.getDeltaTime(), 0.f);
		// character.character_draw(window);
		window.display();
		// gameState.characterPositions.clear();
		
	}
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





