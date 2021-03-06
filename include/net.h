#include "include.h"

#ifndef NET_H
#define NET_H

int inet_pton(int af, const char *server, in_addr *addr);

/*
	UDP non blocking
*/
class Net
{
public:
	void connect(char *server, int port);
	void bind(char *address, int port);
	void accept();
	int send(char *buffer, int size);
	int sendto(char *buff, int size, char *to);
	int recv(char *buff, int size);
	int recv(char *buffer, int size, int delay);
	int recvfrom(char *buff, int size, char *from, int length);
	void strtoaddr(char *str, sockaddr_in &addr);

	sockaddr_in		servaddr;
	SOCKET			sockfd;
private:


};

#endif