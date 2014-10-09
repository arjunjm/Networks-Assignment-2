ECEN602 HW2 Programming Assignment (TFTP protocol implementation)
-----------------------------------------------------------------

Team Number: 25
Member 1 # Moothedath, Arjun Jayaraj(UIN: 722008073)
Member 2 # Wilkins, Janessa (UIN: 724000155)
---------------------------------------

Description/Comments:
--------------------
Testing was done on both localhost and across 2 machines in 213A. Both seemed to work as intended.

We have implemented the assignment in C++. Most of the code from the 1st assignment was reused here as well, especially the multiplexing of multiple clients. That said, we had to add in new structures for TFTP packets. There are described in more detail below.

1. The helper header file has an enum TFTPHeaderTypeT to indicate the type of TFTP packet (ie whether it is RRQ, DATA, ACK or ERR). The structure definition is as shown.

typedef enum TFTPHeaderType
{
    RRQ  = 1,
    WRQ  = 2,
    DATA = 3,
    ACKN = 4,
    ERR  = 5 
} TFTPHeaderTypeT;

2. The helper header file also contains the defintion for another structure called TransferInfoT which contains the file transfer information for a particular client request. The defintion is as shown.

typedef struct TransferInfo
{
    FILE *filep;
    int numBlocks;
    int currentBlock;
    time_t timeLastAckRecvd;
    int bytesRead;
    char *lastPacketSent;
    struct sockaddr_in clientAddr;
} TransferInfoT;

The structure attributes are mostly self-explanatory.
    * filep is the file pointer to the file being transfered.
    * numBlocks is the number of blocks that have to be transfered. (Each block has a maximum size of 512 bytes). This is computed as ceil(fileSize/512)
    * currentBlock indicates the current block which has to be transfered.
    * timeLastAckRecvd indicates the time at which the last ACK was received. This is used for checking timeouts.
    * bytesRead indicates the total number of bytes that have been read from the file in the current transfer.
    * lastPackSent is used as a buffer packet in case a timeout occurs.
    * clientAddr stores the client address information.

3. The helper header also has a TIMEOUT macro variable which is set to 10 milliseconds.

4. The Server class is as shown.

class Server
{
    private:
        struct addrinfo hints;
        int nextFreePort;
        int sockFd;
        char serverIP[20];
        char portNum[10];
        std::map<int, TransferInfoT> fdTransferInfoMap;

    public:
        Server(char *serverIP, char *portNum);
        int createSocketAndBind();
        int acceptConnection();
};

The fdTransferInfoMap is a map from the socket FDs to the corresponding TransferInfoT structs. We have a primary sockFd socket descriptor which listens for RRQ requests from clients. All subsequent transactions (transfer of TFTP packets and receiving ACKs) with the clients are done through new sockets bound to new random ports.

5. In order to support multiple simultaneous clients, we have used the select system call (the usage remains similar to 1st assignment). The new sockets created for communication with each new client is pushed into the read_fds set. If the main socket FD is ready to be read, it indicates that the server has received an RRQ request from a client. Whenever a client FD is ready(indicating that the ACK from client has arrived), we read the ACK packet and send the next block. The fdTransferInfoMap is changed accordingly with each transaction (the timeLastAckRecvd is reset to current time, currentBlock is updated, lastPacketSent buffer updated)

6. Timeout detection
====================

If none of the FDs in the read_fd set are ready, we check if the difference between current time and timeLastAckRecvd field has exceeded 10 ms. If it has, then we retransmit the last packet stored in the lastPacketSent field of the TransferInfoT struct in the map.

Unix command for starting server:
------------------------------------------
./server SERVER_IP SERVER_PORT


