#include "cnetlib.h"

#include "serializer.h"

#include <string>
#include <fstream>

namespace CN {
	unsigned int num_connections = 0;
}

//Constructors

CN::Connection::Connection(tcp::socket *new_sock): sock(new_sock), active(false), reading(false), validated(false) {
	this->address = CNetLib::get_address(new_sock);
	this->cur_msg = new CN::Message(this);
	this->id = ++num_connections;
}

CN::NetObj::NetObj(short _port): port(_port) {
	this->active = false;
}

//Server

void CN::Server::listen() {
	this->active = true;
	tcp::socket *new_socket;
	asio::error_code err;
	CNetLib::log("Server listening on port ",this->port);
	while(this->active) {
		new_socket = new tcp::socket(this->m_io_context);
		this->acceptor->accept(*new_socket,err);
		if(this->active) {
			this->register_connection(std::move(new_socket),true);
		}
	}
}

//Client

CN::Connection* CN::Client::connect(std::string address) {
	tcp::socket *new_sock;
	try {
		new_sock = new tcp::socket(this->m_io_context);
		tcp::resolver::results_type endpoints = this->resolver->resolve(address, std::to_string(this->port));
		asio::connect(*new_sock, endpoints);
	} catch(const std::exception &e) {
		CNetLib::log(e.what());
		delete new_sock;
		return nullptr;
	}

	CNetLib::print("Connection successful");
	return this->register_connection(std::move(new_sock));
}

//Connection

void CN::Connection::handle() {
	byte_t read_buf[CN_BUFFER_SIZE];
	size_t len;
	asio::error_code ec;
	while(!ec and this->active) {
		try {
			len = this->sock->read_some(asio::buffer(read_buf,CN_BUFFER_SIZE),ec);
//			len = asio::read(*this->sock,asio::buffer(read_buf,CN_BUFFER_SIZE));
//			CNetLib::log("Connection ",this->id," got ",len," bytes");
			if(this->active and !ec) {
				this->process_data(read_buf,len);
			}
		} catch(const std::exception &e) {
			CNetLib::log("Error reading from connection ",this->id,": ",e.what());
			this->disconnect();
			return;
		}
	}
	this->disconnect();
	delete this;
}

void CN::Connection::process_data(byte_t *data, size_t len) {
	this->s.set_data(data);
	if(this->reading == false) {

		//Unpack basic header data (type,length)
		DataType n_type = (DataType)s.get_int();
		int rem = s.get_int();
//		CNetLib::log("Got message of type ",(int)n_type," along with ",len," bytes");
		//Ignore some messages depending on
		// the current state of the connection
		bool proceed = this->precheck_message_type(n_type);
		if(!proceed) {
			CNetLib::log("Terminating connection due to suspicious activity");
			return this->graceful_disconnect();
		}

		//Set type
		this->cur_msg->init_transfer(rem);
		this->cur_msg->type = (int)n_type;

		//Unpack type specific data and handle info messages
		switch(n_type) {
			case CN::DataType::VC_QUERY: {
				std::string challenge = s.get_str(VC_QUERY_LEN);
				CNetLib::log("Got challenge: ",challenge,(this->incoming)? " (from client)" : " (from server)");
				this->package_and_send(DataType::VC_RESP,this->owner->make_validation_hash(challenge));
				CNetLib::log("Sent response: ",this->owner->make_validation_hash(challenge),(this->incoming)? " (to client)" : " (to server)");
				return this->reinit_or_reset_transfer(data,len);
			}
			case CN::DataType::VC_RESP: {
				if(!CNetLib::contains(this->owner->validation_challenges,this->id)) {
//					CNetLib::log("Error: No validation challenge",(this->incoming)? " (for client)" : " (for server)");
					return this->reinit_or_reset_transfer(data,len);
				}
				std::string resp = s.get_str(sizeof(vc_resp_t)*2);
				this->check_validation_response(resp);
				return this->reinit_or_reset_transfer(data,len);
			}
			case CN::DataType::CLOSE:
				return this->graceful_disconnect(); //Deletes this object
			case CN::DataType::TEXT:
				break;
			case CN::DataType::FILE:
				this->cur_msg->f_path = s.get_str();
				CNetLib::log("Now reading file: ",this->cur_msg->f_path);
				break;
			default:
				CNetLib::log("Unknown data type ",(int)n_type);
//				return this->reset_transfer();
				break;
		}

		this->reading = true;
	}
	if(this->reading) {
		//Determine whether to retrieve a full chunk or only the
		// remaining data in this transfer
		size_t len_estimate = std::min(this->cur_msg->bytes_left,len);

		//Retrieve data from packet
		size_t actual_len = s.get_data(new_chunk,len_estimate);

		if(this->cur_msg->import_data(new_chunk,actual_len)) {
			this->dispatch_msg();
		}
	}
}

