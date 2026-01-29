#pragma once
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iostream>
#include <string>
#include <vector>
#include <thread>
#include <mutex>

#pragma comment(lib, "Ws2_32.lib")

SOCKET global_socket = INVALID_SOCKET;
std::vector<std::string> chat_messages; // Replaces 'std::cout'
std::mutex chat_mutex;
bool is_connected = false;

// Saves text to a list instead of printing it.
void Receive() {
    char buffer[1024];
    while (is_connected) {
        ZeroMemory(buffer, 1024);
        int bytes = recv(global_socket, buffer, 1024, 0);

        if (bytes > 0) {
            std::string msg(buffer, 0, bytes);

            // Store safely for ImGui to read
            std::lock_guard<std::mutex> lock(chat_mutex);
            chat_messages.push_back(msg);
        }
        else {
            is_connected = false;
            break;
        }
    }
}

// Sets up the socket and starts the Receive thread
bool Connect(const std::string& ip) {
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);

    global_socket = socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in server = {};
    server.sin_family = AF_INET;
    server.sin_port = htons(65432);
    inet_pton(AF_INET, ip.c_str(), &server.sin_addr);

    if (connect(global_socket, (sockaddr*)&server, sizeof(server)) == SOCKET_ERROR) {
        return false;
    }

    is_connected = true;

    // Start the Receiver thread (Background listener)
    std::thread t(Receive);
    t.detach();

    return true;
}

// Called by ImGui when you press "Send"
void SendString(const std::string& msg) {
    if (is_connected) {
        send(global_socket, msg.c_str(), (int)msg.size() + 1, 0);
    }
}