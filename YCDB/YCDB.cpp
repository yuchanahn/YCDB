#include "server.hpp"
#include <iostream>
#include <algorithm>
#include <vector>
#include "packet_reader.hpp"

union uIntChar
{
	size_t uint;
	char uchar[sizeof(size_t)];
};

std::vector<char> buffer;

int main() {
	server_main(56789, [](char* msg, int len, int s) {
		uIntChar size;
		
		for (int i = 0; i < len; ++i) buffer.push_back(msg[i]);

		if (buffer.size() <= sizeof(size_t)) return;
		
		std::copy(&buffer[0], &buffer[0] + sizeof(size_t), size.uchar);

		if (buffer.size() >= size.uint) {
			size_t key = *((size_t*)msg);
			size_t id = *((size_t*)&(msg[sizeof(size_t)]));
			int start = sizeof(size_t) * 2;
			int buffer_size = size.uint - start;
			for (int i = 0; i < buffer_size; ++i) DB_ROW[key][id].push_back(msg[start + i]);
			
			std::vector<char> new_buffer;
			for (int i = size.uint, ii = 0; i < buffer.size(); ++i, ++ii) new_buffer[ii] = buffer[i];
			buffer = new_buffer;
		}
	});
}