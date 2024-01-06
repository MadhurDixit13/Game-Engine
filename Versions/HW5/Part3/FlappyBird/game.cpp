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

//------------------custom shape class inheriting from sfml's default shape class---------------------------------
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

//---------------------------------GameObject Class---------------------------------------------------------
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
    void move(RectangleShape *object,Vector2f offset){
        object->move(offset);
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

//------------------------------------------------Platform which I used as pipes------------------------------------------------------
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
    //---------------------------------------------------------v8 Stuff---------------------------------------------------------------------------------
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
    //---------------------------------------------------------v8 Stuff---------------------------------------------------------------------------------
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

//------------------------------------------------Timeline Class----------------------------------------------------
class Timeline {
public:
    Timeline() : gameSpeed(1.f), paused(false), pauseTime(0.f) {
	}

	void start_realtime(){
		realTimeClock.restart().asSeconds();
	}
    void start_deltaTime(){
        deltaClock.restart().asSeconds();
	}
    void togglePause(sf::Clock& dt_clock) {
        if (paused){
			paused = false;
			dt_clock.restart();
			float currentPauseTime = pauseClock.getElapsedTime().asSeconds();
            pauseTime += currentPauseTime; //adding the current pause time to the total pause time.
            pauseClock.restart();
        } else {
			paused = true;
			dt=0;
			dt_jump=0;
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
    sf::Clock deltaClock;
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
		// mutex.lock();
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
		// mutex.unlock();
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

//------------------------------------------------Side Boundary which i use to increment score---------------------------------------------------------------------
class SideBoundary : public GameObject{
public:
    SideBoundary(Vector2f pos) : sideBoundary(Vector2f(1.f,600.f)), collided(false) {
        sideBoundary.setFillColor(sf::Color::Transparent);
		sideBoundary.setOutlineThickness(5);
		sideBoundary.setOutlineColor(sf::Color::Transparent);
		sideBoundary.setPosition(pos);
    }

    sf::RectangleShape* getSideBoundary(){
		return &sideBoundary;
	}

    bool getFlag(){
        return collided;
    }

    void toggleFlag(){
        collided = true;
    }

private:
    sf::RectangleShape sideBoundary;	
    bool collided;
};

//--------------------------------------------Character Class- flappy bird in this case(also exposed to v8)---------------------------------------------------------
class Character : public PentagonShape, public Spawner, public GameObject{
public:
    Character() : position(200.0f, 300.0f), velocity(0.0f,0.0f), gravity(1000.0f), size(sf::Vector2f(30.0f, 30.0f)), startX(0), startY(0), Spawner(200.0f, 300.0f), jumpSpeed(1000.f), moveX(false), moveY(false), isJumping(false), movementSpeed(150.f), character(20), deathCount(0) {
    }

    //---------------------------------------------------------v8 Stuff---------------------------------------------------------------------------------
    static void getCharacterColor(v8::Local<v8::String> property, const v8::PropertyCallbackInfo<v8::Value>& info)
	{
    	v8::Local<v8::Object> self = info.Holder();
    	v8::Local<v8::External> wrap = v8::Local<v8::External>::Cast(self->GetInternalField(0));
    	void* ptr = wrap->Value();
    	int colorValue = static_cast<Character*>(ptr)->color;
    	info.GetReturnValue().Set(v8::Integer::NewFromUnsigned(info.GetIsolate(), static_cast<uint32_t>(colorValue)));
	}
	static void setCharacterColor(v8::Local<v8::String> property, v8::Local<v8::Value> value, const v8::PropertyCallbackInfo<void>& info)
	{
		v8::Local<v8::Object> self = info.Holder();
		v8::Local<v8::External> wrap = v8::Local<v8::External>::Cast(self->GetInternalField(0));
		void* ptr = wrap->Value();
		static_cast<Character*>(ptr)->color = value->Int32Value();
		
	}
	static void getToggleEvent(v8::Local<v8::String> property, const v8::PropertyCallbackInfo<v8::Value>& info)
	{
		v8::Local<v8::Object> self = info.Holder();
		v8::Local<v8::External> wrap = v8::Local<v8::External>::Cast(self->GetInternalField(0));
		void* ptr = wrap->Value();
		std::string toggleEventValue = static_cast<Character*>(ptr)->toggleEvent;
		v8::Local<v8::String> v8_toggleEvent = v8::String::NewFromUtf8(info.GetIsolate(), toggleEventValue.c_str(), v8::String::kNormalString);
		info.GetReturnValue().Set(v8_toggleEvent);
	}

	static void setToggleEvent(v8::Local<v8::String> property, v8::Local<v8::Value> value, const v8::PropertyCallbackInfo<void>& info)
	{
		v8::Local<v8::Object> self = info.Holder();
		v8::Local<v8::External> wrap = v8::Local<v8::External>::Cast(self->GetInternalField(0));
		void* ptr = wrap->Value();
		v8::String::Utf8Value utf8_str(info.GetIsolate(), value->ToString());
		static_cast<Character*>(ptr)->toggleEvent = *utf8_str;
	}
	static void getHandleEvent(v8::Local<v8::String> property, const v8::PropertyCallbackInfo<v8::Value>& info)
	{
    	v8::Local<v8::Object> self = info.Holder();
    	v8::Local<v8::External> wrap = v8::Local<v8::External>::Cast(self->GetInternalField(0));
    	void* ptr = wrap->Value();
    	int deathCountValue = static_cast<Character*>(ptr)->deathCount;
    	info.GetReturnValue().Set(v8::Integer::NewFromUnsigned(info.GetIsolate(), static_cast<uint32_t>(deathCountValue)));
	}
	static void setHandleEvent(v8::Local<v8::String> property, v8::Local<v8::Value> value, const v8::PropertyCallbackInfo<void>& info)
	{
		v8::Local<v8::Object> self = info.Holder();
		v8::Local<v8::External> wrap = v8::Local<v8::External>::Cast(self->GetInternalField(0));
		void* ptr = wrap->Value();
		static_cast<Character*>(ptr)->deathCount = value->Int32Value();
		
	}
	std::string get_toggleEvent(){
		return toggleEvent;
	}

	int get_deathCount(){
		return deathCount;
	}
    void set_color(){
		sf::Color newColor(color);
		character.setFillColor(newColor);
	}
    v8::Local<v8::Object> exposeToV8(v8::Isolate *isolate, v8::Local<v8::Context> &context, std::string context_name)
	{
	    std::vector<v8helpers::ParamContainer<v8::AccessorGetterCallback, v8::AccessorSetterCallback>> v;
	    v.push_back(v8helpers::ParamContainer("color", getCharacterColor, setCharacterColor));
	    v.push_back(v8helpers::ParamContainer("toggleEvent", getToggleEvent, setToggleEvent));
	    v.push_back(v8helpers::ParamContainer("deathCount", getHandleEvent, setHandleEvent));
	    return v8helpers::exposeToV8("character", this, v, isolate, context, context_name);
	}
    //---------------------------------------------------------v8 Stuff---------------------------------------------------------------------------------
    void jump() {
        velocity.y = -400.0f;
    }

    PentagonShape* getCharacter(){
		return &character;
	}

    void update(float dt) {
        velocity.y += gravity * dt;
        position.y += velocity.y * dt;
        character.setPosition(position);
    }

    void character_draw(sf::RenderWindow& window) {
        window.draw(character);
    }

    void character_respawn(){
		this->respawnCharacter(character, Vector2f(200.0f, 300.0f));
	}

    void initialize(){
		// character.setSize(20);
        character.setPosition(position);
        character.setFillColor(sf::Color::Red);
	}
	void set_characterPlatformVelocity(float x, float y){
	characterPlatformVelocity.x = x;
	characterPlatformVelocity.y = y;
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
	
private:
    sf::Vector2f position;
    float gravity;
    sf::Vector2f size;
    PentagonShape character;
    float startX;
    float startY;
    float jumpSpeed;
	bool moveX;
    bool moveY;
    bool isJumping;
	float movementSpeed;
	Vector2f velocity;
	Vector2f characterPlatformVelocity; 
    static unsigned int color;
	std::string toggleEvent;
	int deathCount;
};
unsigned int Character::color;

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
		MovingPlatform *movingPlatform;
		Lava *lava;
		Vector2f window_size;
		Character *bird;
		Timeline *time;
		Platform *platform;
        SideBoundary *sideBoundary;
        std::vector<MovingPlatform> *movingPlatforms;
		std::map<int, sf::Vector2f> *characterPositions;
		std::vector<Platform> *platforms;
        int *score;
		sf::RenderWindow *window;
        float timestamp;  // To store the time when the event occurred.
		std::string userInputType;
        sf::View *gameView;
		std::string name;
		bool operator<(const GameEvent& other) const {
            return timestamp > other.timestamp;  // Use > to make the priority queue behave like a min-heap.
        }
    };
    EventManager() {
    }
	void handleCharacterCollision(GameEvent& event) {
		std::string userInputType = event.userInputType;
		PentagonShape *characterShape = event.bird->getCharacter();
		if(userInputType=="platform"){
			sf::RectangleShape* platformShape = event.platform->getPlatform();
			if (event.bird->getBoundsPentagonShape(characterShape).intersects(event.platform->getBoundsRectangleShape(platformShape))) {
                characterDeathEvent(event);
			}
		}
        else if(userInputType=="movingplatform"){
			sf::RectangleShape* movingPlatformShape = event.movingPlatform->getMovingPlatform();
			if (event.bird->getBoundsPentagonShape(characterShape).intersects(event.movingPlatform->getBoundsRectangleShape(movingPlatformShape))) {
    		// Collision detected with the moving platform
    		event.bird->set_characterVelocityY(0.f); // Stop falling due to gravity
    		event.bird->set_isJumping(false);
    		event.bird->set_moveX(true);
    		event.bird->set_position_pentagon(characterShape,Vector2f(event.bird->get_position_pentagon(characterShape).x, event.movingPlatform->getBoundsRectangleShape(movingPlatformShape).top - event.bird->getBoundsPentagonShape(characterShape).height - 1.f));
    		event.bird->set_characterPlatformVelocity(0.f, 0.f);
			}
		}
        else if(userInputType=="sideBoundary"){
            sf::RectangleShape* sideBoundaryShape = event.sideBoundary->getSideBoundary();
			if (event.bird->getBoundsPentagonShape(characterShape).intersects(event.sideBoundary->getBoundsRectangleShape(sideBoundaryShape))) {
                if(event.sideBoundary->getFlag() == false){
                    *event.score+=1;
                    event.sideBoundary->toggleFlag();
					deathFlag = true;
                }   
            }
        }
        else if(userInputType == "lava"){
			characterDeathEvent(event);
		}
	}
	void handleCharacterDeath(GameEvent& event){
            characterSpawnEvent(event);
	}
    void handleCharacterRespawn(const GameEvent& event){
            std::cout << "Game Over! Respawned ! Window Closed" << std::endl;
			event.bird->character_respawn();
            event.window->close();

	}
	void handleUserInputs(GameEvent& event){
    	std::string userInputType = event.userInputType;
        if (userInputType == "A") {
			event.bird->set_characterVelocityX(-event.bird->get_movementSpeed()*event.time->getDeltaTime());
			event.bird->set_moveX(true);
    	} else if (userInputType == "D") {
			event.bird->set_characterVelocityX(event.bird->get_movementSpeed()*event.time->getDeltaTime());
    	    event.bird->set_moveX(true);
    	} else if (userInputType == "Space") {
    		event.bird->jump();
        }
	}
    void characterCollisionPlatformEvent(Character *object, Platform *platform, std::string input, sf::RenderWindow *window) {
        GameEvent event;
        event.type = GameEvent::Type::CharacterCollision;
        event.timestamp = timeline.getGameTime();
		event.platform=platform;
		event.bird = object;
        event.window = window;
		event.userInputType = input;
        eventQueue.push(event);
    }
    void characterCollisionSideBoundaryEvent(Character *object, SideBoundary *sideBoundary, int* score, std::string input){
        GameEvent event;
        event.type = GameEvent::Type::CharacterCollision;
        event.timestamp = timeline.getGameTime();
        event.bird = object;
        event.sideBoundary = sideBoundary;
        event.userInputType = input;
        event.score = score;
        eventQueue.push(event);
    }
    void characterDeathEvent(GameEvent& event_collide) {
        GameEvent event = event_collide;
		event.type = GameEvent::Type::CharacterDeath;
        eventQueue.push(event);
    }
    void deathEvent(Character *object, std::string input, sf::RenderWindow *window){
		GameEvent event;
		event.type = GameEvent::Type::CharacterDeath;
		event.timestamp = timeline.getGameTime();
		event.bird = object;
        event.window = window;
		event.userInputType = input;
        eventQueue.push(event);
	}
    void characterSpawnEvent(GameEvent& event_death) {
        GameEvent event = event_death;
        event.type = GameEvent::Type::CharacterSpawn;
        eventQueue.push(event);
    }
    void userInputEvent(std::string input, Character *object) {
        GameEvent event;
        event.type = GameEvent::Type::UserInput;
        event.timestamp = timeline.getGameTime();
		event.userInputType = input;
		event.bird = object;
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
            } else if (event.type == GameEvent::Type::UserInput) {
                handleUserInputs(event);
            } else if (event.type == GameEvent::Type::CharacterSpawn) {
                handleCharacterRespawn(event);
            }
            eventQueue.pop();
        } else {
            break;
        }
    }
	}

    std::priority_queue<GameEvent> eventQueue; 
	std::vector<Platform> platforms;
	std::vector<Vector2f> upperPipes;
	std::vector<Vector2f> lowerPipes;
	PentagonShape c[30];
	std::vector<GameEvent> replayVector;
    Timeline timeline;
	bool deathFlag;
};

