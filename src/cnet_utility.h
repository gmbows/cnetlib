#pragma once

#include "upnpcommands.h"
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <map>
#include <thread>
#include <mutex>
#include <asio.hpp>
#include <filesystem>

#include <memory>
#include <queue>

#include <miniupnpc/miniupnpc.h>

extern std::mutex print_mutex;



namespace CNetLib {

	//=========
	// GLOBAL
	//=========

	std::string get_address(asio::ip::tcp::socket *sock);
	extern std::function<void(std::string)> log_handler;
	void init();

	std::string simple_encrypt(std::string);
	std::string simple_decrypt(std::string);

	template <typename T>
	extern std::queue<T> print_queue;

	extern std::condition_variable print_cv;

	//int prints = 0;

	template <typename T>
	void await_print_jobs() {
		while(true) {
			while(print_queue<T>.empty()) {
				std::unique_lock<std::mutex> signal_lock(print_mutex);
				 print_cv.wait(signal_lock);
			}
			/* Lock scope */ {
				std::lock_guard<std::mutex> lk{print_mutex};
				T job = print_queue<T>.front();
				print_queue<T>.pop();
				std::cout << job << std::flush;
	//			std::cout << ++prints << job << std::endl;
			}
		}
	}

	template <typename T>
	void add_to_print_queue(std::string job) {
		{
			std::lock_guard<std::mutex> lk{print_mutex};
			print_queue<T>.push(job);
			print_cv.notify_one();
		}
	}

	template <class T>
	void print(T t) {
		{
			std::lock_guard<std::mutex> m{print_mutex};
			std::cout << t << std::endl;
		}
	}

	template <class T,class... Args>
	void print(T t,Args... args) {
		{
			std::lock_guard<std::mutex> m{print_mutex};
			std::cout << t;
		}
		print(args...);
	}

	extern std::stringstream log_statement;
	template <class T>
	void log(T t) {
		{
			std::lock_guard<std::mutex> m{print_mutex};
			log_statement << t;
			log_handler(log_statement.str());
		}
		print(log_statement.str());
		log_statement.str("");
	}

	template <class T,class... Args>
	void log(T t,Args... args) {
		{
			std::lock_guard<std::mutex> m{print_mutex};
			log_statement << t;
		}
		CNetLib::log(args...);
	}

	void print();

	//Util

	std::string conv_bytes(size_t);
	std::string fmt_bytes(unsigned char *bytes,size_t len);
	std::string random_hex_string(int);
	std::string as_hex(int);

	//Wait on a variable
	struct Waiter {
		std::condition_variable cv;
		std::mutex m;
		bool *ready;
		bool override = false;
		void await() {
			while(this->override == false and *this->ready == false) {
				{
					std::unique_lock<std::mutex> lk(this->m);
					this->cv.wait(lk);
				}
			}
			this->override = false;
		}
		void wake_one() {
			this->cv.notify_one();
		}
		void wake_all() {
			this->cv.notify_all();
		}
		void wait() {
			if(*this->ready == false) this->await();
		}
		bool wait(unsigned timeout) {
			bool timed_out = false;
			//Awakens self if unready by timeout
			std::thread([&,this](){
				std::this_thread::sleep_for(std::chrono::milliseconds(timeout));
				if(*this->ready == false) {
					timed_out = true;
					this->override = true;
					this->wake_one();
				}
			}).detach();
			this->wait();
			return timed_out;
		}
		Waiter(bool *_ready) {
			this->ready = _ready;
		}
	};

	//=========
	// UPnP
	//=========

	void create_upnp_mapping(unsigned short port);
	void list_upnp_mappings();

	//=========
	// VECTOR
	//=========

	std::string vector_to_string(const std::vector<unsigned char> v);

	template <class T>
	int v_find(std::vector<T> v, T t) {
		for(int i=0;i<v.size;i++) {
			T e = v.at(i);
			if((char*)e == (char*)t) return i;
		}
		return -1;
	}

	template <typename T>
	bool contains(std::vector<T> v, T t) {
		for(auto e : v) {
			if(e == t) return true;
		}
		return false;
	}

	template <typename T>
	bool contains(std::vector<T*> v, T *t) {
		for(auto &e : v) {
			if((char*)e == (char*)t) return true;
		}
		return false;
	}

	template <typename K,class V>
	bool contains(std::map<K,V> &m,K k) {
		if(m.find(k) == m.end()) {
			return false;
		} else {
			return true;
		}
	}

	std::vector<std::string> split(const std::string&,char);

	//=========
	// ARRAY
	//=========

	void clear_buffer(void*,int);

	//=========
	// I/O
	//=========


	std::string get_filename(std::string path);
	size_t import_file(const std::string &filename,unsigned char*&);
	std::vector<char> import_file(const std::string &filename);
	std::vector<std::string> getlines(std::vector<char>); //Converts char vector into string vector split by newline
	bool export_file(std::string filename,unsigned char*,size_t size);
	bool append_to_file(const std::string &filename,unsigned char*,size_t size);
	bool file_exists(std::string filename);
	size_t file_size(std::string path);

	bool make_directory(std::string);
	bool create_file(std::string);
}
