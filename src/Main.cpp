#include "CNetLib.h"

#include <future>
#include <mutex>
#include <ctime>

int main(int argc, char** argv) {
	CN_Server serv = CN_Server();
	CN_Client cli = CN_Client();

	std::string in;
	while(1) {
		std::getline(std::cin,in);
		if(in == "s") {
			//start a listening thread
			CN_Thread(&serv,&CN_Server::accept);
		} else if (in == "c") {
			cli.connect("127.0.0.1");
		}
	}
}
