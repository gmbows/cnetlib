#pragma once

#include "cnetlib.h"
#include "serializer.h"
#include <random>
#include <chrono>

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
			CNetLib::print("Serializer error at index ",i," (",data[i]," got ",new_data[i]);
			return false;
		}
	}
	free(data);
	free(new_data);
	CNetLib::print("Serializer test passing");
	return true;
}

bool test_verification_hashes(int nchecks) {
	std::vector<std::string> c;
	CN::NetObj hasher = CN::NetObj(1738);
	double _total = 0;
	for(int i=0;i<nchecks;i++) {
		std::string chal = CNetLib::random_hex_string(VC_QUERY_LEN);
		auto t_start = std::chrono::high_resolution_clock::now();
		std::string resp = hasher.make_validation_hash(chal);
		auto t_end = std::chrono::high_resolution_clock::now();
		double elapsed = std::chrono::duration<double, std::milli>(t_end-t_start).count();
		_total+=elapsed;
		if(CNetLib::contains(c,chal)) {
			CNetLib::print("Collision for ",chal,": ",resp);
		} else {
			c.push_back(resp);
		}
		if(resp.size() != VC_RESP_LEN) {
			CNetLib::print("Incorrect response length for challenge ",chal,": ",resp);
		}
	}
	CNetLib::print("Unique hashes: ",c.size(),", Collisions: ",nchecks-c.size());
	CNetLib::print("Completed in ",_total,"ms,","(",_total/nchecks," ms/hash)");
	return true;
}

void run_tests(CN::Server* test_server,CN::Client *test_client) {
	bool passing = true;
	if(test_serializer(CN_BUFFER_SIZE*5) == false) passing = false;

	test_verification_hashes(655);

//	if(test_network_transfer(test_server,test_client) == false) passing = false;

	if(passing) {
		CNetLib::print("All tests passing");
	} else {
		CNetLib::print("One or more tests failing");
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
		CNetLib::print("Got ",pck->len," bytes");
		for(int i=0;i<test_size;i++) {
//			CNetLib::print(i,": ",pck->content[i]," :: ",data[i]);
			if(pck->content[i] != data[i]) {
				CNetLib::print("Data integrity test failed at char ",i,": ",pck->content[i]," :: ",data[i]);
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
			CNetLib::print("Serializer error at index ",i," (",data[i]," got ",new_data[i]);
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
		CNetLib::print("All tests passing");
	} else {
		CNetLib::print("One or more tests failing");
	}

}

#endif
