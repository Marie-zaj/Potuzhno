#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <iostream>
#include <winsock2.h>
#include <vector>
#include <string>
#include <thread>
#include <mutex>
#include <ctime>

#pragma comment(lib, "ws2_32.lib")
using namespace std;

struct ClientInfo
{
    string login;
    string password;
    string ip;
    int port;
    SOCKET socket;
};

vector<ClientInfo> clients;
mutex mtx;

string getTime()
{
    time_t now = time(0);
    tm t;
    localtime_s(&t, &now);
    char buf[20];
    strftime(buf, sizeof(buf), "%H:%M:%S", &t);
    return string(buf);
}

void broadcast(string msg)
{
    lock_guard<mutex> lock(mtx);
    for (auto& c : clients)
    {
        send(c.socket, msg.c_str(), msg.size(), 0);
    }
}

void handleClient(SOCKET sock, sockaddr_in addr)
{
    char buffer[512];

    int bytes = recv(sock, buffer, 512, 0);
    string auth(buffer, bytes);

    string login = auth.substr(0, auth.find(':'));
    string pass = auth.substr(auth.find(':') + 1);

    string ip = inet_ntoa(addr.sin_addr);
    int port = ntohs(addr.sin_port);

    {
        lock_guard<mutex> lock(mtx);
        bool found = false;

        for (auto& c : clients)
        {
            if (c.login == login && c.password == pass)
                found = true;
        }

        if (!found)
            clients.push_back({ login, pass, ip, port, sock });
    }

    string msg = login + " connected [" + getTime() + "]\n";
    cout << msg;
    broadcast(msg);

    while (true)
    {
        int len = recv(sock, buffer, 512, 0);
        if (len <= 0) break;

        string text(buffer, len);
        string full = login + ": " + text + " [" + getTime() + "]\n";

        cout << full;
        broadcast(full);
    }

    string left = login + " disconnected [" + getTime() + "]\n";
    cout << left;
    broadcast(left);

    closesocket(sock);
}

int main()
{
    WSAData wsa;
    WSAStartup(MAKEWORD(2, 2), &wsa);

    SOCKET server = socket(AF_INET, SOCK_STREAM, 0);

    sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(27015);
    addr.sin_addr.s_addr = INADDR_ANY;

    bind(server, (sockaddr*)&addr, sizeof(addr));
    listen(server, 5);

    cout << "Server started...\n";

    while (true)
    {
        sockaddr_in clientAddr;
        int size = sizeof(clientAddr);

        SOCKET client = accept(server, (sockaddr*)&clientAddr, &size);
        thread(handleClient, client, clientAddr).detach();
    }
}