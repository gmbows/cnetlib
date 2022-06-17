#pragma once

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

#define CN_BUFFER_SIZE 16384
#include "serializer.h"

#define _GLIBCXX_DTS2_CONDITION_VARIABLE_ANY


using asio::ip::tcp;


struct CN_Connection;
struct CN_Packet;
struct CN_Channel;

typedef std::function<void(CN_Connection*)> ConnectionHandler;
typedef std::function<void(CN_Packet*)> PacketHandler;
typedef std::function<void(CN_Channel*)> ChannelHandler;

namespace CNetLib {
	extern unsigned int numConnections;
	extern unsigned int numChannels;
	extern ConnectionHandler default_connection_handler;
	extern PacketHandler default_packet_handler;
	extern ChannelHandler default_channel_handler;
	extern std::vector<CN_Channel*> channels;
	CN_Channel* get_channel(int id);
	CN_Channel* get_channel(CN_Connection*);
}

enum class CN_ConnectionOrigin: short {
	CN_OUT,
	CN_INC,
};

enum class CN_DataType: short {
	CN_TEXT,
	CN_FILE,
	CN_STREAM,

	CN_CHAN_CREATE, //Payload should contain an ID for this channel
	CN_CHAN_ATTACH, //Payload should contain an ID for the channel to attach the connection to
					// When an attached connection receives data it will forward it to the packet handler
					// of the base connection of the attached channel
	CN_REQ_CON,
};

enum class CN_NetState: short {
	CN_READ_HEADER,
	CN_READ_DATA,
};

std::stringstream& operator<<(std::stringstream &s, CN_DataType &t);

extern std::string packet_header;
extern asio::io_context io_context;

//======================
//		CN_THREAD
//======================

template <typename CallerType,typename Function>
struct ThreadPackage {
	CallerType caller;
	Function fn;
	ThreadPackage(CallerType __ca, Function __fn): caller(__ca), fn(__fn) {

	}
	ThreadPackage(){}
};

template <typename CallerType,typename Function>
struct CN_Thread {
	pthread_t handle;
	ThreadPackage<CallerType,Function> *p;
	static void* execute(void *package) {
		ThreadPackage<CallerType,Function> *tp = static_cast<ThreadPackage<CallerType,Function>*>(package);
		std::invoke(tp->fn,tp->caller);
		return nullptr;
	}
	void run() {
		pthread_create(&this->handle,NULL,&this->execute,static_cast<void*>(this->p));
	}
	void join() {
		pthread_join(this->handle,NULL);
	}
	CN_Thread(CallerType&& _ca, Function&& _fn) {
		this->p = new ThreadPackage<CallerType,Function>(_ca,_fn);
		this->run();
	}
	CN_Thread(pthread_t __handle, CallerType&& _ca, Function&& _fn): handle(__handle) {
		this->p = new ThreadPackage<CallerType,Function>(_ca,_fn);
		this->run();
	}
	CN_Thread() {}
};

struct CN_Packet {
	CN_Connection *owner;
	unsigned char* content;
	size_t len;
	CN_DataType type;
	bool remote = false;
	bool delivered = false;

	void set_type(CN_DataType _type) {
		free(this->content);
		this->type = _type;
	}

	void set_data(unsigned char* new_data,size_t len) {
		this->content = (unsigned char*)malloc(len);
		memset(this->content,'\0',len);
		memcpy(this->content,new_data,len);
	}

	CN_Packet(CN_Connection *__owner, unsigned char* __content,size_t __len): owner(__owner), len(__len) {
		this->set_data(__content,len);
	}
//	CN_Message(CN_Connection *__owner, std::string __content): owner(__owner), content(__content) {
//		this->len = __content.size();
//	}
	~CN_Packet() {
		free(this->content);
	}
};

struct CN_NetReader {
	size_t bytesRemaining;
	bool reading;
	std::vector<unsigned char> data;

	CN_NetState state;
	CN_DataType type;
	
	std::string junk;
	std::string reading_filename;

	serializer s = serializer(CN_BUFFER_SIZE);
	
	unsigned short packet_index = 0;
	void reset() {
		this->reading = false;
		this->data.clear();
		std::vector<unsigned char>().swap(this->data);
		this->bytesRemaining = 0;
		this->packet_index = 0;
		this->state = CN_NetState::CN_READ_HEADER;
	}
	bool read(unsigned char*, size_t len);
	CN_NetReader(size_t brem): bytesRemaining(brem), reading(false), state(CN_NetState::CN_READ_HEADER) {
		this->data.clear();
	}
	CN_NetReader(){}
};

