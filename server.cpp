/*
 * Implementation of the Server Class
 */

#include "server.h"
#include <iostream>
#include <sstream>

using namespace std;

Server::Server(char *servIP, char *portNum)
{
    strcpy(this->serverIP, servIP);
    strcpy(this->portNum, portNum);
    nextFreePort = atoi(this->portNum) + 1;

    memset(&this->hints, 0, sizeof hints);
    this->hints.ai_family = AF_UNSPEC;
    this->hints.ai_socktype = SOCK_DGRAM;
    this->hints.ai_flags = 0; 
}

int Server::createSocketAndBind()
{
    struct addrinfo *serverInfo, *temp;
    int yes = 1;
    int rv;
    if ((rv = getaddrinfo(this->serverIP, this->portNum, &this->hints, &serverInfo)) != 0)
    {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }

    /*
     * Loop through the results and bind to the first we can
     */
    for(temp = serverInfo; temp != NULL; temp = temp->ai_next)
    {
        sockFd = socket(temp->ai_family, temp->ai_socktype,
                temp->ai_protocol);
        if (sockFd == -1)
        {
            perror("Unable to create socket. Attempting again..");
            continue;
        }

        if (setsockopt(sockFd, SOL_SOCKET, SO_REUSEADDR, &yes, 
                    sizeof(int)) == -1)
        {
            perror("setsockopt failed");
            exit(1);
        }

        if (bind(sockFd, temp->ai_addr, temp->ai_addrlen) == -1)
        {
            close(sockFd);
            perror("server: Unable to bind");
            continue;
        }
        break;
    }

    freeaddrinfo(serverInfo);

    if (temp == NULL)
    {
        fprintf(stderr, "server: failed to bind\n");
        return 2;
    }
    else
    {
        // success!
        return 0;
    }
}


/*
void printMap(std::map<int, string> myMap)
{
    std::map<int, string>::iterator it;
    for (it = myMap.begin(); it != myMap.end(); it++)
    {
        cout << it->first << " : " << it->second << endl;   
    }
}
*/

