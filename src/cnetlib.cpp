#include <cnetlib.h>

#include "serializer.h"

#include <cmath>
#include <string>
#include <fstream>

namespace CN {
unsigned int num_connections = 0;
//unsigned int num_channels = 0;

MessageHandler default_msg_handler = [](UserMessage *msg) {
	CNetLib::log("(Default) ",msg->connection->getname(),": ",msg->str());
};

ConnectionHandler default_connection_handler = [](Connection *cn) {
	CNetLib::log("(Default) ",cn->getname(),": Established");
};

ChannelHandler default_channel_handler = [](Channel *chan) {
	CNetLib::log("(Default) ",chan->getname(),": Created");
};

void init() {
	CNetLib::make_directory("received");
	if(VC_RESP_LEN*4 >= sizeof(size_t)*8) {
		CNetLib::log("Validation response length too large");
	}
	CNetLib::log("(INFO) Header type size ",sizeof(CN_MTYPE_T));
	CNetLib::log("(INFO) Header size size ",sizeof(CN_MSIZE_T));
	CNetLib::log("(INFO) Total ",CN_HEADER_SIZE());
}

std::map<std::string,Channel*> channel_map;

Channel *get_channel(std::string _uid) {
	if(!CNetLib::contains(channel_map,_uid)) return nullptr;
	return channel_map[_uid];
}

}

//Constructors

CN::Connection::Connection(tcp::socket *new_sock,ConnectionType type): sock(new_sock), active(false), reading(false), validated(false), remote_validated(false), type(type) {
	this->address = CNetLib::get_address(new_sock);
	this->cur_msg = new CN::UserMessage(this);
	this->id = ++num_connections;
	this->uid = CNetLib::random_hex_string(8);
	this->remote_validation = new CNetLib::Waiter(&this->validated);
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

//	CNetLib::print("Connection successful");
	if(this->secure)
		return this->register_connection(std::move(new_sock));
	else
		return this->register_connection(std::move(new_sock),false,true);
}

void CN::Client::connect_async(std::string address, std::function<void (CN::Connection*)> callback) {
	auto routine = [=]() {
		CN::Connection *nc = this->connect(address);
		callback(nc);
	};
	std::thread(routine).detach();
}

CN::Channel *CN::Client::create_channel(std::string address) {
	CN::Connection *nc = this->connect(address);
	if(!nc) return nullptr;
	CN::Channel *new_channel = this->register_channel(nc);
	nc->send_info(CN::DataType::CHAN_CREATE,new_channel->uid);
	return new_channel;
}

//Connection

std::string CN::Connection::getname() {
	return	CNetLib::as_hex(this->id) + "." +
			std::to_string(this->owner->secure) +
			std::to_string(this->active) +
			std::to_string(this->validated) +
			std::to_string(this->remote_validated) +
			std::to_string(this->reading) +
			(this->incoming ? ".inc" : ".out") + ".con";
}

//Data processing

void CN::Connection::handle() {
	byte_t read_buf[CN_BUFFER_SIZE];
	size_t len;
	asio::error_code ec;
	while(!ec and this->active) {
		try {
			len = this->sock->read_some(asio::buffer(read_buf,CN_BUFFER_SIZE),ec);
			CNetLib::log("Connection ",this->id," got ",len," bytes");
			if(this->active and !ec) {
				if(this->type != CN::ConnectionType::WRITEONLY)
					this->process_data(read_buf,len);
			}
		} catch(const std::exception &e) {
			CNetLib::log(this->getname(),": Network I/O error: ",e.what());
			this->disconnect();
			return;
		}
	}
	this->disconnect();
	delete this;
}

void CN::Connection::process_data(byte_t *data, size_t len) {
	if(this->owner->ptc == Protocol::CNETLIB) {
		this->process_data_cnetlib(data,len);
	} else {
		this->process_data_http(data,len);
	}
}

