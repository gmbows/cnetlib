#include "cnetlib.h"

#include "serializer.h"

#include <string>

std::map<int,CN_Channel*> channelMap;

asio::io_context io_context;
std::string packet_header = "%%packet_header%%";

namespace CNetLib {

	unsigned int numConnections = 0;
	unsigned int numChannels = 0;

	PacketHandler default_packet_handler = [](CN_Packet *msg) {
		print("(MessageHandler ",msg->owner->id,")["+msg->owner->address+"]: ",msg->content," (",msg->len," bytes)");
	};
	ConnectionHandler default_connection_handler = [](CN_Connection *cn) {
//		print("(MessageHandler ",msg->owner->id,")["+msg->owner->address+"]: ",msg->content," (",msg->len," bytes)");
		print("Got connection");
	};
	ChannelHandler default_channel_handler = [](CN_Channel *chan) {
		print("Created new channel");
	};
	std::vector<CN_Channel*> channels;
	CN_Channel* get_channel(int id) {
		for(auto &c : CNetLib::channels) {
			if(c->id == id) return c;
		}
		print("Channel with id ",id," not found");
		return nullptr;
	}
	CN_Channel* get_channel(CN_Connection *cn) {
		for(auto &chan : CNetLib::channels) {
			if(chan->base_connection == cn) return chan;
			for(auto &connection : chan->connections) {
				if(cn == connection) {
					print("Found connection!");
					return chan;
				}
			}
		}
		return nullptr;
	}
}

std::stringstream& operator<<(std::stringstream &s, CN_DataType &t) {
	s << t;
	return s;
}

CN_Connection::CN_Connection(tcp::socket *_sock): id(++CNetLib::numConnections), active(false), validated(false) {
	this->packet_handler = CNetLib::default_packet_handler;
	this->sock = _sock;
	this->address = get_address(_sock);
	this->reader = CN_NetReader(0);
}

void CN_Connection::handle_connection() {
	//Buffer for reading
	unsigned char data[CN_BUFFER_SIZE];
	clear_buffer((char*)data,CN_BUFFER_SIZE);
	
    std::vector<char> vdata;
	
    asio::error_code error;
	
    while(!error and this->active) {
		//block until read
		size_t len = this->sock->read_some(asio::buffer(data,CN_BUFFER_SIZE), error);

		print("Received data packet (",len," bytes)");

		//Potentially incomplete message or containing multiple messages
		this->handle_data(data,len);
		if(this->validated == false) {
			std::string junk((char*)data);
			print("Warning: Received insecure packet: ",junk,"(",junk.size()," bytes)");
		}
		clear_buffer(data,CN_BUFFER_SIZE);
    }
	print("Connection ",this->id," closed (",error.message(),")");
	this->active = false;
	// this->close();
}

void CN_Connection::generate_packet(unsigned char* data,size_t len) {
	CN_Packet *msg = new CN_Packet(this,data,len);
	msg->owner = this;
	msg->remote = true;
	msg->type = this->reader.type;
	//internal processing

	this->handle_packet(msg);
}

void CN_Connection::handle_data(unsigned char* data,size_t len) {

//	print("Received packet",data);

	if(this->reader.read(data,len)) {
		this->messages_received++;
		//Skip body of validation messages
		if(this->validated == false) {
			this->validated = true;
//			print("Validated connection");
//			this->reader.reset();
		}
		this->generate_packet(this->reader.data.data(),(size_t)this->reader.data.size());
		this->reader.reset();
	}

}

