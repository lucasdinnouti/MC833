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
#define EXIT_KEY_WORD  "EXIT"
#define FILENAME "output.txt"

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
void storeCommandOutput(int connfd, struct sockaddr_in servaddr) {
    FILE *fp;
    char output[MAXDATASIZE];
    char eof[MAXDATASIZE] = {1};
    
    fp = fopen(FILENAME, "a");

    time_t clock = time(NULL);
    struct sockaddr_in addr = getSockName(connfd, sizeof(servaddr));
    fprintf(fp, "%.24s - Command output [%s:%d]\n", ctime(&clock), inet_ntoa(addr.sin_addr), ntohs(addr.sin_port));

    while ((read(connfd, output, MAXDATASIZE) > 0) && (strcmp(output, eof))) {

        fprintf(fp, "%s", output);
        // printf("%s | %s %s", KGRN, output, KNRM);
        bzero(output, MAXDATASIZE);
    }
    fflush(fp);
    fclose(fp);
    return;
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

int acceptConnection(int listenfd, struct sockaddr_in servaddr) {
    int connfd;

    if ((connfd = accept(listenfd, (struct sockaddr *) NULL, NULL)) == -1) {
        perror("accept");
        exit(1);
    }

    time_t clock = time(NULL);
    printf("%s%.24s - Connection accepted \n%s", KGRN, ctime(&clock), KNRM);
    struct sockaddr_in addr = getSockName(connfd, sizeof(servaddr));
    printf("Peer IP address: %s\n", inet_ntoa(addr.sin_addr));
    printf("Peer port      : %d\n", ntohs(addr.sin_port));

    return connfd;
}

void readCommand(char* command) {
    printf("Enter a command to execute on client: ");
    scanf("%s", command);
}

void sendCommand(char* command, int connfd) {
    char   buf[MAXDATASIZE];

    snprintf(buf, sizeof(buf), "%s", command);
    write(connfd, buf, strlen(buf));
    time_t clock = time(NULL);
    printf("%s%.24s - Command sent \n %s", KGRN, ctime(&clock), KNRM);
}

int main (int argc, char **argv) {
    int    listenfd, connfd;
    struct sockaddr_in servaddr;
    time_t ticks;
    char   error[MAXLINE + 1];

    if ((listenfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("socket");
        exit(1);
    }

    if (argc != 2) {
        strcpy(error,"uso: ");
        strcat(error,argv[0]);
        strcat(error,"<Port>");
        perror(error);
        exit(1);
    }

    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family      = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port        = htons(strtod(argv[1], NULL));

    if (bind(listenfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) == -1) {
        perror("bind");
        exit(1);
    }

    if (listen(listenfd, LISTENQ) == -1) {
        perror("listen");
        exit(1);
    }
        
    for ( ; ; ) {

        connfd = acceptConnection(listenfd, servaddr);

        char command[MAXDATASIZE] = "";

        
        while (strcmp(command, EXIT_KEY_WORD)) {
            readCommand(command);
            sendCommand(command, connfd);

            if (strcmp(command, EXIT_KEY_WORD) != 0) {
                storeCommandOutput(connfd, servaddr);
            }
        }

        close(connfd);
        ticks = time(NULL);
        printf("%s%.24s - Connection closed \n %s", KGRN, ctime(&ticks), KNRM);
    }
    return(0);
}