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

#define MAXLINE 4096
#define MAXDATASIZE 100
#define KGRN  "\x1B[32m"
#define KNRM  "\x1B[0m"
#define EXIT_KEY_WORD  "EXIT"


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
 */
FILE* Popen(char* command, char* mode) {
    FILE *fp;
    if ((fp = popen(command, mode)) == NULL) {
        perror("popen error");
        exit(1);
    }
    return fp;
}

void sendCommandOutput(char* command, int sockfd){
    FILE *fp = Popen(command, "r");
    char output[MAXDATASIZE] = {0};
    
    while(fgets(output, MAXDATASIZE, fp) != NULL) {
        write(sockfd, output, MAXDATASIZE);
    }
    char eof[MAXDATASIZE] = {1};
    write(sockfd, eof, MAXDATASIZE);
}

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

int Socket(int family, int type, int flags) {
    int sockfd;

    if ((sockfd = socket(family, type, flags)) < 0) {
        perror("socket error");
        exit(1);
    }
    
    return sockfd;
}

void InetPton(int family, char* ip, void * port) {
    if (inet_pton(family, ip, port) <= 0) {
        perror("inet_pton error");
        exit(1);
    }
}

void Connect(int sockfd, struct sockaddr *servaddr, int size) {
    if (connect(sockfd, servaddr, size) < 0) {
        perror("connect error");
        exit(1);
    }
}

int Read(int sockfd, void *buf, size_t count) {
    int n = read(sockfd, buf, count);

    if (n < 0) {
        perror("read error");
        exit(1);
    }

    return n;
}

void printCommand(char* recvline) {
    char* copy = malloc(strlen(recvline) + 1);
    char* c = copy;
    
    strcpy(copy, recvline);
    
    for (int i = strlen(recvline); i > 0; i--) {
        *c = toupper(recvline[i - 1]);
        c++;
    }
    c = '\0';

    printf("Received Command: %s \n", copy);
}

int main(int argc, char **argv) {
    int    sockfd, n;
    char   recvline[MAXLINE + 1];
    struct sockaddr_in servaddr;

    assertValidArgs(argc, argv);
    sockfd = Socket(AF_INET, SOCK_STREAM, 0);

    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port   = htons(strtod(argv[2], NULL));
    
    InetPton(AF_INET, argv[1], &servaddr.sin_addr);
    Connect(sockfd, (struct sockaddr *) &servaddr, sizeof(servaddr));
    
    time_t clock = time(NULL);
    printf("%s%.24s - Starting \n%s", KGRN, ctime(&clock), KNRM);

    struct sockaddr_in addr;
    socklen_t len = sizeof(servaddr);
    GetSockName(sockfd, (struct sockaddr *) &addr, &len);
    printf("Local IP address: %s\n", inet_ntoa(addr.sin_addr));
    printf("Local port      : %d\n", ntohs(addr.sin_port));
    
    while (((n = Read(sockfd, recvline, MAXLINE)) > 0)) {
        recvline[n] = '\0';

        if (strcmp(recvline, EXIT_KEY_WORD) == 0) {
            close(sockfd);
            break;
        }

        printCommand(recvline);
        sendCommandOutput(recvline, sockfd);
        bzero(recvline, MAXDATASIZE);
    } 

    exit(0);
}