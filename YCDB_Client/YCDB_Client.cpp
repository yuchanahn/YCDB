#include <iostream>
#include <thread>

#include "yc_mem_db_client.hpp"


struct p_test_1
{
    int num;
};

template <typename T>
std::vector<char> set_byte(T& data, int token) {
    auto b = ((char*)(&data));
    std::vector<char> r;

    int code = typeid(T).hash_code();

    for (int i = 0; i < sizeof(int); i++) r.push_back(((char*)&code)[i]);
    for (int i = 0; i < sizeof(int); i++) r.push_back(((char*)&token)[i]);

    for (int i = 0; i < sizeof(T); i++) r.push_back(b[i]);
    return r;
}

template <typename T>
std::vector<char> get_packet_for(int token)
{
    std::vector<char> r;
    int code = typeid(T).hash_code();
    for (int i = 0; i < sizeof(int); i++) r.push_back(((char*)&code)[i]);
    for (int i = 0; i < sizeof(int); i++) r.push_back(((char*)&token)[i]);
    return r;
}


int main()
{

    mem_db db;

    db.db_recv[typeid(p_test_1).hash_code()] = [](int user_number, char* buffer) {
        auto data = *((p_test_1*)buffer);
        printf("data[%d] : %d\n", user_number, data.num);
    };

    db.connect(L"172.30.1.200", 61234);

    char msg[2024];

    std::thread read{ [&] {
        while (1) {
            auto strLen = recv(db.db_Socket, msg, sizeof(msg) - 1, 0);
            if (strLen == -1)
                return false;
            db.packet_reader_run(msg, strLen);
        }
    } };

    while (1) {
        p_test_1 p;
        std::cin >> p.num;
        if (p.num == -99) break;
        if (p.num != -1) db.send_packet_update(set_byte(p, 1));
        db.send_packet_get(get_packet_for<p_test_1>(1));
    }

    db.relese();
    read.join();
    return 1;
}