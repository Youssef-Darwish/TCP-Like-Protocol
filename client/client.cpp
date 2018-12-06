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
#include <string>

#define MAX_LEN 500
#define FIN -100
int counter;
using namespace std ;
struct data_packet {
    /* Header */
    uint16_t cksum; /* Optional bonus part */
    uint16_t len;
    uint32_t seqno;
    time_t sent_time;
    /* Data */
    char data[500]; /* Not always 500 bytes, can be less */
};

string result = "";
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
    packet.len = sizeof(packet.data);
    return packet;
}

void stop_and_wait(char file_name[], int sock_fd, struct sockaddr_in addr_con){

    counter = rand() %10;
    printf("intial counter : %d\n",counter);
    struct data_packet packet = get_packet(file_name);
    char buff[MAX_LEN];
    socklen_t socklen = sizeof(addr_con);
    socklen_t len;
    struct ack_packet ack_pack;
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

    memset (packet.data,0,sizeof(packet.data));
    int flag = 0;

    while (flag != FIN){

        while(recvfrom(sock_fd, (void *) &packet, sizeof(struct data_packet),
                    0, (struct sockaddr *) &addr_con, &len) > 0){
            puts("I am here");
            
            ack_pack.seqno = packet.seqno + 1;
            ack_pack.sent_time = time(NULL);

            flag = packet.seqno;
            if (flag ==FIN)
                break;
            if(sendto(sock_fd, (const void *) &ack_pack, sizeof(struct ack_packet), 0,
            (struct sockaddr *) &addr_con, len) == -1){

                fprintf(stderr, "sending acK failed failed.\n");
                return;
            }
            else {
                printf("Ack # %d sent\n",ack_pack.seqno);
            }
            result += packet.data;
            fwrite(packet.data, sizeof(char), packet.len, file);

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
    timeout.tv_sec = 2;
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