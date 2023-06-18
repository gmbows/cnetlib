#pragma once

#include <cstdint>
#include <cstdlib>
#include <vector>
#include <cstring>
#include <thread>

#include "cnetlib4_common.hpp"
#include "message.h"

#include <gcutils/gcutils.h>

#include <asio.hpp>

using tcp = asio::ip::tcp;
namespace cn {

//Utils


extern unsigned g_num_connections;

struct NetStats {
	size_t total = 0;
	void log(size_t transfer) {
		this->total += transfer;
	}
	size_t check() {
		size_t s = total;
		this->total = 0;
		return s;
	}
};

struct NetInterface {
	NetStats stats;
	struct Connection *parent;
	NetInterface(const NetInterface &other) {
		this->stats = other.stats;
		this->parent = other.parent;
	}
	NetInterface(Connection *p): parent(p) {}
	NetInterface(){}
};

struct NetReader : public NetInterface {

	byte_t netbuf[CNET_BUFSIZE_DEFAULT];
	bool reading_message = false;

	MessageHeader header;
	Serializer body;

	NetReader(NetReader &&other) {
		this->body = std::move(other.body);
		this->parent = other.parent;
		other.parent = nullptr;
	}

	void start_read();
	void read_header(const asio::error_code &err, size_t transmitted);
	void read_body(const asio::error_code &err, size_t transmitted);

	NetworkMessage getMessage() {return std::move(NetworkMessage(this->header,this->body));}

	void init_transfer(MessageHeader &h);
	size_t remaining() {return this->header.size - this->body.size;}
	void reset() {
		this->header.invalidate();
		this->body.empty();
		this->reading_message = false;
	}

	std::string getInfo();

	NetReader(Connection *p): NetInterface(p) {

	}
};

struct NetWriter : public NetInterface {
	std::queue<NetworkMessage> messages;
	std::mutex mtx;
	std::condition_variable waiter;

	void await_write();
	void write_header_async(NetworkMessage &msg);
	void write_body_async(NetworkMessage&,const asio::error_code&,size_t);
	size_t write_header(const NetworkMessage &msg,asio::error_code&);
	size_t write_body(const NetworkMessage &msg,asio::error_code&);
	size_t send_sync(const NetworkMessage &msg);
	void queueMessage(NetworkMessage &msg);

	std::string getInfo();

	NetWriter(Connection *p): NetInterface(p) {
		std::thread(&NetWriter::await_write,this).detach();
	}
};

struct Waiter {
	std::condition_variable v;
	std::mutex mtx;
	bool awakened = false;
	void try_wait() {
		if(this->awakened) return;
		std::unique_lock<std::mutex> lk(mtx);
		v.wait(lk);
	}
	void awaken() {
		this->v.notify_all();
		this->awakened = true;
	}
};

struct Connection {
	tcp::socket *socket;
	struct NetObject *owner;
	std::string address;
	unsigned id;

	gcutils::TaskManager task_manager;

	//Transmission stuff
	NetReader net_reader;
	NetWriter net_writer;

	std::map<MSG_UIDTYPE_T,std::function<void(NetworkMessage&)>> callbacks;
	std::map<MSG_UIDTYPE_T,Waiter&> await_map;

	void register_callback(MSG_UIDTYPE_T uid,std::function<void(NetworkMessage&)> cb) {
//		gcutils::print("Registered callback for ",uid);
		this->callbacks.insert({uid,cb});
	}
	void remove_callback(MSG_UIDTYPE_T uid) {
		gcutils::remove(callbacks,uid);
	}
	bool has_callback(MSG_UIDTYPE_T uid) {
		return gcutils::contains(this->callbacks,uid);
	}
	bool call_callback(MSG_UIDTYPE_T uid) {
		if(this->has_callback(uid)) {
//			gcutils::print("Calling callback for ",uid);
			auto m = NetworkMessage();
			this->callbacks.at(uid)(m);
			this->remove_callback(uid);
			return true;
		}
		return false;
	}
	bool call_callback(MSG_UIDTYPE_T uid,NetworkMessage &msg) {
		if(this->has_callback(uid)) {
//			gcutils::print("Calling callback for ",uid);
			this->callbacks.at(uid)(msg);
			this->remove_callback(uid);
			return true;
		}
		return false;
	}

