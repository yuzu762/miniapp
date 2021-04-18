#include <stdio.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

#define MAX_EVENTS 1024
#define BUFLEN 4096
#define SERV_PORT 6666

void recv_data(int fd, int events, void *arg);
void send_data(int fd, int events, void *arg);

struct myevent_s
{
    int    fd;
    int    events;
    void   *arg;
    void   (*call_back)(int fd, int events, void *arg);
    int    status;
    char   buf[BUFSIZ];
    int    len;
    long   last_active;
};

int g_efd;
struct myevent_s g_events[MAX_EVENTS+1];

void eventset(struct myevent_s *ev, int fd, void (*call_back)(int fd,int events, void *arg), void *arg)
{
    ev->fd = fd;
    ev->call_back = call_back;
    ev->events = 0;
    ev->arg = arg;
    ev->status = 0;
    if (ev->len <= 0) {
        memset(ev->buf, 0, sizeof (ev->buf));
        ev->len = 0;
    }
    ev->last_active = time(NULL);
    return;
}

void eventadd(int efd, int events, struct myevent_s *ev)
{
    struct epoll_event epv = {0,{0}};
    int op = 0;
    epv.data.ptr = ev;
    epv.events = ev->events = events;

    if (ev->status == 0) {
        op = EPOLL_CTL_ADD;
        ev->status = 1;
    }

    if(epoll_ctl(efd, op, ev->fd,&epv) < 0)
        printf("event add failed [fd = %d],events[%d]\n",ev->fd,events);
    else
        printf("event add OK [fd = %d],events[%OX]\n",ev->fd,events);
    return;
}

void eventdel(int efd, struct myevent_s *ev)
{
    struct epoll_event epv = {0,{0}};
    if(ev->status != 1)
        return;
    epv.data.ptr = NULL;
    ev->status = 0;
    epoll_ctl(efd,EPOLL_CTL_DEL, ev->fd, &epv);
    return;
}

void  accept_conn(int lfd, int events, void *arg) {
    struct sockaddr_in cin;
    socklen_t len = sizeof(cin);
    int cfd, i;
    if((cfd = accept(lfd,(struct sockaddr*)&cin,&len)) == -1)
    {
        if(errno != EAGAIN && errno != EINTR)
        {
            sleep(1);
        }
        printf("%s:accept,%s\n",__func__,strerror(errno));
        return;
    }
    do
    {
      for(i = 0; i < MAX_EVENTS; ++i) {
          if(g_events[i].status == 0)
              break;
      }
      if(i == MAX_EVENTS)
      {
          printf("%s: max connect limit[%d]\n",__func__,MAX_EVENTS);
          break;
      }
      int flag = 0;
      if ((flag = fcntl(cfd, F_SETFL,O_NONBLOCK)) < 0) {
          printf("%s: fcntl noblocking failed,%s\n",__func__,strerror(errno));
          break;
      }
      eventset(&g_events[i], cfd, recv_data,&g_events[i]);
      eventadd(g_efd, EPOLLIN, &g_events[i]);
    }while (0);

    printf("new connect[%s,%d],[time:%ld],pos[%d]\n",inet_ntoa(cin.sin_addr),
            ntohs(cin.sin_port), g_events[i].last_active, i);

    return;
}

void recv_data(int fd, int events, void *arg) {
    struct myevent_s *ev = (struct myevent_s *)arg;
    int len;

    len = recv(fd, ev->buf, sizeof(ev->buf), 0);
//   eventdel(g_efd, ev);
    if (len > 0)
    {
        ev->len = len;
        ev->buf[len] = '\0';

        printf("C[%d]:%s\n",fd,ev->buf);
        eventset(ev,fd,send_data,ev->arg);
//        ev->events = EPOLLOUT;
//        int res =epoll_ctl(g_efd,EPOLL_CTL_MOD, ev->fd, ev);
//        if (res < 0) {
//            perror("epoll");
//            exit(-1);
//        }
        eventadd(g_efd,EPOLLOUT,ev);
    }
    else if (len == 0)
    {
      close(ev->fd);
      printf("[fd=%d] pos[%ld],closed\n",fd,ev-g_events);
    }
    else
    {
        close(ev->fd);
        printf("recv[fd=%d] error[%d]:%s\n",fd,errno,strerror(errno));
    }
    return;
}

void send_data(int fd,int events, void *arg) {
    struct myevent_s *ev = (struct myevent_s *)arg;
    printf("bbbbbbccccc\n");
    int len;
    len = send(fd, ev->buf, ev->len, 0);
    eventdel(g_efd, ev);
    printf("bbbbbb\n");
    if (len > 0) {
        printf("send[fd=%d],[%d]%s\n",fd, len, ev->buf);
        eventset(ev,fd,recv_data,ev->arg);
        eventadd(g_efd, EPOLLIN, ev);
//        epoll_ctl(g_efd,EPOLL_CTL_MOD, ev->fd, ev);
    }
}

void initlistensocket(int efd, short port)
{
    struct sockaddr_in sin;
    int lfd = socket(AF_INET,SOCK_STREAM,0);
    fcntl(lfd,F_SETFL,O_NONBLOCK);

    memset(&sin,0, sizeof(sin));
    sin.sin_family = AF_INET;
    sin.sin_addr.s_addr = INADDR_ANY;
    sin.sin_port = htons(port);

    bind(lfd,(struct sockaddr*)&sin, sizeof(sin));
    listen(lfd,20);

    eventset(&g_events[MAX_EVENTS],lfd,accept_conn,&g_events[MAX_EVENTS]);

    eventadd(efd,EPOLLIN,&g_events[MAX_EVENTS]);

    return;
}
int main() {
    int port = SERV_PORT;

    g_efd = epoll_create(MAX_EVENTS + 1);
    if (g_efd <= 0)
        printf("create efd in %s err %s\n",__func__,strerror(errno));

    initlistensocket(g_efd, port);
    struct epoll_event events[MAX_EVENTS + 1];
    printf("server running:port[%d]\n",port);

    int checkpos = 0;
    int i;

    while(1)
    {
      int nfd = epoll_wait(g_efd, events,MAX_EVENTS+1, 1000);
      if (nfd < 0) {
          printf("epoll_wait error,exit\n");
          exit(-1);
      }
      for (i = 0; i < nfd; ++i) {
          struct myevent_s *ev = (struct myevent_s *)events[i].data.ptr;
          if ((events[i].events & EPOLLIN)&&(ev->events & EPOLLIN))
          {
              printf("fd:%d before:%d\n",ev->fd  ,ev->events );
              ev->call_back(ev->fd, events[i].events, ev->arg);
              printf("fd:%d after:%d\n", ev->fd ,ev->events );
          }
//          printf("---fd:%d \n",events[i].events);
          if ((events[i].events & EPOLLOUT) && (ev->events & EPOLLOUT))
          {
              ev->call_back(ev->fd,events[i].events,ev->arg);
          }
      }
    }
    return 0;
}
