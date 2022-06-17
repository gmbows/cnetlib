# CNetLib
CNetLib is a high-level, multithreaded networking library for handling basic client/server based TCP connections 

## Changes in v2
* Serialization
* File streaming
* Fixes and optimizations

## Usage
Provided in the library is a `main.cpp` file that performs a test of basic functionality
```cpp
CN_Server serv;         //Server object
CN_Client cli;          //Client object

serv.initialize(5555);  //Initialize network objects on port 5555
cli.initialize(5555);   //

serv.start();           //Begin listening for connections (asynchronous)
	
//Set server behavior
serv.packet_handler = [](CN_Packet *np) {
  switch(np->type) {
    case CN_DataType::CN_TEXT:
      print("Got message: ",np->content," (",np->len," bytes)");  //Print text and packet size in bytes
      break;
    default:
      print("Unhandled data type","(",np->len," bytes)");
  }
};
	
//Connect client to server
CN_Connection *new_connection = cli.connect("127.0.0.1");
  
//Send greeting
new_connection->pack_and_send(CN_DataType::CN_TEXT,"Hello from client");
```

## Compiling

`make` in a Windows GNU shell (such as MinGW) <br>
`make -f makefile_linux` in a native GNU/Linux shell

## Dependencies
* [ASIO 1.18.1](https://sourceforge.net/projects/asio/files/asio/1.18.1%20%28Stable%29/) (non-boost) Or release 1.13.0+ <br>

