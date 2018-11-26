#include <arpa/inet.h> 
#include <netinet/in.h> 
#include <stdio.h> 
#include <stdlib.h> 
#include <string.h> 
#include <sys/socket.h> 
#include <sys/types.h> 
#include <unistd.h> 
  
#define IP_PROTOCOL 0 
#define IP_ADDRESS "127.0.0.1" // localhost 
#define PORT_NO 15050 
#define NET_BUF_SIZE 500
#define sendrecvflag 0 
  
int counter;

struct data_packet get_packet(char *data){
    
    struct data_packet packet;
    packet.seqno = counter;
    counter++;
    packet.sent_time = time(NULL);
    strcpy(packet.data, data);
    packet.len = sizeof(packet.data);
    return packet;
}


struct data_packet {
/* Header */
    uint16_t cksum; /* Optional bonus part */
    uint16_t len;
    uint32_t seqno;
    time_t sent_time;
    /* Data */
    char data[500]; 
};

/* Ack-only packets are only 8 bytes */
struct ack_packet {
    uint16_t cksum; /* Optional bonus part */
    uint16_t len;
    uint32_t ackno;
    uint32_t seqno;

};

struct data_packet get_packet(char *data){
    
    struct data_packet packet;
    packet.seqno = counter;
    counter++;
    packet.sent_time = time(NULL);
    strcpy(packet.data, data);
    packet.len = sizeof(packet.data);
    return packet;
}


// funtion to clear buffer 
void clearBuf(char* b) 
{ 
    int i; 
    for (i = 0; i < NET_BUF_SIZE; i++) 
        b[i] = '\0'; 
} 
  
// function to receive file 
int recvFile(char* buf, int s) 
{ 
    int i; 
    char ch; 
    for (i = 0; i < s; i++) { 
        ch = buf[i]; 
        if (ch == EOF) 
            return 1; 
        else
            printf("%c", ch); 
    } 
    return 0; 
} 
  
// driver code 
int main() 
{ 
    int sockfd, nBytes; 
    struct data_packet packet;
    packet.seqno=1212;
    packet.sent_time = time(NULL);

    struct sockaddr_in addr_con; 
    int addrlen = sizeof(addr_con); 
    addr_con.sin_family = AF_INET; 
    addr_con.sin_port = htons(PORT_NO); 
    addr_con.sin_addr.s_addr = inet_addr(IP_ADDRESS); 
    char net_buf[sizeof(packet)]; 
    FILE* fp; 
    // socket() 
    sockfd = socket(AF_INET, SOCK_DGRAM, 
                    IP_PROTOCOL); 
  
    if (sockfd < 0) 
        printf("\nfile descriptor not received!!\n"); 
    else
        printf("\nfile descriptor %d received\n", sockfd); 
    while (1) { 
        printf("\nPlease enter file name to receive:\n"); 

        scanf("%s",packet.data);
        packet.sent_time = time(NULL);
        memcpy(net_buf,&packet.seqno,sizeof (packet.seqno));
        memcpy(net_buf+sizeof (packet.seqno),&packet.sent_time,sizeof (packet.sent_time));
        memcpy(net_buf+2*sizeof(int),&packet.data ,sizeof (packet.data));
        printf("%d",sizeof(net_buf));
        sendto(sockfd, net_buf, NET_BUF_SIZE, 
               sendrecvflag, (struct sockaddr*)&addr_con, 
               addrlen); 
  
        printf("\n---------Data Received---------\n"); 


        char buff_recv [sizeof(packet)];
        struct data_packet my_packet;
        while (1) { 
            // receive 
            clearBuf(buff_recv); 
            nBytes = recvfrom(sockfd, buff_recv, sizeof (buff_recv), 
                              sendrecvflag, (struct sockaddr*)&addr_con, 
                              &addrlen); 

            // process
            memcpy(&my_packet.seqno,buff_recv ,sizeof(int));
            memcpy(&my_packet.sent_time,buff_recv +sizeof(int),sizeof(int));
            memcpy(&my_packet.data,buff_recv + 2*sizeof (int)
            ,sizeof(my_packet.data));
            printf(" seq ,time %d %d\n",my_packet.seqno,my_packet.sent_time);


            if (recvFile(buff_recv, NET_BUF_SIZE)) { 
                break; 
            } 
        } 
        printf("\n-------------------------------\n"); 
    } 
    return 0; 
} 














