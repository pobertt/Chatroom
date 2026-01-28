#pragma once
#include <string>
#include <vector>
#include <iostream>
#include <winsock2.h>
#include <ws2tcpip.h>

#pragma comment(lib, "Ws2_32.lib")

class NetworkClient {
public:
    bool Initialize() {
        WSADATA wsaData;
        int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
        if (result != 0) {
            std::cerr << "WSAStartup failed with error: " << result << std::endl;
            return false;
        }
        return true;
    }

    bool Connect(const std::string& ipAddress, int port) {
        std::cout << "[DEBUG] Creating Socket..." << std::endl;
        m_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (m_socket == INVALID_SOCKET) {
            std::cout << "[DEBUG] Socket failed. Error: " << WSAGetLastError() << std::endl;
            return false;
        }

        sockaddr_in server_address = {};
        server_address.sin_family = AF_INET;
        server_address.sin_port = htons(port);
        inet_pton(AF_INET, ipAddress.c_str(), &server_address.sin_addr);

        std::cout << "[DEBUG] Connecting to " << ipAddress << ":" << port << "..." << std::endl;

        // Connect (Blocking for safety test)
        int connResult = connect(m_socket, reinterpret_cast<sockaddr*>(&server_address), sizeof(server_address));

        if (connResult == SOCKET_ERROR) {
            int err = WSAGetLastError();
            std::cout << "[DEBUG] CONNECT FAILED! Error Code: " << err << std::endl;
            closesocket(m_socket);
            return false; // prevents the UI from switching
        }

        std::cout << "[DEBUG] Connection Successful!" << std::endl;

        // Switch to Non-Blocking Mode ONLY after success
        u_long mode = 1;
        ioctlsocket(m_socket, FIONBIO, &mode);

        m_isConnected = true;
        return true;
    }
    
    void SendMessage(const std::string& msg)
    {
        if (!m_isConnected) {
            std::cout << "[ERROR] Cannot send: Not connected!" << std::endl;
            return;
        }

        // Add +1 to include the null terminator so the server knows when the string ends
        int sendResult = send(m_socket, msg.c_str(), msg.size() + 1, 0);

        if (sendResult == SOCKET_ERROR) {
            std::cout << "[ERROR] Send failed! Error: " << WSAGetLastError() << std::endl;

            // If send fails, we are probably disconnected
            closesocket(m_socket);
            m_isConnected = false;
        }
        else {
            std::cout << "[DEBUG] Sent " << sendResult << " bytes to server." << std::endl;
        }
    }

    // Update
    bool ReceiveMessage(std::string& outMessage) {
        if (!m_isConnected) return false;

        char buffer[4096];
        // We use the same recv logic, but we must accept WSAEWOULDBLOCK as "Normal"
        int bytes_received = recv(m_socket, buffer, 4096 - 1, 0);

        if (bytes_received > 0) {
            buffer[bytes_received] = '\0'; // Null-terminate
            outMessage = std::string(buffer);
            std::cout << "Received from server: " << outMessage << std::endl;
            return true;
        }
        else if (bytes_received == 0) {
            // Server closed connection
            std::cout << "Connection closed by server." << std::endl;
            Shutdown();
        }
        else {
            int error = WSAGetLastError();
            if (error != WSAEWOULDBLOCK) {
                std::cerr << "Receive failed with error: " << error << std::endl;
                Shutdown();
            }
        }

        return false;
    }

    // Cleanup
    void Shutdown() {
        if (m_isConnected) {
            closesocket(m_socket);
            m_isConnected = false;
            std::cout << "Closing connection" << std::endl;
        }
    }

    bool IsConnected() const { return m_isConnected; }

private:
    SOCKET m_socket = INVALID_SOCKET;
    bool m_isConnected = false;
};