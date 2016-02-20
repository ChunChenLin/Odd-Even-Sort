/**
 * Simple FTP Proxy For 2015 Introduction to Computer Network.
 * Author: z58085111 @ HSNL
 * 2015/12
 * **/
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <unistd.h>
#include <errno.h>
#include <dirent.h>
#include <arpa/inet.h>
#include <limits.h>
#define Sample 300

#define MAXSIZE 128
#define FTP_PORT 8740
#define FTP_PASV_CODE 227
#define FTP_ADDR "140.114.71.159"
#define max(X,Y) ((X) > (Y) ? (X) : (Y))

int proxy_IP[4];

int connect_FTP(int ser_port, int clifd);
int proxy_func(int ser_port, int clifd);
int create_server(int port);
void rate_control();
int dRate = INT_MAX, uRate = INT_MAX;
int main (int argc, char **argv) {
    int ctrlfd, connfd, port;
    pid_t childpid;
    socklen_t clilen;
    struct sockaddr_in cliaddr;
    if (argc < 3) {
        printf("[v] Usage: ./executableFile <ProxyIP> <ProxyPort> \n");
        return -1;
    }

    sscanf(argv[1], " %d.%d.%d.%d", &proxy_IP[0], &proxy_IP[1], &proxy_IP[2], &proxy_IP[3]);
    port = atoi(argv[2]);

    printf("Please enter the speed limits. (Entering 0 means no limit).\n");
    printf("Downloading rate. (KB/sec)\n");
    scanf("%d",&dRate);
    printf("Uploading rate. (KB/sec)\n");
    scanf("%d",&uRate);

    if(dRate==0)
        dRate = INT_MAX;
    if(uRate==0)
        uRate = INT_MAX;

    ctrlfd = create_server(port);
    clilen = sizeof(struct sockaddr_in);
    for (;;) {
        connfd = accept(ctrlfd, (struct sockaddr *)&cliaddr, &clilen);
        if (connfd < 0) {
            printf("[x] Accept failed\n");
            return 0;
        }

        printf("[v] Client: %s:%d connect!\n", inet_ntoa(cliaddr.sin_addr), htons(cliaddr.sin_port));
        if ((childpid = fork()) == 0) { //child
            close(ctrlfd);
            proxy_func(FTP_PORT, connfd);
            printf("[v] Client: %s:%d terminated!\n", inet_ntoa(cliaddr.sin_addr), htons(cliaddr.sin_port));
            exit(0);
        }
        close(connfd);
    }
    return 0;
}

int connect_FTP(int ser_port, int clifd) {
    int sockfd;
    char addr[] = FTP_ADDR;
    int byte_num;
    char buffer[MAXSIZE];
    struct sockaddr_in servaddr;

    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        printf("[x] Create socket error");
        return -1;
    }

    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(ser_port);

    if (inet_pton(AF_INET, addr, &servaddr.sin_addr) <= 0) {
        printf("[v] Inet_pton error for %s", addr);
        return -1;
    }

    if (connect(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
        printf("[x] Connect error");
        return -1;
    }

    printf("[v] Connect to FTP server\n");
    if (ser_port == FTP_PORT) {
        if ((byte_num = read(sockfd, buffer, MAXSIZE)) <= 0) {
            printf("[x] Connection establish failed.\n");
        }

        if (write(clifd, buffer, byte_num) < 0) {
            printf("[x] Write to client failed.\n");
            return -1;
        }
    }

    return sockfd;
}

