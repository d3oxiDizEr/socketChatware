#define PORT 5000
#define DATA "data.txt"

#define M_LOGIN 1
#define M_REGISTER 2
#define M_PASSWORD 3
#define M_CLOSE 4

#define S_UNLOGIN 5
#define S_AUTHERR 6
#define S_AUTHSUC 7

#include <winsock2.h>
#pragma comment(lib, "ws2_32.lib")

#include <string>
#include <vector>
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


class Server {
private:
	int F_STOP = 0;

	void (*joinmsg)(char*, int, int);
	void (*leavemsg)(char*, int, int);

	// database
	FILE* fp;
	int recordlen = sizeof(userpass);

	void dbinit();
	int login(char*, char*);
	int signup(char*, char*);
	void dbclose();

	// socket
	SOCKET sock;
	SOCKADDR_IN addr;
	vector<SOCKET> clientsocks;
	vector<SOCKADDR_IN> clientaddrs;
	vector<char*> usernames;
	int addrlen = sizeof(addr);
	unsigned long mode = 1;// non-blocking mode
	PACKAGE pkg;
	char buffer[200];
	time_t tsec;
	tm* tstruct;

	void accept1();
	void receive();
	void disconnect(int);

public:
	void set(void (*)(char*, int, int), void (*)(char*, int, int));
	int init();
	void start();
	thread accept1_th();
	thread receive_th();
	void stop();
};


void Server::dbinit() {
	if ((fp = fopen(DATA, "r+")) == NULL) {
		fp = fopen(DATA, "w");
		fclose(fp);
		fp = fopen(DATA, "r+");
	}
}

int Server::login(char* name, char* pass) {
	char rname[21];
	char rpass[21];
	fseek(fp, 0, SEEK_SET);
	while (fgets(rname, 21, fp) != NULL) {
		if (strcmp(rname, name) == 0) {
			// find username
			fgets(rpass, 21, fp);
			if (strcmp(rpass, pass) == 0) {
				// password correct
				return S_AUTHSUC;
			}
			return S_AUTHERR;
		}
		// skip password
		fseek(fp, 20L, SEEK_CUR);
	}
	return S_AUTHERR;
}

int Server::signup(char* name, char* pass) {
	char rname[21];
	fseek(fp, 0, SEEK_SET);
	while (fgets(rname, 21, fp) != NULL) {
		if (strcmp(rname, name) == 0) {
			return S_AUTHERR;
		}
		fseek(fp, 20L, SEEK_CUR);
	}
	// register
	fseek(fp, 0, SEEK_END);
	for (int i = 0; i < 20; i++) {
		fputc(name[i], fp);
	}
	for (int i = 0; i < 20; i++) {
		fputc(pass[i], fp);
	}
	return S_AUTHSUC;
}

void Server::dbclose() {
	fclose(fp);
}


void Server::set(void(*fp1)(char*, int, int), void(*fp2)(char*, int, int)) {
	joinmsg = fp1;
	leavemsg = fp2;
}

int Server::init() {
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
	addr.sin_port = htons(PORT);
	if (bind(sock, (SOCKADDR*)&addr, addrlen) < 0) {
		return 3;
	}

	if (listen(sock, 16) < 0) {
		return 4;
	}

	dbinit();

	return 0;
}

void Server::start() {
	int input;
	scanf("%d", &input);

}

void Server::disconnect(int i) {
	SOCKADDR_IN client_addr = clientaddrs[i];
	char* client_ip = inet_ntoa(client_addr.sin_addr);
	int client_port = ntohs(client_addr.sin_port);
	clientsocks.erase(clientsocks.begin() + i);
	clientaddrs.erase(clientaddrs.begin() + i);
	free(usernames[i]);
	usernames.erase(usernames.begin() + i);
	leavemsg(client_ip, client_port, clientsocks.size());
}

