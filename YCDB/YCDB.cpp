#include "server.hpp"


int main() {
	server_main(56789, [](char* msg, int len, int s) {

		printf("%s\n", msg); 
		
		
		
	});
}