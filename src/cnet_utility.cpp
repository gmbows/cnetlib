#include "cnet_utility.h"

#include <cmath>
#include <string>
#include <fstream>
#include <sstream>
#include <mutex>
#include <thread>
#include <filesystem>

#include <libgen.h>
#include <cstring>

std::mutex print_mutex;




namespace CNetLib {
	std::condition_variable print_cv;
	std::stringstream statement;
	std::stringstream log_statement;
	std::function<void(std::string)> log_handler = [](std::string s) {
//		std::string fn = "log.txt";
//		std::string lsnl = s+"\n";
//		append_to_file(fn,(unsigned char*)lsnl.data(),lsnl.size());
	}; //Defaults to appending to a log file
	void init() {
//		std::thread(await_print_jobs).detach();
	}


	void clear_buffer(void* buf,int len) {
		memset(buf,'\0',len);
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

	size_t import_file(const std::string &filename,unsigned char* &data) {

		if(!CNetLib::file_exists(filename)) {
			CNetLib::log("import_file: file not found");
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
			CNetLib::log("import_file: file not found");
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

	bool export_file(std::string filename,unsigned char* bytes,size_t size) {

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
			CNetLib::log("append_to_file: Unable to open file ",filename);
			return false;
		}

		for(int i=0;i<size;i++) {
			file << bytes[i];
		}
	//	file.close();
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
		if(!success) CNetLib::log("Error creating directory: '",relpath,"'");
		return success;
	}

	size_t file_size(std::string path) {
		if(!CNetLib::file_exists(path)) {
			CNetLib::log("file_size: file not found");
			return false;
		}

		std::ifstream image(path,std::ios::binary);

		//Seek to EOF and check position in stream
		image.seekg(0,image.end);
		size_t size = image.tellg();
		image.close();
		return size;

	}

	std::string conv_bytes(size_t bytes) {
		std::string suffix;
		double b_conv;
		if(bytes < 1000000) {
			b_conv = bytes/1000.0f;
			suffix = "kB";
		} else if(bytes < 1000000000) {
			b_conv = bytes/1000000.0f;
			suffix = "mB";
		} else if(true or bytes < 1000000000000) {
			b_conv = bytes/1000000000.0f;
			suffix = "gB";
		}
		size_t b_i = (size_t)b_conv;
		size_t b_m = 100*(b_conv-b_i);

		std::string b = std::to_string(b_i)+"."+std::to_string(b_m)[0];
		return b+suffix;
	}

	std::string fmt_bytes(unsigned char *bytes,size_t len) {
		std::string s("");
		for(int i=0;i<len;i++) s+=bytes[i];
		return s;
	}

	std::string random_hex_string(int len) {
		std::stringstream s;
		for(int i=0;i<len;i++) s << std::hex << rand()%16;
		return s.str();
	}
}
