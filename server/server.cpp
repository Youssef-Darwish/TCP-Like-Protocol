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
#include <iostream>
#include <fstream>
#include <math.h>
#include <vector>
#include <algorithm>

using namespace std;

#define MAX_LEN 500
#define FIN -100

#ifndef min
#define min(a, b)            (((a) < (b)) ? (a) : (b))
#endif

int counter;
vector <int> random_packets_lost; //all packets that will be lost
int random_generator_seed;
float loss_percent;

struct data_packet {
    /* Header */
    uint16_t cksum; /* Optional bonus part */
    uint16_t len;
    uint32_t seqno;
    time_t sent_time;
    /* Data */
    char data[501]; /* Not always 500 bytes, can be less */
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
    memcpy(packet.data,data,MAX_LEN * sizeof(*data));
    return packet;
}

int get_timeout() {
    return 5;
}
void showlist(vector <int> vec)
{

    for (int i=0;i<vec.size();i++){
        cout << vec[i]<<endl;
    }
}
bool is_found(int seqno){
    for ( int i =0; i<random_packets_lost.size();i++){
        if (random_packets_lost[i] == seqno)
            return true;
    }
    return false;
}

void choose_random_packets(int size) {

    int number_of_losses = ceil(size * loss_percent);
    int pack_to_insert;
    srand(random_generator_seed+1);
    for(int i = 0; i < number_of_losses; i++){
        pack_to_insert = rand() % size;
        random_packets_lost.push_back(pack_to_insert);
    }
    sort(random_packets_lost.begin(),random_packets_lost.end());
    // cout << "random packets size: \n";
    // cout << random_packets_lost.size() <<endl;
    showlist(random_packets_lost);

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

    int packets_number = (fsize / MAX_LEN) + 1;
    choose_random_packets(packets_number);

    while (fsize && fread(buffer, sizeof(char), min(MAX_LEN, fsize), file) > 0){
        struct data_packet packet;
        struct ack_packet ack_pack;
        packet = get_packet(buffer);
        packet.len = min(MAX_LEN, fsize);

        int repeat = 1;
        while (repeat){

            //if not lost : send
            //cout << is_found(packet.seqno) <<endl;
            if (random_packets_lost.size()!= 0 && is_found(packet.seqno)){
                random_packets_lost.erase(random_packets_lost.begin());
            } else {  
                cout << "in send" << endl;
                //cout << packet.data << endl;
                if(sendto(sock_fd, (const void *) &packet, sizeof(struct data_packet), 0,
                    (struct sockaddr *) &client_addr, socklen) == -1)
                    {
                        fprintf(stderr, "sending packet failed.\n");
                    }   
            }
            puts("waiting for ack");

            if(recvfrom(sock_fd, (void *) &ack_pack, sizeof(struct ack_packet),
                    0, (struct sockaddr *) &client_addr, &socklen) == -1)
                {
                    fprintf(stderr, "receiving ACK failed.\n");
                    puts("Packet is lost.. Resending");
                    continue;
                } else 
                    repeat = 0;

            printf("packet # : %d\n", packet.seqno);
            printf("ack # rec : %d\n", ack_pack.seqno);
            if (ack_pack.seqno == packet.seqno + 1){
                printf("Ack #%d received\n", ack_pack.seqno);
                break;
            }
        }
        fsize -= min(fsize, MAX_LEN);
        memset(&buffer, 0, sizeof(buffer));
    }

    data_packet pack;
    pack.seqno = FIN;
    if(sendto(sock_fd, (const void *) &pack, sizeof(struct data_packet), 0,
                    (struct sockaddr *) &client_addr, socklen) == -1){
        
        fprintf(stderr, "sending packet failed.\n");
    }
    cout << "last packet sent" <<endl;

}

void start(int sock_fd){
    int client_len;
    socklen_t socklen = sizeof(struct sockaddr_in);
    struct data_packet packet;
    struct sockaddr_in client_addr;

    while(1) {
        if(recvfrom(sock_fd, (void *) &packet, sizeof(struct data_packet),
                   0, (struct sockaddr *) &client_addr, &socklen) == -1){
                        fprintf(stderr, "recvfrom() failed.\n");
                        return;
                   }

        struct ack_packet ack_pack;
        ack_pack.seqno = packet.seqno + 1;
        ack_pack.sent_time = time(NULL);

        int pid = fork();

        if (pid == 0) {
            char *file_name = packet.data;
            int child_socket_fd = socket(AF_INET,SOCK_DGRAM, 0);

            if(sendto(child_socket_fd, (const void *) &ack_pack, sizeof(struct ack_packet), 0,
                    (struct sockaddr *) &client_addr, socklen) == -1){
                        fprintf(stderr, "sending acK failed failed.\n");
                        return;
            } else {
                printf("Ack # %d sent\n",ack_pack.seqno);
            }

            struct timeval timeout;
            timeout.tv_sec = 3;
            timeout.tv_usec = 0;

            setsockopt(child_socket_fd, SOL_SOCKET,SO_RCVTIMEO, (char *) &timeout, sizeof (timeout));
            //int flags = fcntl(child_socket_fd, F_GETFL);
            //fcntl(child_socket_fd, F_SETFL, flags | O_NONBLOCK);

            stop_and_wait(file_name, client_addr, child_socket_fd);
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

    fscanf(file, "%d", &server_port_no);
    fscanf(file, "%d", &window_size);
    fscanf(file, "%d", &random_generator_seed);
    fscanf(file, "%f", &loss_percent);

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