//
// Created by asuna on 2021/4/16.
//

#include "utility.h"

#define error(msg) \
   do  {perror(msg); exit(EXIT_FAILURE); } while(0)

int main (int argc,char *argv[]) {
    int clienfd = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (clienfd < 0) { error("socket error"); }

    struct sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(SERVER_PORT);
    serverAddr.sin_addr.s_addr = inet_addr(SERVER_IP);

    if (connect(clienfd, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0) {
       error("connect error");
    }

    int pipefd[2];
    if (pipe(pipefd) < 0) {
        error("pipe error");
    }

    int epfd = epoll_create(EPOLL_SIZE);
    if (epfd < 0) {
        error("epfd error");
    }

    static struct epoll_event events[2];

    addfd(epfd, clienfd, true);
    addfd(epfd, pipefd[0], true);

    bool isClientwork = true;
    char message[BUF_SIZE];

    int pid = fork();
    if (pid < 0) {
        error("fork error");
    } else if (pid == 0) {
        close(pipefd[0]);
        printf("Please input 'exit' to exit the char room\n");

        while (isClientwork) {
            bzero(&message,BUF_SIZE);
            fgets(message, BUF_SIZE, stdin);

            if (strncasecmp(message,EXIT,strlen(EXIT))== 0){
                isClientwork = 0;
            } else {
                if (write(pipefd[1], message, strlen(message)-1) < 0) {
                    error("fork error");
                }
            }
        }
    } else {
        close(pipefd[1]);
        while (isClientwork) {
            int epoll_events_count = epoll_wait(epfd, events,2, -1);
            for (int i = 0; i < epoll_events_count; ++i) {
                bzero(&message,BUF_SIZE);
               if (events[i].data.fd == clienfd) {
                   int ret = recv(clienfd, message, BUF_SIZE, 0);
                   if (ret == 0) {
                       printf("Server closed connection:%d\n",clienfd);
                       close(clienfd);
                       isClientwork = 0;
                   } else printf("%s\n", message);
               }
               else {
                   int ret = read(events[i].data.fd,message,BUF_SIZE);
                   if (ret == 0) {
                       isClientwork = 0;
                   } else {
                       send(clienfd,message,BUF_SIZE,0);
                   }
               }
            }//for
        }//while
    }
    if (pid) {
        close(pipefd[0]);
        close(clienfd);
    } else {
        close(pipefd[1]);
    }
    return 0;
}