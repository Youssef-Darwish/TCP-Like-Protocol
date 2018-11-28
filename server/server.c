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
    packet.len = sizeof(packet.data);
    return packet;
}

void start(int sock_fd){
    char buffer[MAX_LEN];
    int n, client_len;
    struct sockaddr_in client_addr;
    struct data_packet packet;
    socklen_t socklen = sizeof(struct sockaddr_in);

    while(1) {
        memset(&client_addr, 0, sizeof(client_addr));
        if(recvfrom(sock_fd, (void *) &packet, sizeof(struct data_packet),
                   0, (struct sockaddr *) &client_addr, &socklen) == -1){
                        fprintf(stderr, "recvfrom() failed.\n");
                        return;
                   }

        int pid = fork();
        char *file_name = packet.data;
        char buffer[500];

        if (pid == 0) {
            FILE *file = fopen(file_name, "r");
            if (file == NULL) {
                fprintf(stderr, "File not found\n");
                return;
            }

            fseek(file, 0L, SEEK_END);
            long fsize = ftell(file);
            fseek(file, 0L, SEEK_SET);

            while (fsize && fread(buffer, sizeof(char), min(MAX_LEN, fsize), file) > 0){
                struct data_packet packet;
                packet = get_packet(buffer);

                if(sendto(sock_fd, (const void *) &packet, sizeof(struct data_packet), 0,
                        (struct sockaddr *) &client_addr, socklen) == -1)
                    {
                        fprintf(stderr, "sending packet failed.\n");
                    }
                fsize -= min(fsize, MAX_LEN);
                memset(&buffer, 0, sizeof(buffer));
            }
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