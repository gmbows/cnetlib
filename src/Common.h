#pragma once

#include <asio.hpp>
#include <pthread.h>
#include <string>

#define TWT_BUFFER_SIZE 1024
#define TWT_PAD_TYPE 2
#define TWT_PAD_SOCKET 2
#define TWT_PAD_SIZE 16
#define TWT_PAD_FILENAME 255

enum DataType {
	
	TWT_DATA,
	TWT_EMPTY,
	
    TWT_TEXT,
	TWT_FILE,
	
	TWT_FILE_ACK,
	TWT_CLOSE,
	TWT_SIGN,
	TWT_ACK,
	TWT_PROG,
	TWT_GREET,
	
	TWT_SET_TITLE,
};

extern std::string packet_header;


extern pthread_mutex_t printLock;

extern int thread_count;

extern asio::io_context io_context;


extern pthread_mutex_t printLock;

enum CG_Alignment : short {CG_ALIGN_LEFT, CG_ALIGN_RIGHT,CG_ALIGN_CENTER,CG_ALIGN_DEFAULT};
enum CG_ElementType : short {CG_TEXTBOX,CG_BUTTON, CG_INPUT,CG_STATIC,CG_DYNAMICTEXTBOX};
enum CG_Position : short {CG_POS_ABSOLUTE,CG_POS_CENTERED};
enum CG_LayoutUpdateType : short {CG_LAYOUT_DYNAMIC,CG_LAYOUT_INSTANCED};