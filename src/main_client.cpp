#include "cnetlib.h"

int main() {

	CN::Client cli = CN::Client(5555);
	
	CN::Connection *new_connection = cli.connect("127.0.0.1");
	new_connection->send_info(CN::DataType::CHAN_JOIN,"test");
	new_connection->package_and_send("Joined");
	
	std::string s;
	while(s != "q") {
		std::getline(std::cin,s);
		new_connection->package_and_send(s);
	}
}