int main() {
    //--------------------------------------------v8stuff-------------------------------------------------------
    std::unique_ptr<v8::Platform> platform = v8::platform::NewDefaultPlatform();
    v8::V8::InitializePlatform(platform.release());
    v8::V8::InitializeICU();
    v8::V8::Initialize();
    v8::Isolate::CreateParams create_params;
    create_params.array_buffer_allocator = v8::ArrayBuffer::Allocator::NewDefaultAllocator();
    v8::Isolate *isolate = v8::Isolate::New(create_params);{
    
    sf::RenderWindow window(sf::VideoMode(800, 600), "Flappy Bird");
    Character bird;
    float pipeSpawnTimer = 0.0f;
    float pipeSpawnInterval = 2.0f;
    Timeline timeline;
    EventManager eventManager;
    int score;
        bird.initialize();
        std::vector<Platform> upperPipes;
        std::vector<Platform> lowerPipes;
        std::vector<SideBoundary> sideBoundaries;
        sf::Clock clock;
        sf::Clock pipeSpawnClock;
        srand(time(NULL));
        Clock dt_clock; //global time
        
        while (window.isOpen()) {
		   eventManager.deathFlag=false;
           //-----------------handle events-------------------------------------------
           sf::Event event;
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
				bird.exposeToV8(isolate, default_context, "default_context");
				sm->runOne("handle_event", false);
				std::cout<<"Score:"<<bird.get_deathCount()<<std::endl;
		   }
           while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed) {
                window.close();
            }

            if (event.type == sf::Event::KeyPressed) {
                if (event.key.code == sf::Keyboard::Space) {
                    eventManager.userInputEvent("Space",&bird);
                    v8::Isolate::Scope isolate_scope(isolate); 
        			v8::HandleScope handle_scope(isolate);
        			v8::Local<v8::ObjectTemplate> global = v8::ObjectTemplate::New(isolate);
        			global->Set(isolate, "print", v8::FunctionTemplate::New(isolate, v8helpers::Print));
        			v8::Local<v8::Context> default_context =  v8::Context::New(isolate, NULL, global);
					v8::Context::Scope default_context_scope(default_context); 

        			ScriptManager *sm = new ScriptManager(isolate, default_context); 
					v8::Local<v8::Context> object_context = v8::Context::New(isolate, NULL, global);
					sm->addScript("modify_color", "scripts/modify_color.js");
					bird.exposeToV8(isolate, default_context, "default_context");
					sm->runOne("modify_color", false);
					bird.set_color();

                }
            }
            }
           //-----------------handle events ends--------------------------------------
           timeline.updateDeltaTime(dt_clock);
           timeline.start_realtime();

           float dt = clock.restart().asSeconds();
           bird.update(dt);
           //--------------------update pipes------------------------------------------
            float diff = 140.f;
            float platform1size;
            float platform2size;

            
            // Update pipe positions
            for (auto& pipe : upperPipes) {
                pipe.move(pipe.getPlatform(),Vector2f(-150.0f * timeline.getDeltaTime(), 0.0f));
            }
            for (auto& pipe : lowerPipes) {
                pipe.move(pipe.getPlatform(),Vector2f(-150.0f * timeline.getDeltaTime(), 0.0f));
            }
            for (auto& sideBoundary : sideBoundaries) {
                sideBoundary.move(sideBoundary.getSideBoundary(),Vector2f(-150.0f * timeline.getDeltaTime(), 0.0f));
            }

            // Spawn new pipes
            pipeSpawnTimer += timeline.getDeltaTime();
            if (pipeSpawnTimer > pipeSpawnInterval) {
                pipeSpawnTimer = 0.0f;
                platform1size = (float)rand() / RAND_MAX * (600.f - diff);
                upperPipes.push_back(Platform(sf::Color(165,42,42), sf::Color(0,255,0), sf::Vector2f(800.f,0.f),sf::Vector2f(30.f, platform1size)));
                platform2size = 600 - platform1size - diff;
                lowerPipes.push_back(Platform(sf::Color(165,42,42), sf::Color(0,255,0), sf::Vector2f(800.f,600.f - platform2size),sf::Vector2f(30.f, platform2size)));
                sideBoundaries.push_back(SideBoundary(sf::Vector2f(855.f,0.f)));
            }

            // Remove off-screen pipes
            upperPipes.erase(std::remove_if(upperPipes.begin(), upperPipes.end(),
                                   [&](Platform& pipe) { return pipe.get_position_rectangle(pipe.getPlatform()).x + pipe.getPlatform()->getSize().x < 0; }),
                                    upperPipes.end());
            lowerPipes.erase(std::remove_if(lowerPipes.begin(), lowerPipes.end(),
                                   [&](Platform& pipe) { return pipe.get_position_rectangle(pipe.getPlatform()).x + pipe.getPlatform()->getSize().x < 0; }),
                                    lowerPipes.end());
            sideBoundaries.erase(std::remove_if(sideBoundaries.begin(), sideBoundaries.end(),
                                   [&](SideBoundary& sideBoundary) { return sideBoundary.get_position_rectangle(sideBoundary.getSideBoundary()).x + sideBoundary.getSideBoundary()->getSize().x < 0; }),
                                    sideBoundaries.end());
           //--------------------update pipes ends-------------------------------------


           //--------------------check collision----------------------------------------
           for (auto& pipe : upperPipes) {
                eventManager.characterCollisionPlatformEvent(&bird,&pipe,"platform",&window);
            }
            for (auto& pipe : lowerPipes) {
                eventManager.characterCollisionPlatformEvent(&bird,&pipe,"platform",&window);
            }
            for (auto& sideBoundary : sideBoundaries) {
                eventManager.characterCollisionSideBoundaryEvent(&bird, &sideBoundary, &score, "sideBoundary");
            }
            if (bird.get_position_pentagon(bird.getCharacter()).y < 0 || bird.get_position_pentagon(bird.getCharacter()).y > window.getSize().y) {
                	v8::Isolate::Scope isolate_scope(isolate); 
        			v8::HandleScope handle_scope(isolate);
        			v8::Local<v8::ObjectTemplate> global = v8::ObjectTemplate::New(isolate);
        			global->Set(isolate, "print", v8::FunctionTemplate::New(isolate, v8helpers::Print));
        			v8::Local<v8::Context> default_context =  v8::Context::New(isolate, NULL, global);
					v8::Context::Scope default_context_scope(default_context); 

        			ScriptManager *sm = new ScriptManager(isolate, default_context); 
					v8::Local<v8::Context> object_context = v8::Context::New(isolate, NULL, global);
					sm->addScript("raise_event", "scripts/raise_event.js");
					bird.exposeToV8(isolate, default_context, "default_context");
					sm->runOne("raise_event", false);
					if(bird.get_toggleEvent() == "death"){
						eventManager.deathEvent(&bird,"platform",&window);
					}
            }
            //--------------------check collision ends-----------------------------------

            window.clear();
            
            bird.character_draw(window);
            // drawPipes(window, upperPipes, lowerPipes, sideBoundaries);
            //--------------------draw pipes--------------------------
            for (auto& pipe : upperPipes) {
                window.draw(*pipe.getPlatform());
            }
            for (auto& pipe : lowerPipes) {
                window.draw(*pipe.getPlatform());
            }
            for (auto& sideBoundary : sideBoundaries) {
                window.draw(*sideBoundary.getSideBoundary());
            }
            //-------------------draw pipes ends--------------------------

            window.display();
        }
        }
    isolate->Dispose();
    v8::V8::Dispose();
    v8::V8::ShutdownPlatform();
    
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


