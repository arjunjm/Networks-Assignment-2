#ifndef __SERVER__H__
#define __SERVER__H__

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>
#include <map>
#include "helper.h"
#include <math.h>
#include <string>
#include <fstream>

#define PORT "3490"
#define BACKLOG 10

using namespace std;

class Server
{
    private:
        struct addrinfo hints;
        int sockFd;
        int newConnFd;
        char serverIP[20];
        char portNum[10];

    public:
        Server(char *serverIP, char *portNum);
        int createSocketAndBind();
        int acceptConnection();
};

#endif /* __SERVER__H__ */
