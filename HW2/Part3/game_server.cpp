#include <zmq.hpp>
#include <iostream>
#include <vector>
#include <iostream>
#include <map>
// #include <unordered_map>
#include <iomanip>
#include <string> 
#include <SFML/System.hpp>

using namespace std;

struct Iterations{
    std::map<int, int> iter;
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
// int iteration;
};


int main() {
    gameServer server;
    Iterations iteration;

    zmq::context_t context(1);

    // Socket for receiving messages from clients
    zmq::socket_t req_socket(context, ZMQ_REP);
    req_socket.bind("tcp://*:5556");

    // Socket for broadcasting game state to clients
    zmq::socket_t pub_socket(context, ZMQ_PUB);
    pub_socket.bind("tcp://*:5555");

    // std::unordered_map<std::string, int> clientIds;

    std::cout<<"connection established"<<std::endl;


    while (true) {
        // Handle incoming messages from clients
        zmq::message_t response;
        req_socket.recv(response, zmq::recv_flags::none);

        std::string responseStr(static_cast<char*>(response.data()), response.size());
        // std::cout<<responseStr<<std::endl;
        int clientId;

        if (responseStr == "Joined") {
            server.update_clientID();
            clientId = server.get_clientId();
            iteration.iter[clientId] = 0;
            
            std::string reply = std::to_string(clientId);
            std::cout<<reply<<std::endl;

            req_socket.send(zmq::const_buffer(reply.c_str(), reply.size()), zmq::send_flags::none);
        } 
        else {
            sscanf(responseStr.c_str(), "%d", &clientId);
            std::string serializedData;
            for (int i=1; i<= server.get_clientId();i++) 
            {
                serializedData = serializedData + "Client Id:" +std::to_string(i) + "," + " Iteration:" +std::to_string(iteration.iter[i]) + "\n"; 
                iteration.iter[i]=iteration.iter[i]+1;
            }
            // std::string serializedData = "Updating the coordinates";
            std::string reply = "clientId received";
            req_socket.send(zmq::const_buffer(reply.c_str(), reply.size()), zmq::send_flags::none);
            pub_socket.send(zmq::const_buffer(serializedData.c_str(), serializedData.size()), zmq::send_flags::none);
                    
        }
    }

    return 0;
}