// #include <SFML/Graphics.hpp>
// #include <SFML/Window.hpp>
// #include <SFML/System.hpp>
// #include <SFML/System/Mutex.hpp>
// #include <thread>
// #include <iostream>
// #include <map>
// #include <cmath>
// #include <queue>

// using namespace sf;

// // Global mutex for shared resource management
// sf::Mutex mutex;

// class Snake {
// public:
//   Snake() : direction(sf::Vector2f(0.f, 0.f)) {
//     // Initialize the snake with a starting position and length
//     body.push_back(sf::RectangleShape(sf::Vector2f(10.f, 10.f)));
//     body[0].setPosition(sf::Vector2f(50.f, 50.f));
//     length = 1;
//   }

//   void move(float speed) {
//     // Update snake head position based on direction and speed
//     sf::Vector2f newHeadPos = body[0].getPosition() + direction * speed;

//     // Add new head segment at the front
//     body.insert(body.begin(), sf::RectangleShape(sf::Vector2f(10.f, 10.f)));
//     body[0].setPosition(newHeadPos);

//     // Remove tail segment if length exceeds current value
//     if (body.size() > length) {
//       body.pop_back();
//     }
//   }

//   void changeDirection(sf::Vector2f newDirection) {
//     // Check if new direction is valid
//     if (abs(newDirection.x - direction.x) <= 1.f &&
//         abs(newDirection.y - direction.y) <= 1.f) {
//       direction = newDirection;
//     }
//   }

