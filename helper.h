#ifndef __HELPER_H__
#define __HELPER_H__

#include <sys/types.h>
#include <sys/wait.h>
#include <list>

using namespace std;

#define MAXDATASIZE 100
#define STDIN 0

struct sockaddr;
struct sockaddr_in;

typedef enum UserStatus
{
    ONLINE,
    OFFLINE,
    IDLE
} UserStatusT;

typedef enum AttributeType
{
   ATTR_REASON,
   ATTR_USER,
   ATTR_CLI_CNT,
   ATTR_MSG
} AttributeTypeT;

typedef enum SBMPMessageType
{
    JOIN = 2,
    FWD  = 3,
    SEND = 4,
    ACK = 5,
    NACK = 6,
    ONLINE_INFO = 7,
    OFFLINE_INFO = 8
} SBMPMessageTypeT;

typedef union
{
    char username[16];
    char message[512];
    char reason[32];
    unsigned short clientCount;
}Payload;

typedef struct SBMPAttribute
{
    AttributeTypeT type;
    unsigned short length;
    Payload payload;
} __attribute__((packed)) SBMPAttributeT;

typedef struct SBMPHeader
{
    unsigned int version : 9;
    unsigned int type : 7;
    unsigned short length;
    SBMPAttributeT attributes[2];
} __attribute__((packed)) SBMPHeaderT;

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

SBMPHeaderT* createMessagePacket(SBMPMessageTypeT msgType, const char *userName, const char *msg)
{
    SBMPAttributeT sbmpAttr;
    SBMPHeaderT *sbmpHeader;

    switch(msgType)
    {
        case JOIN:

            /*
             * Fill in the SBMP Attribute struct
             */   

            sbmpAttr.type = ATTR_USER;
            strcpy(sbmpAttr.payload.username, userName);
            sbmpAttr.length = strlen(sbmpAttr.payload.username) + 4;

            /*
             * Fill in the SBMP Header struct
             */

            sbmpHeader = new SBMPHeaderT();
            sbmpHeader->version = 1;
            sbmpHeader->type = (int)JOIN;
            sbmpHeader->length = sbmpAttr.length + 4;
            sbmpHeader->attributes[0] = sbmpAttr;

            break;
        
        case FWD:

            /*
             * Fill in the SBMP Attribute struct
             */   

            SBMPAttributeT sbmpUserAttr;
            sbmpUserAttr.type = ATTR_USER;
            strcpy(sbmpUserAttr.payload.username, userName);
            sbmpUserAttr.length = strlen(sbmpUserAttr.payload.username) + 4;

            sbmpAttr.type = ATTR_MSG;
            strcpy(sbmpAttr.payload.message, msg);
            sbmpAttr.length = strlen(sbmpAttr.payload.message) + 4;

            /*
             * Fill in the SBMP Header struct
             */

            sbmpHeader = new SBMPHeaderT();
            sbmpHeader->version = 1;
            sbmpHeader->type = (int)FWD;
            sbmpHeader->length = sbmpAttr.length + 4;
            sbmpHeader->attributes[0] = sbmpUserAttr;
            sbmpHeader->attributes[1] = sbmpAttr;

            break;

        case SEND:

            /*
             * Fill in the SBMP Attribute struct
             */   

            sbmpAttr.type = ATTR_MSG;
            strcpy(sbmpAttr.payload.message, msg);
            sbmpAttr.length = strlen(sbmpAttr.payload.message) + 4;

            /*
             * Fill in the SBMP Header struct
             */

            sbmpHeader = new SBMPHeaderT();
            sbmpHeader->version = 1;
            sbmpHeader->type = (int)SEND;
            sbmpHeader->length = sbmpAttr.length + 4;
            sbmpHeader->attributes[0] = sbmpAttr;

            break;

        case ACK:
            /*
             * Fill in the SBMP Attribute struct
             */   

            sbmpAttr.type = ATTR_MSG;
            strcpy(sbmpAttr.payload.message, msg);
            sbmpAttr.length = strlen(sbmpAttr.payload.message) + 4;

            /*
             * Fill in the SBMP Header struct
             */

            sbmpHeader = new SBMPHeaderT();
            sbmpHeader->version = 1;
            sbmpHeader->type = (int)ACK;
            sbmpHeader->length = sbmpAttr.length + 4;
            sbmpHeader->attributes[0] = sbmpAttr;

            break;

        case NACK:
            /*
             * Fill in the SBMP Attribute struct
             */   

            sbmpAttr.type = ATTR_REASON;
            strcpy(sbmpAttr.payload.message, msg);
            sbmpAttr.length = strlen(sbmpAttr.payload.message) + 4;

            /*
             * Fill in the SBMP Header struct
             */

            sbmpHeader = new SBMPHeaderT();
            sbmpHeader->version = 1;
            sbmpHeader->type = (int)NACK;
            sbmpHeader->length = sbmpAttr.length + 4;
            sbmpHeader->attributes[0] = sbmpAttr;


            break;

        case OFFLINE_INFO:
        case ONLINE_INFO:
            /*
             * Fill in the SBMP Attribute struct
             */   

            sbmpAttr.type = ATTR_MSG;
            strcpy(sbmpAttr.payload.message, msg);
            sbmpAttr.length = strlen(sbmpAttr.payload.message) + 4;

            /*
             * Fill in the SBMP Header struct
             */

            sbmpHeader = new SBMPHeaderT();
            sbmpHeader->version = 1;
            sbmpHeader->type = (int)msgType;
            sbmpHeader->length = sbmpAttr.length + 4;
            sbmpHeader->attributes[0] = sbmpAttr;

            break;

        default:
            printf("Error!!!Did not match any known message types. Exiting");
            exit(1);
    }
    return sbmpHeader;
}


#endif /* __ELPER_H__ */
