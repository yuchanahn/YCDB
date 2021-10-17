#pragma once
#include <iostream>
#include <optional>
#include <functional>

#ifdef _WIN32
// windows
#pragma comment(lib, "ws2_32.lib")
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>
#elif __APPLE__
#include "TargetConditionals.h"
#elif __linux__
// linux
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <cstring>
#include <arpa/inet.h>
#elif __unix__ // all unices not caught above
// Unix
#elif defined(_POSIX_VERSION)
// POSIX
#else
#   error "Unknown compiler"
#endif

#define lmd(x) [&]{ return x; }

auto error = [](auto condition, auto log, auto str) {
    bool b = condition();
    if (b) {
        if (log) log.value().append(str).append("\n");
        else log = str;
    }
    return b;
};

int server_main(int port, std::function<void(char*,int,int)> read_func) {
#ifdef _WIN32
    WSADATA wsaData;
    SOCKET servSock, clntSock;
    SOCKADDR_IN servAddr, clntAddr;
    std::optional<std::string> elog;

    char message[1024];

    error(lmd(WSAStartup(MAKEWORD(2, 2), &wsaData) != 0), elog, "Load WinSock 2.2 DLL Error");

    servSock = socket(PF_INET, SOCK_STREAM, 0);
    error(lmd(servSock == INVALID_SOCKET), elog, "Socket Error");

    memset(&servAddr, 0, sizeof(SOCKADDR_IN));
    servAddr = SOCKADDR_IN{ AF_INET, htons(port) };
    servAddr.sin_addr.s_addr = htonl(INADDR_ANY);

    error(lmd(bind(servSock, (sockaddr*)&servAddr, sizeof(servAddr)) == SOCKET_ERROR), elog, "Bind Error");
    error(lmd(listen(servSock, 2) == SOCKET_ERROR), elog, "Listen Error");

    int fromLen = sizeof(clntAddr);
    clntSock = accept(servSock, (sockaddr*)&clntAddr, &fromLen);

    if (!error(lmd(clntSock == INVALID_SOCKET), elog, "Accept Error")) {
        printf("Start ...\n");
    }
    else {
        closesocket(servSock);
    }

    while (1) {
        int nRcv = recv(clntSock, message, sizeof(message) - 1, 0);
        if (error(lmd(nRcv == SOCKET_ERROR), elog, "Receive Error..")) break;
        read_func(message, nRcv, static_cast<int>(clntSock));
        //send(clntSock, message, (int)strlen(message), 0);
    }

    closesocket(clntSock);
    WSACleanup();
#elif __linux__
    //================================================
    unsigned int rl;
    int list_sock;
    int clnt_sock;
    char buf[1024];
    char bufSend[1024];

    sockaddr_in server_addr, client_addr;
    list_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    error(lmd(list_sock < 0), elog, "socket error");
    int on = 1;
    error(lmd(
        setsockopt(list_sock, SOL_SOCKET, SO_REUSEADDR, (const char*)&on, sizeof(on))
        < 0),
        elog, "socket opt error");

    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    error(lmd(bind(list_sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0), elog, "bind error");
    error(lmd(listen(list_sock, 1024) < 0), elog, "listen error");
    int clientAddrSize = sizeof(client_addr);
    while (true)
    {
        clnt_sock = accept(list_sock, (struct sockaddr*)&client_addr, (socklen_t*)&clientAddrSize);
        if (error(lmd(clnt_sock < 0), elog, "accept error")) return 1;

        while (true)
        {
            memset(buf, 0, 1024);
            rl = recv(clnt_sock, buf, 1024, 0);
            if (rl == 0)
            {
                close(clnt_sock);
                break;
            }
            if (rl > 1024)
            {
                close(clnt_sock);
                break;
            }
            if (rl > 0) read_func(buf, rl, clnt_sock);
        }
    }
    close(list_sock);
#endif
    return 0;
};