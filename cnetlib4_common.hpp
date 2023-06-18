#pragma once

#include <cstdint>

#define MSG_TYPE_T int32_t
#define MSG_SIGTYPE_T int32_t
#define MSG_UIDTYPE_T int32_t
#define CHAN_IDTYPE_T uint16_t
#define MSG_BUFSIZE_DEFAULT 256
#define MSG_STRLEN_DEFAULT 256
#define CNET_BUFSIZE_DEFAULT 65535
#define CNET_STREAMINFO_LEN 63354
#define CNET_STREAM_THRESHOLD 20*1000000
//#define CNET_HEADER_SIG 12
constexpr MSG_SIGTYPE_T CNET_HEADER_SIG() {
	return CNET_BUFSIZE_DEFAULT ^ (MSG_STRLEN_DEFAULT ^ CNET_STREAMINFO_LEN) << sizeof(MSG_TYPE_T);
}
constexpr MSG_SIGTYPE_T CNET_HEADER_SIG_UNSIGNED() {
	return 0xf0ac38b;
}
typedef uint8_t byte_t;

