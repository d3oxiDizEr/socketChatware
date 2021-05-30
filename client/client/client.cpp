#include<winsock2.h>
#pragma comment(lib, "WS2_32")
#include<stdio.h>

class Client {
private:
	SOCKET sock;
	SOCKADDR_IN addr, serveraddr;
	char sendbuf[200];
public:
	int init(int);
	void start();
	void receive();
	void stop();
};

int Client::init(int serverport) {
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

	memset(&serveraddr, 0x00, sizeof(serveraddr));
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_addr.S_un.S_addr = inet_addr("127.0.0.1");
	serveraddr.sin_port = htons(serverport);
	if (connect(sock, (SOCKADDR*)&serveraddr, sizeof(serveraddr)) < 0) {
		return 2;
	}

	return 0;
}

void Client::start() {
	while (1) {
		printf("input>>>");
		scanf("%s", sendbuf);
		send(sock, sendbuf, 200, 0);
	}
}

void Client::receive() {

}

void Client::stop() {
	closesocket(sock);
	WSACleanup();
}

void main() {
	Client client;
	int code = client.init(5000);
	printf("%d\n", code);
	client.start();
	//client.stop();
}