//
// Created by asuna on 2021/4/16.
//client
#include "utility.h"

#define error(msg) \
   do { perror(msg); exit(EXIT_FAILURE); } while(0)

int main(int argc, char* argv[]) {
    int listener = socket(PF_INET,SOCK_STREAM, IPPROTO_TCP);
    if (listener < 0) {
        error("socket error");
    }
    printf("listen socket created\n");
    int on = 1;
    if (setsockopt(listener,SOL_SOCKET, SO_REUSEADDR,&on, sizeof(on)) < 0) {
        error("setsockopt");
    }

    struct sockaddr_in serverAddr;
    serverAddr.sin_family = PF_INET;
    serverAddr.sin_port = htons(SERVER_PORT);
    serverAddr.sin_addr.s_addr = inet_addr(SERVER_IP);

    if (bind(listener , (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
        error("build error");
    }

    if (listen(listener,SOMAXCONN) < 0) {
        error("listen error");
    }
    printf("Start to listen: %s\n",SERVER_IP);

    int epfd = epoll_create(EPOLL_SIZE);
    if (epfd < 0) {
        error("epfd error");
    }
    printf("epoll created, epollfd = %d\n,epfd");
    static struct epoll_event events[EPOLL_SIZE];
    addfd(epfd,listener, true);

    while (1) {
        int epoll_events_count = epoll_wait(epfd,events,EPOLL_SIZE,-1);
        if (epoll_events_count < 0) {
            perror("epoll failure");
            break;
        }

        printf("epoll_events_count = %d\n",epoll_events_count);
        for (int i = 0; i < epoll_events_count; ++i) {
            int sockfd = events[i].data.fd;
            if (sockfd == listener) {
                struct sockaddr_in client_address;
                socklen_t client_addrLength = sizeof(struct sockaddr_in);
                int clientfd = accept(listener,(struct sockaddr*)&client_address,&client_addrLength);

                printf("client connection from %s:%d(IP : PORT),clientfd = %d\n",
                        inet_ntoa(client_address.sin_addr),
                        ntohs(client_address.sin_port),
                        clientfd);

                addfd(epfd , clientfd, true);
                clients_list.push_back(clientfd);
                printf("Add new clientfd = %d to epoll\n",clientfd);
                printf("Now there are %d clientd int the chat room\n",(int)clients_list.size());

                printf("welcome message\n");
                char message[BUF_SIZE];
                bzero(message,BUF_SIZE);
                sprintf(message,SERVER_WELCOME,clientfd);

                int ret = send(clientfd,message,BUF_SIZE,0);
                if (ret < 0) {
                    error("send error");
                }
            } else {
                int ret = sendBroadcastmessage(sockfd);
                if (ret < 0) {
                    error ("error");
                }
            }
        }
    }
    close(listener);
    close(epfd);
    return 0;
}
