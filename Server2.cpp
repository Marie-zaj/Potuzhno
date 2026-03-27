#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <iostream>
#include <winsock2.h>
#include <vector>
#include <thread>
#include <mutex>
#include <fstream>

#pragma comment(lib, "ws2_32.lib")
using namespace std;

struct ClientInfo {
    string login;
    string password;
    string ip;
    int port;
    int color;
    SOCKET socket;
};

vector<ClientInfo> clients;
mutex mtx;

void handleClient(SOCKET sock, sockaddr_in addr) {
    char buffer[512];

    int len = recv(sock, buffer, 512, 0);
    string auth(buffer, len);

    string login = auth.substr(0, auth.find(':'));
    string pass = auth.substr(auth.find(':') + 1);

    int color;
    recv(sock, (char*)&color, sizeof(color), 0);

    bool returning = false;
    ifstream in("clients.txt");

    string l, p, ipf;
    int portf;

    while (in >> l >> p >> ipf >> portf) {
        if (l == login && p == pass)
            returning = true;
    }
    in.close();

    if (!returning) {
        ofstream out("clients.txt", ios::app);
        out << login << " " << pass << " 0 0\n";
        out.close();
    }
    else {
        string msg = "Welcome back, " + login + "\n";
        send(sock, msg.c_str(), msg.size(), 0);
    }

    {
        lock_guard<mutex> lock(mtx);
        clients.push_back({ login, pass, "", 0, color, sock });
    }

    while (true) {
        int bytes = recv(sock, buffer, 512, 0);
        if (bytes <= 0) break;

        string msg(buffer, bytes);
        string full = login + ": " + msg + "\n";

        for (auto& c : clients)
            send(c.socket, full.c_str(), full.size(), 0);
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