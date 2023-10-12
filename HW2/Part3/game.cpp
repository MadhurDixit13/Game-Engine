#include <SFML/Graphics.hpp>
#include <SFML/Window.hpp>
#include <SFML/System.hpp>
//standard sfml headers

#include <SFML/System/Mutex.hpp>
//using for shared resource management

#include <thread>
// c++ thread class header

#include <iostream>
// #include <bits/stdc++.h>
#include <iomanip>
// standard c++ header

#include <zmq.hpp>

using namespace std;

int main()
{


	//network code
	int clientID = 0;
	// CharacterState characterState;
	zmq::context_t context(1);
    zmq::socket_t req_socket(context, ZMQ_REQ);
    req_socket.connect("tcp://localhost:5556");

    zmq::socket_t sub_socket(context, ZMQ_SUB);
    sub_socket.connect("tcp://localhost:5555");
    sub_socket.setsockopt(ZMQ_SUBSCRIBE, "", 0);
	std::string reply = "Joined";
    req_socket.send(zmq::const_buffer(reply.c_str(), reply.size()), zmq::send_flags::none);

	zmq::message_t response;
	req_socket.recv(response, zmq::recv_flags::none);
	std::string responseStr(static_cast<char*>(response.data()), response.size());
	// std::cout<<responseStr<<std::endl;
	clientID = std::stoi(responseStr);

	
	while(true)
	{
		std::string id = to_string(clientID);
		req_socket.send(zmq::const_buffer(id.c_str(), id.size()), zmq::send_flags::none);
		zmq::message_t response2;
		req_socket.recv(response2, zmq::recv_flags::none);
		zmq::message_t response3;
		sub_socket.recv(response3, zmq::recv_flags::none);
		std::string response3Str(static_cast<char*>(response3.data()), response3.size());
		std::cout<<response3Str<<std::endl;
	}
	return 0;
}