/*

#define MAXLEN 100
#define NET_BUF_SIZE 32 

int counter;
struct data_packet {



void recv_stop_and_wait (char * fileName,int sockfd , struct* sockaddr_in addr_con){
    
    counter = rand() %10;
    struct data_packet packet = get_packet(fileName);
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
        int nBytes = recvfrom(sockfd, net_buf, NET_BUF_SIZE, 
                            0, (struct sockaddr*)&addr_con, 
                            &addrlen); 

        // process 
        if (recvFile(net_buf, NET_BUF_SIZE)) { 
            break; 
        } 
    } 
    printf("\n-------------------------------\n"); 
}

*/
































/*
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
    /* Header 
    uint16_t cksum; /* Optional bonus part 
    uint16_t len;
    uint32_t seqno;
    time_t sent_time;
    /* Data 
    char data[500]; 
};


struct ack_packet {
    /* Header 
    uint16_t cksum; /* Optional bonus part 
    uint16_t len;
    uint32_t seqno;
    time_t sent_time;
};

struct data_packet get_packet(char data[]){

    struct data_packet packet;
    packet.seqno = counter;
    counter++;
    packet.sent_time = time(NULL);
    strcpy(packet.data, data);
    packet.len = sizeof(packet.data);
    return packet;
}

void stop_and_wait(char file_name[], int sock_fd, struct sockaddr_in * addr_con){

    counter = rand() %10;
    struct data_packet packet = get_packet(file_name);
    char buff[MAXLEN];

    memcpy (buff, &packet, sizeof(packet));

    //time out
    sendto(sock_fd, &buff, sizeof(buff), MSG_CONFIRM, (struct sockaddr*) &addr_con, sizeof(addr_con));
    puts("msg sent");


    // while (1) {
    //     // receive
    //     nBytes = recvfrom(sockfd, net_buf, NET_BUF_SIZE,
    //                         sendrecvflag, (struct sockaddr*)&addr_con,
    //                         &addrlen);

    //     // process
    //     if (recvFile(net_buf, NET_BUF_SIZE)) {
    //         break;
    //     }
    // }
    // printf("\n-------------------------------\n");



}




int main(int argc, char const *argv[])
{
    /* code */

    // check args , get args     int sockfd;

    // read inputs &args from file

    /*
    IP address of server.
Well-known port number of server.
Port number of client.
Filename to be transferred (should be a large file).
Initial receiving sliding-window size (in datagram units).
    

    FILE *file = fopen("client.in", "r");
    if (file == NULL) {
        fprintf(stderr, "Error opening file\n");
        return 0;
    }

    char ip_addr[20];
    int server_port_no;
    int client_port_no;
    char file_name[20];
    int window_size;

    fscanf(file, "%s", ip_addr);
    fscanf(file, "%d", &server_port_no);
    fscanf(file, "%d", &client_port_no);
    fscanf(file, "%s", file_name);
    fscanf(file, "%d", &window_size);

    //printf("%s %s %s %s %d", ip_addr, server_port_no, client_port_no, file_name, window_size);

     int socket_fd = socket(AF_INET,SOCK_DGRAM, 0);
     if (socket_fd <0)
         puts("error");

     struct sockaddr_in server_addr;
     memset(&server_addr, 0, sizeof(server_addr));
     server_addr.sin_family = AF_INET;
     server_addr.sin_port = htons(server_port_no);
     server_addr.sin_addr.s_addr = INADDR_ANY;


     //TEST CONNECTION

     //char *hello = "hello from client";
     //sendto(socket_fd, (char *)hello, strlen(hello), MSG_CONFIRM, (struct sockaddr *) &server_addr, sizeof(server_addr));

     //printf("hello message sent\n");

     stop_and_wait(file_name, socket_fd , &server_addr);

    return 0;
}
*/