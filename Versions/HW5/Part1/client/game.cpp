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
#include <queue>
// standard c++ header

//zmq header
#include <zmq.hpp>

//v8 headers
#include "ScriptManager.h"
#include <v8/v8.h>
#include <libplatform/libplatform.h>
#include <iostream>
#include "v8helpers.h"
#include <cstdio>
#include <cstdlib> 

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

//---------------------------------GameObject Class--------------------------------
class GameObject {
public:
    GameObject(){}

    sf::Vector2f get_position_rectangle(RectangleShape *object) {
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
		object->setPosition(pos);
	}
	void set_position_rectangle(RectangleShape *object, Vector2f pos){
		object->setPosition(pos);
	}
protected:
};

//------------------------------------------------Spawner class--------------------------------------------------------------
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

//-------------------------------------------------Death Zone-----------------------------------------------------------------
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

//------------------------------------------------Lava Class---------------------------------------
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

class SideBoundary {
public:
    SideBoundary(FloatRect bounds) : boundary(bounds) {}
    FloatRect getBounds() const { return boundary; }
private:
    FloatRect boundary;
};

class Platform:public GameObject{
public:
	Platform(sf::Color color, sf::Color outlineColor, Vector2f pos, Vector2f size) : platform(size), GameObject(), deathCount(0){
		platform.setFillColor(color);
		platform.setOutlineThickness(5);
		platform.setOutlineColor(outlineColor);
		platform.setPosition(pos);
	}
	sf::RectangleShape* getPlatform(){
		return &platform;
	}
	//----------------------------------------------v8stuff---------------------------------------------------
	static void getPlatformColor(v8::Local<v8::String> property, const v8::PropertyCallbackInfo<v8::Value>& info)
	{
    	v8::Local<v8::Object> self = info.Holder();
    	v8::Local<v8::External> wrap = v8::Local<v8::External>::Cast(self->GetInternalField(0));
    	void* ptr = wrap->Value();
    	int colorValue = static_cast<Platform*>(ptr)->color;
    	info.GetReturnValue().Set(v8::Integer::NewFromUnsigned(info.GetIsolate(), static_cast<uint32_t>(colorValue)));
	}
	static void setPlatformColor(v8::Local<v8::String> property, v8::Local<v8::Value> value, const v8::PropertyCallbackInfo<void>& info)
	{
		v8::Local<v8::Object> self = info.Holder();
		v8::Local<v8::External> wrap = v8::Local<v8::External>::Cast(self->GetInternalField(0));
		void* ptr = wrap->Value();
		static_cast<Platform*>(ptr)->color = value->Int32Value();
		
	}
	static void getToggleEvent(v8::Local<v8::String> property, const v8::PropertyCallbackInfo<v8::Value>& info)
	{
		v8::Local<v8::Object> self = info.Holder();
		v8::Local<v8::External> wrap = v8::Local<v8::External>::Cast(self->GetInternalField(0));
		void* ptr = wrap->Value();
		std::string toggleEventValue = static_cast<Platform*>(ptr)->toggleEvent;
		v8::Local<v8::String> v8_toggleEvent = v8::String::NewFromUtf8(info.GetIsolate(), toggleEventValue.c_str(), v8::String::kNormalString);
		info.GetReturnValue().Set(v8_toggleEvent);
	}

	static void setToggleEvent(v8::Local<v8::String> property, v8::Local<v8::Value> value, const v8::PropertyCallbackInfo<void>& info)
	{
		v8::Local<v8::Object> self = info.Holder();
		v8::Local<v8::External> wrap = v8::Local<v8::External>::Cast(self->GetInternalField(0));
		void* ptr = wrap->Value();
		v8::String::Utf8Value utf8_str(info.GetIsolate(), value->ToString());
		static_cast<Platform*>(ptr)->toggleEvent = *utf8_str;
	}
	static void getHandleEvent(v8::Local<v8::String> property, const v8::PropertyCallbackInfo<v8::Value>& info)
	{
    	v8::Local<v8::Object> self = info.Holder();
    	v8::Local<v8::External> wrap = v8::Local<v8::External>::Cast(self->GetInternalField(0));
    	void* ptr = wrap->Value();
    	int deathCountValue = static_cast<Platform*>(ptr)->deathCount;
    	info.GetReturnValue().Set(v8::Integer::NewFromUnsigned(info.GetIsolate(), static_cast<uint32_t>(deathCountValue)));
	}
	static void setHandleEvent(v8::Local<v8::String> property, v8::Local<v8::Value> value, const v8::PropertyCallbackInfo<void>& info)
	{
		v8::Local<v8::Object> self = info.Holder();
		v8::Local<v8::External> wrap = v8::Local<v8::External>::Cast(self->GetInternalField(0));
		void* ptr = wrap->Value();
		static_cast<Platform*>(ptr)->deathCount = value->Int32Value();
		
	}
	std::string get_toggleEvent(){
		return toggleEvent;
	}

