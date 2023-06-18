#include "message.h"

namespace cn {

Serializer& operator<<(Serializer &msg, const char *d) {
	std::string s(d);
	msg << s;
	return msg;
}


Serializer& operator<<(Serializer &msg, const std::string &d) {
	if(d.size() > MSG_STRLEN_DEFAULT) {
		cthrow(ERR_FATAL,"String longer than string padding length");
	}
	byte_t s_data[MSG_STRLEN_DEFAULT];
	memset(s_data,'\0',MSG_STRLEN_DEFAULT);
	for(int i=0;i<d.size();i++) {
		s_data[i] = d[i];
	}
	msg.insert_raw(s_data,MSG_STRLEN_DEFAULT);
	return msg;
}

void operator>>(Serializer &msg, std::string &d) {
	byte_t s_data[MSG_STRLEN_DEFAULT+1];
	msg.extract_raw(s_data,MSG_STRLEN_DEFAULT);
	s_data[MSG_STRLEN_DEFAULT] = '\0';
//	std::string out((char*)s_data);
	d = (char*)s_data;
}

};
