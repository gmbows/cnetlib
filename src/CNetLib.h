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

struct CN_Connection {
	//Generic connection object
	bool active;
	unsigned short id;
	tcp::socket *sock;
	
	std::string address;
	
	std::queue<std::string> messages;
	
	CN_MessageHandler *messageHandler;
	
	void handle();
	void handle_msg(CN_Message*);

	CN_Connection(tcp::socket *_sock): id(0) {
		print("Created new connection object");
		this->sock = _sock;
		this->address = get_address(sock);
		this->active = false;
	}
	~CN_Connection() {
		delete this->sock;
	}
};

struct CN_Message {
	CN_Connection *owner;
	std::string content;
	size_t len;
	CN_Message(CN_Connection *__owner, std::string __content): owner(__owner), content(__content) {
		this->len = __content.size();
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
	
	unsigned short port;
	unsigned int numConnections;

	tcp::acceptor *acceptor;
	
	CN_MessageHandler *messageHandler_Default;


	std::queue<CN_Message*> messages;
	std::queue<CN_Connection*> connections;

	void accept();
	std::string get_msg();

	CN_Server(unsigned short _port): port(_port), numConnections(0) {
		this->messageHandler_Default = new CN_MessageHandler();
		this->acceptor = new tcp::acceptor(io_context, tcp::endpoint(tcp::v4(), _port));
		print("Created new server object");
	}
	//DTOR
};

struct CN_Client {
	
	tcp::resolver *resolver;
	
	bool connect(std::string);
	void send(std::string);
	
	bool connected;
	CN_Connection *connection;
	
	CN_Client(): connected(false) {
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