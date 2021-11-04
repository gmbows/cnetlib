#include <asio.hpp>
#include <memory>
#include <future>
#include <mutex>
#include <pthread.h>
#include <functional>
#include <queue>
#include "Utility.h"

using asio::ip::tcp;

struct CN_MessageHandler;
struct CN_Message;

struct CN_NetReader {
	size_t bytesRemaining;
	bool reading;
	std::string data;
	std::string data_inv;
	unsigned short packet_index = 0;
	void reset() {
		this->reading = false;
		this->data = "";
		this->data_inv = "";
		this->bytesRemaining = 0;
	}
	bool read(char c) {
		if(!this->reading) {
			if(packet_header[this->packet_index] == c) {
				packet_index++;
			} else {
				packet_index = 0;
			}
			return packet_index == packet_header.size();
		}
		this->data += c;
		return --bytesRemaining == 0;
	}
	CN_NetReader(size_t brem): bytesRemaining(brem), reading(false) {
		this->data = "";
	}
	CN_NetReader(){}
};

struct CN_Connection {
	//Generic connection object
	bool active;
	unsigned short id;
	tcp::socket *sock;
	
	std::string address;
	
	CN_NetReader reader;
	bool inline reading() {return this->reader.reading;}
	
	std::queue<std::string> messages;
	
	CN_MessageHandler *messageHandler;
	
	size_t send(std::string data);
	void handle();
	void handle_msg(CN_Message*);
	void handle_data(std::string data);

	CN_Connection(tcp::socket *_sock);
	~CN_Connection() {
		delete this->sock;
	}
};

struct CN_Message {
	CN_Connection *owner;
	std::string content;
	size_t len;
	std::string serialize() {
		std::string size = std::to_string(len);
		pad(size,16,"0");
		return packet_header + size + content;
	}
	void deserialize(std::string s) {
		s.erase(0,packet_header.size());
		this->len = std::stoi(s.substr(0,16));
		print("Got message of length ",this->len);
		s.erase(0,16);
		this->content = s;
	}
	CN_Message(CN_Connection *__owner, std::string __content): owner(__owner), content(__content) {
		this->len = __content.size();
	}
	CN_Message(std::string s) {
		this->deserialize(s);
	}
};


// struct CN_Thread;

struct CN_MessageHandler {
	
	void handle(CN_Message *msg) {
		this->handler(msg);
		delete msg;
		msg = nullptr;
	}
	
	//Awaits new messages, passes to user-defined handling function
	std::function<void(CN_Message*)> handler;
	
	CN_MessageHandler(std::function<void(CN_Message*)> __h): handler(__h) {
		
	}
	
	CN_MessageHandler() {}
};

struct CN_Server {
	//Generic server (incoming connection) object
	
	bool active;
	unsigned short port;
	unsigned int numConnections;

	tcp::acceptor *acceptor;
	
	CN_MessageHandler *messageHandler_Default;

	std::queue<CN_Message*> messages;
	std::queue<CN_Connection*> connections;

	void accept();
	void start();
	std::string get_msg();

	CN_Server(unsigned short _port): port(_port), numConnections(0), active(false) {
		this->messageHandler_Default = new CN_MessageHandler([](CN_Message* msg) {
			print("(MessageHandler ",msg->owner->id,")["+msg->owner->address+"]: ",msg->content," (",msg->len," bytes)");
		});
		this->acceptor = new tcp::acceptor(io_context, tcp::endpoint(tcp::v4(), _port));
		print("Created new server object");
	}
	~CN_Server() {
		delete this->acceptor;
		delete this->messageHandler_Default;
		while(!this->messages.empty()) {
			delete this->messages.front();
			this->messages.pop();
		}
		while(!this->connections.empty()) {
			delete this->connections.front();
			this->connections.pop();
		}
	}
};

struct CN_Client {
	
	tcp::resolver *resolver;
	
	bool connect(std::string);
	size_t send(std::string);
	
	CN_MessageHandler *messageHandler_Default;
	
	bool inline connected() {return this->connection->active;};
	CN_Connection *connection;
	
	CN_Client() {
		this->messageHandler_Default = new CN_MessageHandler([](CN_Message* msg) {
			print("(MessageHandler ",msg->owner->id,")["+msg->owner->address+"]: ",msg->content," (",msg->len," bytes)");
		});
		this->resolver = new tcp::resolver(io_context);
		print("Created new client object");
	}
	//DTOR
};

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
		pthread_create(&handle,NULL,&this->execute,static_cast<void*>(this->p));
	}
	CN_Thread(CallerType&& _ca, Function&& _fn) {
		this->p = new ThreadPackage<CallerType,Function>(_ca,_fn);
		this->run();
	}
	CN_Thread() {}
};