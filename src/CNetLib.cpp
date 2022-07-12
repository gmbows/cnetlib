#include "cnetlib.h"

#include "serializer.h"

#include <string>
#include <fstream>

namespace CN {
	unsigned int num_connections = 0;
}

//Constructors

CN::Connection::Connection(tcp::socket *new_sock): sock(new_sock) {
	this->cur_msg = new CN::Message();
	this->id = ++num_connections;
	this->active = true;
	this->reading = false;
	this->start_handler();
}

//Server

void CN::Server::listen() {
	this->active = true;
	tcp::socket *new_socket;
	asio::error_code err;
	while(this->active) {
		new_socket = new tcp::socket(this->m_io_context);
		this->acceptor->accept(*new_socket,err);
		if(this->active) {
			CNetLib::log("Connection received");
			this->register_connection(std::move(new_socket));
		}
	}
}

//Client

CN::Connection* CN::Client::connect(std::string address) {
	tcp::socket *new_sock;
	try {
		new_sock = new tcp::socket(this->m_io_context);
		tcp::resolver::results_type endpoints = this->resolver->resolve(address, std::to_string(this->port));
		asio::connect(*new_sock, endpoints);
	} catch(const std::exception &e) {
		CNetLib::log(e.what());
		delete new_sock;
		return nullptr;
	}

	print("Connection successful");
	return this->register_connection(std::move(new_sock));
}

//Connection

void CN::Connection::handle() {
	byte_t read_buf[CN_BUFFER_SIZE];
	size_t len;
	while(this->active) {
		try {
			len = this->sock->read_some(asio::buffer(read_buf,CN_BUFFER_SIZE));
//			len = asio::read(*this->sock,asio::buffer(read_buf,CN_BUFFER_SIZE));
			CNetLib::log("Connection ",this->id," got ",len," bytes");
			this->process_data(read_buf,len);
		} catch(const std::exception &e) {
			len = -1;
			CNetLib::log("Error reading from connection ",this->id,": ",e.what());
			this->disconnect();
			return;
		}
	}
}

size_t CN::Connection::send(serializer *s) {
	size_t written;
	try {
		written = asio::write(*this->sock,asio::buffer(s->data,s->true_size()));
	} catch(const std::exception &e) {
		written = -1;
		print("Error writing to socket");
		this->disconnect();
	}
	return written;
}

size_t CN::Connection::package_and_send(std::string data) {

	serializer s = serializer(CN_BUFFER_SIZE);
	s.add_int(data.size());
	s.add_str_auto(data);

	size_t written = this->send(&s);

	return written;
}

void CN::Connection::process_data(byte_t *data, size_t len) {
	this->s.set_data(data);
	if(this->reading == false) {
		int rem = s.get_int();
		this->cur_msg->init_transfer(rem);
		this->reading = true;
	}
	if(this->reading) {
		size_t len_actual = std::min(this->cur_msg->bytes_left,len);
		if(this->cur_msg->import_data(s.data_get_ptr,len_actual)) {
			CNetLib::log("Transfer complete. Got ",this->cur_msg->content.size()," bytes");
			std::string s;
			for(auto c : this->cur_msg->content) {
				s+=c;
			}
			CNetLib::log("Message received: ",s);
		}
	}
}

//NetObj

CN::Connection* CN::NetObj::register_connection(tcp::socket *new_sock) {
	//Adds a connection to this tcp socket
	CN::Connection *nc = new CN::Connection(new_sock);
	nc->owner = this;
	print("Registered new connection ",nc->id);
	this->connections.push_back(nc);
	nc->active = true;
	nc->package_and_send("Welcome!");
	return nc;
}

void CN::NetObj::remove_connection(Connection *cn) {
	if(!contains(this->connections,cn)) return;
	int t_index = 0;
	for(int i=0;i<this->connections.size();i++) {
		CN::Connection *c = this->connections.at(i);
		if((char*)c == (char*)cn) {
			t_index = i;
			break;
		}
	}
	this->connections.erase(this->connections.begin()+t_index);
}