	void register_await(MSG_UIDTYPE_T uid,Waiter &w) {
		gcutils::print("Registered wait for ",uid);
		this->await_map.insert({uid,w});
	}
	void remove_await(MSG_UIDTYPE_T uid) {
		gcutils::remove(await_map,uid);
	}
	bool has_await(MSG_UIDTYPE_T uid) {
		return gcutils::contains(this->await_map,uid);
	}
	bool trigger_awaken(MSG_UIDTYPE_T uid) {
		if(this->has_await(uid)) {
//			gcutils::print("Triggering awaken for ",uid);
			this->await_map.at(uid).awaken();
			return true;
		}
//		cthrow(ERR_WARN,"Await not registered");
		return false;
	}

	//Writing
	void queued_send(NetworkMessage &msg, std::function<void(NetworkMessage&)> callback) {

		//Set the callback to execute when the remote host finishes processing this message
		msg.set_needs_response();
		this->register_callback(msg.uid(),callback);
		this->queued_send(msg);
	}
	void send_sync_callback(NetworkMessage &msg, std::function<void(NetworkMessage &)> callback) {
		//Set the callback to execute when the remote host finishes processing this message
		msg.set_needs_response();
		this->register_callback(msg.uid(),callback);
		this->send_sync(msg);
	}

	//Message handling overview:
	// There are a few options:
	// 1. Message is handled internally (TEXT,FILE,etc.). Freed after handling
	// 2. Message is handled by a user defined handling function. Freed after handling
	// 3. Message is sent with a nular callback. Message is sent expecting a response,
	//		and the callback is executed when the response is handled. Used when we only
	//		care that the server has finished processing the message, and we don't expect any results.
	// 4. Message is sent with a unary callback that expects the response message to be passed to it.
	//		When the response message is received, send it to the callback, then free it.
	// 5. Message is sent without a callback, and is expected to be returned to the sender.
	//		Sent message with a program-defined unary callback that copies the message locally.
	//		Wait on the response. Waiter will be signaled after the callback has run and returned
	//		the response to the function.
	// 6. Message is sent awaiting a response, and the response is discarded.
	//

	//=====================================================================
	//All messages must pass through one of these functions to be validated
	//=====================================================================

	void queued_send(NetworkMessage &msg) {
		msg.finalize();
		this->net_writer.queueMessage(msg);
		//Message is freed on the other end
	}
	void send_sync(NetworkMessage &msg) {
		msg.finalize();
		this->net_writer.send_sync(msg);
		free(msg.body);
	}

	//=====================================================================
	//						Default sending method
	//=====================================================================

	NetworkMessage send_for_response(NetworkMessage &msg) {
		MSG_UIDTYPE_T uid = msg.uid();
		NetworkMessage response = NetworkMessage();
		Waiter w;
		auto callback = [&response](NetworkMessage &msg) {
			gcutils::print("Setting local copy to response");
			response = msg;
		};
		this->register_await(uid,w);
		this->task_manager.add_task(
			std::bind(&Connection::send_sync_callback,this,msg,callback)
		);
		w.try_wait();
		this->remove_await(uid);
		return response;
	}

	void send_and_await(NetworkMessage &msg) {
		MSG_UIDTYPE_T uid = msg.uid();
		msg.set_needs_response();
		Waiter w;
		this->register_await(uid,w);
		this->task_manager.add_task(
			std::bind(&Connection::send_sync,this,msg)
		);
		w.try_wait();
		this->remove_await(uid);
	}

	void send(NetworkMessage &msg, std::function<void(void)> callback) {
		//Default sending method with callback
		this->task_manager.add_task(
			std::bind(&Connection::send_sync_callback,this,msg,
				[callback](NetworkMessage&) {
					//Ignore message and call nular callback
					callback();
				})
		);
	}
	void send(NetworkMessage &msg, std::function<void(NetworkMessage&)> callback) {
		//Default sending method with callback
		this->task_manager.add_task(
			std::bind(&Connection::send_sync_callback,this,msg,callback)
		);
	}
	void send(NetworkMessage &msg) {
		this->task_manager.add_task(
			std::bind(&Connection::send_sync,this,msg)
		);
	}

	//=====================================================================

