#include "CNetLib.h"

#include <string>

void CN_Connection::handle() {
	//Buffer for reading
    char data[TWT_BUFFER_SIZE];
    clear_buffer(data,TWT_BUFFER_SIZE);
	
    std::vector<char> vdata;
	
    asio::error_code error;
	
	print("Handling connection");
	
    while(!error) {
        size_t len = this->sock->read_some(asio::buffer(data,TWT_BUFFER_SIZE), error);

        vdata = array_to_vector(data,TWT_BUFFER_SIZE);
		
		print(this->id,": ",data," (",len," bytes)");

        clear_buffer(data,TWT_BUFFER_SIZE);
    }
	print(error.message());
	delete this;
}

std::string CN_Server::get_msg() {
	//Returns the oldest unprocessed message on the server
	std::string msg = this->connections.front()->messages.front();
	this->connections.front()->messages.pop();
	return msg;
}

void CN_Server::accept() {
	 tcp::socket *sock;

	 print("Listening");

    while(true) {
        try {
            sock = new tcp::socket(io_context);
            this->acceptor->accept(*sock);
			print("Got connection");
			//Create new thread to import messages from this connection
			CN_Connection *c = new CN_Connection(sock);
			c->id = ++this->numConnections;
			CN_Thread(&*c,&CN_Connection::handle);
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
	
	
	asio::error_code error;
	size_t written = asio::write(*sock, asio::buffer("Greetings from client",100), error);

    return;
}