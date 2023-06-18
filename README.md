# CNetLib
CNetLib is a high-level, multithreaded networking library for handling TCP messaging.

## Changes in v4
* Complete rewrite with full support for ASIO features
* Better performance

## Features
* Text messaging
* File transfer + streaming

## Architecture
Each new connection spawns a server thread that reads data, and passes complete messages to user-defined handling functions (subject to change). <br>
Uses a 64k network buffer by default. <br>
Message type is stored in a 32-bit integer in the header of each message, after the size of the message, which is a 64-bit integer.
Users can define their own message type handlers by passing the 32-bit message type id to `NetObj::add_typespec_handler(id,[](){});`<br>
Messages of type `id` will be passed to the callable in the second parameter.
## Usage Examples

### Examples
Examples coming soon <br>
## Dependencies
* [ASIO 1.18.1](https://sourceforge.net/projects/asio/files/asio/1.18.1%20%28Stable%29/) (non-boost) Or release 1.13.0+ <br>
* [gcutils](https://github.com/gmbows/gcutils)
