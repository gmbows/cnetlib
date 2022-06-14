#include "CNetLib.h"
#include "Common.h"

#include <string>

CN_Connection::CN_Connection(tcp::socket *_sock): id(0), active(false) {
	this->messageHandler = new CN_MessageHandler();
	this->sock = _sock;
	this->address = get_address(sock);
	this->active = false;
	this->reader = CN_NetReader(0);
}

void CN_Connection::handle() {
	//Buffer for reading
    char data[TWT_BUFFER_SIZE];
    clear_buffer(data,TWT_BUFFER_SIZE);
	
    std::vector<char> vdata;
	
    asio::error_code error;
	
    while(!error) {
        size_t len = this->sock->read_some(asio::buffer(data,TWT_BUFFER_SIZE), error);

        vdata = array_to_vector(data,TWT_BUFFER_SIZE);
		
		//Potentially incomplete message or containing multiple messages
		this->handle_data(data);

        clear_buffer(data,TWT_BUFFER_SIZE);
    }
	print("Connection ",this->id," closed (",error.message(),")");
	delete this;
}

void CN_Connection::handle_data(std::string data) {
	for(auto &c : data) {
		if(this->reader.read(c)) {
			CN_Message *msg = new CN_Message(this,this->reader.data);
			msg->owner = this;
			this->handle_msg(msg);
			this->reader.reset();
		}
	}
}

void CN_Connection::handle_msg(CN_Message *msg) {
	this->messageHandler->handle(msg);
}

size_t CN_Connection::send(std::string data) {
	asio::error_code error;
	CN_Message message = CN_Message(this,data);
	size_t written = asio::write(*this->sock, asio::buffer(message.serialize(),TWT_BUFFER_SIZE), error);
	return written;
}

std::string CN_Server::get_msg() {
	//Returns the oldest unprocessed message on the server
	std::string msg = this->connections.front()->messages.front();
	this->connections.front()->messages.pop();
	return msg;
}

void CN_Server::accept() {
	 tcp::socket *sock;

	 print("Server listening on port "+std::to_string(this->port));
	 
	 this->active = true;

    while(this->active) {
        try {
            sock = new tcp::socket(io_context);
            this->acceptor->accept(*sock);
			//Create new thread to import messages from this connection
			CN_Connection *c = new CN_Connection(sock);
			c->id = ++this->numConnections;
			c->messageHandler = this->messageHandler_Default;
			c->send("Greetings from server");
			c->active = true;
			CN_Thread(&*c,&CN_Connection::handle);
			this->connections.push(c);
        } catch (const std::exception &e) {
            print(e.what());
            return;
        }
    }
}

void CN_Server::start() {
	CN_Thread(this,&CN_Server::accept);
}

size_t CN_Client::send(std::string msg) {
	return this->connection->send(msg);
}

bool CN_Client::connect(std::string ip) {
	tcp::socket *sock;

    try {
        sock = new tcp::socket(io_context);
        tcp::resolver::results_type endpoints = this->resolver->resolve(ip, std::to_string(5555));
        asio::connect(*sock, endpoints);
    } catch(const std::exception &e) {
        print(e.what());
        return false;
    }

	asio::error_code error;
	this->connection = new CN_Connection(sock);

	this->connection->id = 0;
	this->connection->messageHandler = this->messageHandler_Default;

	CN_Thread(&*this->connection,&CN_Connection::handle);
	
	//Clients only have one connection
	this->send("Greetings from client");
			
    return true;
}