	void send_str(const std::string &str);
	void send_file_with_name(const std::string &path, const std::string &filename);
	void stream_file_with_name(const std::string &path, const std::string &filename);
	void send_file_with_name_async(const std::string &path, const std::string &filename) {
		this->task_manager.add_task(std::bind(&Connection::send_file_with_name,this,path,filename));
	}
	void stream_file_with_name_async(const std::string &path, const std::string &filename) {
		this->task_manager.add_task(std::bind(&Connection::stream_file_with_name,this,path,filename));
	}

	//Reading
	void dispatchMessage(NetworkMessage&);
	void handleMessage(NetworkMessage&);

	//Closing
	void disconnect();
	void closeSocket();

	//Utility
	std::string getInfo();

	Connection(tcp::socket *_sock,NetObject *_owner);
};


struct NetObject {
protected:
	asio::io_context io_context;
	bool is_server = false;

	std::vector<struct CommunicationChannel*> channels;


	//Handlers
	std::map<int,std::function<void(Connection*,NetworkMessage&)>> message_handlers;
	std::function<void(Connection*)> connection_handler = [this](Connection *c) {
		gcutils::log(this->getInfo(),": Got connection from ",c->address);
	};
	std::function<void(CommunicationChannel*)> channel_handler;
	Connection* register_connection(tcp::socket *sock) {
		if(sock->is_open()) {
			auto nc = new Connection(sock,this);
			this->connection_handler(nc);
			this->connections.push_back(nc);
			return nc;
		}
		return nullptr;
	}
	CommunicationChannel *register_channel(CommunicationChannel*);
public:
	bool channel_exists(MSG_UIDTYPE_T cid);
	CommunicationChannel* find_channel(struct Connection *c);
	CommunicationChannel* find_channel(MSG_UIDTYPE_T cid);
	CommunicationChannel* get_channel(int idx) {return this->channels.at(idx);}
	CommunicationChannel *create_channel(Connection *control);
	bool server() {
		return this->is_server;
	}
	void start_io_thread() {
		if(this->io_context.stopped() == false) {
//			gcutils::log("IO Thread already running");
			return;
		}
		this->io_context.restart();
		std::thread([this]() {
			gcutils::log("IO Thread starting");
			this->io_context.run();
			gcutils::log("IO Thread exiting");
		}).detach();
	}
	asio::io_context &ioctx() {return this->io_context;}
	void restart_ioctx() {this->io_context.restart();}
	void setMessageHandler(int id,std::function<void(Connection*,NetworkMessage&)> handler) {
		this->message_handlers.insert({id,handler});
	}
	void setConnectionHandler(std::function<void(Connection*)> handler) {
		this->connection_handler = handler;
	}
	void setChannelHandler(std::function<void(CommunicationChannel*)> handler) {
		this->channel_handler = handler;
	}
	void invoke_handler(Connection *c,NetworkMessage &msg) {
		auto handler = this->message_handlers.at(msg.header.type);
		handler(c,msg);
	}
	bool hasMessageHandlerFor(int id) {
		return gcutils::contains(message_handlers,id);
	}
	std::vector<Connection*> connections;
	void disconnect(Connection* c) {
		gcutils::remove(this->connections,c);
	}
	std::string getDirection() {
		return (this->is_server) ? "i":"o";
	}
	std::string getType() {
		return (this->is_server) ? "server":"client";
	}
	std::string getInfo() {
		return this->getType();
	}
	NetObject();
};
class Server : public NetObject {
	bool io_thread_running = false;
	short port;
	tcp::acceptor acceptor;
	void open_server() {
		gcutils::log("Starting server");
		asio::ip::tcp::endpoint endpoint(asio::ip::tcp::v4(), port);
		this->acceptor.open(endpoint.protocol());
		this->acceptor.bind(endpoint);
		this->acceptor.listen();
	}
	void start_io() {
		this->start_accept();
		this->start_io_thread();
	}
	[[nodiscard]] inline const bool is_open() const {return this->acceptor.is_open();}
	void handle_connection(tcp::socket *new_sock) {
		this->register_connection(new_sock);
		this->start_accept();
	}
	void start_accept() {
		auto *new_sock = new tcp::socket(this->io_context);
		this->acceptor.async_accept(*new_sock,std::bind(&Server::handle_connection,this,new_sock));
	}
public:
	bool active() {
		return this->io_context.stopped() == false and this->is_open();
	}
	void start() {
		if(this->is_open() == false) {
			this->open_server();
		 } else {
			gcutils::log("Server already started");
		}
		this->start_io();
	}
	void stop() {
		gcutils::log("Stopping server");
		this->acceptor.cancel();
		this->io_context.stop();
		if(this->acceptor.is_open()) this->acceptor.close();
	}
	Server(short _port): port(_port),
		acceptor(tcp::acceptor(this->io_context)) {
		this->is_server = true;
		gcutils::log("Created server on port ",port);
	}
	~Server() {
		gcutils::log("Destroying server");
	}
};
class Client : public NetObject {
	short port;
	tcp::resolver resolver;
public:
	struct CommunicationChannel* open(std::string address);
	Connection* connect(std::string address) {
		if(address.empty()) address = "127.0.0.1";
		auto new_sock = new tcp::socket(this->io_context);
		try {
			const auto endpoint = this->resolver.resolve(address,std::to_string(this->port));
			asio::connect(*new_sock,endpoint);
		} catch (const std::exception &e) {
			gcutils::log("Connection to ",address," failed");
			return nullptr;
		}
		auto nc = this->register_connection(new_sock);
		return nc;
	}
	Client(short _port): port(_port),
		resolver(this->io_context) {
		this->start_io_thread();
		gcutils::log("Created new client");
	}
};

struct ConnectionPool {
	unsigned size;
	unsigned idx = 0;
	struct CommunicationChannel *host_channel;
	std::vector<Connection*> connections;
	void remove(Connection *c) {
		gcutils::remove(this->connections,c);
	}
	size_t get_size() const {
		return this->connections.size();
	}
	Connection* get() {
		//Round robin
		auto c = this->connections.at(idx);
		this->idx++;
		this->idx %= this->connections.size();
//		gcutils::log("Round robin selected connection ",c->getInfo());
		return c;
	}
	void add(Connection *c) {
		this->connections.push_back(c);
	}
	ConnectionPool(CommunicationChannel *_host_channel,unsigned _size): host_channel(_host_channel),size(_size) {

	}
};

struct CommunicationChannel {
	NetObject *host;
	Connection *control = nullptr;
	ConnectionPool connection_pool;
	CHAN_IDTYPE_T identifier = 0;
	bool valid() {return this->control != nullptr;}
	bool has_connection(Connection *c) {
		return this->control == c or gcutils::contains(this->connection_pool.connections,c);
	}
	void register_data_connection(Connection *c) {
		auto msg = NetworkMessage(REQ_JOIN_CHANNEL);
		msg << this->identifier;
		c->send(msg);
		this->add_data_connection(c);
	}
	void add_data_connection(Connection *c) {
		this->connection_pool.add(c);
	}
	std::string control_address() const {
		return this->control->address;
	}
	Connection* get() {
		return this->connection_pool.get();
	}
	bool can_initiate() {return this->host->server() == false;}
	void connect(const std::string &addr) {
		if(this->can_initiate()) {
			auto nc = static_cast<Client*>(this->host)->connect(addr);
			if(nc) {
				this->control = nc;
				auto msg = NetworkMessage(REQ_CREATE_CHANNEL);
				this->control->queued_send(msg);
			} else {
				gcutils::log("Connection to ",addr," failed");
			}
		}
	}
	size_t size() {
		return this->connection_pool.get_size();
	}
	std::string getInfo() {
		return "cc"+std::to_string(size())+"."+this->control->getInfo();
	}
	CommunicationChannel(NetObject *_host,Connection* _control,unsigned _size = 4): host(_host), control(_control), connection_pool(this,_size) {
		if(this->can_initiate()) {
			cthrow(ERR_FATAL,"Calling server constructor from client");
		}
		static auto s = [](){srand(time(NULL));return 0;}();
		this->identifier = gcutils::get_uid<CHAN_IDTYPE_T>();
	}
	CommunicationChannel(NetObject *_host,const std::string &addr,unsigned _size = 4): host(_host), connection_pool(this,_size) {
		this->connect(addr);
	}
};

};