void CN::Connection::process_data_cnetlib(byte_t *data, size_t len) {
	//	CNetLib::log("Got ",len," bytes");
	this->s_importer.set_data(data);
	CN_MSIZE_T info_len = 0;
	CN_MSIZE_T data_len = 0;

	//Copy packet to other channel participants
	CN::Channel *parent = this->parent_channel;
	bool hosting = (this->parent_channel != nullptr);
	if(hosting) {
		for(auto &c : parent->participants) {
			if(!c or c == this) continue;
//			CNetLib::log("Routing ",len," bytes to ",c->getname());
			c->send(&this->s_importer,len);
		}
	}

	if(this->reading == false) {

		if(len < CN_HEADER_SIZE()) {
			CNetLib::log(this->getname(),": Terminating, malformed message");
			return this->graceful_disconnect();
		}

		//Unpack basic header data (type,length)
		DataType n_type = (DataType)s_importer.get_auto<CN_MTYPE_T>();
		info_len = s_importer.get_auto<CN_MSIZE_T>();
		data_len = s_importer.get_auto<CN_MSIZE_T>();

//		CNetLib::log("Got message of type ",(int)n_type," along with ",info_len," info bytes and ",data_len," data bytes");
		//Ignore some messages depending on
		// the current state of the connection
//		bool proceed = true;
//		CNetLib::log("Processing insecure data");
		std::string s((const char*)data);
		CNetLib::log(s);
		bool proceed = this->precheck_message_type(n_type);
		if(!proceed) {
			CNetLib::log(this->getname(),": Terminating, suspicious activity");
			return this->graceful_disconnect();
		}

		//Set message type
		this->cur_msg->init_transfer(data_len);
		this->cur_msg->type = (CN_MTYPE_T)n_type;

		//Internally handle system messages
		//Depending on the type, we may:
		// Reset transmission state without dispatching any user messages
		// Begin reading data into the current user message
		//
		switch(n_type) {
			case CN::DataType::VC_QUERY: {
				std::string challenge = s_importer.get_str(VC_QUERY_LEN);
//				CNetLib::log("Got challenge: ",challenge,(this->incoming)? " (from client)" : " (from server)");
				this->send_info(DataType::VC_RESP,this->owner->make_validation_hash(challenge));
//				CNetLib::log("Sent response: ",this->owner->make_validation_hash(challenge),(this->incoming)? " (to client)" : " (to server)");
				return this->reinit_or_reset_transfer(info_len,data,len);
			}
			case CN::DataType::VC_RESP: {
				if(!CNetLib::contains(this->owner->validation_challenges,this->id)) {
//					CNetLib::log("Error: No validation challenge",(this->incoming)? " (for client)" : " (for server)");
					return this->reinit_or_reset_transfer(info_len,data,len);
				}
				std::string resp = s_importer.get_str(VC_RESP_LEN);
				this->check_validation_response(resp);
				return this->reinit_or_reset_transfer(info_len,data,len);
			}
			case CN::DataType::VC_VALID: {
				this->remote_validated = true;
				this->remote_validation->wake_all();
				return this->reinit_or_reset_transfer(info_len,data,len);
			}
			case CN::DataType::CLOSE:
				return this->graceful_disconnect(); //Deletes this object
			case CN::DataType::FILE: {
				std::string filename = s_importer.get_str();
				this->cur_msg->f_name = filename;
				CNetLib::log(this->getname(),": Receiving file ",this->cur_msg->f_name," (",CNetLib::conv_bytes(data_len),")");
				if(this->is_empty_init(info_len,len)) {
					CNetLib::log("Notice: Empty init message for type ",(CN_MTYPE_T)n_type);
					this->reading = true;
					return;
				}
				break;
			}
			case CN::DataType::STREAM_INIT: {
				std::string filename = s_importer.get_str();
				this->cur_msg->f_name = filename;
				CNetLib::create_file("./received/"+filename);
				this->stream_info.insert({filename,data_len});	//Take total stream size from unused header info
				CNetLib::log(this->getname(),": Initializing stream for ",filename," (",CNetLib::conv_bytes(data_len),")");

				if(this->is_empty_init(info_len,len)) {
					CNetLib::log("Notice: Empty init message for type ",(CN_MTYPE_T)n_type);
					this->reading = true;
					return;
				}
				break;

				//We can skip the rest of this packet
				// because in the current stream implementation we send the
				// init message separately from the first data message,
				// whereas in the basic file implementation we send the init and data
				// together in one message
				//Bug: It's possible that
				// The first data message is squashed into this message, in which case
				// we would ignore a partial fragment

//				this->reading = true;
//				return;
			}
			case CN::DataType::CHAN_CREATE: {
				std::string uid = s_importer.get_str(4);
//				CNetLib::log("Creating channel with uid ",uid);
				this->owner->register_channel(this,uid); //Create a new channel with base connection this
				return this->reinit_or_reset_transfer(info_len,data,len);
			}
			case CN::DataType::CHAN_QUERY: {
				std::string uid_list = "";
				for(auto &k : CN::channel_map) {
					uid_list += k.first;
				}
				this->package_and_send(CN::DataType::CHAN_LIST,uid_list); //Create a new channel with base connection this
				return this->reinit_or_reset_transfer(info_len,data,len);
			}
			case CN::DataType::CHAN_JOIN: {
				std::string uid = s_importer.get_str(4);
				if(!CNetLib::contains(CN::channel_map,uid)) {
					CNetLib::log("CHAN_JOIN: Channel ",uid," not found");
					return this->reinit_or_reset_transfer(info_len,data,len);
				}
				CN::Channel *chan = CN::channel_map.at(uid);
				chan->participants.push_back(this); //Add this connection to the specified channel
				this->parent_channel = chan;
				CNetLib::log(this->getname(),": Joined ",chan->getname());
				return this->reinit_or_reset_transfer(info_len,data,len);
			}
			case CN::DataType::CHAN_ATTACH: {
				std::string uid = s_importer.get_str(4);
				CN::Channel *target = CN::get_channel(uid); //Find channel with uid
				if(target) {
					this->parent_channel = target;
					CNetLib::log("Attached ",this->getname()," to ",target->getname());
				} else
					CNetLib::log("Unable to find channel ",uid);
				return this->reinit_or_reset_transfer(info_len,data,len);
			}
			default:
//				CNetLib::log("Unknown or user data type ",(int)n_type);
//				return this->reset_transfer();
				break;
		}
		this->reading = true;
	}
	if(this->reading) {

//		if(len == 0) return;

		//Determine whether to retrieve a full chunk or only the
		// remaining data in this transfer
		// We probably don't need to do this
		size_t len_estimate = std::min(this->cur_msg->bytes_left,len);


		//Retrieve data from packet
		size_t actual_len = s_importer.get_data(new_chunk,len_estimate);


		//new_chunk now contains the body of the new packet
		//If we are receiving a stream, we could append the stream fragments here
		// This solution would avoid having to re-send the header data with each fragment

		switch(this->cur_msg->type) {
			case (int)CN::DataType::STREAM_INIT: {
				//Append data directly to file without copying into current message
//				CNetLib::log("Got stream fragment: ",len," bytes");
				CNetLib::append_to_file("./received/"+this->cur_msg->f_name,new_chunk,actual_len);
				if(this->cur_msg->import_data_size(actual_len)) {
					CNetLib::log("Stream finished for ",this->cur_msg->f_name);
					return this->dispatch_msg();
				}
				this->peek_msg(); //Expose stream progress to user callback
				break;
			}
			default:
				if(this->cur_msg->import_data(new_chunk,actual_len))
					this->dispatch_msg();
		}
	}
}

