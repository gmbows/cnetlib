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

#define CN_MTYPE_T int
#define CN_MSIZE_T size_t

#define CN_BUFFER_SIZE 65536
#define CN_PROTOCOL_VERS 1
//#define CN_HEADER_SIZE 8

#define VC_QUERY_LEN 16
#define VC_RESP_LEN 8

constexpr unsigned CN_HEADER_SIZE() {
	return sizeof(CN_MTYPE_T) + sizeof(CN_MSIZE_T);
}

typedef unsigned char byte_t;

using asio::ip::tcp;

namespace CN {

struct UserMessage;
struct Connection;
struct Channel;

typedef std::function<void(UserMessage*)> MessageHandler;
typedef std::function<void(Connection*)> ConnectionHandler;
typedef std::function<void(Channel*)> ChannelHandler;

extern MessageHandler default_msg_handler;
extern ConnectionHandler default_connection_handler;
extern ChannelHandler default_channel_handler;

//maps uid to Channel*
extern std::map<std::string,Channel*> channel_map;

Channel* get_channel(std::string _uid);

enum class DataType: CN_MTYPE_T {

	//Basic data types
	TEXT,
	FILE,
	JSON,

	//Close connection
	CLOSE,

	//Validation challenge
	VC_QUERY,
	VC_RESP,
	VC_VALID,

	//Channels
	CHAN_CREATE,
	CHAN_ATTACH,

	//Streaming
	STREAM_INIT,
	//Between these two packets, all incoming packets will be streamed
	// directly to the file initialized in the STREAM_INIT message
	STREAM_TERM,
};


void init();

extern unsigned int num_connections;
//extern unsigned int num_channels;

struct NetObj;

struct UserMessage {

	CN_MTYPE_T type;

	bool complete;
	std::string f_name;

	std::vector<byte_t> content;
	CN::Connection *connection;

	size_t size;
	size_t bytes_left;

	std::string str() {
		return CNetLib::fmt_bytes(this->content.data(),this->size);
	}

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

	//Import len of data without copying
	// Used when data is spread over multiple packets
	// And will not be exposed to user-defined handlers
	bool import_data_size(size_t len) {
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

	UserMessage(CN::Connection *parent): connection(parent) {
		this->size = 0;
		this->complete = false;
//		this->bytes_left = 0;
	}

	UserMessage(size_t _bytes): bytes_left(_bytes) {
		this->size = 0;
		this->complete = false;
	}
	~UserMessage() {}
};

struct PackedMessage {
	serializer s = serializer(1024);
	PackedMessage(DataType type,CN_MSIZE_T len) {
		this->s.add_auto((CN_MTYPE_T)type);
		this->s.add_auto(len);
	}
	PackedMessage(CN_MTYPE_T type,CN_MSIZE_T len) {
		this->s.add_auto(type);
		this->s.add_auto(len);
	}
	PackedMessage(CN_MTYPE_T type) {
		this->s.add_auto(type);
		this->s.add_auto<CN_MSIZE_T>(0);
	}
};

struct Connection {

	asio::io_context m_context;

	int id;
	bool incoming;

	bool validated;			//Has this connection received a valid packet yet
	bool remote_validated;	//Has the remote host validated this connection
	bool active;	//Is this connection active
	bool reading;	//Is this connection currently reading a message body

	std::string getname() {
		return	CNetLib::as_hex(this->id) + "." +
				std::to_string(this->active) +
				std::to_string(this->validated) +
				std::to_string(this->remote_validated) +
				std::to_string(this->reading) +
				(this->incoming ? ".inc" : ".out") + ".con";
	}

	CNetLib::Waiter *remote_validation;

	std::string address;

	CN::UserMessage *cur_msg = nullptr;
	serializer s_importer = serializer(CN_BUFFER_SIZE);

	NetObj* owner = nullptr;
	Channel* parent_channel = nullptr;
	tcp::socket *sock;

	//DataType and int overloads to support
	// user defined types
	size_t send(serializer*);
	size_t send(PackedMessage*);
	size_t send_info(CN_MTYPE_T);		//Sends packet with 0-length body
	size_t send_info(DataType); //Sends packet with 0-length body
	size_t package_and_send(CN_MTYPE_T,std::string);
	size_t package_and_send(DataType,std::string);
	size_t package_and_send(CN_MTYPE_T,byte_t*,CN_MSIZE_T);
	size_t package_and_send(DataType,byte_t*,CN_MSIZE_T);

