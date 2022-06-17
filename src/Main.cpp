#include "cnetlib.h"

int main() {
	CN_Server serv;
	CN_Client cli;
	serv.initialize(5555);
	cli.initialize(5555);
	serv.start();
	
	serv.packet_handler = [](CN_Packet *np) {
		switch(np->type) {
			case CN_DataType::CN_TEXT:
				print("Got message: ",np->content," (",np->len," bytes)");
				break;
			default:
				print("Unhandled data type","(",np->len," bytes)");
		}
	};
	
	CN_Connection *new_connection = cli.connect("127.0.0.1");
	new_connection->pack_and_send(CN_DataType::CN_TEXT,"Hello from client");
	
	std::string s;
	std::cin >> s;
}