void CN::Connection::process_data_http(byte_t *data, size_t len) {
	CNetLib::log("Processing insecure data");
	std::string s1((const char*)data);
	CNetLib::log(s1);
}

void CN::Connection::reinit_or_reset_transfer(size_t info_len,byte_t *data, size_t packet_len) {
	size_t bytes_read = this->s_importer.data_get_ptr - this->s_importer.data - CN_HEADER_SIZE();
	if(info_len and bytes_read != info_len) {
		CNetLib::log("Warning: Info size mismatch: ",bytes_read," bytes");
	}
//	CNetLib::log("Attempting reinit with info_len ",info_len,", packet_len ",packet_len);
	if(packet_len > info_len+CN_HEADER_SIZE()) {
		//Info Packet is larger than anticipated message size
		// Meaning this packet contains multiple, potentially incomplete messages
		CNetLib::log(this->getname(),": Reading squashed packet (",packet_len-CN_HEADER_SIZE()-info_len," bytes)");
		//Process next message in packet
		// Offset data pointer by the size of the message we just read
		return this->process_data(data+info_len+CN_HEADER_SIZE(),packet_len-info_len-CN_HEADER_SIZE());
	}
//	CNetLib::log("Resetting transfer");
	return this->reset_transfer();
}

//Validation stuff

std::string CN::Connection::get_expected_validation_hash() {
	if(!CNetLib::contains(this->owner->validation_challenges,this->id)) CNetLib::log("Error: No hash for this connection");
	return this->owner->validation_challenges.at(this->id);
}

void CN::Connection::check_validation_response(std::string actual_response) {
	std::string expected_response = this->get_expected_validation_hash();
	if(actual_response != expected_response) {
		CNetLib::log(this->getname(), ": Hash validation failed");
		CNetLib::log("Expected: ",expected_response," Got: ",actual_response);
	} else {
		CNetLib::log(this->getname(),": Validation successful");
		this->validated = true;
		this->send_info(CN::DataType::VC_VALID);
	}
}

//Data transmission


size_t CN::Connection::send(serializer *s) {
	size_t written;
	try {
		written = asio::write(*this->sock,asio::buffer(s->data,s->true_size()));
	} catch(const std::exception &e) {
		CNetLib::print(this->getname(), ": Error writing to socket");
		this->disconnect();
	}
	return written;
}