	int get_deathCount(){
		return deathCount;
	}
	// static voi raise_event(){
	// 	v8::Isolate *isolate = args.GetIsolate();
	// 	v8::Local<v8::Context> context = isolate->GetCurrentContext();
	// 	v8::EscapableHandleScope context_scope(context);
	// 	v8::Context 
	// }
	void set_color(){
		sf::Color newColor(color);
		platform.setOutlineColor(newColor);
	}

	v8::Local<v8::Object> exposeToV8(v8::Isolate *isolate, v8::Local<v8::Context> &context, std::string context_name)
	{
	std::vector<v8helpers::ParamContainer<v8::AccessorGetterCallback, v8::AccessorSetterCallback>> v;
	v.push_back(v8helpers::ParamContainer("color", getPlatformColor, setPlatformColor));
	v.push_back(v8helpers::ParamContainer("toggleEvent", getToggleEvent, setToggleEvent));
	v.push_back(v8helpers::ParamContainer("deathCount", getHandleEvent, setHandleEvent));
	return v8helpers::exposeToV8("platform", this, v, isolate, context, context_name);
	}
	//--------------------------------------------v8stuff-------------------------------------------------------
	void platform_set_texture(sf::Texture& texture){
		platform.setTexture(&texture);
	}
protected:
	sf::RectangleShape platform;	
	sf::FloatRect platformBorder = platform.getGlobalBounds();	
	static unsigned int color;
	std::string toggleEvent;
	int deathCount;

};
unsigned int Platform::color;

//----------------------------------------------Timeline Class------------------------------
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