class CN_Connection {
	protected:
		void generate_packet(std::string);
		void generate_packet(unsigned char*,size_t);

		void close() {
			asio::error_code err;
			try {
				if (this->sock->is_open()) {
					this->sock->shutdown(tcp::socket::shutdown_send, err);
					this->sock->close();
				}
			} catch(const std::exception &e) {
				print("Error closing socket: ",e.what());
			}
			if(err) {
				print(err.message());
			}
		}
	public:
		void close_ext() {
//			this->pack_and_send(CN_TEXT,"Connection closed by remote user");
			this->active = false;
			this->close();
		}
		//Generic connection object
		bool active;
		bool secure; //Only send/recv data to/from other CNetLib connections
		bool validated;

		unsigned short id; //Used to determine associated channel
		unsigned long messages_sent = 0;
		unsigned long messages_received = 0;

		CN_ConnectionOrigin origin;

		bool wakeup = false;

		std::mutex m;
		std::condition_variable _signal;

		size_t pack_and_send(CN_DataType,std::string data);
		size_t pack_and_send(CN_DataType,unsigned char*,size_t);
		size_t send_file_with_name(std::string filename,unsigned char*,size_t,bool stream=false);

		void await_message() {
			print("Awaiting messages");
			std::unique_lock signal_lock(m);
			this->wakeup = false;
			while(this->wakeup == false) {
				this->_signal.wait(signal_lock);
			}
		}

		tcp::socket *sock;
		
		std::string address;
		
		CN_NetReader reader;
		bool inline reading() {return this->reader.reading;}
			
		pthread_t handler;
		
		CN_Channel *parent_channel = nullptr;

		PacketHandler packet_handler;
		PacketHandler check_channel_creation;

		size_t send(serializer*);

		void loopback() {
			// this->active = false;
			// this->close();
		}
		void handle_connection();
		void handle_packet(CN_Packet*);
		void handle_data(unsigned char*,size_t len);

		CN_Connection(tcp::socket *_sock);
		~CN_Connection() {
			this->close();
			// delete this->sock;
			pthread_join(this->handler,NULL);
		}
};

struct CN_Channel {
	//Communication channel
	// Can have multiple open connections to the same address
	int id;
	CN_Connection* base_connection;
	std::vector<CN_Connection*> connections;
	std::string address;
	PacketHandler packet_handler = CNetLib::default_packet_handler;

	void attach(CN_Connection *cn) {
		print("Attached connection to channel ",this->id);
		this->connections.push_back(cn);
//		cn->packet_handler = [this](CN_Packet *packet) {
//			print("Received packet in attached connection ",this->id,": ",packet->content);
//		};
	}

	CN_Channel(CN_Connection* __base): base_connection(__base) {
		this->address = __base->address;
		CNetLib::channels.push_back(this);
	}

};

struct CN_Server {
	//Generic server (incoming connection) object
	
	bool initialized;
	bool active;
	bool secure; //Only recv data from other CNetLib connections
	bool wakeup = false;
	short port;

	std::mutex m;
	std::condition_variable _signal;

	void await_new_connection() {
		print("Awaiting connection");
		std::unique_lock signal_lock(m);
		this->wakeup = false;
		while(this->wakeup == false) {
			this->_signal.wait(signal_lock);
		}
	}

	void create_channel(CN_Connection*,int);
	
	pthread_t accept_handle;

	tcp::acceptor *acceptor = nullptr;
	
	PacketHandler packet_handler = CNetLib::default_packet_handler;
	ConnectionHandler connection_handler = CNetLib::default_connection_handler;
	ChannelHandler channel_handler = CNetLib::default_channel_handler;

	PacketHandler handle_packet_internal = [this](CN_Packet *packet) {
		if(packet->type == CN_DataType::CN_CHAN_CREATE) {
			print("(Server) Creating new channel with id ",packet->content);
			int id;
			try {
				id = std::atoi((const char*)packet->content);
			} catch(const std::exception &e) {
				print("Error fetching channel ID from string: ",packet->content);
			}
			this->create_channel(packet->owner,id);
		} else if(packet->type == CN_DataType::CN_CHAN_ATTACH) {
			int id;
			try {
				id = std::atoi((const char*)packet->content);
			} catch(const std::exception &e) {
				print("Error fetching channel ID from string: ",packet->content);
			}
			print("Attaching connection to channel ",id);
			CN_Channel *requested_channel = CNetLib::get_channel(id);
			requested_channel->attach(packet->owner);
		}
	};

