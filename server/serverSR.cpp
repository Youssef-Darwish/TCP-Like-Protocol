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
#include <list>

using namespace std;

#define MAX_LEN 500
#define THRESHOLD 8

#ifndef min
#define min(a, b)            (((a) < (b)) ? (a) : (b))
#endif

int counter; //used to assign sequence number
int cwnd; //window size
float loss_percent;
int random_generator_seed;
list <int> number_of_packets_without_loss; //N0 N1 N2 ...
list <int> random_packets_lost; //all packets that will be lost

struct data_packet {
    /* Header */
    uint16_t cksum; /* Optional bonus part */
    uint16_t len;
    uint32_t seqno;
    time_t sent_time;
    /* Data */
    char data[501]; /* Not always 500 bytes, can be less */
};
list <data_packet> packets; //all packets read from file

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

//1: ack before threshold, 2: ack after threshold
//3: timeout, 4: packet loss
void set_window_size(int state){
    switch (state){
        case 1:
            cwnd *= 2;
            break;
        case 2:
            cwnd++;
            break;
        case 3:
            cwnd = 1;
            break;
        case 4:
            cwnd /= 2;
            break;
    }
}

void setup_lists(char file_name[]){
    char buffer[MAX_LEN];
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
        packet = get_packet(buffer);
        packet.len = min(MAX_LEN, fsize);
        packet.data[packet.len] = '\0';

      //  packets.push_back(packet);

        fsize -= min(fsize, MAX_LEN);
        memset(&buffer, 0, sizeof(buffer));
    }
   // showlist(packets);

}

// void showlist(list <data_packet> g)
// {
//     list <int> :: iterator it;
//     for(it = g.begin(); it != g.end(); ++it)
//         puts((*it).data);
// }


void start(int sock_fd){
    int client_len;
    socklen_t socklen = sizeof(struct sockaddr_in);
    struct data_packet packet;
    struct sockaddr_in client_addr;

    if(recvfrom(sock_fd, (void *) &packet, sizeof(struct data_packet),
               0, (struct sockaddr *) &client_addr, &socklen) == -1){
                    fprintf(stderr, "recvfrom() failed.\n");
                    return;
            }

    struct ack_packet ack_pack;
    ack_pack.seqno = packet.seqno + 1;
    ack_pack.sent_time = time(NULL);

    char *file_name = packet.data;

    if(sendto(sock_fd, (const void *) &ack_pack, sizeof(struct ack_packet), 0,
            (struct sockaddr *) &client_addr, socklen) == -1){
                fprintf(stderr, "sending acK failed failed.\n");
                return;
    } else {
        printf("Ack # %d sent\n",ack_pack.seqno);
    }

    setup_lists(file_name);
    //selective_repeat(file_name, client_addr, sock_fd);
}

void selective_repeat(char file_name[], struct sockaddr_in client_addr, int sock_fd) {



}


int main(int argc, char const *argv[])
{
    FILE *file = fopen("server.in", "r");
    if (file == NULL) {
        fprintf(stderr, "Error opening file\n");
        return 0;
    }

    int server_port_no;

    fscanf(file, "%d", &server_port_no);
    fscanf(file, "%d", &cwnd);
    fscanf(file, "%d", &random_generator_seed);
    fscanf(file, "%f", &loss_percent);

    struct timeval timeout;
    timeout.tv_sec = 10;
    timeout.tv_usec = 0;

    //printf("%s %d %d %f", server_port_no, cwnd, random_generator_seed, loss_percent);

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