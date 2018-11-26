// server code for UDP socket programming 
#include <arpa/inet.h> 
#include <netinet/in.h> 
#include <stdio.h> 
#include <stdlib.h> 
#include <string.h> 
#include <sys/socket.h> 
#include <sys/types.h> 
#include <unistd.h> 
  
#define IP_PROTOCOL 0 
#define PORT_NO 15050 
#define NET_BUF_SIZE 500 
#define sendrecvflag 0 
#define nofile "File Not Found!" 
  
int counter;  
int header_size;

struct data_packet get_packet(char data[]){

    struct data_packet packet;
    packet.seqno = counter;
    counter++;
    packet.sent_time = time(NULL);
    strcpy(packet.data, data);
    packet.len = sizeof(packet.data);
    return packet;
}





struct data_packet {
/* Header */
uint16_t cksum; /* Optional bonus part */
uint16_t len;
uint32_t seqno;
time_t sent_time;
/* Data */
char data[500]; /* Not always 500 bytes, can be less */
};


/* Ack-only packets are only 8 bytes */
struct ack_packet {
    uint16_t cksum; /* Optional bonus part */
    uint16_t len;
    uint32_t ackno;
    uint32_t seqno;

};

// funtion to clear buffer 
void clearBuf(char* b) 
{ 
    int i; 
    for (i = 0; i < NET_BUF_SIZE; i++) 
        b[i] = '\0'; 
} 
  
// funtion to encrypt 
  
// funtion sending file 
int sendFile(FILE* fp, char* buf, int s) 
{ 
    int i, len; 
    if (fp == NULL) { 
        strcpy(buf, nofile); 
        len = strlen(nofile); 
        buf[len] = EOF; 
        return 1; 
    } 
  
    char ch, ch2; 
    for (i = 0; i < s; i++) { 
        ch = fgetc(fp); 
        buf[i] = ch; 
        if (ch == EOF) 
            return 1; 
    } 
    return 0; 
} 


  
// driver code 
int main() 
{ 
    int sockfd, nBytes; 
    struct data_packet packet;
    struct ack_packet ack;

    struct sockaddr_in addr_con; 
    int addrlen = sizeof(addr_con); 
    addr_con.sin_family = AF_INET; 
    addr_con.sin_port = htons(PORT_NO); 
    addr_con.sin_addr.s_addr = INADDR_ANY; 
    char net_buf[sizeof(packet.data)]; 
    FILE* fp; 
    printf("size : %d",sizeof(packet.data));
    // socket() 
    sockfd = socket(AF_INET, SOCK_DGRAM, IP_PROTOCOL); 
  
    if (sockfd < 0) 
        printf("\nfile descriptor not received!!\n"); 
    else
        printf("\nfile descriptor %d received\n", sockfd); 
  
    // bind() 
    if (bind(sockfd, (struct sockaddr*)&addr_con, sizeof(addr_con)) == 0) 
        printf("\nSuccessfully binded!\n"); 
    else
        printf("\nBinding Failed!\n"); 
  
    while (1) { 
        printf("\nWaiting for file name...\n"); 

       
        // receive file name 
        clearBuf(net_buf); 
  
        nBytes = recvfrom(sockfd, net_buf, 
                          NET_BUF_SIZE, sendrecvflag, 
                          (struct sockaddr*)&addr_con, &addrlen); 
        
        //printf("%s      ",net_buf+sizeof(packet));
        memcpy(&packet.seqno,net_buf ,sizeof(int));
        memcpy(&packet.sent_time,net_buf +sizeof(int),sizeof(int));
        memcpy(&packet.data,net_buf + 2*sizeof (int)
        ,sizeof(packet.data));

        printf(" Seq Num : %d\n",packet.seqno);
        printf(" Time : %d\n",packet.sent_time);
        printf("data : %s ",packet.data);

        printf("File Name Received\n");        

        fp = fopen(packet.data, "r"); 
        printf("\nFile Name Received: %s\n", packet.data); 



        if (fp == NULL) 
            printf("\nFile open failed!\n"); 
        else
            printf("\nFile Successfully opened!\n"); 
  
        int seq_num = 0;
        while (1) { 
            seq_num++;
            //start sending
            struct data_packet my_packet;
            char buff_to_send[sizeof(packet)]; 
            my_packet.sent_time = time(NULL);
            my_packet.seqno = seq_num;
            memcpy(buff_to_send,&my_packet.seqno,sizeof(int));
            memcpy(buff_to_send + sizeof (int)
                ,&my_packet.sent_time,sizeof(int));
            
            // process 
            if (sendFile(fp, net_buf, NET_BUF_SIZE)) { 
                sendto(sockfd, net_buf, NET_BUF_SIZE, 
                       sendrecvflag,  
                    (struct sockaddr*)&addr_con, addrlen); 
                break; 
            } 

            // send 
            memcpy(buff_to_send + 2 * sizeof(int),net_buf,NET_BUF_SIZE);
            sendto(sockfd, buff_to_send, sizeof (buff_to_send), 
                   sendrecvflag, 
                (struct sockaddr*)&addr_con, addrlen); 
            clearBuf(net_buf);
            clearBuf(buff_to_send); 
        } 
        if (fp != NULL) 
            fclose(fp); 
        
    } 
    return 0; 
} 



/*

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

*/