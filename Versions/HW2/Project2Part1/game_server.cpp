#include <zmq.hpp>
#include <iostream>
#include <vector>
#include <map>
#include <string> 
#include <SFML/System.hpp>

using namespace sf;

struct CharacterState {
    float characterX;
    float characterY; 
};

struct GameState{
	std::map<int, Vector2f> characterPositions;
	float movingPlatformPositionX;
    float movingPlatformPositionY;
};

class gameServer{
public: 
    gameServer(): clientID(0){}

    int get_clientId(){
        return clientID;
    }
    void update_clientID(){
        clientID=clientID+1;
    }
protected:
int clientID;
};


int main() {
    CharacterState characterState;
    GameState gameState;
    gameServer server;

    zmq::context_t context(1);

    // Socket for receiving messages from clients
    zmq::socket_t req_socket(context, ZMQ_REP);
    req_socket.bind("tcp://*:5556");

    // Socket for broadcasting game state to clients
    zmq::socket_t pub_socket(context, ZMQ_PUB);
    pub_socket.bind("tcp://*:5555");

    std::cout<<"Waiting for clients.."<<std::endl;


    while (true) {
        // Handle incoming messages from clients
        zmq::message_t response;
        req_socket.recv(response, zmq::recv_flags::none);

        std::string responseStr(static_cast<char*>(response.data()), response.size());
        std::cout<<responseStr<<std::endl;
        int clientId;
        // When client sends join request
        if (responseStr == "Joined") {
            server.update_clientID();
            clientId = server.get_clientId();
            
            std::string reply = std::to_string(clientId); // sends unique id to that client which client then stores and sends it everytime to the server
            std::cout<<reply<<std::endl;

            req_socket.send(zmq::const_buffer(reply.c_str(), reply.size()), zmq::send_flags::none);
        } 
        else {
            sscanf(responseStr.c_str(), "%d,%f,%f", &clientId,
                                      &characterState.characterX,
                                      &characterState.characterY);
            gameState.characterPositions[clientId] = Vector2f(characterState.characterX, characterState.characterY);
            std::string serializedData="";
            for (auto i : gameState.characterPositions) 
            {
                serializedData = serializedData + std::to_string(i.first) + "," + std::to_string(i.second.x) + "," + std::to_string(i.second.y) + ","; // converting struct fields ti string
            }
            
            std::string reply = "Coordinates received";
            req_socket.send(zmq::const_buffer(reply.c_str(), reply.size()), zmq::send_flags::none);
            pub_socket.send(zmq::const_buffer(serializedData.c_str(), serializedData.size()), zmq::send_flags::none);
                    
        }
    }

    return 0;
}
