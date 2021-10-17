#pragma once
#include <unordered_map>
#include <map>


#pragma push(1)

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
#pragma pop()

std::map<size_t, std::unordered_map<size_t, char*>> DB_ROW;