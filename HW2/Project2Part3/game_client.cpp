#include <SFML/Graphics.hpp>
#include <SFML/Network.hpp>
#include <iostream>
#include <thread>
#include <string>
#include <SFML/Window.hpp>
#include <SFML/System.hpp>
#include <bits/stdc++.h>
#include <zmq.hpp>
using namespace sf;
class GameState {
public:
    float characterX;
    float characterY;
    // Add more game state variables as needed

    // Deserialize the received message into the game state
    void Deserialize(zmq::message_t& message) {
        memcpy(this, message.data(), sizeof(GameState));
    }
};
int main() {

    //initializations
	float dt;
	float nexPos;
	float gravity = 20.f;   //cite https://www.youtube.com/watch?v=PGMdLgk4FD0
	float jumpSpeed = 1250.f;
	bool moveX = false;
    bool moveY = false;
    bool isJumping = false;
	const float movementSpeed = 150.f;
	const unsigned WINDOW_WIDTH = 1800;
	const unsigned WINDOW_HEIGHT = 1600;
	Vector2f velocity;
	Vector2f platformVelocity;
	Vector2f characterPlatformVelocity; //characters relative velocity to platform
	Vector2f prevMovingPlatformPosition;
	Clock dt_clock;

	sf::RenderWindow window(sf::VideoMode(WINDOW_WIDTH, WINDOW_HEIGHT), "Objects Homework");
	sf::RectangleShape platform(sf::Vector2f(400, 100));
	sf::RectangleShape character(sf::Vector2f(50,50));
	sf::RectangleShape movingplatform(sf::Vector2f(500, 100));

	//Borders for collision detection
	sf::FloatRect characterBorder = character.getGlobalBounds();
	sf::FloatRect platformBorder = platform.getGlobalBounds();
	sf::FloatRect movingPlatformBorder = movingplatform.getGlobalBounds();
    window.setFramerateLimit(120);
	
	sf::Texture texture;
	if (!texture.loadFromFile("brick1.png"))  //bricks-texture- https://gamedeveloperstudio.itch.io/sma
    return -1;

	character.setFillColor(sf::Color(255,0,0));
	character.setPosition(50.f, 545.f);//545

	platform.setFillColor(sf::Color(165,42,42));
	platform.setOutlineThickness(5);
	platform.setOutlineColor(sf::Color(0,255,0));
	platform.setTexture(&texture);
	platform.setPosition(10.f,600.f);

	movingplatform.setFillColor(sf::Color(165,42,42));
	movingplatform.setOutlineThickness(5);
	movingplatform.setOutlineColor(sf::Color(0,255,0));
	movingplatform.setTexture(&texture);
	movingplatform.setPosition(480, 400);

	//intial platform velocity
	platformVelocity.x = movementSpeed * dt;
	platformVelocity.y = 0.f;

    // sf::TcpSocket serverSocket;
    // serverSocket.connect("server_ip", 5000); // Replace "server_ip" with your server's IP address
    zmq::context_t context(1);
    zmq::socket_t req_socket(context, ZMQ_REQ);
    req_socket.connect("tcp://localhost:5556");

    zmq::socket_t sub_socket(context, ZMQ_SUB);
    sub_socket.connect("tcp://localhost:5555");
    sub_socket.setsockopt(ZMQ_SUBSCRIBE, "", 0);

    // Your game code and logic here

    while (window.isOpen()) {
        // Handle user input and update game state
        dt = dt_clock.restart().asSeconds();
		gravity = 20.f;
		sf::Event event;
		characterPlatformVelocity.x=0;
		characterPlatformVelocity.y=0;
		prevMovingPlatformPosition = movingplatform.getPosition();
		while(window.pollEvent(event))
		{
			if(event.type == sf::Event::Closed)
			{
				window.close();
			}
			else if (event.type == sf::Event::Resized && sf::Keyboard::isKeyPressed(sf::Keyboard::E))  //cite https://www.sfml-dev.org/tutorials/2.2/graphics-view.php#showing-more-when-the-window-is-resized
    		{
        		sf::FloatRect visibleArea(0, 0, event.size.width, event.size.height);
        		window.setView(sf::View(visibleArea));
    		}
			//cite sfml-dev.org/documentation/2.5.1/classsf_1_1Event.php#details
		}

		characterBorder = character.getGlobalBounds();
    	platformBorder = platform.getGlobalBounds();
    	movingPlatformBorder = movingplatform.getGlobalBounds();

		velocity.y += gravity*dt;
		moveX = false;
        moveY = true;
		
		//collision normal platform
		if(characterBorder.intersects(platformBorder))
		{
			// Collision detected with the platform
   			velocity.y = 0;  // Stop falling due to gravity
    		isJumping = false;
    		std::cout<<"collided";
    		character.setPosition(character.getPosition().x, platformBorder.top - characterBorder.height); //cite https://www.sfml-dev.org/tutorials/2.5/graphics-transform.php
		}
		//collision moving platform
		if(characterBorder.intersects(movingPlatformBorder))
		{
			// Collision detected with the moving platform
			velocity.y = 0;  // Stop falling due to gravity
			isJumping = false;

			moveX=true;
			characterPlatformVelocity = platformVelocity;
			character.setPosition(character.getPosition().x, movingPlatformBorder.top - characterBorder.height);
			// Update the characterPlatformVelocity to match the platform's velocity
			
    		std::cout<<"collided";
		}

		//movement
		//cite https://www.sfml-dev.org/tutorials/2.5/window-inputs.php
		if (sf::Keyboard::isKeyPressed(sf::Keyboard::A))
		{
    		// character.move(-1, 0);
    		velocity.x = -movementSpeed*dt;
    		moveX = true;
		}	
		if (sf::Keyboard::isKeyPressed(sf::Keyboard::D))
		{
    	    // character.move(1, 0);
    	    velocity.x = movementSpeed*dt;
    	    moveX = true;
    		// cout<<pos.x + 1;
		}
		if (sf::Keyboard::isKeyPressed(sf::Keyboard::Space) && !isJumping)
		{
    		// Space key is pressed, and the character is not already jumping
    		velocity.y -= jumpSpeed * dt;  // Apply upward velocity for jumping
    		isJumping = true;  // Set jumping flag to true	
		}

		if(moveX)
		{
			character.setPosition(character.getPosition().x + velocity.x + characterPlatformVelocity.x, character.getPosition().y);
		}
		if(moveY) //now this is redundant since movey is always true
		{
			character.setPosition(character.getPosition().x, character.getPosition().y + velocity.y);
		}

		if(character.getPosition().x<0)
		{
			character.setPosition(0.f, character.getPosition().y);
		}
		if(character.getPosition().x  > WINDOW_WIDTH)
		{
			character.setPosition(WINDOW_WIDTH, character.getPosition().y);
		}
		
		if(movingplatform.getPosition().x<=480) //cite https://www.youtube.com/watch?v=NhXXC37Xoyo
		{
			platformVelocity.x = movementSpeed * dt;
			platformVelocity.y = 0.f;
		}
		if(movingplatform.getPosition().x + movingplatform.getGlobalBounds().width>=WINDOW_WIDTH)
		{
			platformVelocity.x = -movementSpeed * dt;
			platformVelocity.y = 0.f;
		}

        // Send player's input or game state changes to the server
        // You'll need to design a protocol for sending player actions and receiving updates from the server
        zmq::message_t update;
        sub_socket.recv(update, zmq::recv_flags::none);
        std::string msg(static_cast<char*>(update.data()), update.size());
        std::cout << msg << std::endl;

        // Receive updates from the server and update the game state
        movingplatform.move(platformVelocity);
        window.clear();
        // Draw game objects
        window.draw(platform);
		window.draw(movingplatform);
		window.draw(character);
        window.display();
    }

    // Clean up and close sockets when the client exits
    // serverSocket.disconnect();

    return 0;
}
