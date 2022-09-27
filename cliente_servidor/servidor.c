#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <time.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <unistd.h>

#define LISTENQ 10
#define MAXDATASIZE 100
#define MAXLINE 4096
#define KGRN  "\x1B[32m"
#define KNRM  "\x1B[0m"

/** @brief Reads a port number from input and transforms it into a network byte. 
 *
 *  Related to item 4.
 *
 *  @return Network byte to be use as socket port.
 */
int getPort() {
    int port;

    printf("Enter the port number: ");
    scanf("%d", &port);

    return htons(port);
}

/** @brief Gets address from a given socket client using getpeername.
 *
 *  Related to item 6.
 *
 *  @param connfd socket identifier.
 *  @param addrSize size of the returned address.
 *  @return socket address.
 */
struct sockaddr_in getSockName(int connfd, int addrSize) {
    struct sockaddr_in addr;
    socklen_t len = addrSize;
    if (getpeername(connfd, (struct sockaddr*)&addr, &len) == -1 ) {
        perror("getpeername");
        exit(1);
    }
    return addr;
}

/** @brief Reads a message of size MAXLINE from a given open socket connection.
 *
 *  Related to item 7.
 *
 *  @param connfd socket identifier.
 */
void receiveMessage(int connfd) {
    char message[MAXLINE + 1] = { 0 };
    read(connfd, message, 1024);
    time_t clock = time(NULL);
    printf("%s%.24s - Message received: %s\n", KGRN, ctime(&clock), message);
}

/** @brief Sleeps for 20 seconds before closing connection
 *
 *  Related to item 2 - ex 2.2
 *
 *  @param connfd socket identifier.
 */
void serverSleep() {
    printf("%sStarting sleep... \n", KNRM);
    sleep(20);
    printf("%sFinishing sleep... \n", KNRM);    
}

int main (int argc, char **argv) {
    int    listenfd, connfd;
    struct sockaddr_in servaddr;
    char   buf[MAXDATASIZE];
    time_t ticks;

    if ((listenfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("socket");
        exit(1);
    }

    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family      = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port        = getPort();   

    if (bind(listenfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) == -1) {
        perror("bind");
        exit(1);
    }

    if (listen(listenfd, LISTENQ) == -1) {
        perror("listen");
        exit(1);
    }

    for ( ; ; ) {
        if ((connfd = accept(listenfd, (struct sockaddr *) NULL, NULL)) == -1 ) {
            perror("accept");
            exit(1);
        }

        ticks = time(NULL);
        printf("%s%.24s - Connection accepted \n", KGRN, ctime(&ticks));
        struct sockaddr_in addr = getSockName(connfd, sizeof(servaddr));
        printf("%sPeer IP address: %s\n", KNRM, inet_ntoa(addr.sin_addr));
        printf("%sPeer port      : %d\n", KNRM, ntohs(addr.sin_port));
        
        receiveMessage(connfd);

        serverSleep();

        ticks = time(NULL);
        snprintf(buf, sizeof(buf), "Hello from server!\nTime: %.24s\r\n", ctime(&ticks));
        write(connfd, buf, strlen(buf));
        printf("%s%.24s - Message sent \n", KGRN, ctime(&ticks));

        close(connfd);
        ticks = time(NULL);
        printf("%s%.24s - Connection closed \n", KGRN, ctime(&ticks));
    }
    return(0);
}