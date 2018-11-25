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
    time_t sent_time;
    /* Data */
    char data[500]; /* Not always 500 bytes, can be less */
};


struct ack_packet {
    /* Header */
    uint16_t cksum; /* Optional bonus part */
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
    */

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
