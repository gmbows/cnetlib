#include "cnet_utility.h"
#include "cnetlib.h"

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
		bool suc = image.is_open();
		image.close();
		return suc;
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
		for(int i=0;i<s.size();i++) {
			byte_t c = s[i];
			byte_t t = (c + s.size() + 8*i);
			enc += t;
		}
		return "r"+as_hex(enc);
	}

	std::string simple_decrypt(std::string s) {
		std::string dec;
		s = hex_decode_2b(s);
		s.erase(0);
		for(int i=0;i<s.size();i++) {
			byte_t c = s[i];
			byte_t t =(c - s.size() - 8*i);
			dec += t;
		}
		return dec;
	}

	bool make_directory(std::string relpath) {
		std::error_code ec;
		if(file_exists(relpath)) {
			return true;
		}
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

	bool create_file(std::string filename) {
		if(CNetLib::file_exists(filename)) return true;
		bool success = false;
		std::ofstream f(filename,std::ios::binary);
		success = f.is_open();
		f.close();
		return success;
	}

	std::string as_hex(int s)	{
		std::stringstream out;
		out << std::hex << s;
		return out.str();
	}

	void create_upnp_mapping(unsigned short port) {
		int error = 0;
		struct UPNPDev *upnp_dev = upnpDiscover(
					1000    , // time to wait (milliseconds)
					nullptr , // multicast interface (or null defaults to 239.255.255.250)
					nullptr , // path to minissdpd socket (or null defaults to /var/run/minissdpd.sock)
					0       , // source port to use (or zero defaults to port 1900)
					0       , // 0==IPv4, 1==IPv6
					2,
					&error  ); // error
		if(error) {
			CNetLib::log("Error in UPnP device discovery: ",error);
			return;
		}
		CNetLib::log("UPnP found IGD");
//		CNetLib::log("Found UPnP device at ",upnp_dev->descURL);
		//Ignore other devices for now
		//		while(upnp_dev->pNext) {
		//			upnp_dev = upnp_dev->pNext;
		//			CNetLib::log(upnp_dev->descURL);
		//		}

		char lan_address[64];
		struct UPNPUrls upnp_urls;
		struct IGDdatas upnp_data;
		int status = UPNP_GetValidIGD(upnp_dev, &upnp_urls, &upnp_data, lan_address, sizeof(lan_address));
		// look up possible "status" values, the number "1" indicates a valid IGD was found

		CNetLib::log("UPnP IGD Status: ",status);

		// get the external (WAN) IP address
		char wan_address[64];
		UPNP_GetExternalIPAddress(upnp_urls.controlURL, upnp_data.first.servicetype, wan_address);

		//		CNetLib::log("Wan addr: ",wan_address);

		// add a new TCP port mapping from WAN port 12345 to local host port 24680
		error = UPNP_AddPortMapping(
					upnp_urls.controlURL,
					upnp_data.first.servicetype,
					std::to_string(port).c_str()     ,  // external (WAN) port requested
					std::to_string(port).c_str()     ,  // internal (LAN) port to which packets will be redirected
					lan_address , // internal (LAN) address to which packets will be redirected
					"CNetLib UPnP", // text description to indicate why or who is responsible for the port mapping
					"TCP"       , // protocol must be either TCP or UDP
					nullptr     , // remote (peer) host address or nullptr for no restriction
					"0"     ); // port map lease duration (in seconds) or zero for "as long as possible"

		if(error)
			CNetLib::log("Error creating UPnP mapping: ",error);
		else
			CNetLib::log("Created UPnP mapping ",port,"->",port);

	}

	void list_upnp_mappings() {
		// list all port mappings
	}

	int wrap(int n, int lb,int ub) {
		return lb + (n - lb) % (ub - lb);
	}

	std::string as_hex(std::string s) {
		std::stringstream out;
		for(auto c : s) {
			out << std::hex << (int)c;
		}
		return out.str();
	}

	char hex_decode_char(std::string s)	{
		int val = 0;
		int col = 1;
		for(byte_t c : s) {
			if(c >= 'a') {
				val += (pow(16,col))*(c-'a'+10);
			} else {
				val += (pow(16,col))*(c-48);
			}
			col--;
		}
		return val;
	}

	std::string hex_decode_2b(std::string h) {
		if(h.size()%2) {
//			CNetLib::log("Hex decode error: String is not even len");
			h = "0"+h;
		}
		std::string d;
		for(int i=0;i<h.size();i+=2) {
			std::string pair;
			pair += h[i];
			pair += h[i+1];
			d += hex_decode_char(pair);
		}
		return d;
	}

}
