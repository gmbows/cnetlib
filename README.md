# CNetLib
CNetLib is a high-level, multithreaded networking library for handling TCP messaging.

## Changes in v3
* Improved performance
* Bug fixes

## Features
* Text messaging
* File transfer + streaming
* [JSON](https://github.com/gmbows/json-kvs)
* UPnP (optional)
* Hashed validation

## Usage Example

A simple chatroom can be created with the following snippets. <br>
Messages sent by any client will be synchronized between participants.

### Client
```cpp
#include "cnetlib.h"
//main_client.cpp


CN::Client cli = CN::Client(5555); //Create client object on port 5555

//Connect to a local server
CN::Connection *new_connection = cli.connect("127.0.0.1"); 
if(!new_connection) {
	CNetLib::log("Connection failed");
	return 1;
}

//Join channel "test" on local server and send a greeting
new_connection->send_info(CN::DataType::CHAN_JOIN,"test");
new_connection->package_and_send("Joined");

//Send text messages
std::string s;
while(s != "q") {
	std::cout << "->" << std::flush; 
	std::getline(std::cin,s);
	new_connection->package_and_send(s);
}
```

### Server
```cpp
#include "cnetlib.h"
//main_server.cpp


CN::Server serv = CN::Server(5555); //Create server object on port 5555

//Set handler for TEXT messages (otherwise call default)
serv.add_typespec_handler(CN::DataType::TEXT,[](CN::UserMessage *msg) { 
	CNetLib::log("(Message) ",msg->connection->getname(),": ",msg->str());
});

//Create new channel with id "test" (must be 4 chars)
CN::Channel *chan = serv.register_channel(nullptr,"test");

serv.start_listener(); //Start accepting connections

//Enter to close...
std::string s;
std::cin >> s;
```

## Linking

`asio.hpp` must be in your include path.  <br>
Link with `-lstdc++fs -lminiupnpc` <br>
Define `-DASIO_STANDALONE -D_REENTRANT -D_WIN32_WINNT=0x0601` <br>
Include `-lws2_32 -lwsock32` on Windows.

## Dependencies
* [ASIO 1.18.1](https://sourceforge.net/projects/asio/files/asio/1.18.1%20%28Stable%29/) (non-boost) Or release 1.13.0+ <br>
* [miniupnpc](https://github.com/miniupnp/miniupnp/tree/master/miniupnpc)