void CN::Connection::reinit_or_reset_transfer(byte_t *data, size_t len) {
	if(len-CN_HEADER_SIZE > this->cur_msg->bytes_left) {
		//Packet is larger than anticipated message size
		// Meaning this packet contains multiple, potentially incomplete messages
		CNetLib::log("Processing additional packet of size ",len-CN_HEADER_SIZE-this->cur_msg->bytes_left);
		//Process next message in packet
		// Offset data pointer by the size of the message we just read
		return this->process_data(data+this->cur_msg->bytes_left+CN_HEADER_SIZE,len-this->cur_msg->bytes_left-CN_HEADER_SIZE);
	}
	return this->reset_transfer();
}

size_t CN::Connection::send(serializer *s) {
	size_t written;
	try {
		written = asio::write(*this->sock,asio::buffer(s->data,s->true_size()));
	} catch(const std::exception &e) {
		CNetLib::print("Error writing to socket");
		this->disconnect();
	}
	return written;
}

std::string CN::Connection::get_expected_validation_hash() {
	if(!CNetLib::contains(this->owner->validation_challenges,this->id)) CNetLib::log("Error: No hash for this connection");
	return this->owner->validation_challenges.at(this->id);
}

void CN::Connection::check_validation_response(std::string actual_response) {
	std::string expected_response = this->get_expected_validation_hash();
	if(actual_response != expected_response) {
		CNetLib::log("Hash validation failed for connection ",this->id);
		CNetLib::log("Expected: ",expected_response," Got: ",actual_response);
	} else {
		CNetLib::log("Validation successful: ",actual_response);
		this->validated = true;
	}
}

size_t CN::Connection::package_and_send(std::string data) {

	serializer s = serializer(CN_BUFFER_SIZE);
	s.add_int((int)DataType::TEXT);
	s.add_int(data.size());
	s.add_str_auto(data);

	size_t written = this->send(&s);

	return written;
}

size_t CN::Connection::package_and_send(byte_t *data, size_t len) {
	serializer s = serializer(CN_BUFFER_SIZE);
	s.add_int((int)DataType::TEXT);
	s.add_int(len);
	s.add_data(data,len);

	size_t written = this->send(&s);

	return written;
}

size_t CN::Connection::send_info(DataType type) {
	serializer s = serializer(CN_BUFFER_SIZE);
	s.add_int((int)DataType::CLOSE);
	s.add_int(0);

	size_t written = this->send(&s);

	return written;
}

size_t CN::Connection::package_and_send(DataType type, std::string data) {
	serializer s = serializer(CN_BUFFER_SIZE);
	s.add_int((int)type);
	s.add_int(data.size());
	s.add_str_auto(data);

	size_t written = this->send(&s);

	return written;
}

