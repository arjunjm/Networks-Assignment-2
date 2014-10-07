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
    TFTPHeaderTypeT headerType;
    struct sockaddr_storage clientAddr;
    socklen_t sin_size;
    char ipAddr[INET6_ADDRSTRLEN]; 
    char buf[MAXDATASIZE];
    int numBytes;
    fd_set master, read_fds;
    int fdMax;
    struct timeval tv;
    tv.tv_sec = 50;
    tv.tv_usec = 500000;
    FD_ZERO(&master);
    FD_ZERO(&read_fds);

    FD_SET(sockFd, &master);
    fdMax = sockFd;

    while(1)
    {
        sin_size = sizeof clientAddr;
        if ((numBytes = recvfrom(sockFd, buf, MAXDATASIZE-1, 0, 
                    (struct sockaddr*)&clientAddr, &sin_size)) == -1)
        {
            perror("recvfrom");
            exit(1);
        }
        else
        {
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
                int sentBytes = sendto(sockFd, errPacket, strlen(errMessage) + 4, 0,
                        (struct sockaddr*)&clientAddr, sizeof(struct sockaddr_storage));
                continue;
            }

            char fileData[512];
            int fileSize  = getFileSize(fileName);
            bool lastAckRequired = false;
            int numBlocks = ceil(fileSize * 1.0 / 512); 
            int currentBlock = 1;

            if (fileSize % 512 == 0)
            {
                lastAckRequired = true;
                numBlocks += 1;
            }

            while(1)
            {
                int bytesRead = fread(fileData, 1, 512, fp); 
                // Create TFTP Header
                char *dataBuf = createTFTPHeader(DATA, fileData, bytesRead, currentBlock);

                // Send data to client
                int sentBytes = sendto(sockFd, dataBuf, bytesRead + 4, 0,
                        (struct sockaddr*)&clientAddr, sizeof(struct sockaddr_storage));

                // Wait for ACK from client
                if ((numBytes = recvfrom(sockFd, buf, MAXDATASIZE-1, 0, 
                                (struct sockaddr*)&clientAddr, &sin_size)) == -1)
                {
                    perror("recvfrom");
                    exit(1);
                }

                // Check the received header's opcode
                headerType = getHeaderType(buf);

                if (headerType == ACKN)
                {
                    // ACK
                    unsigned short blockNumNetOrder;
                    memcpy(&blockNumNetOrder, buf+2, sizeof(unsigned short));
                    unsigned short blockNumHostOrder = ntohs(blockNumNetOrder);
                    if (blockNumHostOrder == currentBlock)
                    {
                        //cout << "ACK received for block " << currentBlock << endl;
                    }

                }
                currentBlock += 1;

                if(currentBlock > numBlocks)
                    break;

            }
            /*
            if (lastAckRequired)
            {
                char *dataBuf = createTFTPHeader(DATA, NULL, numBlocks);
                // Send data to client
                sendto(sockFd, dataBuf, 4, 0, (struct sockaddr*)&clientAddr, 
                        sizeof(struct sockaddr_storage));

                // Wait for ACK from client
                if ((numBytes = recvfrom(sockFd, buf, MAXDATASIZE-1, 0, 
                                (struct sockaddr*)&clientAddr, &sin_size)) == -1)
                {
                    perror("recvfrom");
                    exit(1);
                }
                else
                {
                    cout << " Got ack..terminating now \n";
                }

            }*/
        }

#if 0
       read_fds = master;
       if (select(fdMax + 1, &read_fds, NULL, NULL, &tv) == -1)
       {
           perror("Select");
           exit(4);
       }

       /*
        * Run through existing connections and check if a client is ready
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
                   newConnFd = accept(i, (struct sockaddr *)&clientAddr, &sin_size);
                   if (newConnFd == -1)
                   {
                       perror("Error while accepting connection..");
                   }
                   else
                   {
                       FD_SET(newConnFd, &master);

                       if (newConnFd > fdMax)
                       {
                           fdMax = newConnFd;
                       }
                       inet_ntop(clientAddr.ss_family, get_in_addr((struct sockaddr *)&clientAddr), ipAddr, sizeof ipAddr);


                   }

               }
               else
               {
                   /*
                    * Handle data from client connection.
                    */
                   int readBytes;
                   SBMPMessageType msgType;
                   char *message = new char[512];
                   if ((readBytes = recvData(i, msgType, message)) <= 0)
                   {
                        /*
                         * This means either there was a error in receiving data
                         * or that the client has closed the connection.
                         */
                        if (readBytes == 0)
                        {
                            if (fdUserMap[i] != "")
                            {
                                string leftMsg = fdUserMap[i] + " has left the chat session.\n";
                                cout << leftMsg;
                                cout << "Connection closed\n";

                                for (int j = 0; j <= fdMax; j++)
                                {
                                    if (FD_ISSET (j, &master))
                                    {
                                        if ((j != i) && (j != sockFd))
                                        {
                                            SBMPHeaderT *sbmpHeader = createMessagePacket(OFFLINE_INFO, NULL, leftMsg.c_str());
                                            if (sendData(j, sbmpHeader, sizeof(SBMPHeaderT), 0) == -1)
                                                perror("Error while broadcasting message");
                                        }
                                    }
                                }
                            }
                            // Close the connection
                            FD_CLR(i, &master);
                            userStatusMap.erase(fdUserMap[i]);
                            fdUserMap.erase(i);
                            close(i);
                        }
                        else
                        {
                            perror("receive");
                        }
                   }
                   else
                   {
                       /*
                        * Client is sending actual data.
                        */
                       const char* userName = fdUserMap[i].c_str();
                       for (int j = 0; j <= fdMax; j++)
                       {
                           if (FD_ISSET (j, &master))
                           {
                               if ((j != i) && (j != sockFd))
                               {
                                   if (msgType == SEND)
                                   {
                                       SBMPHeaderT *sbmpHeader = createMessagePacket(FWD, userName, message);
                                       if (sendData(j, sbmpHeader, sizeof(SBMPHeaderT), 0) == -1)
                                           perror("Error while broadcasting message");
                                   }
                                   else if (msgType == JOIN)
                                   {
                                       string enterMsg;
                                       enterMsg.append(userName);
                                       enterMsg.append(" has entered the chat session");
                                       SBMPHeaderT *sbmpHeader = createMessagePacket(ONLINE_INFO, NULL, enterMsg.c_str());
                                       if (sendData(j, sbmpHeader, sizeof(SBMPHeaderT), 0) == -1)
                                           perror("Error while broadcasting message");

                                   }
                               }
                           }
                       }
                       delete [] message;
                        
                   }
               }
           }
       }
#endif
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
    //s->listenForConnections();

    printf("TFTP Server is now online...\n");
    s->acceptConnection();
    return 0;
}