int proxy_func(int ser_port, int clifd) {
    char buffer[MAXSIZE];
    int serfd = -1, datafd = -1, connfd;
    int data_port;
    int byte_num, last_receive=0, last_sent=0;
    int status, pasv[7];
    int childpid;
    socklen_t clilen;
    struct sockaddr_in cliaddr;
    struct timeval dt1,dt2, ut1, ut2;
    double downloadTimeuse = 0.0, uploadTimeuse = 0.0;
    int d=0, u=0 ;

    gettimeofday(&dt1,NULL);
    gettimeofday(&ut1,NULL);


    // select vars
    int maxfdp1;
    int i, nready = 0;
    fd_set rset, allset;

    // connect to FTP server
    if ((serfd = connect_FTP(ser_port, clifd)) < 0) {
        printf("[x] Connect to FTP server failed.\n");
        return -1;
    }

    datafd = serfd;

    // initialize select vars
    FD_ZERO(&allset);
    FD_SET(clifd, &allset);
    FD_SET(serfd, &allset);

    // selecting
    for (;;) {
        // reset select vars
        rset = allset;
        maxfdp1 = max(clifd, serfd) ;

        // select descriptor
        nready = select(maxfdp1 + 1, &rset, NULL, NULL, NULL);
        if (nready > 0) {
            // check FTP client socket fd
            if (FD_ISSET(clifd, &rset)) {
                memset(buffer, 0, MAXSIZE);
                if ((byte_num = read(clifd, buffer, MAXSIZE)) <= 0) {
                    break;
                    printf("[!] Client terminated the connection.\n");
                }

                if (write(serfd, buffer, byte_num) < 0) {
                    printf("[x] Write to server failed.\n");
                    break;
                }
                last_sent+=byte_num;

                if(u==Sample-1)
                {
                    gettimeofday(&ut2,NULL);
                    uploadTimeuse = ut2.tv_sec-ut1.tv_sec+(ut2.tv_usec-ut1.tv_usec)/1000000.0;
                    double CurrentRate = (last_sent)/(1024*uploadTimeuse);
                    double ratio = CurrentRate/uRate;
                    if(ratio>1)
                    {
                         usleep ( (0.99+0.01*(uRate/1000))*1000000* (   ratio  -1 )  *  uploadTimeuse   )       ;
                         last_sent=0;
                         gettimeofday(&ut1,NULL);
                    }
                     else if(ratio<1)
                    {
                         u =Sample*ratio;
                    }
                }
                u = (u+1)%Sample;
            }

            // check FTP server socket fd
            if (FD_ISSET(serfd, &rset)) {
                memset(buffer, 0, MAXSIZE);
                if ((byte_num = read(serfd, buffer, MAXSIZE)) <= 0) {
                    printf("[!] Server terminated the connection.\n");
                    break;
                }
                if(ser_port == FTP_PORT)
                  buffer[byte_num] = '\0';

                status = atoi(buffer);

                if (status == FTP_PASV_CODE && ser_port == FTP_PORT) {

                    sscanf(buffer, "%d Entering Passive Mode (%d,%d,%d,%d,%d,%d)",&pasv[0],&pasv[1],&pasv[2],&pasv[3],&pasv[4],&pasv[5],&pasv[6]);
                    memset(buffer, 0, MAXSIZE);
                    sprintf(buffer, "%d Entering Passive Mode (%d,%d,%d,%d,%d,%d)\n", status, proxy_IP[0], proxy_IP[1], proxy_IP[2], proxy_IP[3], pasv[5], pasv[6]);

                    if ((childpid = fork()) == 0) {
                        data_port = pasv[5] * 256 + pasv[6];
                        datafd = create_server(data_port);
                        printf("[-] Waiting for data connection!\n");
                        clilen = sizeof(struct sockaddr_in);
                        connfd = accept(datafd, (struct sockaddr *)&cliaddr, &clilen);
                        if (connfd < 0) {
                            printf("[x] Accept failed\n");
                            return 0;
                        }

                        printf("[v] Data connection from: %s:%d connect.\n", inet_ntoa(cliaddr.sin_addr), htons(cliaddr.sin_port));
                        proxy_func(data_port, connfd);
                        printf("[!] End of data connection!\n");
                        exit(0);
                    }
                }

                if (write(clifd, buffer, byte_num) < 0) {
                    printf("[x] Write to client failed.\n");
                    break;
                }

                last_receive+=byte_num;

                if(d==Sample-1)
                {
                    gettimeofday(&dt2,NULL);
                    downloadTimeuse  = dt2.tv_sec-dt1.tv_sec+(dt2.tv_usec-dt1.tv_usec)/1000000.0;
                    double CurrentRate = (last_receive)/(1024*downloadTimeuse );
                    double ratio = CurrentRate/dRate;
                    if(ratio>1)
                    {
                         usleep ( 1000000*(   ratio  -1 )  *  downloadTimeuse    )       ;
                         last_receive=0;
                         gettimeofday(&dt1,NULL);
                    }
                    else if(ratio<1)
                    {
                         d = Sample *ratio;
                    }
                }
                d = (d+1)%Sample;

            }
        } else {
            printf("[x] Select() returns -1. ERROR!\n");
            return -1;
        }
    }
    return 0;
}

int create_server(int port) {
    int listenfd;
    struct sockaddr_in servaddr;

    listenfd = socket(AF_INET, SOCK_STREAM, 0);
    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(port);
    if (bind(listenfd, (struct sockaddr *)&servaddr , sizeof(servaddr)) < 0) {
        //print the error message
        perror("bind failed. Error");
        return -1;
    }

    listen(listenfd, 3);
    return listenfd;
}

void rate_control() {
    /**
     * Implement your main logic of rate control here.
     * Add return variable or parameters you need.
     * **/
}
