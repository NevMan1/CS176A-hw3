#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <time.h>

#define PACKET_COUNT 10
#define TIMEOUT_SEC 1
#define BUFFER_SIZE 1024

int main(int argc, char *argv[]) {
    
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <server_ip> <server_port>\n", argv[0]);
        exit(1);
    }
    
    char *server_ip = argv[1];
    int server_port = atoi(argv[2]);
    
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("Socket creation failed");
        exit(1);
    }
    
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(server_port);
    server_addr.sin_addr.s_addr = inet_addr(server_ip);
    
    struct timeval timeout;
    timeout.tv_sec = TIMEOUT_SEC;
    timeout.tv_usec = 0;
    setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
    
    int transmitted = 0, received = 0;
    double min_rtt = 1e6, max_rtt = 0, total_rtt = 0;
    
    for (int i = 1; i <= PACKET_COUNT; i++) {
        char message[BUFFER_SIZE];
        struct timespec start, end;
        
        clock_gettime(CLOCK_MONOTONIC, &start);
        sprintf(message, "PING %d %ld", i, start.tv_sec * 1000 + start.tv_nsec / 1000000);
        
        if (sendto(sockfd, message, strlen(message), 0, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
            perror("Send failed");
            continue;
        }
        transmitted++;
        
        char buffer[BUFFER_SIZE];
        socklen_t addr_len = sizeof(server_addr);
        if (recvfrom(sockfd, buffer, BUFFER_SIZE, 0, (struct sockaddr *)&server_addr, &addr_len) < 0) {
            printf("Request timeout for seq# %d\n", i);
            continue;
        }
        
        clock_gettime(CLOCK_MONOTONIC, &end);
        double rtt = (end.tv_sec - start.tv_sec) * 1000.0 + (end.tv_nsec - start.tv_nsec) / 1000000.0;
        
        printf("PING received from %s: seq#=%d time=%.2f ms\n", server_ip, i, rtt);
        received++;
        total_rtt += rtt;
        if (rtt < min_rtt) min_rtt = rtt;
        if (rtt > max_rtt) max_rtt = rtt;
        
        sleep(1);
    }
    
    double avg_rtt = (received > 0) ? (total_rtt / received) : 0;
    int loss_percentage = 100 - (received * 100 / transmitted);
    
    printf("--- %s ping statistics ---\n", server_ip);
    printf("%d packets transmitted, %d received, %d%% packet loss\n", transmitted, received, loss_percentage);
    if (received > 0)
        printf("rtt min/avg/max = %.2f/%.2f/%.2f ms\n", min_rtt, avg_rtt, max_rtt);
    
    close(sockfd);
    return 0;
}
