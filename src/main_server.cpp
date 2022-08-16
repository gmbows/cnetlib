#include "cnetlib.h"

int main() {
	
	CN::Server serv = CN::Server(5555);
	serv.start_listener();
	
	CN::Channel *chan = serv.register_channel(nullptr,"test");//Channel ID must be 4 chars
	
	std::string s;
	std::cin >> s;
}