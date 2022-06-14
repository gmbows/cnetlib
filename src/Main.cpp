#include "CNetLib.h"

#include <future>
#include <mutex>
#include <pthread.h>
#include <ctime>

//todo
//	-pool threads

int main(int argc, char** argv) {
	
	pthread_mutex_init(&printLock,NULL);
	
	CN_Server serv = CN_Server(5555);
	CN_Client cli = CN_Client();

	//default message handler
	// serv.messageHandler_Default->handler = [](CN_Message *msg) -> void {
		// print("(MessageHandler ",msg->owner->id,")["+msg->owner->address+"]: ",msg->content," (",msg->len," bytes)");
	// };
	// std::cout << sysconf(_SC_NPROCESSORS_ONLN) << std::endl;
	
	std::string in;
	while(1) {
		std::getline(std::cin,in);
		if(in == "s") {
			//start a listening thread
			serv.start();
			//alternative of
			// std::thread(&CN_Server::accept, &serv).detach();
		} else if (in == "c") {
			cli.connect("127.0.0.1");
		} else if(in == "k") {
			while(in != "q") {
				// std::getline(std::cin,in);
				cli.send(in);
				serv.connections.front()->send("Welcome");
			}
		} else if(in == "l") {
			serv.connections.front()->send("Welcome");
		}
	}
}
