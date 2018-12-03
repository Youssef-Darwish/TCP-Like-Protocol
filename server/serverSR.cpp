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
#define THRESHOLD 8

#ifndef min
#define min(a, b)            (((a) < (b)) ? (a) : (b))
#endif

int counter; //used to assign sequence number
int cwnd; //window size
float loss_percent;
int random_generator_seed;
vector <int> number_of_packets_without_loss; //N0 N1 N2 ...
vector <int> random_packets_lost; //all packets that will be lost

struct data_packet {
    /* Header */
    uint16_t cksum; /* Optional bonus part */
    uint16_t len;
    uint32_t seqno;
    time_t sent_time;
    /* Data */
    char data[501]; /* Not always 500 bytes, can be less */
};

struct info_packet {
    int packets_to_send;
};

vector <data_packet> packets; //all packets read from file

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

void showlist(vector <int> vec)
{

    for (int i=0;i<vec.size();i++){
        cout << vec[i]<<endl;
    }
}

void choose_random_packets() {
    int size = packets.size();
    int number_of_losses = ceil(size * loss_percent);
    int pack_to_insert;
    srand(random_generator_seed);
    for(int i = 0; i < number_of_losses; i++){
        pack_to_insert = rand() % size;
        random_packets_lost.push_back(pack_to_insert);
    }
    sort(random_packets_lost.begin(),random_packets_lost.end());
    showlist(random_packets_lost);

}

void setup_lists(string file_name){
    char buffer[MAX_LEN];
    FILE *file = fopen(file_name.c_str(), "r");
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

        packets.push_back(packet);

        fsize -= min(fsize, MAX_LEN);
        memset(&buffer, 0, sizeof(buffer));
    }

    choose_random_packets();
    file_name = "threeDupAcks.txt";     // TODO : read_it from input file
    fstream input(file_name.c_str());
    int num_of_packets;
    while(input >> num_of_packets){
        number_of_packets_without_loss.push_back(num_of_packets);
    }

}

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

void selective_repeat(struct sockaddr_in client_addr, int sock_fd, socklen_t socklen) {

    int ee = packets.size();
    int number_of_acks = 0;
    // get N0
    int max_packets_without_loss = number_of_packets_without_loss[0];
    number_of_packets_without_loss.erase(number_of_packets_without_loss.begin());

    info_packet info;
    while(!packets.empty()){
        //it should be cwnd - number of unacked
        info.packets_to_send = cwnd - number_of_acks;
        if(sendto(sock_fd, (const void *) &info, sizeof(struct info_packet), 0,
            (struct sockaddr *) &client_addr, socklen) == -1){
                fprintf(stderr, "sending info failed failed.\n");
                return;
        } else {
            printf("info sent , packets to sent : # %d sent\n",info.packets_to_send);
        }

        for (int index = 0; index < cwnd - number_of_acks; index++) {
            packets[index].sent_time = time(NULL);
            data_packet pack = packets[index];

            // If packet is in random lost packets, don't send
            if(find(random_packets_lost.begin,random_packets_lost.end, pack.seqno)){
                continue;
            }
            if(sendto(sock_fd, (const void *) &pack, sizeof(struct data_packet), 0,
            (struct sockaddr *) &client_addr, socklen) == -1){
                fprintf(stderr, "sending packet failed .\n");
                return;
            } else {
                printf("Packet # %d sent\n",pack.seqno);
            }
        }

        struct ack_packet ack_pack;
        if(recvfrom(sock_fd, (void *) &ack_pack, sizeof(struct ack_packet),
            0, (struct sockaddr *) &client_addr, &socklen) == -1)
        {
            fprintf(stderr, "receiving ACK failed.\n");
        }
        else {
            // if acked packet is the base , remove it from window
            if(ack_pack.seqno == packets[0].seqno + 1){
                packets.erase(packets.begin());
            }
        }
        number_of_acks++;

        //TODO , simulate timeout given in file by N0,N1 ...etc
        if (number_of_acks > max_packets_without_loss){
            set_window_size(4);
            number_of_packets_without_loss.erase(number_of_packets_without_loss.begin());
            max_packets_without_loss = number_of_packets_without_loss[0];
        }

        // check timeouts : default for now : 5 sec
        if (find(random_packets_lost.begin,random_packets_lost.end, packets[0].seqno) && time (NULL) - packets[0].sent_time > 5){
            set_window_size(3);
            random_packets_lost.erase(random_packets_lost.begin());
            number_of_acks = 0;
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