#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <netdb.h>
#include <string.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#define MAXLINE 4096
#define MAXDATASIZE 100

int get_port() {
    int port;

    printf("Enter the port number: ");
    scanf("%d", &port);

    return port;
}

void send_message(sockfd) {
    char message[MAXDATASIZE];
    printf("Enter a message to send to the server: ");
    scanf("%s", message);
    write(sockfd, message, strlen(message));
}

int main(int argc, char **argv) {
    int    sockfd, n;
    char   recvline[MAXLINE + 1];
    char   error[MAXLINE + 1];
    struct sockaddr_in servaddr;

    if (argc != 2) {
        strcpy(error,"uso: ");
        strcat(error,argv[0]);
        strcat(error," <IPaddress>");
        perror(error);
        exit(1);
    }

    if ( (sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket error");
        exit(1);
    }

    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port   = htons(get_port());
    if (inet_pton(AF_INET, argv[1], &servaddr.sin_addr) <= 0) {
        perror("inet_pton error");
        exit(1);
    }

    if (connect(sockfd, (struct sockaddr *) &servaddr, sizeof(servaddr)) < 0) {
        perror("connect error");
        exit(1);
    }
    
    
    struct sockaddr_in addr;
    socklen_t len = sizeof(servaddr);
    int name;
    if ((name  = getsockname(sockfd, (struct sockaddr *) &addr, &len)) < 0) {
        perror("getsockname");
        exit(1);
    }
    printf("Local IP address: %s\n", inet_ntoa(addr.sin_addr));
    printf("Local port      : %d\n", ntohs(addr.sin_port));

    send_message(sockfd);
    
    while ( (n = read(sockfd, recvline, MAXLINE)) > 0) {
        recvline[n] = 0;
        if (fputs(recvline, stdout) == EOF) {
            perror("fputs error");
            exit(1);
        }
    }
    
    if (n < 0) {
        perror("read error");
        exit(1);
    }

    exit(0);
}