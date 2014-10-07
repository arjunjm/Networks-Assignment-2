#ifndef __HELPER_H__
#define __HELPER_H__

#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <list>
#include <iostream>

using namespace std;

#define MAXDATASIZE 512 
#define STDIN 0

struct sockaddr;
struct sockaddr_in;

typedef enum TFTPHeaderType
{
    RRQ  = 1,
    WRQ  = 2,
    DATA = 3,
    ACKN = 4,
    ERR  = 5
} TFTPHeaderTypeT;

TFTPHeaderTypeT getHeaderType(char *buf)
{
    unsigned short opcodeNetOrder;
    memcpy(&opcodeNetOrder, buf, sizeof(unsigned short));
    unsigned short opCodeHostOrder = ntohs(opcodeNetOrder);
    return (TFTPHeaderTypeT) opCodeHostOrder;
}

char * createTFTPHeader(TFTPHeaderTypeT hType, char *payload = NULL, int bytesRead =0, int blockNum = 0, int errorNo = 0)
{
    switch(hType)
    {
        case RRQ:
        case WRQ:
        case ACKN:
            {
                char *dataBuf = new char[4];
                unsigned short ackOpcode = (int) hType;
                unsigned short blockNo   = blockNum;

                // Convert to network byte order
                unsigned short ackOpcodeNetOrder = htons(ackOpcode);
                unsigned short blockNumNetOrder  = htons(blockNo);
                memcpy(dataBuf, &ackOpcodeNetOrder, 2);
                memcpy(dataBuf+2, &blockNumNetOrder, 2);
                return dataBuf;
            }

        case ERR:
            {
                char *buf = new char[4+strlen(payload)+1];
                unsigned short errOpcode = (int) hType;
                unsigned short errNumber = errorNo;

                // Convert to network byte order
                unsigned short errOpcodeNetOrder = htons(errOpcode);
                unsigned short errNumberNetOrder = htons(errNumber);
                memcpy(buf,   &errOpcodeNetOrder, 2);
                memcpy(buf+2, &errNumberNetOrder, 2);
                memcpy(buf+4, payload, strlen(payload));
                buf[strlen(payload) + 5] = '\0';
                return buf;
            }
        case DATA:
            {
                char *dataBuf = new char[516];
                unsigned short dataOpcode = (int) hType;
                unsigned short blockNo = blockNum;

                // Convert to network byte order
                unsigned short dataOpCodeNetOrder = htons(dataOpcode);
                unsigned short blockNumNetOrder = htons(blockNo);
                memcpy(dataBuf,   &dataOpCodeNetOrder, 2);
                memcpy(dataBuf+2, &blockNumNetOrder, 2);
                if (payload != NULL)
                {
                    memcpy(dataBuf+4, payload, bytesRead);
                }
                else
                {
                    cout << "Payload is NULL " << endl;
                }
                return dataBuf;
            }
   }
   return NULL;
}

int getFileSize(const char* fileName)
{
    struct stat st;
    if (stat(fileName, &st) == 0)
        return st.st_size;

    fprintf(stderr, "Cannot determine size of %s: %s\n",
            fileName, strerror(errno));

    return -1;
}

void sigchld_handler(int s)
{
    while(waitpid(-1, NULL, WNOHANG) > 0);
}

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

#endif /* __HELPER_H__ */
