#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <time.h>
#include <sys/time.h>

#define MAX_LEN 500

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

void stop_and_wait(char file_name[], int sock_fd, struct sockaddr_in addr_con){
    
    counter = rand() %10;
    struct data_packet packet = get_packet(file_name);
    char buff[MAX_LEN];
    socklen_t socklen = sizeof(addr_con);

    if(sendto(sock_fd, (const void *) &packet, sizeof(struct data_packet), 0,
             (struct sockaddr *) &addr_con, socklen) == -1)
    {
        fprintf(stderr, "send() failed.\n");
        return;
    }

    FILE *file = fopen(file_name, "w");
    if (file == NULL) {
        fprintf(stderr, "File not found\n");
        return;
    }

    //while (1) {
        // receive
        memset (packet.data,0,sizeof(packet.data));
        while(recvfrom(sock_fd, (void *) &packet, sizeof(struct data_packet),
                   0, (struct sockaddr *) &addr_con, &socklen) > 0){
                    fputs(packet.data,file);
                   }
                puts("end");
    //}

}


int main(int argc, char const *argv[])
{

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

    struct timeval timeout;
    timeout.tv_sec = 5;
    timeout.tv_usec = 0;

    setsockopt(socket_fd, SOL_SOCKET,SO_RCVTIMEO, (char * ) &timeout,sizeof (timeout));

    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(server_port_no);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    stop_and_wait(file_name, socket_fd , server_addr);

    return 0;
}