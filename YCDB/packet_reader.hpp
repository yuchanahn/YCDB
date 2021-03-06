#pragma once
#include <unordered_map>
#include <map>
#include <vector>

#pragma pack(push, 1)

template <typename DATA_TYPE>
struct packet_t
{
	int size;
	char buf[sizeof(DATA_TYPE)];
};

template <typename DATA_TYPE>
union upacket_t
{
	DATA_TYPE data;
	char buf[sizeof(DATA_TYPE)];
};
#pragma pack(pop)

std::map<int, std::unordered_map<__int64, std::vector<char>>> DB_ROW;