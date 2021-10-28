#include "CNetLib.h"

#include <string>

void CN_Server::accept() {
	 tcp::socket *sock;
	 
	 print("Listening");

    while(true) {
		print("looping");
        try {
            sock = new tcp::socket(io_context);
            this->acceptor->accept(*sock);
			print("Got connection");
        } catch (const std::exception &e) {
            print(e.what());
            return;
        }
    }
}

void CN_Client::connect(std::string ip) {
	tcp::socket *sock;

    try {
        sock = new tcp::socket(io_context);
        tcp::resolver::results_type endpoints = this->resolver->resolve(ip, std::to_string(5555));
        asio::connect(*sock, endpoints);
    } catch(const std::exception &e) {
        print(e.what());
        return;
    }

    return;
}