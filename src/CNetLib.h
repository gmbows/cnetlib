#pragma once

#include "cnet_utility.h"
#include <asio.hpp>
#include <memory>
#include <future>
#include <mutex>
#include <condition_variable>
#include <pthread.h>
#include <functional>
#include <queue>
#include <map>
#include <string>
#include <serializer.h>

#define CN_BUFFER_SIZE 65536

typedef unsigned char byte_t;

using asio::ip::tcp;

namespace CN {

enum class DataType {
	TEXT,
};

extern unsigned int num_connections;

struct NetObj;

struct Message {

	CN::DataType type;

	bool complete;

	std::vector<byte_t> content;
	CN::NetObj *owner;

	size_t size;
	size_t bytes_left;

	bool import_data(byte_t *data,size_t len) {
		CNetLib::log("Importing ",len," bytes");
		memcpy(this->content.data()+this->size,data,len);
		this->size += len;
		this->bytes_left -= len;
		if(this->bytes_left <= 0) {
			this->complete = true;
		}
		return this->complete;
	}

	void init_transfer(size_t len) {
		print("Initializing transfer of ",len," bytes");
		this->content.clear();
		this->bytes_left = len;
		this->content.resize(this->size+len);
	}

	Message() {
		this->size = 0;
		this->complete = false;
	}

	Message(size_t _bytes): bytes_left(_bytes) {
		this->complete = false;
		this->size = 0;
	}
};

struct Connection {

	int id;

	bool validated; //Has this connection sent a valid packet yet
	bool active;	//Is this connection active
	bool reading;	//Is this connection currently reading a message body

	CN::Message *cur_msg;
	serializer s = serializer(CN_BUFFER_SIZE);

	NetObj* owner;
	tcp::socket *sock;

	size_t package_and_send(std::string);
	size_t send(serializer*);

	void process_data(byte_t*,size_t);
	void handle();
	void start_handler() {
		std::thread(&CN::Connection::handle,this).detach();
	}
	void close() {
		try {
			this->sock->close();
		} catch(const std::exception &e) {

		}
	}
	void disconnect() {
		this->active = false;
		this->close();
	}

	Connection(tcp::socket *new_sock);
};



struct NetObj {

	//Asio specific
	asio::io_context m_io_context;

	//Basic info
	short port;
	bool active;

	//Connections
	std::vector<CN::Connection*> connections;
	CN::Connection* register_connection(tcp::socket*);
	void remove_connection(CN::Connection*);

	inline void start_io() {
		this->active = true;
		this->m_io_context.run();
	}
	inline void stop_io() {
		this->active = false;
		this->m_io_context.stop();
	}

	NetObj(short _port): port(_port) {
		this->active = false;
	}

	~NetObj() {
		CNetLib::log("Destroying net object");
		this->active = false;
		this->stop_io();
	}
};

struct Client : public NetObj {
	tcp::resolver *resolver;
	CN::Connection *connect(std::string);

	Client(short _port): NetObj(_port) {
		this->resolver = new tcp::resolver(this->m_io_context);
	}

	~Client() {
		delete resolver;
		CNetLib::log("Destroying client");
	}
};

struct Server : public NetObj {

	tcp::acceptor *acceptor;

	void start_listener() {
		std::thread(&CN::Server::listen,this).detach();
		this->start_io();
	}

	void listen();

	Server(short _port): NetObj(_port) {
		this->acceptor = new tcp::acceptor(this->m_io_context,tcp::endpoint(tcp::v4(), _port));
	}

	~Server() {
		CNetLib::log("Closing server");
	}
};

}