bool CN_NetReader::read(unsigned char* _data, size_t len) {
	this->s.set_data(_data);
	switch(this->state) {
		case CN_NetState::CN_READ_HEADER: {
			std::string header = this->s.get_str(packet_header.size());
			if(header != packet_header) {
				print("Invalid packet header: ",header);
				return false;
			}
			this->type = (CN_DataType)(this->s.get_char());
			switch(this->type) {
				case CN_DataType::CN_STREAM:
				case CN_DataType::CN_FILE: {
					this->reading_filename = this->s.get_str();
					break;
				}
				case CN_DataType::CN_TEXT:
				case CN_DataType::CN_CHAN_CREATE:
					break;
				default:
					print("Unhandled data type: ",(short)type);
			}
			this->bytesRemaining = this->s.get_int();
			print("Received packet of length ",this->bytesRemaining);
			this->state = CN_NetState::CN_READ_DATA;
		}
		case CN_NetState::CN_READ_DATA:
			size_t old_chunk_len = std::min(this->bytesRemaining,len);
			unsigned char* new_chunk = (unsigned char*)malloc(old_chunk_len);
			memset(&new_chunk,old_chunk_len,'\0');
			int chunk_len = this->s.get_data(new_chunk,old_chunk_len);
			switch(this->type) {
				case CN_DataType::CN_FILE:
				case CN_DataType::CN_TEXT:
				case CN_DataType::CN_CHAN_CREATE:
					for(int i=0;i<chunk_len;++i) {
						char c = new_chunk[i];
						this->data.push_back(c);
					}
					break;
				case CN_DataType::CN_STREAM: {
					std::string fn = "./received/"+this->reading_filename;
					append_to_file(fn,new_chunk,chunk_len);
					break;
				}
				default:
					print("Implicly handling unknown packet type ",this->type);
					for(int i=0;i<chunk_len;++i) {
						char c = new_chunk[i];
						this->data.push_back(c);
					}
					break;
			}
			free(new_chunk);
			this->bytesRemaining -= chunk_len;
			if(this->bytesRemaining <= 0) {
//				print("Finished with",this->bytesRemaining);
				this->state = CN_NetState::CN_READ_HEADER;
				return true;
			} else {
//				print(this->bytesRemaining);
				return false;
			}
			break;
	}

	return false;
}

void CN_Connection::handle_packet(CN_Packet *msg) {
	this->packet_handler(msg);
	this->check_channel_creation(msg);
	delete msg;
}

size_t CN_Connection::pack_and_send(CN_DataType type, std::string data) {
	serializer s = serializer(CN_BUFFER_SIZE);
	s.add_str_auto(packet_header);
	s.add_char((char)type);
	s.add_int(data.size());
	s.add_str_auto(data);
	size_t written = this->send(&s);
	return written;
}

size_t CN_Connection::pack_and_send(CN_DataType type, unsigned char* data,size_t len) {
	serializer s = serializer(CN_BUFFER_SIZE);
	s.add_str_auto(packet_header);
	s.add_char((char)type);
	s.add_int(len);
	s.add_data(data,len);
//	size_t written = this->send(s.data,s.true_size());
	size_t written = this->send(&s);
	return written;
}

size_t CN_Connection::send(serializer *s) {
	asio::error_code error;
	if (!this->sock->is_open() or !this->active) {
		print("Error (Send()): Connection inactive");
		return -1;
	}
	size_t written = asio::write(*this->sock,asio::buffer(s->data,s->true_size()), error);
	if(error) print(error.message());
	this->messages_sent++;
	return written;
}

size_t CN_Connection::send_file_with_name(std::string filename,unsigned char* data,size_t len,bool stream) {
	serializer s = serializer(CN_BUFFER_SIZE);
	s.add_str_auto(packet_header);
	if(stream) {
		s.add_char((char)CN_DataType::CN_STREAM);
	} else {
		s.add_char((char)CN_DataType::CN_FILE);
	}
	s.add_str(filename);
	s.add_int(len);
	s.add_data(data,len);
	size_t written = this->send(&s);
	return written;
}

void CN_Server::create_channel(CN_Connection *cn,int id) {
	if(cn != nullptr) {
		CN_Channel *new_channel = new CN_Channel(cn);
		new_channel->id = id;
		print("(server) set channel id ",id);
		this->channel_handler(new_channel);
	} else {
		print("CN_Server::create_channel(): Creation unsuccessful");
	}
}