size_t CN::Connection::send(serializer *s, size_t len) {
	size_t written;
	try {
		written = asio::write(*this->sock,asio::buffer(s->data,len));
	} catch(const std::exception &e) {
		CNetLib::print(this->getname(), ": Error writing to socket");
		this->disconnect();
	}
	return written;
}

size_t CN::Connection::send(PackedMessage *m) {
	size_t written;
	try {
		written = asio::write(*this->sock,asio::buffer(m->s.data,m->s.true_size()));
	} catch(const std::exception &e) {
		CNetLib::print(this->getname(), ": Error writing to socket");
		this->disconnect();
	}
	return written;
}

size_t CN::Connection::send_info(DataType type) {

	PackedMessage m = PackedMessage((CN_MTYPE_T)type);

	size_t written = this->send(&m);

	return written;
}

size_t CN::Connection::send_info(CN_MTYPE_T type) {

	PackedMessage m = PackedMessage(type);

	size_t written = this->send(&m);

	return written;
}

size_t CN::Connection::send_info(DataType type, std::string info) {
	//Sends info as info, not as data
	PackedMessage m = PackedMessage((CN_MTYPE_T)type,info.size());
	m.s.add_str_auto(info);

	size_t written = this->send(&m);

	return written;
}


size_t CN::Connection::package_and_send(std::string data) {

	PackedMessage m = PackedMessage(DataType::TEXT,data.size(),data.size());
	m.s.add_str_auto(data);

	size_t written = this->send(&m);

	return written;
}

size_t CN::Connection::package_and_send(byte_t *data, CN_MSIZE_T len) {

	PackedMessage m = PackedMessage(DataType::TEXT,0ull,len);
	m.s.add_data(data,len);

	size_t written = this->send(&m);

	return written;
}

bool CN::Connection::stream_file(std::string path, std::string filename) {
	size_t written = 0;

	if(!CNetLib::file_exists(path)) {
		CNetLib::log("stream_file_with_name: file not found");
		return false;
	}

	std::ifstream _file(path,std::ios::binary);

	//Seek to EOF and check position in stream
	_file.seekg(0,_file.end);
	size_t _size = _file.tellg();
	_file.seekg(0,_file.beg);

	PackedMessage m = PackedMessage(CN::DataType::STREAM_INIT,256ull,_size);
	m.s.add_str(filename);
	this->send(&m);

	byte_t *s_data = (byte_t*)malloc(CN_BUFFER_SIZE);

	int data_idx = 0;
	while(_file >> std::noskipws >> s_data[data_idx++]) {
		 if(data_idx == CN_BUFFER_SIZE) {
			 serializer s_body = serializer(CN_BUFFER_SIZE);
			 s_body.add_data(s_data,CN_BUFFER_SIZE);
			 written += this->send(&s_body);
			 data_idx = 0;
		 }
	}

	//Send the last chunk (< CN_BUFFER_SIZE)
	serializer s_last = serializer(CN_BUFFER_SIZE);
	s_last.add_data(s_data,data_idx);
	written += this->send(&s_last);

	free(s_data);
	_file.close();

	return written;
}

size_t CN::Connection::package_and_send(DataType type, std::string data) {

	PackedMessage m = PackedMessage(type,0ull,data.size());
	m.s.add_str_auto(data);

	size_t written = this->send(&m);

	return written;
}

size_t CN::Connection::send_file_with_name(std::string path,std::string filename) {
	byte_t *file_;
	size_t len = CNetLib::import_file(path,file_);

	PackedMessage m = PackedMessage(DataType::FILE,256ull,len);
	m.s.add_str(filename);
	m.s.add_data(file_,len);

	size_t written = this->send(&m);
	return written;
}

size_t CN::Connection::package_and_send(CN_MTYPE_T type, std::string data) {

	PackedMessage m = PackedMessage(type,0ull,data.size());
	m.s.add_str_auto(data);

	size_t written = this->send(&m);

	return written;
}

//Expose message to user-defined message handlers

void CN::Connection::dispatch_msg() {
	if(this->validated) {
		this->owner->call_message_handler(std::move(this->cur_msg));
	} else {
		CNetLib::print("Ignoring ",this->cur_msg->size," bytes from unvalidated connection ");
	}
	this->reset_transfer();
}

void CN::Connection::peek_msg() {
	if(this->validated) {
		this->owner->call_message_handler(this->cur_msg);
	} else {
		CNetLib::print("Ignoring ",this->cur_msg->size," bytes from unvalidated connection ");
	}
}

