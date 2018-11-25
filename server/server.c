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

void stop_and_wait(int sock_fd){
    char buffer[MAXLEN];
    int n, client_len;
    struct sockaddr_in client_addr;
    struct data_packet packet;

    memset(&client_addr, 0, sizeof(client_addr));
    puts("hii");

    n = recvfrom(sock_fd, (char *)buffer, sizeof(buffer), MSG_WAITALL, (struct sockaddr *) &client_addr, &client_len);

    printf("client : %d", n);
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


    stop_and_wait(socket_fd);
    // char buffer[MAXLEN];
    // int n;
    // int client_len;

    // n = recvfrom(socket_fd, (char *)buffer, MAXLEN, MSG_WAITALL, (struct sockaddr *) &client_addr, &client_len);
    // buffer[n] = '\0';

    // printf("client : %s", buffer);

    return 0;
}