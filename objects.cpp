#include <SFML/Graphics.hpp>
#include <SFML/Window.hpp>
#include <SFML/System.hpp>
#include <bits/stdc++.h>
using namespace sf;

int main()
{
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

	//set frame rate
	window.setFramerateLimit(120);
	
	sf::Texture texture;
	if (!texture.loadFromFile("brick1.png"))  //bricks-texture- https://gamedeveloperstudio.itch.io/sma
    return -1;

	character.setFillColor(sf::Color(255,0,0));
	// character.setOutlineThickness(5);
	// character.setOutlineColor(sf::Color(0,255,0));
	// character.setTexture(&texture);
	// rectangle.setTextureRect(sf::IntRect(280, 280, 30, 30));
	character.setPosition(50.f, 545.f);//545

	platform.setFillColor(sf::Color(165,42,42));
	platform.setOutlineThickness(5);
	platform.setOutlineColor(sf::Color(0,255,0));
	platform.setTexture(&texture);
	// rectangle.setTextureRect(sf::IntRect(280, 280, 30, 30));
	platform.setPosition(10.f,600.f);

	movingplatform.setFillColor(sf::Color(165,42,42));
	movingplatform.setOutlineThickness(5);
	movingplatform.setOutlineColor(sf::Color(0,255,0));
	movingplatform.setTexture(&texture);
	movingplatform.setPosition(480, 400);

	//intial platform velocity
	platformVelocity.x = movementSpeed * dt;
	platformVelocity.y = 0.f;

	
	while(window.isOpen())
	{
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
		// if (characterBorder.intersects(platformBorder))
		// {
    	// collision!
		// 	cout<<"Collided with platform 1";

		// 	gravity=0;
		// }
		// if(characterBorder.intersects(movingPlatformBorder))
		// {
    	// collision!
		// 	cout<<"Collided with platform 1";
		// 	gravity = 0;
		// 	move
		// }
		// else
		// {
		// 	velocity.y=0;
		// }

		characterBorder = character.getGlobalBounds();
    	platformBorder = platform.getGlobalBounds();
    	movingPlatformBorder = movingplatform.getGlobalBounds();

		// velocity.x = 0.f;
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

			// Calculate the relative position of the character on the platform
			// float relativePosX = character.getPosition().x - movingPlatformBorder.left;

			// Update the character's position relative to the moving platform
			// character.setPosition(movingplatform.getPosition().x + relativePosX, movingPlatformBorder.top - characterBorder.height);
			moveX=true;
			characterPlatformVelocity = platformVelocity;
			character.setPosition(character.getPosition().x, movingPlatformBorder.top - characterBorder.height);
			// Update the characterPlatformVelocity to match the platform's velocity
			
    		std::cout<<"collided";
    		// moveX = true;
    		// characterPlatformVelocity.x = platformVelocity.x;
    		// characterPlatformVelocity.y = platformVelocity.y;
    		// character.move(platformVelocity);
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
			// spacePressed = true;		
		}

		// if(10<=character.getPosition().x<=400) //collision for normal platform
		// {
		// 	if(character.getPosition().y + character.getGlobalBounds().height == 600)
		// 	{
		// 		gravity=0.f;
		// 	}
		// }
		// if(480<=character.getPosition().x<=980) //collision for normal platform
		// {
		// 	if(character.getPosition().y + character.getGlobalBounds().height == 400)
		// 	{
		// 		gravity=0.f;
		// 	}
		// }
		if(moveX)
		{
			character.setPosition(character.getPosition().x + velocity.x + characterPlatformVelocity.x, character.getPosition().y);
		}
		if(moveY) //now this is redundant since movey is always true
		{
			// std::cout<<"moving";
			character.setPosition(character.getPosition().x, character.getPosition().y + velocity.y);
		}
		// character.move(velocity);


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

		
		movingplatform.move(platformVelocity);

		window.clear();
		window.draw(platform);
		window.draw(movingplatform);
		window.draw(character);
		window.display();
	}
	return 0;
}





//cite https://en.sfml-dev.org/forums/index.php?topic=27820.0






