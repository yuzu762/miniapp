#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <netinet/ip.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/time.h>
#include <errno.h>
#include <unistd.h>

#include "sniffer.h"
#include "tools.h"

#define ETH_P_IP 0x0800

void signal_white_now(int sig) {
    printf("bye bye\n");
}

int exec_cmd(char * buffer, int len)
{
    if (strncmp(buffer, "quit", 4) == 0)
        return (1);
    return (0);
}

int command_interpreter(int sd)
{
    int len;
    char buf[512];

    len = read(0, buf, 512);
    if (len > 0)
    {
        if (exec_cmd(buf, len) == 1)
            return (1);
    }
    return (0);
}

void display_time_and_date(){
    INITCOLOR(RED_COLOR);
    printf("[%s]",__DATE__);
    INITCOLOR(GREEN_COLOR);
    printf("[%s]",__TIME__);
    INITCOLOR(ZERO_COLOR);
}


void getting_started()
{
    CLEARSCREEN();
    display_time_and_date();
    printf("Getting started of Network sniffer\n\n");
}
int main() {
    int     sd;
    int     res;
    int     saddr_size;
    int     data_size;
    struct  sockaddr saddr;
    unsigned char *buffer;
    t_sniffer sniffer;
    fd_set fd_read;

    buffer = malloc(sizeof(unsigned char*)* 65536);
    sniffer.logfile = fopen("log.txt", "w");
    fprintf(sniffer.logfile,"***LOGFILE(%s-%s)***\n",__DATE__,__TIME__);
    if (NULL == sniffer.logfile) {
        perror("fopen(): ");
        return EXIT_FAILURE;
    }

    sniffer.prot = malloc(sizeof (t_protocal *));
    sd = socket(PF_PACKET, SOCK_RAW, htons((ETH_P_IP)));
    if (sd < 0) {
        perror("socket(): ");
        return EXIT_FAILURE;
    }
    getting_started();
    signal(SIGINT,signal_white_now);
    signal(SIGQUIT,signal_white_now);

    while (1) {
      FD_ZERO(&fd_read);
      FD_SET(0, &fd_read);
      FD_SET(sd, &fd_read);
      res = select(sd + 1, &fd_read, NULL, NULL ,NULL);
      if (res < 0) {
          close(sd);
          if (errno == EINTR) continue;
              perror( "select() ");
              return EXIT_FAILURE;
      } else {
          if (FD_ISSET(0,&fd_read)) {
              if (command_interpreter(sd) == 1)
                  break;
          } else if (FD_ISSET(sd, &fd_read))
            {
              saddr_size = sizeof (saddr);
              data_size = recvfrom(sd, buffer,65536, 0, &saddr, (socklen_t*)&saddr_size);
              if (data_size <= 0) {
                  close(sd);
                  perror("recvfrom()\n");
                  return EXIT_FAILURE;
              }
              ProcessPacket(buffer, data_size, &sniffer);
            }
      }

    }
    close(sd);
    return 0;
}

void ProcessPacket(unsigned char* buffer, int size, t_sniffer *sniffer) {
    buffer = buffer + 6 + 6 + 2;
    struct iphdr *iph = (struct iphdr*)buffer;
    ++sniffer->prot->total;
    if (iph->protocol == 6) ++sniffer->prot->tcp;
    printf("tcp:%d ,total:%d\n",sniffer->prot->tcp,sniffer->prot->total);

}
