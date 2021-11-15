#include "server.hpp"
#include <iostream>
#include <algorithm>
#include <vector>
#include "packet_reader.hpp"

enum EPacketType : short
{
	get_data = 1,
	update_data = 2,
	has_id = 3,
};

union uIntChar
{
	int uint;
	char uchar[sizeof(int)];
};

std::vector<char> buffer;


struct p_test_1
{
	int num;
};


struct check_id_t
{
	int key;
	int id;
	bool r;
};

int main() {
	server_main(61234, [](char* msg, int len, int s) {
		uIntChar size;
		
		for (int i = 0; i < len; ++i) buffer.push_back(msg[i]);

		if (buffer.size() <= sizeof(int)) return;
		
		std::copy(&buffer[0], &buffer[0] + sizeof(int), size.uchar);
		if (buffer.size() >= size.uint) {

			int start = sizeof(int);
			int start_key_idx = sizeof(int) + sizeof(short);
			int start_id_idx = sizeof(int) + sizeof(short) + sizeof(int);

			EPacketType packet_type = static_cast<EPacketType>(*((short*)(&buffer[start])));
			int key = *((int*)(&buffer[start_key_idx]));
			int id = *((int*)(&buffer[start_id_idx]));



			if (packet_type == EPacketType::has_id) {
				check_id_t p = check_id_t{
					key, id, DB_ROW[key].find(id) != DB_ROW[key].end()
				};
				std::vector<char> r;
				for (int i = 0; i < sizeof(int); i++) r.push_back(((char*)&key)[i]);
				for (int i = 0; i < sizeof(int); i++) r.push_back(((char*)&id)[i]);
				for (int i = 0; i < sizeof(check_id_t); i++) { r.push_back(((char*)(&p))[i]); }
				uIntChar itoc;
				itoc.uint = r.size() + sizeof(int);
				r.insert(r.begin(), itoc.uchar, itoc.uchar + sizeof(int));
				send(s, &r[0], r.size(), 0);
			} else if (packet_type == EPacketType::update_data) {
				int buffer_size = size.uint - start_id_idx;
				DB_ROW[key][id].clear();
				for (int i = 0; i < buffer_size; ++i) DB_ROW[key][id].push_back(buffer[start_id_idx + sizeof(int) + i]);
			} else if (packet_type == EPacketType::get_data) {
				std::vector<char> r;
				for (int i = 0; i < sizeof(int); i++) r.push_back(((char*)&key)[i]);
				for (int i = 0; i < sizeof(int); i++) r.push_back(((char*)&id)[i]);
				for (auto& i : DB_ROW[key][id]) { r.push_back(i); }
				uIntChar itoc;
				itoc.uint = r.size() + sizeof(int);
				r.insert(r.begin(), itoc.uchar, itoc.uchar + sizeof(int));
				send(s, &r[0], r.size(), 0);
			}

			std::vector<char> new_buffer;
			for (int i = size.uint; i < buffer.size(); ++i) new_buffer.push_back(buffer[i]);
			buffer = new_buffer;
		}
	},
	[&] {

	});
}