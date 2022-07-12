#include "serializer.h"

void serializer::show_bytes() {
	for(int i=0;i<24;i+=4) {
		std::bitset<8> b4(*(this->data+i));
		std::bitset<8> b3(*(this->data+i+1));
		std::bitset<8> b2(*(this->data+i+2));
		std::bitset<8> b1(*(this->data+i+3));
		std::cout << b1 << " " << b2 << " " << b3 << " " << b4 << std::flush;
		if(i%4 == 0) std::cout << std::endl;
	}
}

void serializer::set_data(void *new_data) {
//	this->reset();
//	free(this->data);
	this->data = (unsigned char*)new_data;
	this->data_set_ptr = (unsigned char*)this->data;
	this->data_get_ptr = (unsigned char*)this->data;
	this->data_end = (unsigned char*)(data+data_size);
}
