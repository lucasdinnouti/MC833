#include <stdio.h>
#include <stdlib.h>

#include <arpa/inet.h>
#include <errno.h>
#include <netdb.h>
#include <netinet/in.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>
#include <unistd.h>
#include <signal.h>

#define MAXDATASIZE 100
#define MAXLINE 4096
#define KGRN  "\x1B[32m"
#define KNRM  "\x1B[0m"

// WRAPPER FUNCTIONS

/** @brief Wrapper function for getpeername: gets socket information.
 *
 *  @param connfd socket identifier.
 *  @param addrSize size of the returned address.
 *  @return socket address.
 */
struct sockaddr_in GetPeerName(int connfd, int addrSize) {
    struct sockaddr_in addr;
    socklen_t len = addrSize;
    if (getpeername(connfd, (struct sockaddr*)&addr, &len) == -1 ) {
        perror("getpeername");
        exit(1);
    }
    return addr;
}

/** @brief Wrapper function for accept: accepts a client connection.
 *
 *  @param listenfd socket identifier.
 *  @param servaddr server address.
 *  @param addrlen server address size.
 *  @return new socket identifier.
 */
int Accept(int listenfd, struct sockaddr* addr, unsigned int* addrlen) {
    int connfd;
    
    if ((connfd = accept(listenfd, addr, addrlen)) == -1) {
        perror("accept");
        exit(1);
    }

    return connfd;
}

/** @brief Wrapper function for socket: Creates a socket.
 *
 *  @param family address family.
 *  @param type socket type.
 *  @param flags family flags.
 *  @return new socket identifier.
 */
int Socket(int family, int type, int flags) {
    int listenfd;

    if ((listenfd = socket(family, type, flags)) == -1) {
        perror("socket");
        exit(1);
    }
    
    return listenfd;
}

/** @brief Wrapper function for bind: connects a socket to an address.
 *
 *  @param listenfd socket identifier.
 *  @param servaddr server address.
 *  @param size server address size.
 */
void Bind(int listenfd, struct sockaddr_in servaddr, int size) {
    if (bind(listenfd, (struct sockaddr *)&servaddr, size) == -1) {
        perror("socket");
        exit(1);
    }
}

/** @brief Wrapper function for listen: Makes a socket passive.
 *
 *  @param listenfd socket identifier.
 *  @param backlog connection buffer size.
 */
void Listen(int listenfd, int backlog) {
    if (listen(listenfd, backlog) == -1) {
        perror("listen");
        exit(1);
    }
}

/** @brief Section copied from the slides, regarding SIGCHLD handling.
 */
typedef void Sigfunc(int);

Sigfunc* Signal (int signo, Sigfunc *func) {
    struct sigaction act, oact;

    act.sa_handler = func;
    
    sigemptyset (&act.sa_mask); /* Outros sinais não são bloqueados*/
    
    act.sa_flags = 0;
    
    if (signo == SIGALRM) { /* Para reiniciar chamadas interrompidas */
        #ifdef SA_INTERRUPT
        act.sa_flags |= SA_INTERRUPT; /* SunOS 4.x */
        #endif
    } else {
        #ifdef SA_RESTART
        act.sa_flags |= SA_RESTART; /* SVR4, 4.4BSD */
        #endif
    }

    if (sigaction (signo, &act, &oact) < 0) {
        return (SIG_ERR);
    }
    
    return (oact.sa_handler);
}

void sig_chld(int signo) {
	pid_t pid;
	int stat;

	while ((pid = waitpid(-1, &stat, WNOHANG)) > 0) {
		printf("child %d terminated\n", pid);
    }

	return;
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

/** @brief Sends a client addr port to another client.
 *
 *  @param sourceConn the connection that will have the addr port sent.
 *  @param destConn the connection that will receive the addr port.
 *  @param servaddr the addr of this server.
 */
void notifyClient(int sourceConn, int destConn, struct sockaddr_in servaddr) {
    char buf[MAXDATASIZE];

    struct sockaddr_in addr = GetPeerName(sourceConn, sizeof(servaddr));

    snprintf(buf, sizeof(buf), "%d",  ntohs(addr.sin_port));
    printf("\nSending %d to %d", ntohs(addr.sin_port), destConn);
    write(destConn, buf, sizeof(buf));
}

/** @brief Sends the list of connected clients to a connected client.
 *
 *  @param clients the list of connected clients.
 *  @param clients_count size of conneted clients list.
 *  @param connfd socket identifier.
 */
void sendConnectedClients(int* clients, int clients_count, int connfd) {
    char buf[MAXDATASIZE];

    if (clients_count == 0) {
        snprintf(buf, sizeof(buf), "No clients connected");
        write(connfd, buf, strlen(buf));
    } else {
        for (int i = 0; i < clients_count; i++) {
            snprintf(buf, sizeof(buf), "%d\n",  clients[i]);
            write(connfd, buf, strlen(buf));
        }
    }
}

/** @brief Validate program arguments.
 *
 *  @param argc number of arguments.
 *  @param argv arguments.
 */
void assertValidArgs(int argc, char **argv) {
    char error[MAXLINE + 1];

    if (argc != 2) {
        strcpy(error,"uso: ");
        strcat(error,argv[0]);
        strcat(error,"<Port>\n");
        perror(error);
        exit(1);
    }
}

int main(int argc, char **argv) {
    int    listenfd, connfd, n;
    struct sockaddr_in servaddr;
    char   recvline[MAXLINE + 1];
    int clients[MAXDATASIZE] = {};
    int clients_count = 0;
    socklen_t len = sizeof(servaddr);

    assertValidArgs(argc, argv);

    listenfd = Socket(AF_INET, SOCK_STREAM, 0);

    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family      = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port        = htons(strtod(argv[1], NULL));

    Bind(listenfd, servaddr, sizeof(servaddr));
    void sig_chld(int);
    Listen(listenfd, 2);
    Signal(SIGCHLD, sig_chld);
    

    for ( ; ; ) {

        if ((connfd = Accept(listenfd, (struct sockaddr *) NULL, &len)) < 0) {
            if (errno == EINTR) {
                continue; /* se for tratar o sinal, quando voltar dá erro em funções lentas */
            } else {
                perror("accept error");
            }
        }
    
        time_t clock = time(NULL);
        printf("%s%.24s - Client connected: %d \n%s", KGRN, ctime(&clock), connfd, KNRM);

        sendConnectedClients(clients, clients_count, connfd);

        clients[clients_count] = connfd;
        clients_count = clients_count + 1;

        if (fork() == 0) {
            close(listenfd);

            while (((n = Read(connfd, recvline, MAXLINE)) > 0)) {
                recvline[n] = '\0';

                if (strcmp(recvline, "finalizar_chat") == 0) {
                    fprintf(stdout, "\n%d ended the conversation.", connfd);
                    fflush(stdout);
                } else {
                    printf("\nClient %d wants to talk to %s\n", connfd, recvline);
                    
                    notifyClient(connfd, atoi(recvline), servaddr);
                    notifyClient(atoi(recvline), connfd, servaddr);
                    
                    bzero(recvline, MAXDATASIZE);
                }

            }
        }
    }
    return(0);
}