	//Always uses internal types/no type
	size_t send_file_with_name(std::string,std::string);
	size_t package_and_send(std::string);
	size_t package_and_send(byte_t*,CN_MSIZE_T);
	size_t init_stream_old(std::string,std::string);	 //takes path,filename
	size_t term_stream_old(std::string);				 //terminates stream for this filename

	//Streaming
	bool stream_file(std::string,std::string);
	std::map<std::string,size_t> stream_info;

	//Holds packet data
	byte_t *new_chunk = (byte_t*)malloc(CN_BUFFER_SIZE);

	bool precheck_message_type(DataType type) {
		if(this->validated == false)
			return type == DataType::VC_QUERY or type == DataType::VC_RESP or type == DataType::VC_VALID;
		return true;
	}

	std::string get_expected_validation_hash();
	void check_validation_response(std::string);

	void handle();
	void process_data(byte_t*,size_t);
	void dispatch_msg();
	void peek_msg(); //Call message handler on an incomplete message
	void reinit_or_reset_transfer(byte_t*,size_t);
	void reset_transfer() {
		delete this->cur_msg;
		this->cur_msg = new CN::UserMessage(this);
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
	}
	void disconnect() {
		if(this->active) this->close();
		this->active = false;
		this->m_context.stop();
	}
	void graceful_disconnect(); //Disconnect socket, send disconnect packet, unregister from netobj

	Connection(tcp::socket *new_sock);
	~Connection() {
		CNetLib::log(this->getname(),": Destroying");
		free(new_chunk);
		delete this->cur_msg;
		delete this->sock;
	}
};

struct Channel {
	std::string uid;
	CN::Connection *base_connection = nullptr;
	std::string address() {
		if(!this->base_connection) {
			CNetLib::log("Channel error: No base connection");
			return "0.0.0.0";
		}
		return this->base_connection->address;
	}

	std::string getname() {
		return this->uid +
				(!this->base_connection ? ".nul" :
					(this->base_connection->incoming ? ".inc" : ".out")
				) +
				".chan";
	}

	Channel() {

	}
	Channel(CN::Connection *cn): uid(CNetLib::random_hex_string(4)) {
//		CNetLib::log("New chan.",uid);
		this->base_connection = cn;
		cn->parent_channel = this;
	}
	Channel(CN::Connection *cn,std::string _uid): uid(_uid) {
//		CNetLib::log("New chan.",uid);
		this->base_connection = cn;
		cn->parent_channel = this;
	}
};

struct NetObj {

	//Asio specific
	asio::io_context m_io_context;

	//Basic info
	unsigned short port;
	bool active;


	//Handlers
	MessageHandler msg_handler;
	ConnectionHandler connection_handler;
	ChannelHandler channel_handler;

	bool has_msg_handler = false;
	bool has_connection_handler = false;
	bool has_channel_handler = false;

	void set_msg_handler(MessageHandler fnc) {
		this->msg_handler = fnc;
		this->has_msg_handler = true;
	}
	void set_connection_handler(ConnectionHandler fnc) {
		this->connection_handler = fnc;
		this->has_connection_handler = true;
	}
	void set_channel_handler(ChannelHandler fnc) {
		this->channel_handler = fnc;
		this->has_channel_handler = true;
	}

	void call_message_handler(UserMessage *msg) {
		if(this->has_msg_handler) {
			this->msg_handler(msg);
		} else {
			CN::default_msg_handler(msg);
		}
	}
	void call_connection_handler(Connection *nc) {
		if(this->has_connection_handler) {
			this->connection_handler(nc);
		} else {
//			CNetLib::log("No connection handler set");
		}
	}	
	void call_channel_handler(Channel *cn) {
		if(this->has_channel_handler) {
			this->channel_handler(cn);
		} else {
//			CNetLib::log("No channel handler set");
		}
	}

	//Channels
	std::vector<CN::Channel*> channels;
	CN::Channel* register_channel(CN::Connection*,std::string id = "none");

	//Connections
	std::vector<CN::Connection*> connections;
	CN::Connection* register_connection(tcp::socket*,bool is_incoming = false);

	//Validation
	unsigned timeout = 2000; //2000 ms validation timeout
	std::map<int,std::string> validation_challenges;
	inline std::string generate_validation_challenge() {return CNetLib::random_hex_string(VC_QUERY_LEN);}

	//Closing
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

	CN::Channel* create_channel(std::string);

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
		std::thread(&CNetLib::create_upnp_mapping,this->port).detach();
		std::thread(&CN::Server::listen,this).detach();
		this->start_io();
//		CNetLib::create_upnp_mapping(this->port);
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
