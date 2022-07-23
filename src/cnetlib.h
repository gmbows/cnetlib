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
#include "serializer.h"

#define CN_BUFFER_SIZE 65536
#define CN_PROTOCOL_VERS 1
#define CN_HEADER_SIZE 8

#define VC_QUERY_LEN 16
#define VC_RESP_LEN 8

typedef unsigned char byte_t;

using asio::ip::tcp;

namespace CN {

struct Message;
struct Connection;
typedef std::function<void(Message*)> MessageHandler;
typedef std::function<void(Connection*)> ConnectionHandler;

enum class DataType: int {
	TEXT,
	FILE,

	CLOSE,

	VC_QUERY,
	VC_RESP,

	STREAM_INIT,
	STREAM_DATA,
	STREAM_FIN,
};

void init();

extern unsigned int num_connections;

struct NetObj;

struct Message {

	int type;

	bool complete;
	std::string f_path;

	std::vector<byte_t> content;
	CN::NetObj *owner;
	CN::Connection *connection;

	size_t size;
	size_t bytes_left;

	bool import_data(byte_t *data,size_t len) {
//		CNetLib::log("Importing ",len," bytes");
		memcpy(this->content.data()+this->size,data,len);
		this->size += len;
		this->bytes_left -= len;
		if(this->bytes_left <= 0) {
			this->complete = true;
		}
		return this->complete;
	}

	void init_transfer(size_t len) {
//		print("Initializing transfer of ",len," bytes");
//		this->content.clear();
		this->bytes_left = len;
		this->content.resize(len,'\0');
	}

	Message(CN::Connection *parent): connection(parent) {
		this->size = 0;
		this->complete = false;
//		this->bytes_left = 0;
	}

	Message(size_t _bytes): bytes_left(_bytes) {
		this->size = 0;
		this->complete = false;
	}
	~Message() {}
};

struct Connection {

	asio::io_context m_context;

	int id;
	bool incoming;

	bool validated; //Has this connection sent a valid packet yet
	bool active;	//Is this connection active
	bool reading;	//Is this connection currently reading a message body

	std::string address;

	CN::Message *cur_msg;
	serializer s = serializer(CN_BUFFER_SIZE);

	NetObj* owner;
	tcp::socket *sock;

	//DataType and int overloads to support
	// user defined types
	size_t send_info(int);		//Sends packet with 0-length body
	size_t send_info(DataType); //Sends packet with 0-length body
	size_t package_and_send(int,std::string);
	size_t package_and_send(DataType,std::string);
	size_t package_and_send(int,byte_t*,size_t);
	size_t package_and_send(DataType,byte_t*,size_t);

	//Always uses internal types
	size_t send_file_with_name(std::string,std::string);
	size_t send(serializer*);
	size_t package_and_send(std::string);
	size_t package_and_send(byte_t*,size_t);

	byte_t *new_chunk = (byte_t*)malloc(CN_BUFFER_SIZE);

	bool precheck_message_type(DataType type) {
		if(this->validated == false)
			return type == DataType::VC_QUERY or type == DataType::VC_RESP;
		return true;
	}

	std::string get_expected_validation_hash();
	void check_validation_response(std::string);

	void handle();
	void process_data(byte_t*,size_t);
	void dispatch_msg();
	void reinit_or_reset_transfer(byte_t*,size_t);
	void reset_transfer() {
		delete this->cur_msg;
		this->cur_msg = new CN::Message(this);
		this->reading = false;
	}

	void start_handler() {
		std::thread(&CN::Connection::handle,this).detach();
	}
	void close() {
		try {
			this->sock->close();
		} catch(const std::exception &e) {

		}
		delete this->sock;
	}
	void disconnect() {
		if(this->active) this->close();
		this->active = false;
	}
	void graceful_disconnect(); //Disconnect socket, send disconnect packet, unregister from netobj

	Connection(tcp::socket *new_sock);
	~Connection() {
		CNetLib::log("Destroying connection ",this->id);
		free(new_chunk);
		delete this->cur_msg;
	}
};

struct NetObj {

	//Asio specific
	asio::io_context m_io_context;

	//Basic info
	short port;
	bool active;

	MessageHandler msg_handler;
	ConnectionHandler connection_handler;

	bool has_msg_handler = false;
	bool has_connection_handler = false;

	void set_msg_handler(MessageHandler fnc) {
		this->msg_handler = fnc;
		this->has_msg_handler = true;
	}
	void set_connection_handler(ConnectionHandler fnc) {
		this->connection_handler = fnc;
		this->has_connection_handler = true;
	}
	void call_message_handler(Message *msg) {
		if(this->has_msg_handler) {
			this->msg_handler(msg);
		} else {
			CNetLib::log("No message handler set");
		}
	}
	void call_connection_handler(Connection *nc) {
		if(this->has_connection_handler) {
			this->connection_handler(nc);
		} else {
			CNetLib::log("No connection handler set");
		}
	}

	//Connections
	std::vector<CN::Connection*> connections;
	CN::Connection* register_connection(tcp::socket*,bool is_incoming = false);

	std::map<int,std::string> validation_challenges;
	inline std::string generate_validation_challenge() {return CNetLib::random_hex_string(VC_QUERY_LEN);}

	void graceful_disconnect(CN::Connection *c);
	void remove_connection(CN::Connection*);

	std::string make_validation_hash(std::string input);

	inline void start_io() {
		this->active = true;
		this->m_io_context.run();
	}
	inline void stop_io() {
		this->active = false;
		this->m_io_context.stop();
	}

	NetObj(short _port);

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
		for(auto &c : this->connections) {
			this->graceful_disconnect(c);
		}
	}
};

}
