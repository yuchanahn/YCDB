#include "server.hpp"
#include <iostream>
#include <algorithm>
#include <vector>

union uIntChar
{
	size_t uint;
	char uchar[sizeof(size_t)];
};

std::vector<char> buffer;

int main() {
	server_main(56789, [](char* msg, int len, int s) {
		uIntChar size;
		if (len <= sizeof(size_t)) return;
		std::copy(msg, msg+ sizeof(size_t), size.uchar);

		if (len-sizeof(size_t) >= size.uint) {
			for (int i = 0; i < size.uint; i++) msg[sizeof(size_t) + i];
		}
		else {
			for (int i = 0; i < len - sizeof(size_t); i++) 
				buffer.push_back(msg[sizeof(size_t) + i]);
		}


	});
}