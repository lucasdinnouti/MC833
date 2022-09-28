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
#include <time.h>

#define MAXLINE 4096
#define MAXDATASIZE 100
#define KGRN  "\x1B[32m"
#define KNRM  "\x1B[0m"

/** @brief Reads a port number from input. 
 *
 *  Related to item 4.
 *
 *  @return Port to be used on socket address.
 */
int getPort() {
    int port;

    printf("Enter the port number: ");
    scanf("%d", &port);

    return port;
}

/** @brief Gets address from a given socket using getsockname.
 *
 *  Related to item 5.
 *
 *  @param connfd socket identifier.
 *  @param addrSize size of the returned address.
 *  @return socket address.
 */
struct sockaddr_in getSockName(int sockfd, int addrSize) {
    struct sockaddr_in addr;
    socklen_t len = addrSize;
    if (getsockname(sockfd, (struct sockaddr *) &addr, &len) < 0) {
        perror("getsockname");
        exit(1);
    }
    return addr;
}

/** @brief Sends a message of maximum size MAXDATASIZE to a given open socket connection.
 *
 *  Related to item 7.
 *
 *  @param sockfd socket identifier.
 */
void sendMessage(int sockfd) {
    char message[MAXDATASIZE];
    printf("Enter a message to send to the server: ");
    scanf("%s", message);
    write(sockfd, message, strlen(message));
}

void sendCommandOutput(char* command, int sockfd){
    FILE *fp;
    char output[MAXDATASIZE] = {0};
    
    if ((fp = popen(command, "r")) == NULL) {
        perror("fputs error");
        exit(1);
    }    

    while(fgets(output, MAXDATASIZE, fp) != NULL) {
        // printf("DEBUG: %s", output);
        write(sockfd, output, MAXDATASIZE);
    }

}

int main(int argc, char **argv) {
    int    sockfd, n;
    char   recvline[MAXLINE + 1];
    char   error[MAXLINE + 1];
    struct sockaddr_in servaddr;

    if (argc != 3) {
        strcpy(error,"uso: ");
        strcat(error,argv[0]);
        strcat(error," <IPaddress> <Port>");
        perror(error);
        exit(1);
    }

    if ( (sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket error");
        exit(1);
    }

    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port   = htons(strtod(argv[2], NULL));
    if (inet_pton(AF_INET, argv[1], &servaddr.sin_addr) <= 0) {
        perror("inet_pton error");
        exit(1);
    }

    if (connect(sockfd, (struct sockaddr *) &servaddr, sizeof(servaddr)) < 0) {
        perror("connect error");
        exit(1);
    }
    
    time_t clock = time(NULL);
    printf("%s%.24s - Starting \n%s", KGRN, ctime(&clock), KNRM);
    struct sockaddr_in addr = getSockName(sockfd, sizeof(servaddr));
    printf("Local IP address: %s\n", inet_ntoa(addr.sin_addr));
    printf("Local port      : %d\n", ntohs(addr.sin_port));

    // sendMessage(sockfd);
    
    while ( (n = read(sockfd, recvline, MAXLINE)) > 0) {
        recvline[n] = 0;
        if (fputs(recvline, stdout) == EOF) {
            perror("fputs error");
            exit(1);
        }
        
        sendCommandOutput(recvline, sockfd);
    }
    
    if (n < 0) {
        perror("read error");
        exit(1);
    }

    exit(0);
}