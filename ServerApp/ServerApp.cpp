#define WIN32_LEAN_AND_MEAN

#include <Windows.h>
#include <winsock2.h>
#include <WS2tcpip.h>
#include <iostream>
#include <conio.h>
#define DEFAULT_PORT "27015"
#define DEFAULT_BUFLEN 512

#pragma comment(lib, "Ws2_32.lib")

using namespace std;

struct Client {

    bool active = false;

    SOCKET socket = INVALID_SOCKET;

    HANDLE ListenThread = NULL;

    char name[255];

    ULONGLONG lastMessage = GetTickCount64();

};

SOCKET ListenSocket = INVALID_SOCKET;
Client clients[256];
int index = 0;

HANDLE ClientJoinedEvent;

void SendToAllClients(char message[DEFAULT_BUFLEN], int excludeIndex)
{
    for (int i = 0; i < index; i++)
    {
        if (i == excludeIndex)
            continue;
        if (clients[i].active == false)
            continue;

        int iResult;
        iResult = send(clients[i].socket, message, (int)strlen(message), 0);

        if (iResult == SOCKET_ERROR) {
            cout << "[LOG]: send failed for " << clients[i].name
                << " error: " << WSAGetLastError() << endl;
            continue;
        }
    }
}

DWORD WINAPI ListenThread(LPVOID id)
{
    int clientIndex = (int)id;

    do
    {
        int result;
        char message[DEFAULT_BUFLEN];
        result = recv(clients[clientIndex].socket, message, DEFAULT_BUFLEN, 0);

        if (result > 0) {
            message[result] = '\0';

            if (strcmp(message, "/exit") == 0) {
                cout << "[LOG]: " << clients[clientIndex].name << " typed /exit" << endl;
                cout << "[LOG]: " << clients[clientIndex].name << " has left" << endl;

                char clientMessage[DEFAULT_BUFLEN]{ 0 };

                strcat_s(clientMessage, sizeof(clientMessage), "\033[31m");
                strcat_s(clientMessage, sizeof(clientMessage), "[SERVER]: user ");
                strcat_s(clientMessage, sizeof(clientMessage), clients[clientIndex].name);
                strcat_s(clientMessage, sizeof(clientMessage), " has left the chat");
                strcat_s(clientMessage, sizeof(clientMessage), "\n");
                strcat_s(clientMessage, sizeof(clientMessage), "\033[0m");

                SendToAllClients(clientMessage, clientIndex);

                clients[clientIndex].active = false;
                break;
            }
            else if (strcmp(message, "/users") == 0)
            {
                cout << "[LOG]: " << clients[clientIndex].name << " typed /users" << endl;
                cout << "[LOG]: " << clients[clientIndex].name << " asked for users list" << endl;
                int iResult;

                char usersText[4096]{ 0 };

                for (int i = 0; i < index; i++)
                {
                    if (!clients[i].active)
                        continue;

                    strcat_s(usersText, sizeof(usersText), "\033[32m");
                    strcat_s(usersText, sizeof(usersText), clients[i].name);
                    strcat_s(usersText, sizeof(usersText), "\n");
                }

                strcat_s(usersText, sizeof(usersText), "\033[0m");

                iResult = send(clients[clientIndex].socket, usersText, (int)strlen(usersText), 0);
                if (iResult == SOCKET_ERROR) {
                    cout << "[LOG]: send failed for " << clients[clientIndex].name
                        << " error: " << WSAGetLastError() << endl;
                    clients[clientIndex].active = false;
                    break;
                }
            }
            else {
                if (GetTickCount64() - clients[clientIndex].lastMessage < 1000)
                {
                    char msg[DEFAULT_BUFLEN] = "\033[31m[SERVER]: DONT SPAM\n\033[0m";
                    int iResult;

                    iResult = send(clients[clientIndex].socket, msg, (int)strlen(msg), 0);
                    if (iResult == SOCKET_ERROR) {
                        cout << "[LOG]: send failed for " << clients[clientIndex].name
                            << " error: " << WSAGetLastError() << endl;
                        clients[clientIndex].active = false;
                        break;
                    }

                    continue;
                }
                cout << "[LOG]: [" << clients[clientIndex].name << "]: " << message << endl;

                char clientMessage[DEFAULT_BUFLEN] { 0 };
                strcat_s(clientMessage, sizeof(clientMessage), "\033[36m");
                strcat_s(clientMessage, sizeof(clientMessage), "[");
                strcat_s(clientMessage, sizeof(clientMessage), clients[clientIndex].name);
                strcat_s(clientMessage, sizeof(clientMessage), "]: ");
                strcat_s(clientMessage, sizeof(clientMessage), message);
                strcat_s(clientMessage, sizeof(clientMessage), "\n");
                strcat_s(clientMessage, sizeof(clientMessage), "\033[0m");

                SendToAllClients(clientMessage, clientIndex);

                clients[clientIndex].lastMessage = GetTickCount64();
            }
        }
        else if (result == 0) {
            cout << "[LOG]: " << clients[clientIndex].name << " disconnected" << endl;
            clients[clientIndex].active = false;
            break;
        }
        else {
            int error = WSAGetLastError();

            if (error == WSAECONNRESET) {
                cout << "[LOG]: " << clients[clientIndex].name
                    << " disconnected" << endl;
            }
            else if (error == WSAENOTSOCK) {
                cout << "[LOG]: " << clients[clientIndex].name
                    << " disconnected" << endl;
            }
            else if (error == WSAETIMEDOUT) {
                cout << "[LOG]: " << clients[clientIndex].name
                    << " disconnected" << endl;
            }
            else {
                cout << "[LOG]: recv failed for " << clients[clientIndex].name << " error: " << error << endl;
            }

            clients[clientIndex].active = false;
            break;
        }

    } while (clients[clientIndex].active == true);

    if (clients[clientIndex].socket != INVALID_SOCKET) {
        closesocket(clients[clientIndex].socket);
        clients[clientIndex].socket = INVALID_SOCKET;
    }

    clients[clientIndex].active = false;

    return 0;
}