//---------------------------------------------------------------Moving Platform Class--------------------------------------------------
class MovingPlatform : public Timeline, public GameObject{
public:
	MovingPlatform(sf::Color color, sf::Color outlineColor, Vector2f size, Vector2f pos, Vector2f bound, Vector2f speed) :movingplatform(size), GameObject(),movementSpeed(speed), movingPlatformPosition(pos), boundary(bound){
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

//--------------------------------------------Character Class---------------------------------------------------------
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

//-------------------------------------------------------------------------Network Manager Class---------------------------------------------------------
class NetworkManager {
public:
    NetworkManager(int clientID) : clientID(clientID) {
        context = zmq::context_t(1);
        req_socket = zmq::socket_t(context, ZMQ_REQ);
        req_socket.connect("tcp://localhost:5556");
        sub_socket = zmq::socket_t(context, ZMQ_SUB);
        int conflate = 1;
        sub_socket.setsockopt(ZMQ_CONFLATE, &conflate, sizeof(conflate));
		event_socket = zmq::socket_t(context, ZMQ_REQ);
		event_socket.connect("tcp://localhost:5557");
        sub_socket.connect("tcp://localhost:5555");
        sub_socket.setsockopt(ZMQ_SUBSCRIBE, "", 0);
    }
	int getClientId(){
		return clientID;
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
	//NEW CODE TO SEND EVENT DATA FROM CLIENT TO SERVER
	void sendEvents(int id, std::string name, Timeline *time){
		if(name=="ChangeGameSpeed"){
		std::string serializedData = std::to_string(id) + "," + name;
		event_socket.send(zmq::const_buffer(serializedData.c_str(), serializedData.size()), zmq::send_flags::none);
		zmq::message_t response;
		event_socket.recv(response, zmq::recv_flags::none);
		std::string responseStr(static_cast<char*>(response.data()), response.size());
		time->setGameSpeed(std::stof(responseStr));
		}
		else{
			std::string serializedData = "Empty";
		event_socket.send(zmq::const_buffer(serializedData.c_str(), serializedData.size()), zmq::send_flags::none);
		zmq::message_t response;
		event_socket.recv(response, zmq::recv_flags::none);
		}
	}
	//TILL HERE
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
	//NEW SOCKET
	zmq::socket_t event_socket;
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

//----------------------------------------------------------Event Manager Class--------------------------------------------------------------------------
class EventManager {
public:
    struct GameEvent {
        GameEvent(){}

        enum class Type {
            CharacterCollision,
            CharacterDeath,
            CharacterSpawn,
            UserInput,
			ChangeGameSpeed,
			Replay
            // Add more event types as needed
        } type;
		Character *character;
		Timeline *time;
		Platform *platform;
		MovingPlatform *movingPlatform;
		Lava *lava;
		Vector2f window_size;
		std::vector<MovingPlatform> *movingPlatforms;
		std::map<int, sf::Vector2f> *characterPositions;
		std::vector<Platform> *platforms;
		sf::RenderWindow *window;
        float timestamp;  // To store the time when the event occurred.
		std::string userInputType;
		std::string name;
		sf::View *gameView;
		int clientId;
		bool replay;
		bool operator<(const GameEvent& other) const {
            return timestamp > other.timestamp;  // Use > to make the priority queue behave like a min-heap.
        }
    };

    EventManager() {
    }
	void handleCharacterCollision(GameEvent& event) {
		std::string userInputType = event.userInputType;
		PentagonShape *characterShape = event.character->getCharacter();
		if(userInputType=="movingplatform"){
			sf::RectangleShape* movingPlatformShape = event.movingPlatform->getMovingPlatform();
			if (event.character->getBoundsPentagonShape(characterShape).intersects(event.movingPlatform->getBoundsRectangleShape(movingPlatformShape))) {
    		// Collision detected with the moving platform
    		event.character->set_characterVelocityY(0.f); // Stop falling due to gravity
    		event.character->set_isJumping(false);
    		event.character->set_moveX(true);
    		event.character->set_position_pentagon(characterShape,Vector2f(event.character->get_position_pentagon(characterShape).x, event.movingPlatform->getBoundsRectangleShape(movingPlatformShape).top - event.character->getBoundsPentagonShape(characterShape).height - 1.f));
    		event.character->set_characterPlatformVelocity(0.f, 0.f);
			}
		}
		else if(userInputType=="platform"){
			sf::RectangleShape* platformShape = event.platform->getPlatform();
			if (event.character->getBoundsPentagonShape(characterShape).intersects(event.platform->getBoundsRectangleShape(platformShape))) {
    		// Collision detected with the platform
    		event.character->set_characterVelocityY(0.f); // Stop falling due to gravity
    		event.character->set_isJumping(false);
    		event.character->set_position_pentagon(characterShape,Vector2f(event.character->get_position_pentagon(characterShape).x, event.platform->getBoundsRectangleShape(platformShape).top - event.character->getBoundsPentagonShape(characterShape).height - 1.f));
			}
		}
		else if(userInputType == "lava"){
			characterDeathEvent(event);
		}
	}
	void handleCharacterDeath(GameEvent& event){
			event.gameView->setCenter(event.window_size);
        	event.window->setView(*event.gameView);
			event.lava->lava_respawn();
			characterSpawnEvent(event);
	}
	void handleCharacterRespawn(const GameEvent& event){
			event.character->character_respawn();
	}
	void handleReplay(GameEvent& event){
			
	}
	void handleUserInputs(GameEvent& event){
    	std::string userInputType = event.userInputType;
    	if (userInputType == "A") {
			event.character->set_characterVelocityX(-event.character->get_movementSpeed()*event.time->getDeltaTime());
			event.character->set_moveX(true);
    	} else if (userInputType == "D") {
			event.character->set_characterVelocityX(event.character->get_movementSpeed()*event.time->getDeltaTime());
    	    event.character->set_moveX(true);
    	} else if (userInputType == "Space") {
    		event.character->set_characterVelocityY(event.character->get_characterVelocity().y - (event.character->get_jumpSpeed() * event.time->getDeltaJumpTime()));  // Apply upward velocity for jumping
    		event.character->set_isJumping(true); // Set jumping flag to true	
    	} else if (userInputType == 'Q'){
				event.type = GameEvent::Type::ChangeGameSpeed;
				event.name = "ChangeGameSpeed";
				NetworkManager network(event.clientId);
				network.sendEvents(event.clientId, event.name, event.time);
    	}
	}
    void characterCollisionPlatformEvent(Character *object, Platform *platform, std::string input) {
        GameEvent event;
        event.type = GameEvent::Type::CharacterCollision;
        event.timestamp = timeline.getGameTime();
		event.platform=platform;
		event.character = object;
		event.userInputType = input;
        eventQueue.push(event);
    }
	void characterCollisionLavaEvent(Character *object, Lava *lava, sf::RenderWindow *window, sf::View *gameView, unsigned window_width, unsigned window_height,std::string input) {
		GameEvent event;
        event.type = GameEvent::Type::CharacterCollision;
        event.timestamp = timeline.getGameTime();
		event.character = object;
		event.userInputType = input;
		event.lava = lava;
		event.window=window;
		event.window_size = Vector2f(window_width/2, window_height/2);
		event.gameView = gameView;
        eventQueue.push(event);
	}
	void characterCollisionMovingPlatformEvent(Character *object, MovingPlatform *movingPlatform, std::string input) {
        GameEvent event;
        event.type = GameEvent::Type::CharacterCollision;
        event.timestamp = timeline.getGameTime();
		event.movingPlatform=movingPlatform;
		event.character = object;
		event.userInputType = input;
        eventQueue.push(event);
    }
    void characterDeathEvent(GameEvent& event_collide) {
        GameEvent event = event_collide;
		event.type = GameEvent::Type::CharacterDeath;
        eventQueue.push(event);
    }
	void deathEvent(Character *object, Lava *lava, sf::RenderWindow *window, sf::View *gameView, unsigned window_width, unsigned window_height){
		GameEvent event;
		event.type = GameEvent::Type::CharacterDeath;
		event.timestamp = timeline.getGameTime();
		event.character = object;
		event.lava = lava;
		event.window=window;
		event.window_size = Vector2f(window_width/2, window_height/2);
		event.gameView = gameView;
        eventQueue.push(event);
	}
    void characterSpawnEvent(GameEvent& event_death) {
        GameEvent event = event_death;
        event.type = GameEvent::Type::CharacterSpawn;
        eventQueue.push(event);
    }
	void replayEvent(Character *object, bool rep, std::vector<MovingPlatform> *movingPlatform, std::vector<Platform> *platform, std::map<int, sf::Vector2f> *characterPositions){
		GameEvent event;
        event.type = GameEvent::Type::Replay;
        event.timestamp = timeline.getGameTime();
		event.character = object;
		event.movingPlatforms = movingPlatform;
		event.platforms = platform;
		event.characterPositions = characterPositions;
		event.replay = rep;
		replay = rep;
        eventQueue.push(event);
	}
    void userInputEvent(std::string input, Character *object, Timeline* time, int clientId) {
        GameEvent event;
        event.type = GameEvent::Type::UserInput;
        event.timestamp = timeline.getGameTime();
		event.time = time;
		event.userInputType = input;
		event.clientId = clientId;
		event.character = object;
        eventQueue.push(event);
    }
	void handleEvents() {
		GameEvent replay_event;
    while (!eventQueue.empty()) {
        GameEvent event = eventQueue.top();
        if (timeline.getGameTime() >= event.timestamp) {
            if (event.type == GameEvent::Type::CharacterCollision) {
                handleCharacterCollision(event);
            } else if (event.type == GameEvent::Type::CharacterDeath) {
                handleCharacterDeath(event);
				deathFlag = true;
            } else if (event.type == GameEvent::Type::CharacterSpawn) {
                handleCharacterRespawn(event);
            } else if (event.type == GameEvent::Type::UserInput) {
                handleUserInputs(event);
            }
			else if(event.type == GameEvent::Type::Replay){
				replay_event = event;
			}
			if(replay == true){
				
				platforms.push_back(Platform(sf::Color(165,42,42), sf::Color(0,255,0), sf::Vector2f(10.f,600.f),sf::Vector2f(400.f, 100.f)));
				platforms.push_back(Platform(sf::Color(165,42,42), sf::Color(0,0,255), sf::Vector2f(1800.f,500.f), sf::Vector2f(200.f, 100.f)));
				movingPlatforms.push_back(MovingPlatform(sf::Color(165,42,42), sf::Color(255,0,0),sf::Vector2f(500.f, 100.f), Vector2f(480.f, 500.f), Vector2f(480.f, 500.f), Vector2f(150.f, 0.f)));
				movingPlatforms.push_back(MovingPlatform(sf::Color(165,42,42), sf::Color(200,255,0),sf::Vector2f(500.f, 100.f), Vector2f(2150.f, 200.f), Vector2f(2150.f, 200.f), Vector2f(0.f, 150.f)));
				replayVector.push_back(event);
			}
			else if(replay == false){
				if (event.type == GameEvent::Type::Replay){
					handleReplay(event);
				}
			}
            eventQueue.pop();
        } else {
            break;
        }
    }
	}


	bool replay;
    std::priority_queue<GameEvent> eventQueue; 
	std::vector<Platform> platforms;
	std::vector<Vector2f> movingplatform1_positions;
	std::vector<Vector2f> movingplatform2_positions;
	PentagonShape c[30];
	std::vector<MovingPlatform> movingPlatforms;
	std::vector<GameEvent> replayVector;
    Timeline timeline;
	bool deathFlag;
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


int main()
{
	//--------------------------------------------v8stuff-------------------------------------------------------
    std::unique_ptr<v8::Platform> platform = v8::platform::NewDefaultPlatform();
    v8::V8::InitializePlatform(platform.release());
    v8::V8::InitializeICU();
    v8::V8::Initialize();
    v8::Isolate::CreateParams create_params;
    create_params.array_buffer_allocator = v8::ArrayBuffer::Allocator::NewDefaultAllocator();
    v8::Isolate *isolate = v8::Isolate::New(create_params);

    { 
		CharacterState characterState;
		GameState gameState;
		
		Clock dt_clock; //global time
		sf::Clock realTimeClock; //real-time clock
    	Timeline timeline;
		Lava lava(1800.f,1600.f,20.f);
		Spawner characterSpawner(50.f,545.f);
		EventManager eventManager;
		Character character;
		int clientID=1;
		NetworkManager networkManager(clientID);
    	float gravity=20.f;
    	unsigned WINDOW_WIDTH=1600;
    	unsigned WINDOW_HEIGHT=1600;
    	sf::RenderWindow window(sf::VideoMode(WINDOW_WIDTH, WINDOW_HEIGHT), "Platforms");
		window.setVerticalSyncEnabled(true);
		const float SIDE_SCROLL_SPEED = 100.0f; // Adjust the scroll speed as needed
    	const float CHARACTER_BOUNDARY_X = 0.0f;
		std::vector<Vector2f> movingPlatform1_position;
		std::vector<Vector2f> movingPlatform2_position;
		std::string lastKeyPressed;
		int replay_count = 0;
		bool event_message;
		bool replay = false;
		bool replayStopped = false;
		std::map<int, std::vector<Vector2f>> characterPositionsReplay;
		PentagonShape *characterShape = character.getCharacter();
		RectangleShape *lavaShape = lava.getLava();
		std::string reply = "Joined";
		clientID = networkManager.getClientId();
		timeline.start_realtime();
	    networkManager.sendMessage(reply);
	    networkManager.receiveMessage();
		std::vector<MovingPlatform> movingPlatformsReplay;
		std::vector<Platform> platformsReplay;
		platformsReplay.push_back(Platform(sf::Color(165,42,42), sf::Color(0,255,0), sf::Vector2f(10.f,600.f),sf::Vector2f(400.f, 100.f)));
		platformsReplay.push_back(Platform(sf::Color(165,42,42), sf::Color(0,0,255), sf::Vector2f(1800.f,500.f), sf::Vector2f(200.f, 100.f)));
		movingPlatformsReplay.push_back(MovingPlatform(sf::Color(165,42,42), sf::Color(255,0,0),sf::Vector2f(500.f, 100.f), Vector2f(480.f, 500.f), Vector2f(480.f, 500.f), Vector2f(150.f, 0.f)));
		movingPlatformsReplay.push_back(MovingPlatform(sf::Color(165,42,42), sf::Color(200,255,0),sf::Vector2f(500.f, 100.f), Vector2f(2150.f, 200.f), Vector2f(2150.f, 200.f), Vector2f(0.f, 150.f)));
		std::vector<Platform> platforms;
		platforms.push_back(Platform(sf::Color(165,42,42), sf::Color(0,255,0), sf::Vector2f(10.f,600.f),sf::Vector2f(400.f, 100.f)));
		platforms.push_back(Platform(sf::Color(165,42,42), sf::Color(0,0,255), sf::Vector2f(1800.f,500.f), sf::Vector2f(200.f, 100.f)));
		std::vector<MovingPlatform> movingPlatforms;
		movingPlatforms.push_back(MovingPlatform(sf::Color(165,42,42), sf::Color(255,0,0),sf::Vector2f(500.f, 100.f), Vector2f(480.f, 500.f), Vector2f(480.f, 500.f), Vector2f(150.f, 0.f)));
		movingPlatforms.push_back(MovingPlatform(sf::Color(165,42,42), sf::Color(200,255,0),sf::Vector2f(500.f, 100.f), Vector2f(2150.f, 200.f), Vector2f(2150.f, 200.f), Vector2f(0.f, 150.f)));
		character.initialize();
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
		sf::View gameView;
        gameView.setSize(WINDOW_WIDTH, WINDOW_HEIGHT);
        gameView.setCenter(WINDOW_WIDTH / 2, WINDOW_HEIGHT / 2);
        window.setView(gameView);
		
		std::thread t1(&thread1, std::ref(texture), std::ref(platforms), std::ref(movingPlatforms), platforms.size(), movingPlatforms.size());
		t1.join();
		std::thread t2(&thread1, std::ref(texture), std::ref(platformsReplay), std::ref(movingPlatformsReplay), platformsReplay.size(), movingPlatformsReplay.size());
		t2.join();

    	static bool pKeyPressed = false; 
    	static bool qKeyPressed = false;
		static bool rKeyPressed = false;
		bool isAPressed = false;
		sf::Clock aPressed;
		sf::Clock keyPress;
		lastKeyPressed = " ";

		while(window.isOpen())
		{
		eventManager.deathFlag = false;
		std::map<int, sf::Vector2f> characterPositions;
		
		sf::Event event;
	
		character.set_moveX(false);
		event_message = false;
		if (window.hasFocus()) {
		while(window.pollEvent(event))
		{
			if(event.type == sf::Event::Closed)
			{
					window.close();
			}
			else if (event.type == sf::Event::Resized && sf::Keyboard::isKeyPressed(sf::Keyboard::E))
    		{
				// isEPressed = true;
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
			else if (event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::R && !rKeyPressed)
			{
				rKeyPressed = true;
				if(replay == false){
					replay=true;
					
				}
				else{

					replayStopped = true;
					replay=false;
				}
			}
			else if(event.type == sf::Event::KeyReleased && event.key.code == sf::Keyboard::R){
				rKeyPressed = false;
			}
		}
		if(sf::Keyboard::isKeyPressed(sf::Keyboard::Q))
		{
			eventManager.userInputEvent("Q", &character,&timeline, clientID+1);
			event_message = true;

		}
		}
		if (!timeline.isPaused()) 
		{
        // Only update dt if the game is not paused
		timeline.updateDeltaTime(dt_clock);
        timeline.setDeltaJumpTime(timeline.getDeltaTime()/timeline.getGameSpeed());
    	}	
		gravity = 20.f;
		character.set_characterPlatformVelocity(0.f, 0.f);
		character.getBoundsPentagonShape(characterShape);
		character.set_characterVelocityY(character.get_characterVelocity().y+gravity*timeline.getDeltaJumpTime());
		character.set_moveY(true);
		for(int i = 0; i < platforms.size(); i++)
		{
			eventManager.characterCollisionPlatformEvent(&character,&platforms[i],"platform");
		}
		// Collision detection with the moving platform
		for(int i = 0; i < movingPlatforms.size(); i++)
		{
			eventManager.characterCollisionMovingPlatformEvent(&character,&movingPlatforms[i],"movingplatform");
		}
		//movement
		if (window.hasFocus()) {

		//--------------------------------------------v8stuff-------------------------------------------------------
		//--------------------------------------------chord logic---------------------------------------------------
		
				if(sf::Keyboard::isKeyPressed(sf::Keyboard::A) && aPressed.getElapsedTime() > sf::milliseconds(20) && lastKeyPressed !="A"){
                    lastKeyPressed ="A";
                    aPressed.restart();
                }
                else if(lastKeyPressed == "A" && aPressed.getElapsedTime() > sf::milliseconds(80) ){
                    lastKeyPressed ="";
                    std::cout<<"A pressed   "<<std::endl;
					v8::Isolate::Scope isolate_scope(isolate); 
        			v8::HandleScope handle_scope(isolate);
        			v8::Local<v8::ObjectTemplate> global = v8::ObjectTemplate::New(isolate);
        			global->Set(isolate, "print", v8::FunctionTemplate::New(isolate, v8helpers::Print));
        			v8::Local<v8::Context> default_context =  v8::Context::New(isolate, NULL, global);
					v8::Context::Scope default_context_scope(default_context);

        			ScriptManager *sm = new ScriptManager(isolate, default_context); 
					v8::Local<v8::Context> object_context = v8::Context::New(isolate, NULL, global);
					sm->addScript("modify_color", "scripts/modify_color.js");
					platforms[0].exposeToV8(isolate, default_context, "default_context");
					sm->runOne("modify_color", false);
					platforms[0].set_color();
					eventManager.userInputEvent("A", &character, &timeline, clientID);
                    aPressed.restart();
                }
                if(sf::Keyboard::isKeyPressed(sf::Keyboard::D) && lastKeyPressed == "A" && aPressed.getElapsedTime() < sf::milliseconds(80) ){
                    lastKeyPressed ="D";
					v8::Isolate::Scope isolate_scope(isolate); 
        			v8::HandleScope handle_scope(isolate);

        			v8::Local<v8::ObjectTemplate> global = v8::ObjectTemplate::New(isolate);
        			global->Set(isolate, "print", v8::FunctionTemplate::New(isolate, v8helpers::Print));
        			v8::Local<v8::Context> default_context =  v8::Context::New(isolate, NULL, global);
					v8::Context::Scope default_context_scope(default_context); 

        			ScriptManager *sm = new ScriptManager(isolate, default_context); 
					v8::Local<v8::Context> object_context = v8::Context::New(isolate, NULL, global);
					sm->addScript("raise_event", "scripts/raise_event.js");
					platforms[0].exposeToV8(isolate, default_context, "default_context");
					sm->runOne("raise_event", false);
					if(platforms[0].get_toggleEvent() == "death"){
					eventManager.deathEvent(&character, &lava, &window, &gameView, WINDOW_WIDTH, WINDOW_HEIGHT);
					}
					std::cout<<"chord pressed"<<std::endl;
                    aPressed.restart();
                }

                if(sf::Keyboard::isKeyPressed(sf::Keyboard::D) && aPressed.getElapsedTime() > sf::milliseconds(20) && lastKeyPressed !="D"){
                    lastKeyPressed ="D";
                    aPressed.restart();
                }
                else if(lastKeyPressed == "D" && aPressed.getElapsedTime() > sf::milliseconds(80) ){
                    lastKeyPressed ="";
                    std::cout<<"D pressed"<<std::endl;
					eventManager.userInputEvent("D", &character, &timeline, clientID);
                    aPressed.restart();
                }
                if(sf::Keyboard::isKeyPressed(sf::Keyboard::A) && lastKeyPressed == "D" && aPressed.getElapsedTime() < sf::milliseconds(80) ){
                    lastKeyPressed ="A";
					v8::Isolate::Scope isolate_scope(isolate);
        			v8::HandleScope handle_scope(isolate);
        			v8::Local<v8::ObjectTemplate> global = v8::ObjectTemplate::New(isolate);
        			global->Set(isolate, "print", v8::FunctionTemplate::New(isolate, v8helpers::Print));
        			v8::Local<v8::Context> default_context =  v8::Context::New(isolate, NULL, global);
					v8::Context::Scope default_context_scope(default_context); 

        			ScriptManager *sm = new ScriptManager(isolate, default_context); 
					v8::Local<v8::Context> object_context = v8::Context::New(isolate, NULL, global);
					sm->addScript("raise_event", "scripts/raise_event.js");
					platforms[0].exposeToV8(isolate, default_context, "default_context");
					sm->runOne("raise_event", false);
					if(platforms[0].get_toggleEvent() == "death"){
					eventManager.deathEvent(&character, &lava, &window, &gameView, WINDOW_WIDTH, WINDOW_HEIGHT);
					}
                    std::cout<<"chord pressed"<<std::endl;
                    aPressed.restart();
                }
		//--------------------------------------------v8stuff-------------------------------------------------------
		//--------------------------------------------chord logic---------------------------------------------------
		//rest same
		if (sf::Keyboard::isKeyPressed(sf::Keyboard::Space) && !character.get_isJumping())
		{	
			eventManager.userInputEvent("Space", &character, &timeline, clientID);
			lastKeyPressed = "Space";
		}
		}
		eventManager.handleEvents();
		if(eventManager.deathFlag == true){
			v8::Isolate::Scope isolate_scope(isolate); 
        			v8::HandleScope handle_scope(isolate);

        			v8::Local<v8::ObjectTemplate> global = v8::ObjectTemplate::New(isolate);
        			global->Set(isolate, "print", v8::FunctionTemplate::New(isolate, v8helpers::Print));
        			v8::Local<v8::Context> default_context =  v8::Context::New(isolate, NULL, global);
					v8::Context::Scope default_context_scope(default_context); 

        			ScriptManager *sm = new ScriptManager(isolate, default_context); 
					v8::Local<v8::Context> object_context = v8::Context::New(isolate, NULL, global);
					sm->addScript("handle_event", "scripts/handle_event.js");
					platforms[0].exposeToV8(isolate, default_context, "default_context");
					sm->runOne("handle_event", false);
					std::cout<<platforms[0].get_deathCount()<<std::endl;
		}
		//Thread 2 logic
		for(int i = 0; i < movingPlatforms.size(); i++){
			std::thread t2(&MovingPlatform::platformMove, &movingPlatforms[i], WINDOW_WIDTH);
    		t2.detach();
		}
		if(character.get_moveX())
		{
			character.set_position_pentagon(characterShape,Vector2f(character.get_position_pentagon(characterShape).x + character.get_characterVelocity().x + character.get_characterPlatformVelocity().x, character.get_position_pentagon(characterShape).y));
		}
		if(character.get_moveY())
		{
			character.set_position_pentagon(characterShape,Vector2f(character.get_position_pentagon(characterShape).x, character.get_position_pentagon(characterShape).y + character.get_characterVelocity().y));
		}
		if(character.get_position_pentagon(characterShape).x<0)
		{
			character.set_position_pentagon(characterShape,Vector2f(0.f, character.get_position_pentagon(characterShape).y));
		}	
		if(lava.isCharacterColliding(character.getBoundsPentagonShape(characterShape)))
		{
			eventManager.characterCollisionLavaEvent(&character, &lava, &window, &gameView, WINDOW_WIDTH, WINDOW_HEIGHT,"lava");
		}
		if (character.get_position_pentagon(characterShape).x < WINDOW_WIDTH/2) {
                sf::Vector2f currentCenter = gameView.getCenter();
				if(currentCenter.x!=WINDOW_WIDTH / 2){
				while(currentCenter.x==WINDOW_WIDTH / 2)
				{
                gameView.setCenter(currentCenter.x - (WINDOW_WIDTH/2)-character.get_position_pentagon(characterShape).x, currentCenter.y);
				sf::RectangleShape *lavaShape = lava.getLava();
				lava.set_position_rectangle(lavaShape,Vector2f(lava.get_position_rectangle(lavaShape).x - 1000.f-character.get_position_pentagon(characterShape).x, lava.get_position_rectangle(lavaShape).y ));
				}
			}
                window.setView(gameView);
            } 
			else if (character.get_position_pentagon(characterShape).x >= WINDOW_WIDTH/2) {
				sf::Vector2f currentCenter = gameView.getCenter();
				gameView.setCenter(character.get_position_pentagon(characterShape).x, currentCenter.y);
				sf::RectangleShape *lavaShape = lava.getLava();
				lava.set_position_rectangle(lavaShape,Vector2f(character.get_position_pentagon(characterShape).x - (WINDOW_WIDTH/2), lava.get_position_rectangle(lavaShape).y ));
                window.setView(gameView);
        }
		window.clear();
		
		if (event_message == false){
			networkManager.sendEvents(clientID, "Empty", &timeline);
		}
		
		//network code
		characterState.characterX = character.get_position_pentagon(characterShape).x;
		characterState.characterY = character.get_position_pentagon(characterShape).y;
		networkManager.sendGameData(characterState.characterX, characterState.characterY, timeline.getGameTime());
	
		
		characterPositions = networkManager.receiveGameData(std::ref(platforms),std::ref(movingPlatforms));
		window.draw(*lava.getLava());
		if(replay == true){
		
			movingPlatform1_position.push_back(movingPlatforms[0].get_position_rectangle(movingPlatforms[0].getMovingPlatform()));
			movingPlatform2_position.push_back(movingPlatforms[1].get_position_rectangle(movingPlatforms[1].getMovingPlatform()));
			int count = 0;
			for (auto i : characterPositions) {
				characterPositionsReplay[count].push_back(sf::Vector2f(i.second.x, i.second.y));
				count++;
			}
		}
		else if(replay == false && replayStopped == true){
        
			movingPlatformsReplay[0].set_position_rectangle(movingPlatformsReplay[0].getMovingPlatform(), movingPlatform1_position[0]);
			movingPlatform1_position.erase(movingPlatform1_position.begin());
			movingPlatformsReplay[1].set_position_rectangle(movingPlatformsReplay[1].getMovingPlatform(), movingPlatform2_position[0]);
			movingPlatform2_position.erase(movingPlatform2_position.begin());
			window.draw(*movingPlatformsReplay[0].getMovingPlatform());
			window.draw(*movingPlatformsReplay[1].getMovingPlatform());
			window.draw(*platformsReplay[0].getPlatform());
			window.draw(*platformsReplay[0].getPlatform());
				
			int count = 0;
			for (auto i : characterPositions) 
        	{
				PentagonShape c[30];
				c[count].setPosition(characterPositionsReplay[count][0]);
				c[count].setFillColor(sf::Color(255,0,0));
				window.draw(c[count]);
				count++;
        	}
			for(int i =0 ;i<characterPositions.size();i++){
				PentagonShape ch[30];
				ch[i].setPosition(characterPositionsReplay[i][0]);
				characterPositionsReplay[i].erase(characterPositionsReplay[i].begin());
				ch[i].setFillColor(sf::Color(255,0,0));
				window.draw(ch[i]);
			}
			replay_count++;
        	if(movingPlatform1_position.empty()){
				replayStopped=false;
				movingPlatform1_position.clear();
				movingPlatform2_position.clear();
				characterPositionsReplay.clear();
				replay_count=0;
			}
			
		}
		if (replayStopped == false){
		for (int i = 0; i < platforms.size(); i++) {
            window.draw(*platforms[i].getPlatform());
        }
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
		}
		window.display();
	}
	}

    isolate->Dispose();
    v8::V8::Dispose();
    v8::V8::ShutdownPlatform();
	//--------------------------------------------v8stuff-------------------------------------------------------
	
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