#include <asio.hpp>
#include <memory>
#include <future>
#include <mutex>
#include <pthread.h>
#include <functional>
#include <queue>
#include "Utility.h"

using asio::ip::tcp;

struct CN_Connection {
	//Generic connection object
	bool active;
	unsigned short id;
	tcp::socket *sock;
	
	std::queue<std::string> messages;
	
	void handle();

	CN_Connection(tcp::socket *_sock) {
		print("Created new connection object");
		this->sock = _sock;
		this->active = false;
	}
	~CN_Connection() {
		delete this->sock;
		print("Deleted connection");
	}
};

struct CN_Message {
	CN_Connection *owner;
	std::string content;
	CN_Message(CN_Connection *__owner, std::string __content): owner(__owner), content(__content) {
		
	}
};

struct CN_Server {
	//Generic server (incoming connection) object
	
	unsigned short port;
	unsigned int numConnections;

	tcp::acceptor *acceptor;


	std::queue<CN_Message*> messages;
	std::queue<CN_Connection*> connections;

	void accept();
	std::string get_msg();

	CN_Server(unsigned short _port): port(_port) {
		this->acceptor = new tcp::acceptor(io_context, tcp::endpoint(tcp::v4(), _port));
		print("Created new server object");
	}
	//DTOR
};

struct CN_Client {
	
	tcp::resolver *resolver;
	
	void connect(std::string);
	
	CN_Client() {
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
	~CN_Thread() {
		delete this->p;
	}
};