//Disconnect both ends of the connection
void CN::Connection::graceful_disconnect() {
	this->owner->graceful_disconnect(this);
}

//NetObj

CN::Channel *CN::NetObj::register_channel(Connection *nc, std::string id) {
	CN::Channel *new_channel = [&]() {
		if(id == "none")
			return new CN::Channel(nc,CNetLib::random_hex_string(4));
		else
			return new CN::Channel(nc,id);
	}();
	this->channels.push_back(new_channel);
	this->call_channel_handler(new_channel);
	CN::channel_map.insert({new_channel->uid,new_channel});
	return new_channel;
}

CN::Connection* CN::NetObj::register_connection(tcp::socket *new_sock,bool is_incoming, bool is_insecure) {
	//Registers a connection to this tcp socket
	CN::Connection *nc = new CN::Connection(new_sock);
	nc->owner = this;
	nc->active = true;
	nc->incoming = is_incoming;	//Indicate if connection is outgoing or incoming
	nc->secure = !is_insecure;

	this->connections.push_back(nc);
	CNetLib::log(nc->getname(),": Registered");

	//Create validation challenge
//	CNetLib::log("Creating new validation challenge");
	nc->start_handler();
	if(nc->secure == true) {
		std::string new_challenge = this->generate_validation_challenge();
		std::string new_hash = this->make_validation_hash(new_challenge);
		this->validation_challenges.insert({nc->id,new_hash});
		//Wait for remote validation
		nc->send_info(DataType::VC_QUERY,new_challenge);
	//	CNetLib::log("Sent validation query ",(nc->incoming)? " (to client)" : " (to server)");
		if(nc->remote_validation->wait(this->timeout)) {
			if(nc and nc->active) CNetLib::log(nc->getname(),": Terminating, failed validation timeout (",this->timeout,"ms)");
			this->graceful_disconnect(nc); //Close and unregister
			return nullptr;
		} else {
			//Only expose valid connections to user callbacks
			this->call_connection_handler(nc);
		}
	} else {
		//Skip validation
//		nc->start_handler();
		CNetLib::log("Registered insecure connection");
		nc->validated = true;
		nc->remote_validated = true;
		this->call_connection_handler(nc);
	}
	return nc;
}

void CN::NetObj::remove_connection(Connection *cn) {
	if(!CNetLib::contains(this->connections,cn)) return;
	int t_index = 0;
	for(int i=0;i<this->connections.size();i++) {
		CN::Connection *c = this->connections.at(i);
		if(c == cn) {
			t_index = i;
			break;
		}
	}
	this->connections.erase(this->connections.begin()+t_index);
	//	delete cn;
}

void CN::NetObj::remove_channel(std::string uid) {
	if(!CNetLib::contains(CN::channel_map,uid)) return;
	CN::Channel *cn = CN::channel_map[uid];
	CN::channel_map.erase(uid);

	delete cn;
}

void CN::NetObj::graceful_disconnect(Connection *c) {
	if(c and c->active) {
		c->send_info(CN::DataType::CLOSE);
		c->disconnect();
		this->remove_connection(c);
	}
}

std::string CN::NetObj::make_validation_hash(std::string input) {
	if(input.size() == 0) return "88888888";

	//Constants
	const byte_t c1 = 0x8aU+CN_PROTOCOL_VERS;
	const byte_t c2 = 0x2bU+sizeof(CN_MTYPE_T);
	const byte_t c3 = 0xa7U+sizeof(CN_MSIZE_T);
	const byte_t c4 = 0xE3U+CN_HEADER_SIZE();

	const unsigned short affix = input[input.size()/2];

	size_t hash = c3; //32 bit int = 8 char hex

	//Should be at least enough for the resulting hex to be 8 chars
	const short bit_min = 4*(VC_RESP_LEN-1);
	const short bit_max = 4*(VC_RESP_LEN);
	const size_t _min = pow(2,bit_min);
	const size_t _max = pow(2,bit_max)-1;
	const size_t _target = (_min+_max)/2;

	const short steps = VC_RESP_LEN;

	auto hash_step = [&](size_t &_hash, const std::string _input,const int _it) {
		_hash = _hash ^ (input[_it%input.size()]) ^ (1+affix);
		_hash *= (_it%4==0) ? (c1) : (_it%3 == 0) ? (c2) : (_it%2==0) ? (c3) : (c4);
	};

	int i=0;
	while(hash < _target) {
		hash_step(hash,input,i);
		i++;
	}
	std::stringstream hexout;
	hexout << std::hex << hash;
	return hexout.str().substr(hexout.str().size()-VC_RESP_LEN,VC_RESP_LEN);
}
