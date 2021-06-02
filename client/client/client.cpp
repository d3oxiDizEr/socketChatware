#define PORT 5000

#define M_LOGIN 1
#define M_REGISTER 2
#define M_PASSWORD 3
#define M_CLOSE 4

#define S_UNLOGIN 5
#define S_AUTHERR 6
#define S_AUTHSUC 7

#include <winsock2.h>
#pragma comment(lib, "ws2_32.lib")

#include <thread>

#include <stdio.h>

using namespace std;


struct userpass {
	char c;
	char name[20];
	char pass[20];
};


struct PACKAGE {
	tm time;//36
	char name[20];
	char msg[144];
};


void copychars(char* dst, char* src, int num) {
	for (int i = 0; i < num; i++) {
		dst[i] = src[i];
	}
}


class Client {
private:
	void (*input)(char*);
	void (*recvmsg)(tm, char*, char*);
	void (*authin)(userpass*);

	SOCKET sock;
	SOCKADDR_IN addr, serveraddr;
	char sendbuf[200];
	char recvbuf[200];
	PACKAGE pkgp;

	void receive();

public:
	void set(void (*)(char*), void (*)(tm, char*, char*), void (*)(userpass*));
	int init();
	void login();
	void start();
	thread receive_th();
	void stop();
};

void Client::set(void (*fp1)(char*), void (*fp2)(tm, char*, char*), void (*fp3)(userpass*)) {
	input = fp1;
	recvmsg = fp2;
	authin = fp3;
}

int Client::init() {
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
	serveraddr.sin_port = htons(PORT);
	if (connect(sock, (SOCKADDR*)&serveraddr, sizeof(serveraddr)) < 0) {
		return 2;
	}

	return 0;
}

void Client::login() {
	userpass kvp;
	while (recvbuf[0] != S_AUTHSUC) {
		(*authin)(&kvp);
		copychars(sendbuf, (char*)&kvp, sizeof(kvp));
		send(sock, sendbuf, 200, 0);
		recv(sock, recvbuf, 200, 0);
	}
}

void Client::start() {
	while (1) {
		(*input)(sendbuf);
		if (sendbuf[0] == '0') {
			break;
		}
		send(sock, sendbuf, 200, 0);
	}
}

void Client::receive() {
	int code;
	while (1) {
		code = recv(sock, (char*)&pkgp, 200, 0);
		if (code == SOCKET_ERROR) {
			break;
		}
		(*recvmsg)(pkgp.time, pkgp.name, pkgp.msg);
	}
}

thread Client::receive_th() {
	return thread(&Client::receive, this);
}

void Client::stop() {
	sendbuf[0] = M_CLOSE;
	send(sock, sendbuf, 200, 0);
	Sleep(100);// needed to get send() function done
	closesocket(sock);
	WSACleanup();
}


void input(char* s) {
	scanf("%s", s);
}

void recv_msg(tm time, char* name, char* msg) {
	printf("[%d:%d]%s: %s\n", time.tm_hour, time.tm_min, name, msg);
}

void auth(userpass* kvp) {
	printf("login[1], register[2]: ");
	scanf("%d", &(kvp->c));
	printf("username: ");
	scanf("%s", kvp->name);
	printf("password: ");
	scanf("%s", kvp->pass);
}

void main() {
	Client client;
	client.set(input, recv_msg, auth);
	int code = client.init();
	if (code != 0) {
		printf("error code: %d.", code);
		return;
	}
	printf("client on.\n");
	client.login();

	thread th = client.receive_th();
	client.start();

	
	client.stop();
	th.join();
}