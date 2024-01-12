#include <stdio.h>
#include <winsock2.h>
#include <ws2tcpip.h>

int main() {
    WSADATA wsaData;
    SOCKET mySocket;
    struct sockaddr_in server;
    int connectResult;
    char *address = "79.127.127.35";  
    int port = 445;

    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        printf("Failed to initialize Winsock.\n");
        return 1;
    }

    mySocket = socket(AF_INET, SOCK_STREAM, 0);


    if (mySocket == INVALID_SOCKET) {
        printf("Failed to create socket.\n");
        WSACleanup();
        return 1;
    }


    server.sin_addr.s_addr = inet_addr(address);
    server.sin_family = AF_INET;
    server.sin_port = htons(port);


    connectResult = connect(mySocket, (struct sockaddr*)&server, sizeof(server));
    if (connectResult == SOCKET_ERROR) {
        printf("Host is offline.\n");
        printf("Or you may have entered a closed or wrong port number.\n");
    } else {
        printf("Host is online.\n");
    }

    closesocket(mySocket);
    WSACleanup();

    return 0;
}