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

enum EPacketType : short
{
	get_data = 1,
	update_data = 2,
};

union uIntChar
{
	int uint;
	char uchar[sizeof(int)];
};
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

	void send_packet_update(std::vector<char> packet)
	{
		uIntChar itoc;
		itoc.uint = packet.size() + sizeof(int) + sizeof(short);
		EPacketType type = EPacketType::update_data;
		packet.insert(packet.begin(), (char*)&type, (char*)&type + sizeof(short));
		packet.insert(packet.begin(), itoc.uchar, itoc.uchar + sizeof(int));
		send(db_Socket, &packet[0], packet.size(), 0);
	}

	void send_packet_get(std::vector<char> packet)
	{
		uIntChar itoc;
		itoc.uint = packet.size() + sizeof(int) + sizeof(short);
		EPacketType type = EPacketType::get_data;
		packet.insert(packet.begin(), (char*)&type, (char*)&type + sizeof(short));
		packet.insert(packet.begin(), itoc.uchar, itoc.uchar + sizeof(int));
		send(db_Socket, &packet[0], packet.size(), 0);
	}

	std::vector<char> buffer;
	std::unordered_map<int, std::function<void(int, char*)>> db_recv;

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
			int id = *((int*)(&buffer[start_id_idx]));
			db_recv[key](id, (char*)&(buffer[start_id_idx+sizeof(int)]));

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
