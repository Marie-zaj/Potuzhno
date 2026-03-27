#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <iostream>
#include <winsock2.h>
#include <vector>
#include <thread>
#include <mutex>
#include <ctime>

#pragma comment(lib, "ws2_32.lib")
using namespace std;

struct ClientInfo {
    string login;
    string password;
    string ip;
    int port;
    SOCKET socket;
};

vector<ClientInfo> clients;
mutex mtx;

string getTime() {
    time_t now = time(0);
    tm t;
    localtime_s(&t, &now);
    char buf[10];
    strftime(buf, sizeof(buf), "%H:%M:%S", &t);
    return string(buf);
}

void broadcast(string msg) {
    lock_guard<mutex> lock(mtx);
    for (auto& c : clients)
        send(c.socket, msg.c_str(), msg.size(), 0);
}

void handleClient(SOCKET sock, sockaddr_in addr) {
    char buffer[512];

    int len = recv(sock, buffer, 512, 0);
    string auth(buffer, len);

    string login = auth.substr(0, auth.find(':'));
    string pass = auth.substr(auth.find(':') + 1);

    {
        lock_guard<mutex> lock(mtx);
        clients.push_back({ login, pass, "", 0, sock });
    }

    string join = login + " connected\n";
    broadcast(join);

    while (true) {
        int bytes = recv(sock, buffer, 512, 0);
        if (bytes <= 0) break;

        string msg(buffer, bytes);
        string full = "[" + getTime() + "] " + login + ": " + msg + "\n";

        cout << full;
        broadcast(full);
    }

    closesocket(sock);
}

int main() {
    WSAData wsa;
    WSAStartup(MAKEWORD(2, 2), &wsa);

    SOCKET server = socket(AF_INET, SOCK_STREAM, 0);

    sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(27015);
    addr.sin_addr.s_addr = INADDR_ANY;

    bind(server, (sockaddr*)&addr, sizeof(addr));
    listen(server, 5);

    while (true) {
        sockaddr_in clientAddr;
        int size = sizeof(clientAddr);
        SOCKET client = accept(server, (sockaddr*)&clientAddr, &size);

        thread(handleClient, client, clientAddr).detach();
    }
}