//   bool isColliding(const sf::Shape &obstacle) const {
//     // Check for collision between snake head and obstacle
//     return body[0].getGlobalBounds().intersects(obstacle.getGlobalBounds());
//   }

//   sf::Vector2f getHeadPosition() const {
//     return body[0].getPosition();
//   }

//   void grow() {
//     length++;
//   }

//   void draw(sf::RenderWindow &window) const {
//     for (const auto& segment : body) {
//       window.draw(segment);
//     }
//   }

// private:
//   std::vector<sf::RectangleShape> body;
//   sf::Vector2f direction;
//   int length;
// };

// class Food {
// public:
//   Food() {
//     // Initialize food with random position within game boundaries
//     shape.setRadius(5.f);
//     shape.setFillColor(sf::Color::Green);
//     spawnRandomPosition();
//   }

//   void spawnRandomPosition() {
//     // Generate random position within game boundaries
//     float x = (float) (rand() % (window_width - 10)) + 5;
//     float y = (float) (rand() % (window_height - 10)) + 5;
//     shape.setPosition(sf::Vector2f(x, y));
//   }

//   bool isConsumed(const Snake &snake) const {
//     // Check if snake head intersects with food
//     return snake.isColliding(shape);
//   }

//   void draw(sf::RenderWindow &window) const {
//     window.draw(shape);
//   }

