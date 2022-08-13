#pragma once

#include <iostream>
#include <vector>
#include <cstring>
#include <bitset>

#include "cnet_utility.h"

class serializer {
public:
//	unsigned long long int* byte_lengths;
//	int byte_length_set;
//	int byte_length_get;

	bool using_ext_data = false;

	unsigned char* data;
	unsigned char* data_end;
	unsigned char* data_set_ptr;
	unsigned char* data_get_ptr;

	size_t data_size;

	bool deleted = false;

	void reset() {
//		free(this->data);
//		this->initialize(CN_BUFFER_SIZE);
	}

	size_t true_size() {
		size_t offset = (size_t)(this->data_set_ptr-this->data);
//		 CNetLib::print("True size: ",offset);
		return offset;
	}

	void realloc_data() {
		unsigned int new_data_size = 2*this->data_size;
		unsigned int data_set_offset = this->data_set_ptr - this->data;
		unsigned int data_get_offset = this->data_get_ptr - this->data;
		this->data = (unsigned char*)realloc(this->data,new_data_size);
		this->data_size = new_data_size;
//		CNetLib::print("Data size expanded to ",new_data_size);
//		return;
//		unsigned int new_data_size = 2*data_size;
//		unsigned char* new_data = (unsigned char*)malloc(new_data_size*sizeof(unsigned char));
//		memset(new_data,'\0',new_data_size*sizeof(unsigned char));
//		memcpy(new_data,this->data,this->data_size*sizeof(unsigned char));
//		unsigned int data_set_offset = this->data_set_ptr - this->data;
//		unsigned int data_get_offset = this->data_get_ptr - this->data;

//		this->data_size = new_data_size;

//		CNetLib::print("Data size expanded to ",new_data_size);

//		free(this->data);
//		this->data = new_data;

		this->data_set_ptr = this->data+data_set_offset;
		this->data_get_ptr = this->data+data_get_offset;

		this->data_end = (unsigned char*)(data+data_size*sizeof(unsigned char));

		if(this->data + this->data_size != this->data_end) {
			CNetLib::print("Data end set incorrectly");
		}
	}

	int get_int() {
		int *i = (int*)data_get_ptr;
//		if(this->byte_lengths[byte_length_get] != sizeof(int)) {
//			CNetLib::print("Invalid fetch (",sizeof(int),", expected ",this->byte_lengths[byte_length_get],")");
//			return -1;
//		}
		data_get_ptr += sizeof(int);
		if(this->data_get_ptr > this->data_end) {
			CNetLib::print("Error: Accessing beyond end of serialized data");
		}
//		byte_length_get++;
		return *i;
	}

	float get_float() {
		float *i = (float*)data_get_ptr;
//		if(this->byte_lengths[byte_length_get] != sizeof(int)) {
//			CNetLib::print("Invalid fetch (",sizeof(int),", expected ",this->byte_lengths[byte_length_get],")");
//			return -1;
//		}
		data_get_ptr += sizeof(float);
		if(this->data_get_ptr > this->data_end) {
			CNetLib::print("Error: Accessing beyond end of serialized data");
		}
//		byte_length_get++;
		return *i;
	}

	int get_long() {
		size_t *i = (size_t*)data_get_ptr;
//		if(this->byte_lengths[byte_length_get] != sizeof(size_t)) {
//			CNetLib::print("Invalid fetch (",sizeof(int),", expected ",this->byte_lengths[byte_length_get],")");
//			return -1;
//		}
		data_get_ptr += sizeof(size_t);
		if(this->data_get_ptr > this->data_end) {
//			CNetLib::print("Error get_long(): Accessing beyond end of serialized data");
		}
//		byte_length_get++;
		return *i;
	}

	char get_char() {
		char *c = (char*)data_get_ptr;
		data_get_ptr += sizeof(char);
		if(this->data_get_ptr > this->data_end) {
			CNetLib::print("Error get_char(): Accessing beyond end of serialized data");
		}
		return *c;
	}

