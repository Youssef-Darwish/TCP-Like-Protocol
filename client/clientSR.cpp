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



#define MAX_LEN 5000
using namespace std;
int counter;

struct data_packet {
    /* Header */
    uint16_t cksum; /* Optional bonus part */
    uint16_t len;
    uint32_t seqno;
    time_t sent_time;
    /* Data */
    char data[5000]; /* Not always 500 bytes, can be less */
};

struct ack_packet {
    /* Header */
    uint16_t cksum; /* Optional bonus part */
    uint16_t len;
    uint32_t seqno;
    time_t sent_time;
};

struct info_packet {
    int packets_to_send;
};
vector <data_packet> packets_to_write;
vector <data_packet> packets_buffer;
/*
send file name
receive ack
while true:
    receive info packet
    receive n packets
    store acks
    send ack
    // empty ack list after timeout

*/


struct data_packet get_packet(char data[]){

    struct data_packet packet;
    packet.seqno = counter;
    counter++;
    packet.sent_time = time(NULL);
    memcpy(packet.data,data,MAX_LEN * sizeof(*data));
    packet.len = sizeof(packet.data);
    return packet;
}

void selective_repeat(char file_name[], int sock_fd, struct sockaddr_in addr_con){

    counter = 0;
    printf("intial counter : %d\n",counter);
    cout << sizeof(*file_name) <<endl;
    struct data_packet packet = get_packet(file_name);
    char buff[MAX_LEN];
    socklen_t socklen = sizeof(addr_con);
    struct ack_packet ack_pack;
    cout << "file  " << packet.data <<endl;
    if(sendto(sock_fd, (const void *) &packet, sizeof(struct data_packet), 0,
             (struct sockaddr *) &addr_con, socklen) == -1)
    {
        fprintf(stderr, "send() failed.\n");
        return;
    }

    
    if(recvfrom(sock_fd, (void *) &ack_pack, sizeof(ack_pack),
            0, (struct sockaddr *) &addr_con, &socklen) == -1){
        fprintf(stderr, "receiving ACK failed.\n");
    }
    printf("ack # rec : %d\n",ack_pack.seqno);
    puts("file name sent\n");


    FILE *file = fopen(file_name, "wb");
    if (file == NULL) {
        fprintf(stderr, "File not found\n");
        return;
    }

    memset (packet.data, 0, sizeof(packet.data));
    int flag = 0;
    info_packet info;
    int expected_seq_num = 0;

    struct timeval timeout;
    
    while (1){

        if(recvfrom(sock_fd, (void *) &info, sizeof(struct info_packet),
            0, (struct sockaddr *) &addr_con, &socklen) == -1){
            
            fprintf(stderr, "receving info failed.\n");
            break;
        }
        else {
            cout << "info received , packets to rec : " << info.packets_to_send <<endl;
        }
        if (packets_to_write.size() == 0 && info.packets_to_send == 0)
            break;

        timeout.tv_sec = 0;
        timeout.tv_usec = 100000;
        setsockopt(sock_fd, SOL_SOCKET,SO_RCVTIMEO, (char * ) &timeout,sizeof (timeout));
        for (int i =0; i < info.packets_to_send; i++){
            
            if(recvfrom(sock_fd, (void *) &packet, sizeof(struct data_packet),
                    0, (struct sockaddr *) &addr_con, &socklen) != -1){

                // puts("I am here");
                cout << "seq received : " << packet.seqno <<endl;
                //checking ack order:
                cout << "expected : " << expected_seq_num <<endl;
                if (packet.seqno == expected_seq_num){
                    puts("entered");
                    packets_to_write.push_back(packet);
                    fwrite(packet.data, sizeof(char), packet.len, file);
                    // fputs(packet.data,file);
                    expected_seq_num++;
                }
            }      
        }

        //incase the buffer of packets is empty
        if (packets_to_write.size()== 0){
            // timeout
            ack_pack.seqno = -1;

        }else {
            ack_pack.seqno = packets_to_write[0].seqno + 1;
        }
            
        if(sendto(sock_fd, (const void *) &ack_pack, sizeof(struct ack_packet), 0,
        (struct sockaddr *) &addr_con, socklen) == -1){

            fprintf(stderr, "sending acK failed failed.\n");
            return;
        }
        else {
            printf("Ack # %d sent\n",ack_pack.seqno);
            
            if (ack_pack.seqno != -1){
                packets_to_write.erase(packets_to_write.begin());
            }
        }
    }
    puts("end");
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
    timeout.tv_sec = 1;
    timeout.tv_usec = 0;
    setsockopt(socket_fd, SOL_SOCKET,SO_RCVTIMEO, (char * ) &timeout,sizeof (timeout));

    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(server_port_no);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    selective_repeat(file_name, socket_fd , server_addr);

    return 0;
}