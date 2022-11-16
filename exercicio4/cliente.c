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
#include <ctype.h>

#include "unp.h"

#define MAXLINE 4096
#define MAXDATASIZE 100
#define SERVER_SETSIZE 2
#define KGRN  "\x1B[32m"
#define KNRM  "\x1B[0m"
#define EXIT_KEY_WORD  "EXIT"

// WRAPPER FUNCTIONS

/** @brief Wrapper function for getsockname: gets socket information.
 *
 *  @param sockfd socket identifier.
 *  @param addr address for which the information will be retrieved.
 *  @param addrlen size of the address.
 */
void GetSockName(int sockfd, struct sockaddr *addr, socklen_t * addrlen) {
    if (getsockname(sockfd, addr, addrlen) < 0) {
        perror("getsockname");
        exit(1);
    }
}

/** @brief Wrapper function for popen: executes bash commands.
 *
 *  @param command command which is being executed.
 *  @param mode execution mode for command.
 *  @return file pointer to the command's execution output.
 */
FILE* Popen(char* command, char* mode) {
    FILE *fp;
    if ((fp = popen(command, mode)) == NULL) {
        perror("popen error");
        exit(1);
    }
    return fp;
}


/** @brief Wrapper function for socket: creates socket given configurations.
 *
 *  @param family address family. For example members of AF_INET address family are IPv4 addresses.
 *  @param type defines the communication semantics, if it is datagram based, two-way, etc.
 *  @param flags extra information for the socket behaviour.
 *  @return identification of the created socket.
 */
int Socket(int family, int type, int flags) {
    int sockfd;

    if ((sockfd = socket(family, type, flags)) < 0) {
        perror("socket error");
        exit(1);
    }
    
    return sockfd;
}

 /** @brief Wrapper function for inet_pton: transforms ip and port from text form to binary form.
 *
 *  @param family address family. For example members of AF_INET address family are IPv4 addresses
 *  @param ip ip being transformed 
 *  @param port port being transformed 
 */
void InetPton(int family, char* ip, void * port) {
    if (inet_pton(family, ip, port) <= 0) {
        perror("inet_pton error");
        exit(1);
    }
}

/** @brief Wrapper function for connect: creates socket given configurations.
 *
 *  @param sockfd socket identifier.
 *  @param servaddr address of server to connect.
 *  @param addrlen size of the address.
 */
void Connect(int sockfd, struct sockaddr *servaddr, int addrlen) {
    if (connect(sockfd, servaddr, addrlen) < 0) {
        perror("connect error");
        exit(1);
    }
}


/** @brief Wrapper function for read: reads information from socket.
 *
 *  @param sockfd socket identifier.
 *  @param buf buffer where the information read from the socket will be written.
 *  @param count how many bytes will be read at most.
 *  @return size of read information.
 */
int Read(int sockfd, void *buf, size_t count) {
    int n = read(sockfd, buf, count);

    if (n < 0) {
        perror("read error");
        exit(1);
    }

    return n;
}


// HELPER FUNCTIONS

/** @brief validates the number of parameters, suggesting the correct usage in case of error.
 *
 *  @param command command which is being executed.
 *  @param sockfd socket identifier.
 */
void assertValidArgs(int argc, char **argv) {
    char   error[MAXLINE + 1];

    if (argc != 3) {
        strcpy(error,"uso: ");
        strcat(error,argv[0]);
        strcat(error," <IPaddress> <Port>");
        perror(error);
        exit(1);
    }
}

int main(int argc, char **argv) {
    int i, tmp, maxi, maxfd, connfd, sockfd;
    int nready, server[SERVER_SETSIZE] = {-1};
    
    ssize_t n;
    
    fd_set rset, allset;
    
    char buf[MAXLINE];
    char recvline[MAXLINE + 1];
    
    struct sockaddr_in servaddr;
 
    maxfd = connfd = 0; /* initialize */
    maxi = -1; /* index into server[] array */
    
    FD_ZERO(&allset);
    FD_SET(sockfd, &allset);

    for ( ; ; ) {
        
        rset = allset; /* structure assignment */
        nready = select(maxfd + 1, &rset, NULL, NULL, NULL);
        
        if (FD_ISSET(sockfd, &rset)) { /* new server connection */
            // START OF SERVER CONNECTION CODE

            sockfd = Socket(AF_INET, SOCK_STREAM, 0);

            bzero(&servaddr, sizeof(servaddr));
            servaddr.sin_family = AF_INET;
            servaddr.sin_port   = htons(strtod(argv[2], NULL));
            
            InetPton(AF_INET, argv[1], &servaddr.sin_addr);
            Connect(sockfd, (struct sockaddr *) &servaddr, sizeof(servaddr));
            
            time_t clock = time(NULL);
            printf("%s%.24s - Starting \n%s", KGRN, ctime(&clock), KNRM);

            // struct sockaddr_in addr;
            // socklen_t len = sizeof(servaddr);
            // GetSockName(sockfd, (struct sockaddr *) &addr, &len);
            // printf("Local IP address: %s\n", inet_ntoa(addr.sin_addr));
            // printf("Local port      : %d\n", ntohs(addr.sin_port));

            // Read Hello from server
            Read(sockfd, recvline, MAXLINE);
            printf("%s\n", recvline);

            // END OF SERVER CONNECTION CODE
                        
            for (i = 0; i < SERVER_SETSIZE; i++)             
                if (server[i] < 0) {
                    server[i] = connfd; /* save descriptor */
                    break;
                }
            
            if (i == SERVER_SETSIZE)
                perror("too many servers");
            
            FD_SET(connfd, &allset); /* add new descriptor to set */
            
            if (connfd > maxfd)
                maxfd = connfd; /* for select */
            
            if (i > maxi)
                maxi = i; /* max index in server[] array */
            
            if (--nready <= 0)
                continue; /* no more readable descriptors */
        }

        for (i = 0; i <= maxi; i++) { /* check all servers for data */
            if ((sockfd = server[i]) < 0) {
                continue;                
            }

            if (FD_ISSET(sockfd, &rset)) {
                if ( (n = Read(sockfd, buf, MAXLINE)) == 0) {
                    /* connection closed by server */
                    close(sockfd);
                    FD_CLR(sockfd, &allset);
                    server[i] = -1;
                } else {
                    write(sockfd, buf, n);
                    
                    if (--nready <= 0)
                        break; /* no more readable descriptors */
                }
            }
        }
    }

    /////////////////////////
    
    while(fgets(recvline, MAXDATASIZE, stdin) > 0) {
        // Send string to server
        write(sockfd, recvline, MAXDATASIZE);

        // Receive echo from server
        tmp = Read(sockfd, recvline, MAXLINE);
        recvline[tmp] = '\0';

        // Print echo received
        printf("%s", recvline);
    }

    exit(0);
}