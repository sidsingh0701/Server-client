#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <netdb.h>
#include <errno.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <fcntl.h>
#include <time.h>
#include <sys/timeb.h>
#define fileSize 1000000000


struct addrinfo receiverAddr, *receiverInfo, *receiverPointer;
char data_buffer[fileSize];
int portNo, MSS, N;
char* ipAddress;
typedef unsigned short int word16;
typedef unsigned int       word32;
typedef unsigned char      byte;
uint32_t ackRx = 0;
uint32_t packetSeq = 0;
uint32_t ackNo = 0;
int dataSent = 0;
int nPackets = 0;
struct timeb startMS, endMS;
time_t cTime, sTime, eTime;
int intialSeq = 0;
int maxSeq;
char* headerSize;
int dataSize;
int sock, n;
int ackSize = 8;
char ackPacket[8];
float timer = 0.4;
double delay, delayMs;


char* UDPheader(int packetLength, char* data, uint32_t seqNo) {
    
    
    
    uint32_t seq = seqNo;
    seq = htonl(seq);
    
    uint16_t data_packet_sequence;
    data_packet_sequence = htons(0X5555);
    
    int total_additions = (packetLength / 2);
    register u_long checksum_temp = 0;
    uint16_t checksum;
    while (total_additions--) {
        checksum_temp += *data++;
        if ((checksum_temp & 0XFFFF0000)) {
            checksum_temp &= 0XFFFF;
            checksum_temp++;
        }
    }
    checksum = htons(~(checksum_temp & 0xFFFF));
    
    char *headerSize = (char*) malloc(8);
    char* headerSize_temp = headerSize;
    memcpy(headerSize_temp, &seq, sizeof(uint32_t));
    headerSize_temp = headerSize_temp + sizeof(uint32_t);
    
    memcpy(headerSize_temp, &checksum, sizeof(uint16_t));
    headerSize_temp = headerSize_temp + sizeof(uint16_t);
    
    memcpy(headerSize_temp, &data_packet_sequence, sizeof(uint16_t));
    headerSize_temp = headerSize_temp + sizeof(uint16_t);
    return headerSize;
}

void fullPacket(char* currentPacket, int currentPacketLength)
{
    int header_length = 8;
    char currentPacket_message_tempbuf[MSS];
    char* currentPacket_tempbuf = currentPacket;
    char* buffer_ptr1 = data_buffer;
    
    buffer_ptr1 = &data_buffer[0] + packetSeq * MSS;
    memcpy(currentPacket_message_tempbuf, buffer_ptr1, currentPacketLength);
    headerSize = UDPheader(currentPacketLength,currentPacket_message_tempbuf, packetSeq);
    memcpy(currentPacket_tempbuf, headerSize, header_length);
    currentPacket_tempbuf += header_length;
    memcpy(currentPacket_tempbuf, currentPacket_message_tempbuf, currentPacketLength);
    return;
}

