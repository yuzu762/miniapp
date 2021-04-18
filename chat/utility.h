//
// Created by asuna on 2021/4/16.
//

#ifndef CHAT_UTILITY_H
#define CHAT_UTILITY_H

#include <iostream>
#include <list>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/epoll.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

using namespace std;

list<int> clients_list;
#define SERVER_IP "127.0.0.1"
#define SERVER_PORT 9527
#define EPOLL_SIZE 5000
#define BUF_SIZE 0XFFFF

#define SERVER_WELCOME "Welcome you join to the chat room! Your chat ID isL Client #%d"
#define SERVER_MESSAGE  "ClientID %d say >> %s"

#define EXIT "EXIT"
#define CAUTION "There is only one in the chat room"

int setnooblockint(int sockfd) {
    fcntl(sockfd,F_SETFL,fcntl(sockfd,F_GETFD,0) | O_NONBLOCK);
    return  0;
}

void addfd(int epollfd, int fd, bool enable_et) {
    struct epoll_event ev;
    ev.data.fd = fd;
    ev.events = EPOLLIN;
    if (enable_et) {
        ev.events = EPOLLIN | EPOLLET;
    }
    epoll_ctl(epollfd,EPOLL_CTL_ADD,fd,&ev);
    setnooblockint(fd);
    printf("fd added to epoll");
}

int sendBroadcastmessage(int clientfd) {
    char buf[BUF_SIZE];
    char message[BUF_SIZE];
    bzero(buf,BUF_SIZE);
    bzero(message,BUF_SIZE);

    printf("read from client(clientID = %d)\n",clientfd);

    int len = recv(clientfd,buf,BUF_SIZE,0);

    if (0 == len) {
        close(clientfd);
        clients_list.remove(clientfd);
        printf("clientID = %d closed .\n now there are %d in the char room\n");
    } else {
        if (1 == clients_list.size()) {
           send(clientfd,CAUTION,strlen(CAUTION),0);
        }
        sprintf(message, SERVER_WELCOME, clientfd, buf);
        list<int> ::iterator it;
        for (it = clients_list.begin(); it != clients_list.end(); ++it) {
            if (*it != clientfd ) {
               if (send(*it, buf, len, 0) < 0) {
                   perror("error");
                   exit(-1);
               }
            }
        }
    }
    return len;
}
#endif //CHAT_UTILITY_H
