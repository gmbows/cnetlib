#include "cnet_utility.h"

#include <string>
#include <fstream>
#include <sstream>
#include <mutex>
#include <thread>
#include <filesystem>

#include <libgen.h>
#include <cstring>

std::mutex printLock;
std::stringstream statement;
std::string last_printed;
unsigned int dupes = 0;

void clear_buffer(void* buf,int len) {
	memset(buf,'\0',len);
}

std::string operator *(const std::string &s, int len) {
	std::string output = "";
	for(int i=0;i<len;++i) {
		output += s;
	}
	return output;
}

std::vector<std::string> split(const std::string &s,char token) {
    std::vector<std::string> v;
    std::string run = "";
    char e;

    for(int i=0;i<s.size();++i) {
		e = s[i];
        if(e == token) {
            if(run.size() == 0) continue;
            v.push_back(run);
            run = "";
        } else {
            run += e;
        }
    }
    v.push_back(run);
    return v;
}

std::string command = "";
std::string cursor = "";

void pad(std::string &s, int len,std::string t) {
    while(s.size() < len) {
        s.insert(0,t);
    }
}

size_t import_file(const std::string &filename,unsigned char* &data) {

    if(!file_exists(filename)) {
        print("import_file: file not found");
        return false;
    }

    std::ifstream image(filename,std::ios::binary);

    //Seek to EOF and check position in stream
    image.seekg(0,image.end);
    size_t size = image.tellg();
    image.seekg(0,image.beg);

	data = (unsigned char*)malloc(size);

    char c;
    int i = 0;
    while(image >> std::noskipws >> c) {
         data[i++] = c;
    }

    image.close();
    return size;
}

std::vector<char> import_file(const std::string &filename) {
	
	std::vector<char> v;

    if(!file_exists(filename)) {
        print("import_file: file not found");
        return v;
    }

    std::ifstream image(filename,std::ios::binary);

    char c;
    int i = 0;
    while(image >> std::noskipws >> c) {
         v.push_back(c);
    }

    image.close();
    return v;
}

bool export_file(const std::string &filename,char* bytes,size_t size) {

    std::ofstream image(filename,std::ios::binary);

    for(int i=0;i<size;i++) {
        image << bytes[i];
    }
    image.close();
    return true;
}

bool append_to_file(const std::string &filename,unsigned char* bytes,size_t size) {

	std::ofstream file(filename,std::ios::binary | std::ios_base::app);

	if(!file.is_open()) {
		print("append_to_file: Unable to open file ",filename);
		return false;
	}

	for(int i=0;i<size;i++) {
		file << bytes[i];
	}
	file.close();
	return true;
}

bool file_exists(std::string filename) {
    std::ifstream image(filename,std::ios::binary);
    return image.is_open();
}

std::vector<std::string> getlines(std::vector<char> data) {
	std::vector<std::string> v;
	std::string entry = "";
	for(auto &e : data) {
		if(e == '\n') {
			v.push_back(entry);
			entry = "";
		} else {
			entry += e;
		}
	}
	v.push_back(entry);
	return v;
}

void print() {
	print("");
}

std::string get_address(asio::ip::tcp::socket *sock) {
	return sock->remote_endpoint().address().to_string();
}

std::string get_filename(std::string _path) {
	std::filesystem::path p{_path};
	return p.filename().u8string();
}

std::string simple_encrypt(std::string s) {
	std::string enc;
	for(char c : s) {
		enc += (c - s.size()-15);
	}
	return enc;
}

std::string simple_decrypt(std::string s) {
	std::string dec;
	for(char c : s) {
		dec += (c + s.size()+15);
	}
	return dec;
}

bool make_directory(std::string relpath) {
	std::error_code ec;
	bool success = std::filesystem::create_directory(relpath.c_str(),ec);
	if(!success) print("Error creating directory: '",relpath,"'");
	return success;
}