	std::queue<CN_Packet*> messages;
	std::vector<CN_Channel*> channels;
	std::vector<CN_Connection*> connections;
	
	// std::string query();
	
	bool connected() {
		return !this->connections.empty();
	}

	void accept();
	void start();
	std::string get_msg();

	void cleanup_connections();
	void remove_connection(CN_Connection*);
	
	void set_packet_handler(PacketHandler __handler) {
		this->packet_handler = __handler;
	}
	void set_connection_handler(ConnectionHandler __handler) {
		this->connection_handler = __handler;
	}
	void set_channel_handler(ChannelHandler __handler) {
		this->channel_handler = __handler;
	}

	void create_acceptor(short _port) {
		this->acceptor = new tcp::acceptor(io_context, tcp::endpoint(tcp::v4(), _port));
	}
	
	void set_port(short _port) {
		//If we already set a port, delete old acceptor before allocating new one
		if(this->port != -1) {
			//delete this->acceptor;
		}
		this->port = _port;
	}

	void initialize(short _port) {
		this->set_port(_port);
		this->create_acceptor(_port);
		this->set_packet_handler(CNetLib::default_packet_handler);
		this->initialized = true;
		print("Server initialized on port ",_port);
	}

	void set_secure(bool sec) {
		this->secure = sec;
		for(auto &c : this->connections) {
			c->secure = sec;
		}
	}

	CN_Server():
		initialized(false),
		active(false)
	{
	}

	CN_Server(short _port):
		active(false),
		initialized(false)
	{
		this->initialize(_port);
	}
	
	void close() {
		pthread_join(this->accept_handle,NULL);
	}
	
	~CN_Server() {
		if(this->port != (short)-1) print("Server closing. Goodbye!");
//		this->active = false;
//		if(this->initialized) this->close();
		// for(auto &c : this->connections) c->close_ext();
		// this->close_connection();
		// pthread_join(this->accept_handle,NULL);
	}
};

struct CN_Client {
	
	tcp::resolver *resolver;
	
	std::string port;

	bool initialized;
	bool secure; //Only send data to other CNetLib connections
	
	CN_Connection* connect(std::string);
	
	PacketHandler packet_handler = CNetLib::default_packet_handler;
	ConnectionHandler connection_handler = CNetLib::default_connection_handler;
	ChannelHandler channel_handler = CNetLib::default_channel_handler;

	PacketHandler handle_packet_internal = [this](CN_Packet *packet) {
		if(packet->type == CN_DataType::CN_CHAN_CREATE) {
			print("(Client) Creating new channel"); //This probably will never be used
			this->create_channel(packet->owner);
		}
	};

	CN_Channel* create_channel(std::string); //Connects to host and generates channel
	void create_channel(CN_Connection*); //Generates channel from existing connection
	
	std::vector<CN_Channel*> channels;
	std::vector<CN_Connection*> connections;
	bool connected() {
		return !this->connections.empty();
	}
	
	void set_packet_handler(PacketHandler __handler) {
		this->packet_handler = __handler;
	}
	void set_connection_handler(ConnectionHandler __handler) {
		this->connection_handler = __handler;
	}
	void set_channel_handler(ChannelHandler __handler) {
		this->channel_handler = __handler;
	}

	void cleanup_connections();
	void remove_connection(CN_Connection*);
	
	void set_port(std::string _port) {
		this->port = _port;
	}

	void create_resolver() {
		this->resolver = new tcp::resolver(io_context);
	}

	void initialize(short _port) {
		this->set_port(std::to_string(_port));
		this->set_packet_handler(CNetLib::default_packet_handler);
		this->create_resolver();
		this->initialized = true;
		print("Initialized client on port ",_port);
	}

	void set_secure(bool sec) {
		this->secure = sec;
		for(auto &c : this->connections) {
			c->secure = sec;
		}
	}

	CN_Client():
		initialized(false)
	{

	}
	
	CN_Client(short _port):
		initialized(false)
	{
		this->initialize(_port);
	}		

	~CN_Client() {
		print("Deleting client ",this->port);
		for(auto &c : this->connections) c->close_ext();
		// this->close_connection();
	}
};

