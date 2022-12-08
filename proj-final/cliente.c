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


/** @brief function that uses popen to execute a bash command and sends its output back to the server.
 *
 *  @param command command which is being executed.
 *  @param sockfd socket identifier.
 */
void sendCommandOutput(char* command, int sockfd){
    FILE *fp = Popen(command, "r");
    char output[MAXDATASIZE] = {0};
    
    while(fgets(output, MAXDATASIZE, fp) != NULL) {
        write(sockfd, output, MAXDATASIZE);
    }
    char eof[MAXDATASIZE] = {1};
    write(sockfd, eof, MAXDATASIZE);
}

void storeMessage(char* message, char* sender, char* filename) {
    FILE *fp;
    time_t clock = time(NULL);

    fp = fopen(filename, "a");
    
    fprintf(fp, "[%.24s] %s: %s\n", ctime(&clock), sender, message);
    fclose(fp);
    return;
}


int main(int argc, char **argv) {
    int    sockfd, peerfd, n;
    char   recvline[MAXLINE + 1];
    struct sockaddr_in servaddr, peeraddr, myaddr;

    assertValidArgs(argc, argv);
    sockfd = Socket(AF_INET, SOCK_STREAM, 0);

    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port   = htons(strtod(argv[2], NULL));
    
    InetPton(AF_INET, argv[1], &servaddr.sin_addr);
    Connect(sockfd, (struct sockaddr *) &servaddr, sizeof(servaddr));
    
    time_t clock = time(NULL);
    printf("%s%.24s - Connected to server \n%s", KGRN, ctime(&clock), KNRM);
    
    if (((n = Read(sockfd, recvline, MAXLINE)) > 0)) {
        recvline[n] = '\0';
            
        printf("Clients connected: \n");
        printf("%s\n", recvline);
        bzero(recvline, MAXDATASIZE);

        printf("Do you want to talk to someone? \n");
        char initiate = '\0';
        fscanf(stdin,"%c", &initiate);

        if (initiate == 'y') {
            printf("Which client do you want to talk to? \n");

            char client[5];
            fscanf(stdin,"%s", client);
            write(sockfd, client, sizeof(client));
        }

        // Wait for server to send peer port
        Read(sockfd, recvline, MAXLINE);
        printf("Starting chat with %s\n", recvline);

        
        // Set peer address and socket
        peerfd = Socket(AF_INET, SOCK_DGRAM, 0);    
        peeraddr.sin_family = AF_INET;
        peeraddr.sin_addr.s_addr = INADDR_ANY;
        peeraddr.sin_port   = htons(atoi(recvline));
        storeMessage("starting chat", "-", recvline);

        char message[MAXDATASIZE];
        unsigned int len = sizeof(peeraddr);

   
        if (initiate == 'y') {
            GetSockName(sockfd, (struct sockaddr *) &myaddr, &len);
            myaddr.sin_family = AF_INET;
            myaddr.sin_addr.s_addr = INADDR_ANY;
            
            if (bind(peerfd, (const struct sockaddr *) &myaddr, len) < 0) {
                fprintf(stdout, "Failed to bind\n");
            }

            recvfrom(peerfd, message, MAXDATASIZE, 0, (struct sockaddr *) &peeraddr, &len);
            fprintf(stdout, "\n%s: %s", recvline, message);
            storeMessage(message, recvline, recvline);
            memset(message, 0, MAXDATASIZE);
        }

        for (;;) {
            fprintf(stdout, "\nMe: ");

            // fgets(message, MAXDATASIZE, stdin); // Reads a whole line instead of a word, but reads a couple of phantom lines... 
            fscanf(stdin, "%s", message);
            sendto(peerfd, message, strlen(message), 0, (const struct sockaddr *) &peeraddr , len);
            storeMessage(message, "me", recvline);
            memset(message, 0, MAXDATASIZE);

            recvfrom(peerfd, message, MAXDATASIZE, 0, (struct sockaddr *) &peeraddr, &len);
            fprintf(stdout, "\n%s: %s", recvline, message);
            storeMessage(message, recvline, recvline);
            memset(message, 0, MAXDATASIZE);

        }

        // while (((n = Read(sockfd, recvline, MAXLINE)) > 0)) {
        //     printf("Starting chat with %s\n", recvline);
        // }


    } 

    exit(0);
}