	bool get_str(std::string *s, int len) {
		//returns true if reached end of serialized data
//		CNetLib::print("Fetching",len);
		for(int i=0;i<len;i++) {
			*s += this->get_char();
			if(this->data_get_ptr >= this->data_end) {
//				CNetLib::print("get_str(): Reached end of data");
				return true;
			}
		}
		return false;
	}

	std::string get_str(int len) {
		std::string s;
		for(int i=0;i<len;i++) {
			s += this->get_char();
		}
		return s;
	}

	int get_data(unsigned char* data, int len) {
//		memset(data,'N',len);
		int i;
		for(i=0;i<len;i++) {
			data[i] = this->get_char();
			if(this->data_get_ptr == this->data_end) {
//				CNetLib::print("get_data(): Reached end of data");
				return i+1;
			}
		}
		return len;
	}

	std::string get_str() {
		return this->get_str(256);
	}

	void add_int(int i) {
		memcpy(this->data_set_ptr,&i,sizeof(int));
		data_set_ptr += sizeof(i);
		if(data_set_ptr >= this->data_end) this->realloc_data();
	}

	template <typename T>
	T get_auto() {
		T *i = (T*)data_get_ptr;
//		if(this->byte_lengths[byte_length_get] != sizeof(int)) {
//			CNetLib::print("Invalid fetch (",sizeof(int),", expected ",this->byte_lengths[byte_length_get],")");
//			return -1;
//		}
		data_get_ptr += sizeof(T);
		if(this->data_get_ptr > this->data_end) {
			CNetLib::print("Error: Accessing beyond end of serialized data");
		}
//		byte_length_get++;
		return *i;
	}

	template <typename T>
	void add_auto(T t) {
		memcpy(this->data_set_ptr,&t,sizeof(T));
		data_set_ptr += sizeof(t);
		if(data_set_ptr >= this->data_end) this->realloc_data();
	}

	void add_float(float i) {
		memcpy(this->data_set_ptr,&i,sizeof(float));
		data_set_ptr += sizeof(i);
		if(data_set_ptr >= this->data_end) this->realloc_data();
	}

	void add_long(size_t i) {
		memcpy(this->data_set_ptr,&i,sizeof(size_t));
		data_set_ptr += sizeof(i);
		if(data_set_ptr >= this->data_end) this->realloc_data();
	}

	void add_char(unsigned char c) {
		*this->data_set_ptr = c;
		data_set_ptr += sizeof(c);
		if(data_set_ptr >= this->data_end) {
			this->realloc_data();
		}
	}

	void add_str_auto(std::string s) {
		for(int i=0;i<s.size();i++) {
			this->add_char(s[i]);
		}
	}

	//Adds a padded 256-char string
	void add_str(std::string s) {
		for(int i=0;i<256;i++) {
			if(i < s.size()) {
				this->add_char(s[i]);
			} else {
				this->add_char('\0');
			}
		}
	}

	void add_data(unsigned char* data, size_t len) {
		for(int i=0;i<len;i++) {
			this->add_char(data[i]);
		}
	}

	void show_bytes();

	void set_data(void *new_data);

	bool test_ptrs() {
		if(data != data_set_ptr) {
			CNetLib::print("Data set pointer set incorrectly");
			return false;
		}
		if(data_get_ptr != data_set_ptr) {
			CNetLib::print("Data get pointer set incorrectly");
			return false;
		}
		if(this->data + this->data_size != this->data_end) {
			CNetLib::print("Data end set incorrectly");
			return false;
		}
		return true;
	}

	void initialize(size_t size) {
		this->data = (unsigned char*)malloc(size);
//		memset(this->data,'\0',size);
		this->data_set_ptr = data;
		this->data_get_ptr = data;
		this->data_end = data+size;

		if(!this->test_ptrs()) exit(0);
	}

	serializer(size_t __data_size): data_size(__data_size) {
		this->initialize(__data_size);
	}

	~serializer() {
		if(this->using_ext_data == false) free(this->data);
	}
};
