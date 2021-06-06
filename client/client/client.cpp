#define PORT 5000

#define C_LOGIN 1
#define C_REGISTER 2
#define C_PASSWORD 3
#define C_CLOSE 4

#define S_UNLOGIN 5
#define S_AUTHERR 6
#define S_AUTHSUC 8
#define S_CLOSE 14

#define M_END 15
#define M_PART 16

#include <winsock2.h>
#pragma comment(lib, "ws2_32.lib")

#include <thread>

#include <stdio.h>

using namespace std;


struct userpass {
	char c;
	char name[20];
	char pass[20];
	char placeholder[159];
};


struct PACKAGE {
	char c;
	char name[20];
	char msg[142];
	char eomsg;
	tm time;// size: 36 (time put last for align)
};


void copychars(char* dst, char* src, int bias1, int bias2, int num) {
	for (int i = 0; i < num; i++) {
		dst[i + bias1] = src[i + bias2];
	}
}


class Client {
private:
	int (*input)(char*, userpass*);
	void (*recvmsg)(tm, char*, char*);
	void (*authin)(userpass*);

	SOCKET sock;
	SOCKADDR_IN addr, serveraddr;
	char sendbuf[200];
	char recvbuf[200];
	PACKAGE pkg;

	void receive();

public:
	void set(int (*)(char*, userpass*), void (*)(tm, char*, char*), void (*)(userpass*));
	int init();
	void login();
	void start();
	thread receive_th();
	void stop();
};

void Client::set(int (*fp1)(char*, userpass*), void (*fp2)(tm, char*, char*), void (*fp3)(userpass*)) {
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
		send(sock, (char*)&kvp, 200, 0);
		recv(sock, recvbuf, 200, 0);
	}
}

void Client::start() {
	int value, i;
	PACKAGE pkg;
	userpass kvp;
	while (1) {
		value = (*input)(sendbuf, &kvp);
		switch (value) {
		case 0:
			return;
		case 1:
			i = 0;
			pkg.c = M_PART;
			while (strlen(sendbuf) - i * 142 > 142) {
				memset(pkg.msg, 0x00, 142);
				copychars(pkg.msg, sendbuf, 0, i * 142, 142);
				send(sock, (char*)&pkg, 200, 0);
				i++;
			}
			pkg.c = M_END;
			memset(pkg.msg, 0x00, 142);
			copychars(pkg.msg, sendbuf, 0, i * 142, strlen(sendbuf) - i * 142);
			send(sock, (char*)&pkg, 200, 0);
			break;
		case 2:
			kvp.c = C_PASSWORD;
			send(sock, (char*)&kvp, 200, 0);
			break;
		}
		Sleep(100);
	}
}

void Client::receive() {
	int code;
	string s;
	while (1) {
		s.~string();
		code = recv(sock, (char*)&pkg, 200, 0);
		if (code == SOCKET_ERROR) {
			break;
		}
		while (pkg.c != M_END) {
			s += pkg.msg;
			recv(sock, (char*)&pkg, 200, 0);
		}
		s += pkg.msg;
		(*recvmsg)(pkg.time, pkg.name, (char*)s.c_str());
	}
}

thread Client::receive_th() {
	return thread(&Client::receive, this);
}

void Client::stop() {
	sendbuf[0] = C_CLOSE;
	send(sock, sendbuf, 200, 0);
	Sleep(100);// needed to get send() function done
	closesocket(sock);
	WSACleanup();
}


int input(char* s, userpass* kvp) {
	int choice;
	printf("stop[0], send message[1], change password[2]: ");
	scanf("%d", &choice);
	switch (choice) {
	case 0:
		break;
	case 1:
		printf("msg: ");
		scanf("%s", s);
		break;
	case 2:
		printf("new password: ");
		scanf("%s", kvp->pass);
		break;
	}
	return choice;
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
	printf("sizeof PACKAGE: %d\n", sizeof(PACKAGE));
	printf("client on.\n");
	client.login();

	thread th = client.receive_th();
	client.start();

	
	client.stop();
	th.join();
}