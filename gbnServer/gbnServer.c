#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>
#include <netdb.h>
#define fileSize 2000

int ackPacket(uint32_t packetNo, int sock, struct sockaddr_in senderAddr)
{
    int headerLength = 8;
    char     ackBuffer[headerLength];
    char *ackPointer = ackBuffer;
    uint16_t ack_id = 0xCCCC;
    
    packetNo   = htonl(packetNo);
    ack_id    = htons(ack_id);
    
    memcpy(ackPointer,&packetNo,sizeof(uint32_t));
    ackPointer+=sizeof(uint32_t);
    memset(ackPointer,0,sizeof(uint16_t));
    ackPointer+=sizeof(uint16_t);
    memcpy(ackPointer,&ack_id,sizeof(uint16_t));
    ackPointer+=sizeof(uint16_t);
    
    int temp;
    if ((temp = sendto(sock, ackBuffer, headerLength, 0, (struct sockaddr *)&senderAddr , sizeof (senderAddr) )) < 0)
    {
        return 0;
    }
    return 0;
}

int UDPheader(int packetLength, char *buffer,uint32_t seqNum,int sock, FILE *outputFile, float errorRate)
{
    uint16_t checkSum;
    uint16_t packetID;
    uint32_t seqNoLocal;
    char *packetRecieved;
    int headerLength = 8;
    char *packetBuffer = buffer;
    packetRecieved = (char*)malloc(sizeof(char) * (packetLength-8));
    int count = (packetLength - headerLength) / 2;
    register u_long checkSumBuffer = 0;
    uint16_t calcCheckSum;
    int value = 0;
    double r;
    int i,j;
    static unsigned long check = 0;
    
    if(buffer == NULL)
    {
        return -1;
    }
    
    memcpy(&seqNoLocal, packetBuffer, sizeof(uint32_t));
    packetBuffer+=sizeof(uint32_t);
    
    memcpy(&checkSum, packetBuffer, sizeof(uint16_t));
    packetBuffer+=sizeof(uint16_t);
    
    memcpy(&packetID, packetBuffer, sizeof(uint16_t));
    packetBuffer+=sizeof(uint16_t);
    
    seqNoLocal = ntohl(seqNoLocal);
    checkSum = ntohs(checkSum);
    packetID = ntohs(packetID);
    
    if(seqNoLocal != seqNum)
    {
        if (seqNoLocal == 0xFFFFFFFF)
        {
            printf("File Transfer Complete\n");
            close(sock);
            exit(1);
        }
        else
        {
            if (seqNoLocal > seqNum)
            {
                return -1;
            }
            else
            {
                printf(" Error: Duplicate Packet Received\n");
                return 100;
            }
            
        }
    }
    
    memcpy(packetRecieved,packetBuffer, packetLength - 8);
    while (count--)
    {
        checkSumBuffer += *packetBuffer++;
        if (checkSumBuffer & 0xFFFF0000)
        {
            checkSumBuffer &= 0xFFFF;
            checkSumBuffer++;
        }
    }
    
    calcCheckSum = ~(checkSumBuffer & 0xFFFF);
    
    if(calcCheckSum != checkSum)
    {
        printf("Error: Checksum MisMatch\n");
        return -1;
    }
    else
    {
        
        double p = (double)rand()/ (RAND_MAX + 1.0);
        
        if (p < errorRate)
        {
            printf("Probability %f R = %f\n", errorRate,p);
            printf("Packet Loss, Sequence Number = %d\n",seqNoLocal);
            return -1;
        }
        else
        {
            value = fwrite(packetRecieved,1,packetLength-8,outputFile);
            free(packetRecieved);
            return 0;
        }
    }
    
}


int main (int argc, char *argv[])
{
    char *data_buffer = (char*)malloc(sizeof(char) * fileSize);
    char *filePointer;
    int sock;
    int port;
    struct addrinfo receiverAddr, *receiverInfo, *receiverPointer;
    socklen_t senderSize;
    struct sockaddr_in senderAddr;
    filePointer=data_buffer;
    float errorRate;
    
    uint32_t packetNo = 0;
    
    
    srand(time(0));
    
    sscanf(argv[1],"%d",&port);
    sscanf(argv[3],"%f",&errorRate);
    FILE *ftp_filename=fopen(argv[2],"w");
    
    memset(&receiverAddr, 0, sizeof(struct addrinfo));
    receiverAddr.ai_family = AF_UNSPEC;
    receiverAddr.ai_socktype = SOCK_DGRAM;
    receiverAddr.ai_flags = AI_PASSIVE;
    
    int info;
    info = getaddrinfo(NULL, argv[1], &receiverAddr, &receiverInfo);
    if (info != 0) {
        exit(EXIT_FAILURE);
    }
    
    for (receiverPointer = receiverInfo; receiverPointer != NULL; receiverPointer = receiverPointer->ai_next) {
        sock = socket(receiverPointer->ai_family, receiverPointer->ai_socktype,
                                receiverPointer->ai_protocol);
        if (sock == -1)
        continue;
        
        if (bind(sock, receiverPointer->ai_addr, receiverPointer->ai_addrlen) == 0)
        break;
        
        close(sock);
    }
    
    if (receiverPointer == NULL) {
        exit(EXIT_FAILURE);
    }
    
    freeaddrinfo(receiverInfo);
    printf("Ready to receive on %d\n", port);
    
    int temp = 1;
    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &temp, sizeof(int));
    while(1)
    {  data_buffer=filePointer;
        char *data_buffer_temp = data_buffer;
        senderSize = sizeof senderAddr;
        int size;
        if ((size = recvfrom(sock, data_buffer,fileSize-1, 0,(struct sockaddr *)&senderAddr, &senderSize)) < 0)
        {
            exit(1);
        }
        
        int ack;
        ack = UDPheader(size,data_buffer,packetNo,sock,ftp_filename,errorRate);
        if(ack != -1)
        {
            
            if(ack == 100)
            {
                ackPacket(packetNo - 1, sock, senderAddr);
                
            }
            else
            {
                ackPacket(packetNo,sock, senderAddr);
                packetNo++;
            }
        }
        
    }
    close(sock);
    fclose(ftp_filename);
    return 0;
}

