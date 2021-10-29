#include <asio.hpp>
#include <memory>
#include <future>
#include <mutex>
#include <pthread.h>
#include <functional>
#include "Utility.h"

using asio::ip::tcp;

typedef void(*void_fn)(void);
typedef void*(*threadptr)(void*);
typedef void*(*real)(void);

struct CN_Connection {
	//Generic connection object
	bool active;
	std::shared_ptr<tcp::socket> sock;

	CN_Connection() {
		print("Created new connection object");
	}
};

struct CN_Server {
	//Generic server (incoming connection) object

	std::shared_ptr<tcp::acceptor> acceptor;

	void accept();

	CN_Server() {
		this->acceptor = std::make_shared<tcp::acceptor>(tcp::acceptor(io_context, tcp::endpoint(tcp::v4(), 5555)));
		print("Created new server object");
	}
};

struct CN_Client {
	//Generic client (outgoing connection) object
	
	std::shared_ptr<tcp::resolver> resolver;
	
	void connect(std::string);
	
	CN_Client() {
		this->resolver = std::make_shared<tcp::resolver>(tcp::resolver(io_context));
		print("Created new client object");
	}
};

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
	pthread_t handler;
	ThreadPackage<CallerType,Function> *p;
	static void* execute(void *package) {
		ThreadPackage<CallerType,Function> *tp = static_cast<ThreadPackage<CallerType,Function>*>(package);
		std::invoke(tp->fn,tp->caller);
		return nullptr;
	}
	void run() {
		pthread_create(&handler,NULL,&this->execute,static_cast<void*>(this->p));
	}
	CN_Thread(CallerType&& _ca, Function&& _fn) {
		this->p = new ThreadPackage<CallerType,Function>(_ca,_fn);
		this->run();
	}
};