#include "../cnetlib.h"

int main() {
	
	CNetLib::make_directory("received");

	CN::Client cli = CN::Client(5555); //Create client object on port 5555

	//Connect to a local server
	CN::Connection *new_connection = cli.connect("127.0.0.1"); 
	if(!new_connection) {
		CNetLib::log("Connection failed");
		return 1;
	}

	//Join channel "test" on local server and send a greeting
	new_connection->send_file_with_name("./data.txt","data.txt");

	//Send text messages
	std::string s;
	std::cin >> s;
	return 0;
}