size_t CN::Connection::send_file_with_name(std::string path,std::string filename) {
	byte_t *file_;
	size_t len = CNetLib::import_file(path,file_);
	serializer s = serializer(CN_BUFFER_SIZE);

	s.add_int((int)DataType::FILE);
	s.add_int(len);
	s.add_str(filename);
	s.add_data(file_,len);

	size_t written = this->send(&s);
	return written;
}

size_t CN::Connection::send_info(int type) {
	serializer s = serializer(CN_BUFFER_SIZE);
	s.add_int(type);
	s.add_int(0);

	size_t written = this->send(&s);

	return written;
}

size_t CN::Connection::package_and_send(int type, std::string data) {
	serializer s = serializer(CN_BUFFER_SIZE);
	s.add_int(type);
	s.add_int(data.size());
	s.add_str_auto(data);

	size_t written = this->send(&s);

	return written;
}

void CN::Connection::dispatch_msg() {
	if(this->validated) {
		this->owner->call_message_handler(std::move(this->cur_msg));
	} else {
		CNetLib::print("Ignoring ",this->cur_msg->size," bytes from unvalidated connection ");
	}
	this->reset_transfer();
}

void CN::Connection::graceful_disconnect() {
	this->owner->graceful_disconnect(this);
}

//NetObj

CN::Connection* CN::NetObj::register_connection(tcp::socket *new_sock,bool is_incoming) {
	//Registers a connection to this tcp socket
	CN::Connection *nc = new CN::Connection(new_sock);
	nc->owner = this;
	CNetLib::log("Registered new connection ",nc->id);
	this->connections.push_back(nc);
	nc->active = true;
	nc->incoming = is_incoming;	//Indicate if connection is outgoing or incoming

	//Create validation challenge
	CNetLib::log("Creating new validation challenge");
	std::string new_challenge = this->generate_validation_challenge();
	std::string new_hash = this->make_validation_hash(new_challenge);
	while(new_hash.size() != 8) {
		new_challenge = this->generate_validation_challenge();
		new_hash = this->make_validation_hash(new_challenge);
	}
	this->validation_challenges.insert({nc->id,new_hash});
	nc->package_and_send(DataType::VC_QUERY,new_challenge);
	CNetLib::log("Sent validation query ",(nc->incoming)? " (to client)" : " (to server)");
	nc->start_handler();
	this->call_connection_handler(nc);
	return nc;
}

void CN::NetObj::remove_connection(Connection *cn) {
	if(!CNetLib::contains(this->connections,cn)) return;
	int t_index = 0;
	for(int i=0;i<this->connections.size();i++) {
		CN::Connection *c = this->connections.at(i);
		if((char*)c == (char*)cn) {
			t_index = i;
			break;
		}
	}
	this->connections.erase(this->connections.begin()+t_index);
//	delete cn;
	cn->m_context.stop();
}

void CN::NetObj::graceful_disconnect(Connection *c) {
	c->send_info(CN::DataType::CLOSE);
	c->disconnect();
	this->remove_connection(c);
}

std::string CN::NetObj::make_validation_hash(std::string input) {
	if(input.size() == 0) return "5REE";

	//Constants
	const byte_t c1 = 0x8aU+CN_PROTOCOL_VERS;
	const byte_t c2 = 0x2bU+CN_PROTOCOL_VERS;
	const byte_t c3 = 0xa7U+CN_PROTOCOL_VERS;
	const byte_t c4 = 0xF3U+CN_PROTOCOL_VERS;

	short affix = input[input.size()/2];

	vc_resp_t hash = c3; //32 bit int = 8 char hex

	//Should be at least enough for the resulting hex to be 8 chars
	int steps = VC_QUERY_LEN;

	for(int i=0;i<steps;i++) {
		hash = hash ^ (input[i%input.size()]) ^ (1+affix);
		hash *= (i%4==0) ? (c1) : (i%3 == 0) ? (c2) : (i%2==0) ? (c3) : (c4);
	}
	std::stringstream hexout;
	hexout << std::hex << hash;
	return hexout.str();
}