void CN_Server::accept() {

	if(this->active) {
		print("Already listening on ",this->port);
		return;
	}

	tcp::socket *sock;
	print("Server listening on port "+std::to_string(this->port));
	this->active = true;

    while(this->active) {
        try {
            sock = new tcp::socket(io_context);
			this->acceptor->accept(*sock);

			print("Received connection");

			//Create new thread to import messages from this connection
			CN_Connection *new_connection = new CN_Connection(sock);
			new_connection->id = ++CNetLib::numConnections;
			new_connection->packet_handler = this->packet_handler;
			new_connection->check_channel_creation = this->handle_packet_internal;

			new_connection->active = true;
			new_connection->secure = this->secure;

			new_connection->origin = CN_ConnectionOrigin::CN_INC;

			this->connections.push_back(new_connection);

			this->connection_handler(new_connection);
			CN_Thread(&*new_connection,&CN_Connection::handle_connection);

        } catch (const std::exception &e) {
            print(e.what());
            return;
        }
    }
	print("Server shutting down");
}

void CN_Server::start() {
	CN_Thread(this->accept_handle,this,&CN_Server::accept);
}

CN_Connection* CN_Client::connect(std::string ip) {
	tcp::socket *sock;

    try {
        sock = new tcp::socket(io_context);
        tcp::resolver::results_type endpoints = this->resolver->resolve(ip, this->port);
        asio::connect(*sock, endpoints);
    } catch(const std::exception &e) {
        print(e.what());
        return nullptr;
    }

	asio::error_code error;
	CN_Connection *new_connection = new CN_Connection(sock);

	new_connection->id = ++CNetLib::numConnections;
	new_connection->packet_handler = this->packet_handler;
	new_connection->check_channel_creation = this->handle_packet_internal;
	new_connection->active = true;
	new_connection->secure = this->secure;

	new_connection->origin = CN_ConnectionOrigin::CN_OUT;

//new_connection->send("Greetings from client");
	
	this->connections.push_back(new_connection);
	this->connection_handler(new_connection);

	CN_Thread(new_connection->handler,&*new_connection,&CN_Connection::handle_connection);
			
	return new_connection;
}

CN_Channel* CN_Client::create_channel(std::string addr) {
	CN_Connection *new_connection = this->connect(addr);
	CN_Channel *new_channel = nullptr;
	if(new_connection != nullptr) {
		int id = ++CNetLib::numChannels;
		new_channel = new CN_Channel(new_connection);
		new_channel->id = id;
		print("(client) set channel id to ",new_channel->id);
		this->channel_handler(new_channel);
		new_connection->pack_and_send(CN_DataType::CN_CHAN_CREATE,std::to_string(id)); //Sync channel ids between hosts
		return new_channel;
	} else {
		print("create_channel(): Connection unsuccessful");
	}
	return new_channel;
}

void CN_Client::create_channel(CN_Connection *cn) {
	if(cn != nullptr) {
		CN_Channel *new_channel = new CN_Channel(cn);
		this->channel_handler(new_channel);
	} else {
		print("CN_Client::create_channel(): Creation unsuccessful");
	}
}

void CN_Server::cleanup_connections() {
	// auto it = this->connections.begin();
	std::vector<CN_Connection*> new_connections;
	unsigned short i = 0;
	for(auto &cn : this->connections) {
		if(cn != nullptr and cn->active) {
			new_connections.push_back(cn);
		}
	}
	this->connections = new_connections;
}

void CN_Client::cleanup_connections() {
	// auto it = this->connections.begin();
	std::vector<CN_Connection*> new_connections;
	unsigned short i = 0;
	for(auto &cn : this->connections) {
		if(cn != nullptr and cn->active) {
			new_connections.push_back(cn);
		}
	}
	this->connections = new_connections;
}

void CN_Client::remove_connection(CN_Connection *c) {
	unsigned short i = 0;
	for(auto &cn : this->connections) {
		if((char*)cn == (char*)c) {
			this->connections.erase(this->connections.begin()+i);
		}
		i++;
	}
}

void CN_Server::remove_connection(CN_Connection *c) {
	unsigned short i = 0;
	for(auto &cn : this->connections) {
		if((char*)cn == (char*)c) {
			this->connections.erase(this->connections.begin()+i);
		}
		i++;
	}
}
