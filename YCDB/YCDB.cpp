#include "server.hpp"
#include <iostream>
#include <algorithm>
#include <vector>
#include "packet_reader.hpp"
#include <filesystem>
#include <io.h>
#include <fstream>
#include <direct.h>

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
	int id;
};

union uIntChar
{
	int uint;
	char uchar[sizeof(int)];
};

using std::string;

std::vector<char> buffer;

std::vector<std::string> get_files_inDirectory(const std::string& _path, const std::string& _filter)
{
	std::string searching = _path + _filter;
	std::vector<std::string> return_;
	_finddata_t fd;
	long handle = _findfirst(searching.c_str(), &fd);  //현재 폴더 내 모든 파일을 찾는다.
	if (handle == -1)    return return_;
	int result = 0;
	do
	{
		return_.push_back(fd.name);
		result = _findnext(handle, &fd);
	} while (result != -1);
	_findclose(handle);
	return return_;
}

void save_to_directory(string key, string id, std::vector<char>& datas) {
	string DB_dirname("DB");
	if (_access(DB_dirname.c_str(), 0) == -1) 
		std::filesystem::create_directory(DB_dirname);
		
	if (_access(("DB\\" + key).c_str(), 0) == -1)
		std::filesystem::create_directory("DB\\" + key);
		
	std::ofstream wf;
	wf.open(std::format("{}\\{}\\{}.txt", DB_dirname, key, id));
	if (wf.is_open()) wf.write(&datas[0], datas.size());
	wf.close(); 
}


void load_to_directory() {
	char curDir[1000];
	_getcwd(curDir, 1000);

	auto DB_path = std::format("{}\\DB", curDir);

	for (const auto& key_ : std::filesystem::directory_iterator(DB_path))
	{
		auto path = key_.path().string();
		std::string dirName;
		for (int i = 0; i < path.size(); i++) {
			if ('\\' == path[i]) {
				dirName.clear();
			} else {
				dirName.push_back(path[i]);
			}
		}
		for (auto& i : std::filesystem::directory_iterator(path)) {
			int key = std::stoi(dirName);

			auto path = i.path().string();
			std::string fileName;
			for (int i = 0; i < path.size(); i++) {
				if ('\\' == path[i]) {
					fileName.clear();
				}
				else {
					fileName.push_back(path[i]);
				}
			}
			int id = std::stoi(fileName);
			std::ifstream ist(key_.path().string() + "\\" + fileName);
			while (!ist.eof())
			{
				char c;
				ist.get(c);
				DB_ROW[key][id].push_back(c);
			}
			ist.close();
		}
	}
}


int main() {
	load_to_directory();
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
			if (packet_type == EPacketType::update_data) {
				DB_ROW[key][id].clear();
				for (int i = start_id_idx + sizeof(int); i < size.uint; ++i) DB_ROW[key][id].push_back(buffer[i]);
			} else if (packet_type == EPacketType::get_data) {
				if (DB_ROW[key].find(id) == DB_ROW[key].end()) {
					check_id_t p{ key ,id};
					key = NOID_KEY;
					std::vector<char> r;
					for (int i = 0; i < sizeof(int); i++) r.push_back(((char*)&key)[i]);
					for (int i = 0; i < sizeof(check_id_t); i++) { r.push_back(((char*)(&p))[i]); }
					uIntChar itoc;
					itoc.uint = r.size() + sizeof(int);
					r.insert(r.begin(), itoc.uchar, itoc.uchar + sizeof(int));
					send(s, &r[0], r.size(), 0);
				} else {
					std::vector<char> r;
					for (int i = 0; i < sizeof(int); i++) r.push_back(((char*)&key)[i]);
					for (int i = 0; i < sizeof(int); i++) r.push_back(((char*)&id)[i]);
					for (auto& i : DB_ROW[key][id]) { r.push_back(i); }
					uIntChar itoc;
					itoc.uint = r.size() + sizeof(int);
					r.insert(r.begin(), itoc.uchar, itoc.uchar + sizeof(int));
					send(s, &r[0], r.size(), 0);
				}
			}

			std::vector<char> new_buffer;
			for (int i = size.uint; i < buffer.size(); ++i) new_buffer.push_back(buffer[i]);
			buffer = new_buffer;
		}
	},
	[&] {
		for (auto& i : DB_ROW) {
			auto& d = i.second;
			for (auto& j : d) {
				save_to_directory(std::to_string(i.first), std::to_string(j.first), j.second);
			}
			
		}
	});
}