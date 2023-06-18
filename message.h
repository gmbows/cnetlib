#pragma once

#include <cstdint>
#include <vector>
#include <cstring>

#include "cnetlib4_common.hpp"
#include <gcutils/gcutils.h>

namespace cn {

//Data types

//Message types
enum MessageType : MSG_TYPE_T {
	BLANK,
	TEXT,
	FILE,
	STREAM,
	REQ_CREATE_CHANNEL, //Client requests that the server creates a new channel for this control connection
	SETID, //Set internal identifier
	REQ_CONNECTIONS, //Request new connections be made
	REQ_JOIN_CHANNEL,
	NOTIFY,	//Notify remote host that a message was processed
};

struct MessageHeader {
	MSG_SIGTYPE_T sig = CNET_HEADER_SIG_UNSIGNED();
	MessageType type;
	size_t size;

	MSG_UIDTYPE_T uid = 0;
	bool needs_response = false;
	bool is_response = false;

	MessageHeader(MessageType _type, size_t _size): type(_type), size(_size) {
		this->assign_uid();
	}
	MessageHeader(): MessageHeader(BLANK,0){
		this->assign_uid();
	}

	std::string getSize() {
		return gcutils::conv_bytes(this->size);
	}

	void assign_uid() {
		this->uid = gcutils::get_uid<MSG_UIDTYPE_T>();
	}

	void sign() {
		this->sig = CNET_HEADER_SIG();
	}

	void invalidate() {this->sig = CNET_HEADER_SIG_UNSIGNED();}

};

static_assert(sizeof(MessageHeader) < CNET_BUFSIZE_DEFAULT);

struct Serializer {

	byte_t *body;
	size_t size = 0;

	Serializer& operator=(const Serializer &other) {
//		gcutils::log("Calling Serializer copy-assignment constructor");
		this->size = other.size;
		this->body = (byte_t*)malloc(this->size);
		memcpy(this->body,other.body,this->size);
		return *this;
	}

	Serializer(const Serializer &other,std::string caller = __builtin_FUNCTION()) {
//		gcutils::log("Calling Serializer copy constructor from ",caller);
		this->size = other.size;
		this->body = (byte_t*)malloc(this->size);
		memcpy(this->body,other.body,this->size);
	}

	Serializer(Serializer &&other) {
//		gcutils::log("Calling move constructor");
		this->size = other.size;
		this->body = other.body;
		other.body = nullptr;
	}

	//Utils
	byte_t* begin() {return this->body;}
	byte_t* end() {return this->body + this->size;}
	inline bool can_extract(size_t bytes) const {return this->size >= bytes;}
	void realloc(size_t len) {
		if(len == 0) return;
		byte_t *new_data = (byte_t*)malloc(len);
		memcpy(new_data,this->body,this->size);
		free(this->body);
		this->body = new_data;
	}
	void empty() {
		this->size = 0;
//		this->start = 0;
//		this->body.resize(0);
	}

	void erase_from_front(size_t len) {
		this->body += len;
//		this->start += len;
		this->size -= len;
//		this->body.erase(this->body.begin(),this->body.begin()+len);
	}

