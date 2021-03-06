# CNetLib
CNetLib is a high-level, multithreaded networking library for handling TCP messaging

## Changes in v3
* Improved performance
* Bug fixes

## Usage
```cpp
CN::Client cli = CN::Client(5555);  //Initialize network objects on port 5555
CN::Server serv = CN::Server(5555);   //

serv.start_listener();          //Begin listening for connections (asynchronous)
	
//Set server behavior
serv.set_message_handler([](CN::Message *msg) {
  switch(msg->type) {
    case (int)CN::DataType::CN_TEXT:
      //Print text and packet size in bytes
      CNetLib::log("Got message: ",CNetLib::fmt_bytes(msg->content.data(),msg->size)," (",msg->size," bytes)");
      break;
    default:
      CNetLib::log("Unhandled message type"," (",msg->size," bytes)");
  }
});
	
//Connect client to server
CN::Connection *new_connection = cli.connect("127.0.0.1");
  
//Send greeting
new_connection->package_and_send(CN::DataType::TEXT,"Hello from client");

//Send greeting with a custom message type
new_connection->package_and_send(43,"Message type 43");
```

## Compiling

`make` in a Windows GNU shell (such as MinGW) <br>
`make -f makefile_linux` in a native GNU/Linux shell

## Dependencies
* [ASIO 1.18.1](https://sourceforge.net/projects/asio/files/asio/1.18.1%20%28Stable%29/) (non-boost) Or release 1.13.0+ <br>

