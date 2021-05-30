#include<winsock2.h>
#pragma comment(lib, "ws2_32.lib")
#include<windows.h>
#include<stdio.h>
#include<vector>
#include<thread>
using namespace std;

class Server {
private:
	SOCKET sock;
	SOCKADDR_IN addr;
	vector<SOCKET> clientsocks;
	vector<SOCKADDR_IN> clientaddrs;
	int addrlen = sizeof(addr);
	unsigned long mode = 1;// non-blocking mode
	//struct timeval timeout = { 0,10000 };
	char buffer[200];
public:
	int init(int);
	void start();
	void accept1();
	thread accept1th();
	void stop();
};

int Server::init(int port) {
	// Initialise Winsock
	WSADATA wsa;
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
	{
		WSACleanup();
		return -1;
	}

	sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock == INVALID_SOCKET) {
		return 1;
	}
	//setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (char*) & timeout, sizeof(timeout));

	if (ioctlsocket(sock, FIONBIO, &mode) != NO_ERROR) {
		return 2;
	}

	memset(&addr, 0x00, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_addr.S_un.S_addr = htonl(INADDR_ANY);// any IP
	addr.sin_port = htons(port);
	if (bind(sock, (SOCKADDR*)&addr, addrlen) < 0) {
		return 3;
	}

	if (listen(sock, 16) < 0) {
		return 4;
	}
	return 0;
}

void Server::start() {
	int code;
	SOCKADDR_IN client_addr;
	char* client_ip;
	int client_port;
	while (1) {
		for (int i = 0; i < clientsocks.size(); i++) {
			code = recv(clientsocks[i], buffer, 200, 0);
			if (code == 0) {
				//client_addr = clientaddrs[i];
				//client_ip = inet_ntoa(client_addr.sin_addr);
				//client_port = ntohs(client_addr.sin_port);
				//printf("%s:%d disconnected.\n", client_ip, client_port);
				//clientsocks.erase(clientsocks.begin() + i);
				//clientaddrs.erase(clientaddrs.begin() + i);
				continue;
			}
			else if (code == SOCKET_ERROR) {
				//printf("%d\n", code);
				continue;
			}
			else {
				printf("%s\n", buffer);
			}
		}
		Sleep(50);
	}
}

void Server::accept1() {
	SOCKET client;
	SOCKADDR_IN clientaddr;
	while (1) {
		client = accept(sock, (SOCKADDR*)&clientaddr, &addrlen);
		if (client == INVALID_SOCKET) {
			Sleep(200);
			continue;
		}
		char* client_ip = inet_ntoa(clientaddr.sin_addr);
		int client_port = ntohs(clientaddr.sin_port);
		printf("connect from %s:%d.\n", client_ip, client_port);
		clientsocks.push_back(client);
		clientaddrs.push_back(clientaddr);
		//printf("now %d client(s) online.\n", clientsocks.size());
	}
}

thread Server::accept1th() {
	return thread(&Server::accept1, this);
}

void Server::stop() {
	for (int i = 0; i < clientsocks.size(); i++) {
		closesocket(clientsocks[i]);
	}
	closesocket(sock);
	WSACleanup();
}


void main()
{
	Server server;
	int code = server.init(5000);
	printf("%d\n", code);
	thread th = server.accept1th();
	server.start();
	th.join();
}