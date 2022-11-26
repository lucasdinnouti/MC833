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

#define LISTENQ 10
#define N_COMMANDS 4
#define MAXDATASIZE 100
#define MAXLINE 4096
#define KGRN  "\x1B[32m"
#define KNRM  "\x1B[0m"
#define EXIT_KEY_WORD  "EXIT"
#define FILENAME "output.txt"

/** @brief Wrapper function for getpeername: gets socket information.
 *
 *  @param connfd socket identifier.
 *  @param addrSize size of the returned address.
 *  @return socket address.
 */
struct sockaddr_in getPeerName(int connfd, int addrSize) {
    struct sockaddr_in addr;
    socklen_t len = addrSize;
    if (getpeername(connfd, (struct sockaddr*)&addr, &len) == -1 ) {
        perror("getpeername");
        exit(1);
    }
    return addr;
}


/** @brief Sleeps for given seconds before closing connection
 *
 *  Related to item 2.
 *
 *  @param time seconds to sleep.
 */
void serverSleep(int time) {
    printf("%sStarting sleep... \n", KNRM);
    sleep(time);
    printf("%sFinishing sleep... \n", KNRM);    
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

/** @brief Accepts a connection and saves client information to the output file.
 *
 *  @param listenfd socket identifier.
 *  @param servaddr server address.
 *  @return new socket identifier.
 */
int acceptConnection(int listenfd, struct sockaddr_in servaddr) {
    socklen_t len = sizeof(servaddr);
    int connfd = Accept(listenfd, (struct sockaddr *) NULL, &len);

    time_t clock = time(NULL);
    struct sockaddr_in addr = getPeerName(connfd, sizeof(servaddr));

    // Keep for assessment
    // printf("%s%.24s - Connection accepted \n%s", KGRN, ctime(&clock), KNRM);
    // printf("Peer IP address: %s\n", inet_ntoa(addr.sin_addr));
    // printf("Peer port      : %d\n", ntohs(addr.sin_port));
    FILE *fp;
    fp = fopen(FILENAME, "a");
    fprintf(fp, "%.24s - Connection accepted \n", ctime(&clock));
    fprintf(fp, "Peer IP address: %s\n", inet_ntoa(addr.sin_addr));
    fprintf(fp, "Peer port      : %d\n", ntohs(addr.sin_port));
    fclose(fp);
        
    return connfd;
}

/** @brief Reads a command from stdin.
 *
 *  @param command buffer to save input.
 */
void readCommand(char* command) {
    scanf("%s", command);
}

/** @brief Sends a command to a connected client.
 *
 *  @param command buffer to save input.
 *  @param connfd socket identifier.
 *  @param servaddr server address.
 */
void sendConnectedClients(char** clients, int clients_count, int connfd, struct sockaddr_in addr) {
    char buf[MAXDATASIZE];

    for (int i = 0; i < clients_count; i++) {
        snprintf(buf, sizeof(buf), "%s", clients[i]);
    }
    write(connfd, buf, strlen(buf));
}

/** @brief Validate program arguments.
 *
 *  @param argc number of arguments.
 *  @param argv arguments.
 */
void assertValidArgs(int argc, char **argv) {
    char error[MAXLINE + 1];

    if (argc != 3) {
        strcpy(error,"uso: ");
        strcat(error,argv[0]);
        strcat(error,"<Port> <Backlog>\n");
        perror(error);
        exit(1);
    }
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

int main(int argc, char **argv) {
    int    listenfd, connfd;
    struct sockaddr_in servaddr;
    char* clients[MAXDATASIZE] = {};
    int clients_count = 0;

    assertValidArgs(argc, argv);

    listenfd = Socket(AF_INET, SOCK_STREAM, 0);

    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family      = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port        = htons(strtod(argv[1], NULL));

    Bind(listenfd, servaddr, sizeof(servaddr));
    void sig_chld(int);
    Listen(listenfd, atoi(argv[2]));
    Signal(SIGCHLD, sig_chld);
    

    for ( ; ; ) {

        if ((connfd = acceptConnection(listenfd, servaddr)) < 0) {
            if (errno == EINTR) {
                continue; /* se for tratar o sinal, quando voltar dá erro em funções lentas */
            } else {
                perror("accept error");
            }
        }
    
        if (fork() == 0) {
            close(listenfd);

            struct sockaddr_in addr = getPeerName(connfd, sizeof(servaddr));

            char* port = malloc(128);
            snprintf(port, 128, "%u", addr.sin_port);

            clients[clients_count] = port;
            clients_count = clients_count + 1;
            sendConnectedClients(clients, clients_count, connfd, addr);
            
            exit(0);
        }
    }
    return(0);
}