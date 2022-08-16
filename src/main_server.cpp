#include "cnetlib.h"

int main() {
	
	CN::Server serv = CN::Server(5555);
	
	serv.add_typespec_handler(CN::DataType::TEXT,[](CN::UserMessage *msg) {
		CNetLib::log("(Message) ",msg->connection->getname(),": ",msg->str());
	});
	
	CN::Channel *chan = serv.register_channel(nullptr,"test");//Channel ID must be 4 chars
	
	serv.start_listener();
	
	std::string s;
	std::cin >> s;
	return 0;
}