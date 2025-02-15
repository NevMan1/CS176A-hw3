#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <time.h>

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <server_ip> <server_port>\n", argv[0]);
        exit(1);
    }
    
    char *server_ip = argv[1];
    int server_port = atoi(argv[2]);
    int BUFFER_SIZE = 1024;
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
    timeout.tv_sec = 1;
    timeout.tv_usec = 0;
    setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
    
    int transmitted = 0, received = 0;
    double min_rtt = 1e6, max_rtt = 0, total_rtt = 0;
    
    for (int i = 1; i <= 10; i++) {
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
        socklen_t len = sizeof(server_addr);
        if (recvfrom(sockfd, buffer, BUFFER_SIZE, 0, (struct sockaddr *)&server_addr, &len) < 0) {
            printf("Request timeout for seq#=%d\n", i);
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
    
    
    min_rtt = ((int)(min_rtt * 1000)) / 1000.0;
    avg_rtt = ((int)(avg_rtt * 1000)) / 1000.0;
    max_rtt = ((int)(max_rtt * 1000)) / 1000.0;

    printf("--- %s ping statistics ---\n", server_ip);
    
    if (received == 0) {
        printf("10 packets transmitted, %d received, %d%% packet loss\n", 
            transmitted, loss_percentage);
    } else {
        printf("10 packets transmitted, %d received, %d%% packet loss rtt min/avg/max = %.3f %.3f %.3f ms\n",
             received, loss_percentage, min_rtt, avg_rtt, max_rtt);
    }
    
    close(sockfd);
    return 0;
}
