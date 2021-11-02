#include "CNetLib.h"

#include <future>
#include <mutex>
#include <pthread.h>
#include <ctime>

int main(int argc, char** argv) {
	CN_Server serv = CN_Server(5555);
	CN_Client cli = CN_Client();

	std::string in;
	while(1) {
		std::getline(std::cin,in);
		if(in == "s") {
			//start a listening thread
			CN_Thread(&serv,&CN_Server::accept);
			//alternative of
			// std::thread(&CN_Server::accept, &serv).detach();
		} else if (in == "c") {
			cli.connect("127.0.0.1");
		}
	}
}
