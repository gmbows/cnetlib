#include "../cnetlib.h"

int main() {

	CN::Client cli = CN::Client(5555); //Create client object on port 5555

	//Connect to a local server
	CN::Connection *new_connection = cli.connect("127.0.0.1"); 
	if(!new_connection) {
		CNetLib::log("Connection failed");
		return 1;
	}

	//Join channel "test" on local server and send a greeting
	new_connection->send_info(CN::DataType::CHAN_JOIN,"test");
	new_connection->package_and_send("Joined");

	//Send text messages
	std::string s;
	while(s != "q") {
		std::cout << "->" << std::flush; 
		std::getline(std::cin,s);
		new_connection->package_and_send(s);
	}
	return 0;
}