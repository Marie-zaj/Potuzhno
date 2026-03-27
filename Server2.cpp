#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include <iostream>
#include <winsock2.h>
#include <vector>
#include <thread>
#include <mutex>
#include <fstream>
#include <ctime>

#pragma comment(lib, "ws2_32.lib")
using namespace std;

struct ClientInfo {
    string login;
    string password;
    int color;
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
    for (auto& c : clients) {
        send(c.socket, msg.c_str(), msg.size(), 0);
    }
}

void handleClient(SOCKET sock) {
    char buffer[512];

    // авторизация
    int len = recv(sock, buffer, 512, 0);
    if (len <= 0) return;

    string auth(buffer, len);
    string login = auth.substr(0, auth.find(':'));
    string pass = auth.substr(auth.find(':') + 1);

    int color;
    recv(sock, (char*)&color, sizeof(color), 0);

    // проверка файла
    bool returning = false;
    ifstream in("clients.txt");

    string l, p;
    while (in >> l >> p) {
        if (l == login && p == pass)
            returning = true;
    }
    in.close();

    if (!returning) {
        ofstream out("clients.txt", ios::app);
        out << login << " " << pass << "\n";
        out.close();
    }
    else {
        string msg = "Welcome back, " + login + "\n";
        send(sock, msg.c_str(), msg.size(), 0);
    }

    {
        lock_guard<mutex> lock(mtx);
        clients.push_back({ login, pass, color, sock });
    }

    string join = login + " connected [" + getTime() + "]\n";
    cout << join;
    broadcast(join);

    while (true) {
        int bytes = recv(sock, buffer, 512, 0);
        if (bytes <= 0) break;

        string msg(buffer, bytes);
        string full = "[" + getTime() + "] " + login + ": " + msg + "\n";

        cout << full;
        broadcast(full);
    }

    string left = login + " disconnected [" + getTime() + "]\n";
    cout << left;
    broadcast(left);

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

    cout << "Server started...\n";

    while (true) {
        SOCKET client = accept(server, NULL, NULL);
        thread(handleClient, client).detach();
    }
}