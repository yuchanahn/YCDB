#include <iostream>
#include <thread>

#include "yc_mem_db_client.hpp"


struct p_test_1
{
    int num;
};


int main()
{
    mem_db db;

    db.db_recv[typeid(p_test_1).hash_code()] = [](int user_number, char* buffer) {
        auto data = *((p_test_1*)buffer);
        printf("data[%d] : %d\n", user_number, data.num);
    };
    db.no_id = [](check_id_t c) {
        printf("no id To [%d][%lld]\n", c.key, c.id);
    };

    db.connect(L"127.0.0.1", 61234);
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
        if (p.num != -1) db.send_packet_update(set_byte(p, 3));
        db.send_packet_get(get_packet_for<p_test_1>(3));
    }

    db.relese();
    read.join();
    return 1;
}