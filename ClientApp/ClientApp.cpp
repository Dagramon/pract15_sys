#define WIN32_LEAN_AND_MEAN

#include <Windows.h>
#include <winsock2.h>
#include <WS2tcpip.h>
#include <iphlpapi.h>
#include <iostream>
#define DEFAULT_PORT "27015"
#define DEFAULT_BUFLEN 512

#pragma comment(lib, "Ws2_32.lib")

SOCKET ConnectSocket = INVALID_SOCKET;
bool connected = true;

using namespace std;

DWORD WINAPI ListenThread(LPVOID num)
{
	while (ConnectSocket != INVALID_SOCKET) {

		int result;
		char message[DEFAULT_BUFLEN];
		result = recv(ConnectSocket, message, DEFAULT_BUFLEN, 0);
		if (result > 0) {
			message[result] = '\0';
			cout << message;
		}
		else if (result == 0) {

			if (connected) {
				cout << "\n[LOG]: Server disconnected" << endl;
			}
			break;

		}
		else
		{
			int error = WSAGetLastError();
			if (error != WSAECONNRESET && connected) {
				cout << "\n[LOG]: recv failed error: " << error << endl;
			}
			break;
		}
	}
}

int main(int argc, char* argv[])
{
	char ip[255]{ 0 };

	if (argc > 1)
	{
		strcpy_s(ip, argv[1]);
	}
	else
	{
		strcpy_s(ip, "127.0.0.1");
	}

	SetConsoleCP(1251);
	SetConsoleOutputCP(1251);
	setlocale(LC_ALL, "Russian");
	WSAData wsaData;

	int iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);

	if (iResult != 0) {
		cout << "WSAStartup failed: " << iResult << endl;
		return 1;
	}

	struct addrinfo* result = NULL,
		* ptr = NULL,
		hints;
	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;

	iResult = getaddrinfo(ip, DEFAULT_PORT, &hints, &result);
	if (iResult != 0) {
		cout << "getaddrinfo failed " << iResult << endl;
		WSACleanup();
		return 1;
	}

	ptr = result;
	ConnectSocket = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);
	if (ConnectSocket == INVALID_SOCKET) {
		cout << "Error at socket() " << WSAGetLastError << endl;
		WSACleanup();
		return 1;
	}

	iResult = connect(ConnectSocket, ptr->ai_addr, (int)ptr->ai_addrlen);
	if (iResult == SOCKET_ERROR) {
		closesocket(ConnectSocket);
		ConnectSocket = INVALID_SOCKET;
	}
	if (ConnectSocket == INVALID_SOCKET) {
		cout << "Unable to connect to server" << endl;
		WSACleanup();
		return 1;
	}
	freeaddrinfo(result);

	char sendName[DEFAULT_BUFLEN];

	cout << "connection successful, enter your name: ";
	cin.getline(sendName, DEFAULT_BUFLEN);

	iResult = send(ConnectSocket, sendName, (int)strlen(sendName), 0);
	if (iResult == SOCKET_ERROR) {
		cout << "send failed " << WSAGetLastError() << endl;
		closesocket(ConnectSocket);
		WSACleanup();
		return 1;
	}

	DWORD idThread;
	HANDLE listenThread = CreateThread(NULL, 0, ListenThread, (void*)1, 0, &idThread);
	if (!listenThread) {
		closesocket(ConnectSocket);
		WSACleanup();
		return GetLastError();
	}

	char input[DEFAULT_BUFLEN];

	do { 
		cin.getline(input, DEFAULT_BUFLEN);

		if (input[0] != '/')
		{
			iResult = send(ConnectSocket, input, (int)strlen(input), 0);
			if (iResult == SOCKET_ERROR) {
				cout << "send failed " << WSAGetLastError() << endl;
				closesocket(ConnectSocket);
				WSACleanup();
			}
		}
		else
		{
			if (strcmp(input, "/exit") == 0)
			{
				iResult = send(ConnectSocket, input, (int)strlen(input), 0);
				if (iResult == SOCKET_ERROR) {
					cout << "send failed " << WSAGetLastError() << endl;
					closesocket(ConnectSocket);
					WSACleanup();
				}
				break;
			}
			if (strcmp(input, "/users") == 0)
			{
				iResult = send(ConnectSocket, input, (int)strlen(input), 0);
				if (iResult == SOCKET_ERROR) {
					cout << "send failed " << WSAGetLastError() << endl;
					closesocket(ConnectSocket);
					WSACleanup();
				}
				continue;
			}
			cout << "unknown command" << endl;
		}

	} while (true);

	connected = false;
	CloseHandle(listenThread);
	closesocket(ConnectSocket);
	WSACleanup();

	cout << "disconnected" << endl;

	return 0;
}