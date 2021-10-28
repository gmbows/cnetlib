#include "Common.h"

pthread_mutex_t printLock = PTHREAD_MUTEX_INITIALIZER;

int thread_count = 0;
asio::io_context io_context;
std::string packet_header;

// enum CG_Alignment : short {CG_ALIGN_LEFT, CG_ALIGN_RIGHT,CG_ALIGN_CENTER};
// enum CG_ElementType : short {CG_TEXTBOX,CG_BUTTON, CG_INPUT};