int Server::acceptConnection()
{
    int newConnFd;
    TFTPHeaderTypeT headerType;
    struct sockaddr_storage clientAddr;
    socklen_t sin_size;
    //char ipAddr[INET6_ADDRSTRLEN]; 
    char buf[MAXDATASIZE];
    int numBytes;
    fd_set master, read_fds;
    int fdMax;
    /*
    struct timeval tv;
    tv.tv_sec = 100;
    tv.tv_usec = 500000;
    */
    FD_ZERO(&master);
    FD_ZERO(&read_fds);

    FD_SET(sockFd, &master);
    fdMax = sockFd;

    while(1)
    {
        read_fds = master;
        if (select(fdMax + 1, &read_fds, NULL, NULL, NULL) == -1)
        {
            perror("Select");
            exit(4);
        }

        /*
         * Check if there is a new incoming connection or if 
         * there are any ACKs from existing clients
         */
        for (int i = 0; i <= fdMax; i++)
        {
            if (FD_ISSET(i, &read_fds))
            {
                if (i == sockFd)
                    /*
                     * This is the listening socket. 
                     * Check new incoming connections.
                     */
                {
                    sin_size = sizeof clientAddr;
                    if ((numBytes = recvfrom(sockFd, buf, MAXDATASIZE-1, 0, 
                                    (struct sockaddr*)&clientAddr, &sin_size)) == -1)
                    {
                        perror("recvfrom");
                        continue;
                    }

                    // Check the received header's opcode
                    headerType = getHeaderType(buf);
                    if (headerType != RRQ)
                        continue;

                    char *fileName = new char();
                    int i = 0;
                    int offset = sizeof(short);
                    while (true)
                    {
                        if (buf[i + offset] == '\0')
                            break;
                        fileName[i] = buf[i + offset];
                        i++;
                    }
                    fileName[i] = '\0';

                    cout << "Filename = " << fileName << endl;
                    // Get File Size
                    FILE *fp = fopen(fileName, "rb"); 
                    if (fp == NULL)
                    {
                        perror("Error while trying to open file");

                        /*
                         * Sending ERR TFTP Header
                         */
                        char errMessage[30] = "No such file or directory";
                        char *errPacket = createTFTPHeader(ERR, errMessage, strlen(errMessage), 0, errno);
                        sendto(sockFd, errPacket, strlen(errMessage) + 4, 0,
                                (struct sockaddr*)&clientAddr, sizeof(struct sockaddr_storage));
                        continue;
                    }


                    // Create a new socket for communicating with the client
                    newConnFd = socket(AF_INET, SOCK_DGRAM, 0);
                    if (newConnFd == -1)
                    {
                        perror("Error in creating a new socket..");
                    }
                    else
                    {
                        /*
                         * Bind the socket to the next available port
                         */
                        //struct sockaddr_in newClient = *((struct sockaddr_in*)&clientAddr);
                        struct sockaddr_in newClient = *((struct sockaddr_in*)&this->hints);
                        newClient.sin_port = htons(nextFreePort);

                        if(bind(newConnFd, (sockaddr*)&newClient, sin_size) == - 1)
                        {
                            close(newConnFd);
                            perror("Server: Unable to bind");
                            continue;
                        }

                        nextFreePort++;
                        FD_SET(newConnFd, &master);

                        if (newConnFd > fdMax)
                        {
                            fdMax = newConnFd;
                        }

                        /*
                         * The following piece of code fills the TransferInfo struct and adds an entry in the FDTransferInfo Map.
                         * We also send 1 packet to the client here using the newly created socket
                         */
                        int fileSize = getFileSize(fileName); 
                        int numBlocks = ceil(fileSize * 1.0 / 512); 

                        if (fileSize % 512 == 0)
                        {
                            numBlocks += 1;
                        }
                        TransferInfoT tInfo;
                        tInfo.filep = fp; 
                        tInfo.numBlocks = numBlocks;
                        tInfo.currentBlock = 1;
                        tInfo.timeLastAckRecvd = time(NULL);
                        tInfo.clientAddr = newClient;

                        fdTransferInfoMap[newConnFd] = tInfo; 

                        char fileData[512];
                        int bytesRead = fread(fileData, 1, 512, fdTransferInfoMap[newConnFd].filep);
                        // Create TFTP Header
                        char *dataBuf = createTFTPHeader(DATA, fileData, bytesRead, fdTransferInfoMap[newConnFd].currentBlock);

                        fdTransferInfoMap[newConnFd].bytesRead= bytesRead;
                        fdTransferInfoMap[newConnFd].lastPacketSent = dataBuf;

                        // Send data to the client using the new socket FD
                        sendto(newConnFd, dataBuf, bytesRead + 4, 0,
                                (struct sockaddr*)&clientAddr, sizeof(struct sockaddr_storage));

                    }

                }
                else
                {
                    /*
                     * Get ACK from the client and transmit the next block.
                     */
                    if ((numBytes = recvfrom(i, buf, MAXDATASIZE-1, 0, 
                                    (struct sockaddr*)&fdTransferInfoMap[i].clientAddr, &sin_size)) == -1)
                    {
                        perror("recvfrom");
                        exit(1);
                    }

                    // Check the received header's opcode
                    headerType = getHeaderType(buf);

                    if (headerType == ACKN)
                    {
                        unsigned short blockNumNetOrder;
                        memcpy(&blockNumNetOrder, buf+2, sizeof(unsigned short));
                        unsigned short blockNumHostOrder = ntohs(blockNumNetOrder);
                        if (blockNumHostOrder == fdTransferInfoMap[i].currentBlock)
                        {
                            cout << "ACK received for block " << fdTransferInfoMap[i].currentBlock << " on socket " << i << ". Sending next block" << endl;
                        }
                        fdTransferInfoMap[i].timeLastAckRecvd = time(NULL);
                    }

                    fdTransferInfoMap[i].currentBlock += 1;

                    if (fdTransferInfoMap[i].currentBlock > fdTransferInfoMap[i].numBlocks)
                    {
                        FD_CLR(i, &master);
                        fdTransferInfoMap.erase(i);
                        close(i);
                        break;
                    }

                    char fileData[512];
                    int bytesRead = fread(fileData, 1, 512, fdTransferInfoMap[i].filep);

                    //Create TFTP Header
                    char *dataBuf = createTFTPHeader(DATA, fileData, bytesRead, fdTransferInfoMap[i].currentBlock);

                    fdTransferInfoMap[newConnFd].bytesRead = bytesRead;
                    fdTransferInfoMap[newConnFd].lastPacketSent = dataBuf;

                    // Send the data
                    sendto(i, dataBuf, bytesRead + 4, 0,
                            (struct sockaddr*)&fdTransferInfoMap[i].clientAddr, sizeof(struct sockaddr_storage)); 

                }
            }
            else
            {
                /*
                 * Increment Timeout values
                 */
                if (fdTransferInfoMap.find(i) != fdTransferInfoMap.end())
                {
                    /*
                     * Socket FD exists in the map
                     */
                    time_t timeNow = time(NULL); 
                    double timeElapsed = difftime(timeNow, fdTransferInfoMap[i].timeLastAckRecvd);
                    if (timeElapsed > TIMEOUT/1000)
                    {
                        /*
                         * Since ACK has not been received, send the last packet again
                         */
                        sendto(i, fdTransferInfoMap[i].lastPacketSent, fdTransferInfoMap[i].bytesRead + 4, 0,
                                (struct sockaddr*)&fdTransferInfoMap[i].clientAddr, sizeof(struct sockaddr_storage)); 

                    }
                }
            }
        }
    }   
    return 0;
}

int main(int argc, char *argv[])
{
    if (argc != 3)
    {
       fprintf(stderr, "Usage: server <server IP> <server PORT>\n");
       exit(1);
    }

    Server *s = new Server(argv[1], argv[2]);
    s->createSocketAndBind();

    printf("TFTP Server is now online...\n");
    s->acceptConnection();
    return 0;
}
