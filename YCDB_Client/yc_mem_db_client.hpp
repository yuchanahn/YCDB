#pragma once
#include <unordered_map>
#include <cstdio>
#include <iostream>
#include <fstream>
#include <optional>
#include <algorithm>
#include <functional>

#pragma comment(lib, "ws2_32.lib")

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>
#include <WS2tcpip.h>

#define NOID_KEY 99999

enum EPacketType : short
{
	get_data = 1,
	update_data = 2,
	no_id = 3,
};

struct check_id_t
{
	int key;
	__int64 id;
};

union uIntChar
{
	int uint;
	char uchar[sizeof(int)];
};


struct q_packet_t
{
	int code;
	__int64 token;
	std::vector<char> data;
};

template <typename T>
q_packet_t set_byte(T& data, __int64 token) {
	auto b = ((char*)(&data));
	std::vector<char> r;

	int code = typeid(T).hash_code();

	for (int i = 0; i < sizeof(int); i++) r.push_back(((char*)&code)[i]);
	for (int i = 0; i < sizeof(__int64); i++) r.push_back(((char*)&token)[i]);

	for (int i = 0; i < sizeof(T); i++) r.push_back(b[i]);

	return q_packet_t { code, token, r};
}

template <typename T>
q_packet_t get_packet_for(__int64 token)
{
	std::vector<char> r;
	int code = typeid(T).hash_code();
	for (int i = 0; i < sizeof(int); i++) r.push_back(((char*)&code)[i]);
	for (int i = 0; i < sizeof(__int64); i++) r.push_back(((char*)&token)[i]);
	return q_packet_t{ code, token, r };
}

class mem_db
{
public:
	SOCKET db_Socket;
	bool connect(std::wstring ip, int port)
	{
		WSADATA     wsaData;
		SOCKADDR_IN servAddr;

		if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) return false;

		db_Socket = socket(PF_INET, SOCK_STREAM, 0);

		if (db_Socket == INVALID_SOCKET) return false;

		memset(&servAddr, 0, sizeof(servAddr));
		servAddr.sin_family = AF_INET;
		servAddr.sin_port = htons(port);
		InetPton(AF_INET, ip.c_str(), &servAddr.sin_addr.s_addr);

		if (::connect(db_Socket, (SOCKADDR*)&servAddr, sizeof(servAddr)) == SOCKET_ERROR) return false;
		return true;
	}
	void relese()
	{
		closesocket(db_Socket);
		WSACleanup();
	}

	void send_packet_update(q_packet_t data)
	{
		auto& packet = data.data;
		uIntChar itoc;
		itoc.uint = packet.size() + sizeof(int) + sizeof(short);
		EPacketType type = EPacketType::update_data;
		packet.insert(packet.begin(), (char*)&type, (char*)&type + sizeof(short));
		packet.insert(packet.begin(), itoc.uchar, itoc.uchar + sizeof(int));

		db_save[data.code][data.token].is_update = false;

		send(db_Socket, &packet[0], packet.size(), 0);
	}

	void send_packet_get(q_packet_t data)
	{
		if (db_save[data.code][data.token].is_update) {
			db_recv[data.code](data.token, (char*)&(db_save[data.code][data.token].data[0]));
			return;
		}
		auto& packet = data.data;
		uIntChar itoc;
		itoc.uint = packet.size() + sizeof(int) + sizeof(short);
		EPacketType type = EPacketType::get_data;
		packet.insert(packet.begin(), (char*)&type, (char*)&type + sizeof(short));
		packet.insert(packet.begin(), itoc.uchar, itoc.uchar + sizeof(int));
		send(db_Socket, &packet[0], packet.size(), 0);
	}

	struct db_save_file_t {
		bool is_update = false;
		std::vector<char> data;
	};
	std::vector<char> buffer;
	std::unordered_map<int, std::function<void(__int64, char*)>> db_recv;
	std::function<void(check_id_t)> no_id;
	std::unordered_map<int, std::unordered_map<__int64, db_save_file_t>> db_save;

	void packet_reader_run(char* msg, int len)
	{
		uIntChar size;
		for (int i = 0; i < len; ++i) buffer.push_back(msg[i]);
		if (buffer.size() <= sizeof(int)) return;
		std::copy(&buffer[0], &buffer[0] + sizeof(int), size.uchar);
		if (buffer.size() >= size.uint) {
			int start_key_idx = sizeof(int);
			int start_id_idx = sizeof(int) + sizeof(int);
			int key = *((int*)(&buffer[start_key_idx]));
			__int64 id = *((__int64*)(&buffer[start_id_idx]));

			if (key == NOID_KEY) {
				no_id(*((check_id_t*)(&buffer[start_id_idx])));
			}
			else {
				db_save[key][id].is_update = true;
				db_save[key][id].data.resize((int)(buffer.size() - (start_id_idx + sizeof(__int64))));
				std::copy(buffer.begin() + (start_id_idx + sizeof(__int64)), buffer.end(), db_save[key][id].data.begin());
				db_recv[key](id, (char*)&(buffer[start_id_idx + sizeof(__int64)]));
			}

			std::vector<char> new_buffer;
			if (buffer.size() > size.uint) {
				for (int i = 0; i < buffer.size() - size.uint; i++) {
					new_buffer.push_back(buffer[size.uint + i]);
				}
				buffer = new_buffer;
			}
			else {
				buffer.clear();
			}
		}
	}
};