DWORD WINAPI ConnectClient(LPVOID num)
{
    do {

        WaitForSingleObject(ClientJoinedEvent, INFINITE);
        ResetEvent(ClientJoinedEvent);

        SOCKET ClientSocket = INVALID_SOCKET;

        sockaddr i;

        ClientSocket = accept(ListenSocket, &i, NULL);

        if (ClientSocket == INVALID_SOCKET) {
            cout << "accept failed: " << WSAGetLastError() << endl;
            continue;
        }
        else
        {
            int result;
            char message[DEFAULT_BUFLEN];
            result = recv(ClientSocket, message, DEFAULT_BUFLEN, 0);
            if (result == SOCKET_ERROR || result == 0) {
                cout << "receiven failed, client not connected " << WSAGetLastError() << endl;
                closesocket(ClientSocket);
                continue;
            }
            else
            {
                Client client = Client();

                message[result] = '\0';
                std::cout << "[LOG]: " << message << " has joined" << std::endl;
                
                strcpy_s(client.name, message);
                client.socket = ClientSocket;
                client.active = true;
                clients[index] = client;

                DWORD idThread = index;
                HANDLE listenThread = CreateThread(NULL, 0, ListenThread, (void*)index, NULL, &idThread);

                if (listenThread == NULL)
                {
                    cout << "error creating listen thread" << endl;
                }

                char clientMessage[DEFAULT_BUFLEN]{ 0 };

                strcat_s(clientMessage, sizeof(clientMessage), "\033[32m");
                strcat_s(clientMessage, sizeof(clientMessage), "[SERVER]: user ");
                strcat_s(clientMessage, sizeof(clientMessage), clients[index].name);
                strcat_s(clientMessage, sizeof(clientMessage), " has joined the chat");
                strcat_s(clientMessage, sizeof(clientMessage), "\n");
                strcat_s(clientMessage, sizeof(clientMessage), "\033[0m");

                SendToAllClients(clientMessage, index);

                index++;
            }
        }

    } while (true);
    return 0;
}
int main()
{
    SetConsoleCP(1251);
    SetConsoleOutputCP(1251);
    setlocale(LC_ALL, "Russian");
    WSADATA wsaData;

    ClientJoinedEvent = CreateEvent(NULL, FALSE, FALSE, (LPCWSTR)"Joined");

    int iResult;

    iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (iResult != 0)
    {
        cout << "WSAStartup failed " << GetLastError() << endl;
        return 1;
    }

    struct addrinfo* result = NULL, * ptr = NULL, hints;

    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    hints.ai_flags = AI_PASSIVE;

    memset(&hints, 0, sizeof(hints));

    iResult = getaddrinfo(NULL, DEFAULT_PORT, &hints, &result);
    if (iResult != 0) {
        cout << "getaddrinfo failed " << iResult << endl;
        WSACleanup();
        return 1;
    }

    ListenSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);

    if (ListenSocket == INVALID_SOCKET) {
        cout << "error at socket() " << WSAGetLastError();
        freeaddrinfo(result);
        WSACleanup();
        return 1;
    }

    iResult = bind(ListenSocket, result->ai_addr, (int)result->ai_addrlen);
    if (iResult == SOCKET_ERROR) {
        cout << "bind failed with error " << WSAGetLastError();
        freeaddrinfo(result);
        closesocket(ListenSocket);
        WSACleanup();
        return 1;
    }
    freeaddrinfo(result);

    if (listen(ListenSocket, SOMAXCONN) == SOCKET_ERROR) {
        cout << "listen failed with error " << WSAGetLastError();
        closesocket(ListenSocket);
        WSACleanup();
        return 1;
    }

    DWORD idThread = 1;
    HANDLE connectThread = CreateThread(NULL, 0, ConnectClient, (void*)idThread, 0, &idThread);
    if (!connectThread) {
        return GetLastError();
    }

    char input{ 0 };
    do {

        input = _getch();

    } while (input != 'e');

    CloseHandle(connectThread);
    CloseHandle(ClientJoinedEvent);
    closesocket(ListenSocket);
    for (int i = 0; i < index; i++)
    {
        closesocket(clients[i].socket);
        CloseHandle(clients[i].ListenThread);
    }
    WSACleanup();

    return 0;
}
