#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/time.h>
#include <time.h>

#define MAX_LEN 500

#ifndef min
#define min(a, b)            (((a) < (b)) ? (a) : (b))
#endif

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
    return packet;
}

void stop_and_wait(char file_name[], struct sockaddr_in client_addr, int sock_fd){
    char buffer[MAX_LEN];
     int client_len;
    socklen_t socklen = sizeof(struct sockaddr_in);

    FILE *file = fopen(file_name, "r");
    if (file == NULL) {
        fprintf(stderr, "File not found\n");
        return;
    }

    fseek(file, 0L, SEEK_END);
    long fsize = ftell(file);
    fseek(file, 0L, SEEK_SET);
    memset(&buffer, 0, sizeof(buffer));

    while (fsize && fread(buffer, sizeof(char), min(MAX_LEN, fsize), file) > 0){
        struct data_packet packet;
        struct ack_packet ack_pack;
        packet = get_packet(buffer);
        packet.len = min(MAX_LEN, fsize);

        //printf(" counter : %d\n",counter);
        while (1){
            if(sendto(sock_fd, (const void *) &packet, sizeof(struct data_packet), 0,
                    (struct sockaddr *) &client_addr, socklen) == -1)
                {
                    fprintf(stderr, "sending packet failed.\n");
                }
            puts("waiting for ack");
            if(recvfrom(sock_fd, (void *) &ack_pack, sizeof(struct ack_packet),
                    0, (struct sockaddr *) &client_addr, &socklen) == -1)
                {
                    fprintf(stderr, "receiving ACK failed.\n");
                }
            printf("packet # : %d\n", packet.seqno);
            printf("ack # rec : %d\n", ack_pack.seqno);
            if (ack_pack.seqno == packet.seqno + 1){
                printf("Ack #%d received\n", ack_pack.seqno);
                break;
            }
            puts("resending the packet\n");
        }
        fsize -= min(fsize, MAX_LEN);
        memset(&buffer, 0, sizeof(buffer));
    }
}

void start(int sock_fd){
    int client_len;
    socklen_t socklen = sizeof(struct sockaddr_in);

    while(1) {
        struct data_packet packet;
        struct sockaddr_in client_addr;
    //    memset(&client_addr, 0, sizeof(client_addr));
    //    memset(&packet, 0, sizeof(packet));
        if(recvfrom(sock_fd, (void *) &packet, sizeof(struct data_packet),
                   0, (struct sockaddr *) &client_addr, &socklen) == -1){
                        fprintf(stderr, "recvfrom() failed.\n");
                        return;
                   }
        // printf("%d\n", n);
        // if(strcmp(packet.data, "") == 0){
        //     puts("packet empty");
        //     continue;
        // }
        puts("start");
        //TODO : send ack to client
        int pid = fork();

        if (pid == 0) {
            char *file_name = packet.data;
            struct ack_packet ack_pack;

            //ack_pack.seqno = packet.seqno + 1;
            //ack_pack.sent_time = time(NULL);
            //printf("Initial ACK : %d\n",ack_pack.seqno);

            //if(sendto(sock_fd, (const void *) &ack_pack, sizeof(struct ack_packet), 0,
                    // (struct sockaddr *) &client_addr, socklen) == -1){
                    //     fprintf(stderr, "sending acK failed failed.\n");
                    //     return;
            // } else {
            //     printf("Ack # %d sent\n",ack_pack.seqno);
            //}

            stop_and_wait(file_name, client_addr, sock_fd);
        }
    }
}


int main(int argc, char const *argv[])
{
    FILE *file = fopen("server.in", "r");
    if (file == NULL) {
        fprintf(stderr, "Error opening file\n");
        return 0;
    }

  //  struct timeval timeout;
   // timeout.tv_sec = 5;
  //  timeout.tv_usec = 0;

    int server_port_no;
    int window_size;
    int random_generator_seed;
    float probability_of_data_loss;


    fscanf(file, "%d", &server_port_no);
    fscanf(file, "%d", &window_size);
    fscanf(file, "%d", &random_generator_seed);
    fscanf(file, "%f", &probability_of_data_loss);

    //printf("%s %d %d %f", server_port_no, window_size, random_generator_seed, probability_of_data_loss);

    int socket_fd = socket(AF_INET,SOCK_DGRAM, 0);
    if (socket_fd <0)
        puts("error");

    // setsockopt(socket_fd, SOL_SOCKET,SO_RCVTIMEO, (char * ) &timeout,sizeof (timeout));
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(server_port_no);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    if(bind(socket_fd, (const struct sockaddr *) &server_addr, sizeof(server_addr)) < 0)
        puts("bind error");


    start(socket_fd);

    return 0;
}