int main(int argc, char *argv[])
{

    struct sockaddr_storage ackAddr;
    socklen_t recvaddr_len = sizeof ackAddr;
    int numbytes;
    char checkFile[100];
    
    sscanf(argv[1],"%s", ipAddress);
    sscanf(argv[2],"%d", &portNo);
    sscanf(argv[4],"%d", &N);
    sscanf(argv[5],"%d", &MSS);
    
    maxSeq = N - 1;
    
    
    
    memset(&receiverAddr, 0, sizeof receiverAddr);
    receiverAddr.ai_family = AF_UNSPEC;
    receiverAddr.ai_socktype = SOCK_DGRAM;
    
    
    n = getaddrinfo(argv[1], argv[2], &receiverAddr, &receiverInfo);
    if (n != 0) {

        exit(EXIT_FAILURE);
    }
    for (receiverPointer = receiverInfo; receiverPointer != NULL; receiverPointer = receiverPointer->ai_next) {
        
        sock= socket(receiverPointer->ai_family, receiverPointer->ai_socktype,receiverPointer->ai_protocol);
        if (sock == -1)
        continue;
        else
        {
            fcntl(sock,F_SETFL,O_NONBLOCK);
            if (connect(sock, receiverPointer->ai_addr, receiverPointer->ai_addrlen) != -1)
            break;
            
            close(sock);
        }
        
        
    }
    
    bzero(checkFile, 100);
    
    int nPackets = 0;
    //--- Opening a File -----//
    FILE *inputFile = fopen(argv[3], "r");
    int pos = 0;
    
    while (!feof(inputFile))
    {
        fread(&data_buffer[pos],1,1,inputFile);
        pos = pos + 1;
    }
    //------------------------//
    dataSize = ftell(inputFile); //
    
    int remainder = (dataSize % MSS);
    if (remainder == 0)
    nPackets = (dataSize / MSS);
    else
    nPackets = (dataSize / MSS) + 1;
    
    int header_length = 8;
    int packet_size;
    packet_size = MSS + header_length;
    char* currentPacket=(char*)malloc(packet_size);
    time_t time_watch[(dataSize+1)/MSS];
    
    char currentPacket_message_tempbuf[MSS];
    char* currentPacket_tempbuf = currentPacket;
    char* buffer_ptr1 = data_buffer;
    
    
    
    ftime(&startMS);
    time(&sTime);
    
    while ((dataSent!=dataSize) && (ackNo != nPackets - 1))
    {  currentPacket_tempbuf = currentPacket;
        if(packetSeq <= maxSeq && packetSeq < nPackets)
        {
            if((dataSize - dataSent) >= MSS)
            {
                fullPacket(currentPacket,MSS);
                
                if ((n = sendto(sock, currentPacket,(MSS+8),0,receiverPointer->ai_addr,receiverPointer->ai_addrlen)) != -1)
                {
                    time(&time_watch[packetSeq]);
                    dataSent = dataSent + MSS;
                }
                
            }
            else
            {
                
                fullPacket(currentPacket, dataSize-dataSent);
                
                if(dataSize!=dataSent)
                {
                    if ((numbytes = sendto(sock, currentPacket, dataSize - dataSent + 8, 0,receiverPointer->ai_addr,receiverPointer->ai_addrlen)) != -1)
                    {
                        time(&time_watch[packetSeq]);
                    }
                }
            }
            
            packetSeq++;
        }
        
        if((n = recvfrom(sock, ackPacket, ackSize,0,(struct sockaddr *)&ackAddr, &recvaddr_len)) > 0)
        {
            memcpy(&ackRx,ackPacket,sizeof(uint32_t));
            ackRx = ntohl(ackRx);
            if (ackRx==nPackets-1)
            {
                dataSent = dataSize;
            }
            
            if(ackRx >= intialSeq )
            {
                ackNo = ackRx;
                
                if (ackNo > (nPackets - 1))
                intialSeq = nPackets - 1;
                else
                intialSeq = ackNo + 1;
                
                
                if ((intialSeq + N - 1) > (nPackets - 1))
                maxSeq = nPackets -1;
                else
                maxSeq = intialSeq + N - 1;
            }
            else
            {
                time(&cTime);
                if( difftime(cTime,time_watch[intialSeq]) > timer)
                {
                    printf("Timeout, Sequence Number = %0d\n",intialSeq);
                    packetSeq = intialSeq;
                    dataSent = MSS * intialSeq;
                }
            }
        }
        else
        {
            time(&cTime);
            
            if( difftime(cTime,time_watch[intialSeq]) > timer)
            {
                
                printf("Timeout, Sequence Number = %0d\n",intialSeq);
                packetSeq = intialSeq;
                dataSent = MSS * intialSeq;
            }
            
        }
        
    }
    
    headerSize = UDPheader(0,currentPacket_message_tempbuf,0xFFFFFFFF);
    memcpy(currentPacket, headerSize, header_length);
    currentPacket_tempbuf = currentPacket + header_length;
    memcpy(currentPacket_tempbuf, currentPacket_message_tempbuf, 0);    
    
    ftime(&endMS);
    time(&eTime);
    delay = difftime(eTime,sTime);
    delayMs = (int) (1000.0 * (endMS.time - startMS.time) + (endMS.millitm - startMS.millitm));
    
    if ((numbytes = sendto(sock, currentPacket, 8, 0,receiverPointer->ai_addr, receiverPointer->ai_addrlen)) < 0)
    {
        exit(1);
    }
    printf("File Transfer Complete\n");
    printf("File Size = %d\n", dataSize);
    printf("Time Taken = %0.4fms\n", delayMs);
    
    
    freeaddrinfo(receiverInfo);
    close(sock);
    return 0;
    
}
