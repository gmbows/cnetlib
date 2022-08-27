#include "../cnetlib.h"

int main() {
	
	CN::Server serv = CN::Server(5555);
	
	serv.add_typespec_handler(CN::DataType::FILE,[](CN::UserMessage *msg) {
		CNetLib::log("(File) Got ",msg->f_name," (",CNetLib::conv_bytes(msg->size));
		CNetLib::export_file("./received/"+msg->f_name,msg->content.data(),msg->size);
	});
	
	serv.start_listener();
	
	std::string s;
	std::cin >> s;
	return 0;
}