// private:
//   static const int window_width = 800;
//   static const int window_height = 600;
//   sf::CircleShape shape;
// };

// int main() {
//   sf::RenderWindow window(sf::VideoMode(800, 600), "Snake Game");
//   sf::Clock clock;
//   float speed = 100.f;
//   float dt = 0.f;

//   // Initialize game objects
//   Snake snake;
//   Food food;

//   while (window.isOpen()) {
//     sf::Event event;
//     while (window.pollEvent(event)) {
//       if (event.type == sf::Event::Closed) {
//         window.close();
//       } else if (event.type == sf::Event::KeyPressed) {
//         switch (event.key.code) {
//           case sf::Keyboard::Up:
//             snake.changeDirection(sf::Vector2f(0.
// #include <SFML/Graphics.hpp>
// #include <SFML/System.hpp>
// #include <queue>
// #include <vector>

// // Include your existing game engine classes here

// class Food : public sf::CircleShape, public GameObject {
// public:
//     Food(float radius, sf::Color color, sf::Vector2f position)
//         : CircleShape(radius), GameObject() {
//         setFillColor(color);
//         setPosition(position);
//     }
// };

// class Snake : public sf::RectangleShape, public GameObject {
// public:
//     Snake(float size, sf::Color color, sf::Vector2f position)
//         : RectangleShape(sf::Vector2f(size, size)), GameObject() {
//         setFillColor(color);
//         setPosition(position);
//     }

