#pragma once

#include "cnetlib.h"
#include "serializer.h"
#include <random>

bool test_network_transfer(CN::Server *serv,CN::Client *client) {

	serv->start_listener();

	while(serv->active == false) {
	}

	CN::Connection *nc = client->connect("127.0.0.1");

	if(nc == nullptr) {
		print("Connection test failing");
		return false;
	}

	print("Connection test passing");

	int written = nc->package_and_send("test message");

	return true;
}

bool test_serializer(size_t test_size) {
	serializer s = serializer(CN_BUFFER_SIZE);

	unsigned char* data = (unsigned char*)malloc(test_size);
	for(int i=0;i<test_size;i++) {
		data[i] = (unsigned char)(rand()%256);
	}

	s.add_data(data,test_size);

	unsigned char* new_data = (unsigned char*)malloc(test_size);

	s.get_data(new_data,test_size);

	for(int i=0;i<test_size;i++) {
		if(data[i] != new_data[i]) {
			print("Serializer error at index ",i," (",data[i]," got ",new_data[i]);
			return false;
		}
	}
	free(data);
	free(new_data);
	print("Serializer test passing");
	return true;
}

void run_tests() {
	bool passing = true;
	if(test_serializer(CN_BUFFER_SIZE*5) == false) passing = false;

	CN::Server *test_server = new CN::Server(5555);
	CN::Client *test_client = new CN::Client(5555);

	if(test_network_transfer(test_server,test_client) == false) passing = false;

	if(passing) {
		print("All tests passing");
	} else {
		print("One or more tests failing");
	}

}

#ifdef VERS2

bool test_network_transfer() {
	CN_Server serv;
	CN_Client cli;

	serv.initialize(5555);
	cli.initialize(5555);

	serv.start();

	int test_size = 65536*2;

	bool passing = true;
	bool test_complete = false;

	unsigned char* data = (unsigned char*)malloc(test_size);
	for(int i=0;i<test_size;i++) {
		data[i] = (unsigned char)(rand()%256);
	}

	serv.set_connection_handler([&](CN_Connection *nc) {
		nc->pack_and_send(CN_DataType::CN_TEXT,data,test_size);
	});

	cli.set_message_handler([&](CN_Message *pck) {
		print("Got ",pck->len," bytes");
		for(int i=0;i<test_size;i++) {
//			print(i,": ",pck->content[i]," :: ",data[i]);
			if(pck->content[i] != data[i]) {
				print("Data integrity test failed at char ",i,": ",pck->content[i]," :: ",data[i]);
				passing = false;
//				break;
			}
		}
//		serv.active = false;
		test_complete = true;
	});

	CN_Connection *nc = cli.connect("127.0.0.1");

	while(test_complete == false) {}

	nc->close_ext();

//	serv.close();

//	free(data);
	return passing;
}

bool test_serializer(size_t test_size) {
	serializer s = serializer(CN_BUFFER_SIZE);

	unsigned char* data = (unsigned char*)malloc(test_size);
	for(int i=0;i<test_size;i++) {
		data[i] = (unsigned char)(rand()%256);
	}

	s.add_data(data,test_size);

	unsigned char* new_data = (unsigned char*)malloc(test_size);

	s.get_data(new_data,test_size);

	for(int i=0;i<test_size;i++) {
		if(data[i] != new_data[i]) {
			print("Serializer error at index ",i," (",data[i]," got ",new_data[i]);
			return false;
		}
	}
	free(data);
	free(new_data);
	return true;
}

void run_tests() {
	bool passing = true;
	if(test_serializer(CN_BUFFER_SIZE*5) == false) passing = false;
	if(test_network_transfer() == false) passing = false;

	if(passing) {
		print("All tests passing");
	} else {
		print("One or more tests failing");
	}

}

#endif
