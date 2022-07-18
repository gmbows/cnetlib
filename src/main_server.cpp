#include "cnetlib.h"

int main() {
	
	CN::Server serv = CN::Server(5555);
	serv.start_listener();
	
	// CN::Client cli = CN::Client(5555);
	
	// CN::Connection *new_connection = cli.connect("127.0.0.1");
	// new_connection->package_and_send("Hello from client");
	
	std::string s;
	std::cin >> s;
}