//     void move(sf::Vector2f offset) {
//         RectangleShape::move(offset);
//     }
// };

// class SnakeGame {
// public:
//     SnakeGame()
//         : window(sf::VideoMode(800, 600), "Snake Game"),
//           snake(20, sf::Color::Green, sf::Vector2f(100, 100)),
//           food(10, sf::Color::Red, sf::Vector2f(400, 300)) {
//         window.setFramerateLimit(10); // Adjust the frame rate limit as needed
//     }

//     void run() {
//         while (window.isOpen()) {
//             handleEvents();
//             update();
//             render();
//         }
//     }

// private:
//     sf::RenderWindow window;
//     Snake snake;
//     Food food;

//     void handleEvents() {
//         sf::Event event;
//         while (window.pollEvent(event)) {
//             if (event.type == sf::Event::Closed)
//                 window.close();
//             else if (event.type == sf::Event::KeyPressed)
//                 handleKeyPress(event.key.code);
//         }
//     }

//     void handleKeyPress(sf::Keyboard::Key key) {
//         switch (key) {
//         case sf::Keyboard::Left:
//             snake.move(sf::Vector2f(-20, 0));
//             break;
//         case sf::Keyboard::Right:
//             snake.move(sf::Vector2f(20, 0));
//             break;
//         case sf::Keyboard::Up:
//             snake.move(sf::Vector2f(0, -20));
//             break;
//         case sf::Keyboard::Down:
//             snake.move(sf::Vector2f(0, 20));
//             break;
//         default:
//             break;
//         }
//     }

//     void update() {
//         // Check for collisions, update game logic, etc.
//     }

//     void render() {
//         window.clear();
//         window.draw(snake);
//         window.draw(food);
//         window.display();
//     }
// };

// int main() {
//     SnakeGame game;
//     game.run();

//     return 0;
// }