	template <typename DataType>
	void insert_raw(const DataType *new_data,size_t len) {
		size_t new_size = this->size + len;
//		this->body.reserve(new_size);
		this->realloc(new_size);
		memcpy(this->end(),new_data,len);
		this->size = new_size;
//		gcutils::print("Imported ",len," bytes, ",this->size);
	}
	template <typename DataType>
	void extract_raw(DataType *new_data,size_t len) {
		if(this->can_extract(len) == false) cthrow(ERR_FATAL,"Seeking beyond end of data");
		memcpy(new_data,this->begin(),len);
		this->erase_from_front(len);
//		this->realloc(this->size);
//		gcutils::print("Extracted ",len," bytes, ",this->size);
	}
	std::string extract_string(size_t len) {
		byte_t s_bytes[len+1];
		this->extract_raw(s_bytes,len);
		s_bytes[len] = '\0';
		std::string s((char*)s_bytes);
		return s;
	}
	Serializer() {
		this->body = (byte_t*)malloc(MSG_BUFSIZE_DEFAULT);
	}
	~Serializer() {
//		free(this->body);
//		if(this->size > 0) cthrow(ERR_WARN,"Discarding unread data");
	}
};

//String stuff
Serializer& operator<<(Serializer &msg,const char* d);
Serializer& operator<<(Serializer &msg,const std::string &d);
void operator>>(Serializer &msg,std::string &d);


//==========
//Generics
//==========

//Insertion
template <typename DataType>
Serializer& operator<<(Serializer &msg,const DataType &d) {
	static_assert(std::is_standard_layout<DataType>::value);
	msg.insert_raw(&d,sizeof(DataType));
	return msg;
}

//Extraction
template <typename DataType>
void operator >>(Serializer &msg,DataType &d) {
	static_assert(std::is_standard_layout<DataType>::value);
	msg.extract_raw(&d,sizeof(DataType));
}

//=================
//  Data structures
//=================

template <typename DataType>
Serializer& operator<<(Serializer &msg,const std::vector<DataType> &v) {
	msg << v.size();
	msg << sizeof(DataType);
	for(auto &e : v) {
		msg << e;
	}
	return msg;
}

template <typename DataType>
void operator>>(Serializer &msg, std::vector<DataType> &v) {
	size_t len;
	size_t dtype_size;
	msg >> len;
	msg >> dtype_size;
	if(sizeof(DataType) != dtype_size) cthrow(ERR_FATAL,"Data type size mismatch. "+std::to_string(dtype_size)+" vs. "+std::to_string(sizeof(DataType)));
	DataType e;
	for(int i=0;i<len;i++) {
		msg >> e;
		v.push_back(e);
	}
}

struct NetworkMessage : public Serializer {
	MessageHeader header;
	bool needs_response() {return this->header.needs_response;}

	NetworkMessage& operator=(const NetworkMessage &other) {
//		gcutils::log("Calling NetworkMessage copy-assignment constructor");
		this->header = other.header;
		this->size = other.size;
		this->body = (byte_t*)malloc(this->size);
		memcpy(this->body,other.body,this->size);
		return *this;
	}

	NetworkMessage(const NetworkMessage &other,std::string caller = __builtin_FUNCTION()) {
//		gcutils::log("Calling NetworkMessage copy constructor from ",caller);
		this->header = other.header;
		this->size = other.size;
		this->body = (byte_t*)malloc(this->size);
		memcpy(this->body,other.body,this->size);
	}

	NetworkMessage(NetworkMessage &&other) {
//		gcutils::log("Calling NetworkMessage move constructor");
		this->header = other.header;
		this->size = other.size;
		this->body = other.body;
		other.body = nullptr;
	}

	//Utils
	void setType(MessageType t) {this->header.type = t;}
	inline MessageType type() const {return this->header.type;}
	void finalize() {
//		if(this->needs_response()) *this << this->uid;
		this->header.size = this->size;
		this->header.sign();
	}
	std::string getSize() {
		return gcutils::conv_bytes(this->size);
	}

	bool is_response() {
		return this->header.is_response;
	}

	void set_as_response(NetworkMessage &to) {
		//Mark this as a response, the other as responded to,
		// Push the UID of the message we're responding to into the
		// Payload of this message
		this->header.is_response = true;
		to.header.needs_response = false;
		if(this->size > 0) cthrow(ERR_WARN,"UID NOT added first in payload");
		*this << to.uid();
	}

	void set_needs_response() {
		this->header.needs_response = true;
	}

	MSG_UIDTYPE_T uid() const {
		return this->header.uid;
	}

	void set_uid(MSG_UIDTYPE_T uid) {
		this->header.uid = uid;
	}


	//Data stuff
	void reset() {
		this->header.type = BLANK;
		this->empty();
	}
	NetworkMessage(const MessageHeader &h,const Serializer &s) {
		this->header = h;
		this->body = s.body;
		this->size = s.size;
	}
	NetworkMessage(MessageType t,byte_t *init_data,size_t init_data_len): header(t,0) {
		this->insert_raw(init_data,init_data_len);
	}
	NetworkMessage(MessageHeader _header): header(_header) {
	}
	//Response constructors
	NetworkMessage(MSG_TYPE_T response_type,NetworkMessage &responding_to) {
		this->header = MessageHeader((MessageType)response_type,0);
		this->set_as_response(responding_to);
	}
	NetworkMessage(MessageType response_type,NetworkMessage &responding_to) {
		this->header = MessageHeader(response_type,0);
		this->set_as_response(responding_to);
	}
	NetworkMessage(MessageType t): header(t,0) {
	}
	NetworkMessage(MSG_TYPE_T id): header((MessageType)id,0) {
	}
	NetworkMessage(): NetworkMessage(BLANK) {}
};

};
