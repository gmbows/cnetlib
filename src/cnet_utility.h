#pragma once

#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <map>
#include <thread>
#include <mutex>
#include <asio.hpp>

#include <memory>
extern std::mutex printLock;

std::string get_address(asio::ip::tcp::socket *sock);

//=========
// GLOBAL
//=========

std::string simple_encrypt(std::string);
std::string simple_decrypt(std::string);

extern std::string command,cursor,last_printed;

extern std::stringstream statement;

extern unsigned int dupes;

std::string operator *(const std::string &s, int len);
void pad(std::string &s, int len,std::string t);

template <class T>
void print(T t) {
	std::lock_guard<std::mutex> p(printLock);
    std::string space = " ";
    statement << t;
	//Dupe culling
	if(statement.str() == last_printed) {
		//Rewrite previous line with dupe count
		dupes++;
		std::string line = statement.str() + " (" + std::to_string(dupes-1) + " more)";
		std::cout << "\r" << space*(line.size()+1) << "\r" << std::flush;
		std::cout << line << std::flush;
	} else {
		if(dupes != 0) std::cout << std::endl;
		std::cout << "\r" << space*(cursor.size()+command.size()+1) << "\r" << std::flush;
		dupes = 0;
		std::cout << statement.str() << std::endl;
		last_printed = statement.str();
		std::cout << cursor << command << std::flush;
	}
    statement.str("");
    printLock.unlock();
}

template <class T,class... Args>
void print(T t,Args... args) {
    printLock.lock();
    statement << t;
    printLock.unlock();
    print(args...);
}

void print();

//=========
// VECTOR
//=========

std::string vector_to_string(const std::vector<unsigned char> v);

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
bool export_file(const std::string &filename,char*,size_t size);
bool append_to_file(const std::string &filename,unsigned char*,size_t size);
bool file_exists(std::string filename);

bool make_directory(std::string);
