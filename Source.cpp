#include <ws2tcpip.h>
#include <windows.h>
#include <iostream>
#include <string>

#pragma comment(lib, "Ws2_32.lib")

#define DEFAULT_BUFLEN 4096
#define SERVER_IP "127.0.0.1"
#define DEFAULT_PORT "8888"

using namespace std;

SOCKET client_socket;
string nickname;

DWORD WINAPI Sender(void* param) {
    while (true) {
        char query[DEFAULT_BUFLEN];

        cin.getline(query, DEFAULT_BUFLEN);

        int len = strlen(query);
        if (len == 0) continue;

        int sent = send(client_socket, query, len, 0);

        if (sent == SOCKET_ERROR) {
            cout << "Ошибка send: " << WSAGetLastError() << endl;
            break;
        }
    }
    return 0;
}

DWORD WINAPI Receiver(void* param) {
    while (true) {
        char response[DEFAULT_BUFLEN];

        int result = recv(client_socket, response, DEFAULT_BUFLEN - 1, 0);

        if (result > 0) {
            response[result] = '\0';
            cout << "\n" << response << endl;
        }
        else if (result == 0) {
            cout << "\nСервер закрыл соединение.\n";
            break;
        }
        else {
            cout << "\nОшибка recv: " << WSAGetLastError() << endl;
            break;
        }
    }
    return 0;
}

BOOL ExitHandler(DWORD whatHappening) {
    if (whatHappening == CTRL_CLOSE_EVENT ||
        whatHappening == CTRL_C_EVENT) {

        send(client_socket, "off", 3, 0);
        closesocket(client_socket);
        WSACleanup();
    }
    return TRUE;
}

int main() {
    SetConsoleOutputCP(1251);
    SetConsoleCP(1251);

    SetConsoleCtrlHandler(ExitHandler, TRUE);

    system("title Клиент");

    cout << "Введите ваш ник: ";
    getline(cin, nickname);

    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        cout << "WSAStartup ошибка\n";
        return 1;
    }

    addrinfo hints{}, * result = nullptr;

    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    if (getaddrinfo(SERVER_IP, DEFAULT_PORT, &hints, &result) != 0) {
        cout << "getaddrinfo ошибка\n";
        WSACleanup();
        return 2;
    }

    client_socket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);

    if (client_socket == INVALID_SOCKET) {
        cout << "Ошибка создания сокета\n";
        freeaddrinfo(result);
        WSACleanup();
        return 3;
    }

    if (connect(client_socket, result->ai_addr, (int)result->ai_addrlen) == SOCKET_ERROR) {
        cout << "Ошибка подключения\n";
        closesocket(client_socket);
        WSACleanup();
        return 4;
    }

    freeaddrinfo(result);

    cout << "Подключено к серверу\n";

    // отправляем ник сразу после подключения
    send(client_socket, nickname.c_str(), nickname.size(), 0);

    CreateThread(NULL, 0, Sender, NULL, 0, NULL);
    CreateThread(NULL, 0, Receiver, NULL, 0, NULL);

    while (true) Sleep(1000);
}