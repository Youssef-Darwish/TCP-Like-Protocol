#include <arpa/inet.h> 
#include <netinet/in.h> 
#include <stdio.h> 
#include <stdlib.h> 
#include <string.h> 
#include <sys/socket.h> 
#include <sys/types.h> 
#include <unistd.h> 
  

#define MAXLEN 100

int counter;
struct data_packet {
/* Header */
uint16_t cksum; /* Optional bonus part */
uint16_t len;
uint32_t seqno;
time_t sent_time
/* Data */
char data[500]; /* Not always 500 bytes, can be less */
};


struct ack_packet {
/* Header */
uint16_t cksum; /* Optional bonus part */
uint16_t len;
uint32_t seqno;
time_t sent_time
};

struct data_packet get_packet(char *data){
    
    data_packet packet;
    packet.seqno = counter;
    counter++;
    packet.sent_time = time(NULL);
    strcpy(packet.data, data);
    packet.len = sizeof(packet.data);
    return packet;
}

void recv_stop_and_wait (char * fileName,int sockfd , struct *sockaddr_in addr_con){
    
    counter = rand() %10;
    data_packet packet = get_packet(fileName);
    char buff [MAXLEN];

    memcpy (buff,&packet,sizeof(packet));

    //time out
    sendto(sockfd, buff,sizeof(fileName), 
            0, (struct sockaddr*)&addr_con, 
            sizeof(addr_con)); 
    
    // 
    printf("\n---------Data Received---------\n"); 

    while (1) { 
        // receive  
        nBytes = recvfrom(sockfd, net_buf, NET_BUF_SIZE, 
                            sendrecvflag, (struct sockaddr*)&addr_con, 
                            &addrlen); 

        // process 
        if (recvFile(net_buf, NET_BUF_SIZE)) { 
            break; 
        } 
    } 
    printf("\n-------------------------------\n"); 
    


}




int main(int argc, char const *argv[])
{
    /* code */

    // check args , get args     int sockfd;

    // read inputs &args from file 

    in_port_t port_number;
    int socket_fd = socket(AF_INET,SOCK_DGRAM,IP_PROTOCOL);
    if (socket_fd <0)
        //error

    struct sockaddr_in server_address;
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(port_number);


    return 0;
}