void Server::accept1() {
	SOCKET client;
	SOCKADDR_IN clientaddr;
	char* client_ip;
	int client_port;
	while (1) {
		client = accept(sock, (SOCKADDR*)&clientaddr, &addrlen);
		if (client == INVALID_SOCKET) {
			Sleep(200);
			if (F_STOP == 1) {
				return;
			}
			continue;
		}
		client_ip = inet_ntoa(clientaddr.sin_addr);
		client_port = ntohs(clientaddr.sin_port);
		clientsocks.push_back(client);
		clientaddrs.push_back(clientaddr);
		usernames.push_back(NULL);
		(*joinmsg)(client_ip, client_port, clientsocks.size());
	}
}

thread Server::accept1_th() {
	return thread(&Server::accept1, this);
}

void Server::receive() {
	int code;
	userpass* p;
	SOCKADDR_IN client_addr;
	char* client_ip;
	int client_port;
	while (1) {
		for (int i = 0; i < clientsocks.size(); i++) {
			code = recv(clientsocks[i], buffer, 200, 0);
			if (code == 0) {
				// client closed gracefully
				// now useless for some reason
				//disconnect(i);
				continue;
			}
			else if (code == SOCKET_ERROR) {
				// no receive
				// notice: client forced shutdown belongs here
				continue;
			}
			else {
				if (buffer[0] == M_CLOSE) {
					disconnect(i);
				}
				else if (buffer[0] == M_LOGIN) {
					p = (userpass*)buffer;
					buffer[0] = login(p->name, p->pass);
					if (buffer[0] == S_AUTHSUC) {
						usernames[i] = (char*)malloc(20 * sizeof(char));
						strcpy(usernames[i], p->name);
					}
					send(clientsocks[i], buffer, 200, 0);
				}
				else if (buffer[0] == M_REGISTER) {
					p = (userpass*)buffer;
					buffer[0] = signup(p->name, p->pass);
					if (buffer[0] == S_AUTHSUC) {
						usernames[i] = (char*)malloc(20 * sizeof(char));
						strcpy(usernames[i], p->name);
					}
					send(clientsocks[i], buffer, 200, 0);
				}
				else if (usernames[i] == NULL) {
					buffer[0] = S_UNLOGIN;
					send(clientsocks[i], buffer, 200, 0);
				}
				else {
					time(&tsec);
					tstruct = gmtime(&tsec);
					copychars((char*)&pkg.time, (char*)tstruct, 36);
					copychars(pkg.name, usernames[i], 20);
					copychars(pkg.msg, buffer, 144);
					printf("%s\n", buffer);
					for (int j = 0; j < clientsocks.size(); j++) {
						if (usernames[j] == NULL) {
							continue;
						}
						send(clientsocks[j], (char*)&pkg, 200, 0);
					}
				}
			}
		}
		Sleep(50);
		if (F_STOP == 1) {
			return;
		}
	}
}

thread Server::receive_th() {
	return thread(&Server::receive, this);
}

void Server::stop() {
	F_STOP = 1;
	dbclose();

	for (int i = 0; i < clientsocks.size(); i++) {
		disconnect(i);
	}
	closesocket(sock);
	WSACleanup();
}


void join_msg(char* client_ip, int client_port, int total) {
	printf("connect from %s:%d.\n", client_ip, client_port);
	printf("now %d client(s) online.\n", total);
}

void leave_msg(char* client_ip, int client_port, int total) {
	printf("%s:%d disconnected.\n", client_ip, client_port);
	printf("now %d client(s) online.\n", total);
}

void main() {
	Server server;
	server.set(join_msg, leave_msg);
	int code = server.init();
	if (code != 0) {
		printf("error code: %d.", code);
		return;
	}
	printf("server on.\n");
	thread th1 = server.accept1_th();
	thread th2 = server.receive_th();
	server.start();
	
	printf("server off.\n");
	server.stop();

	th1.join();
	th2.join();
}