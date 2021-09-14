#pragma once
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>
#include <iostream>
#include <optional>

#define lmd(x) [&]{ return x; }

auto error = [](auto&& condition, auto log, auto str) {
    bool b = condition();
    if (!b) {
        if (log) log.value().append(str).append("\n");
        else log = str;
    }
    return b;
};

int server_main(int port) {
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
        printf("%s Connection Complete!\n", inet_ntoa(clntAddr.sin_addr));
        printf("Start ...\n");
    }

    closesocket(servSock);

    while (1) {
        int nRcv = recv(clntSock, message, sizeof(message) - 1, 0);
        if (error(lmd(nRcv == SOCKET_ERROR), elog, "Receive Error..")) break;
        message[nRcv] = '\0';
        send(clntSock, message, (int)strlen(message), 0);
    }

    closesocket(clntSock);
    WSACleanup();

    return 0;
}