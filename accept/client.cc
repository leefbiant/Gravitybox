#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <sys/time.h> 
#include <sys/types.h> 
#include <syscall.h>
#include <sys/types.h>          /* See NOTES */
#include <sys/socket.h>


#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <time.h>
#include <sys/epoll.h>

#ifndef debug
#define debug(fmt, args...) \
    do { \
          struct timeval tv; \
          gettimeofday(&tv, NULL); \
          struct tm tm_now = *localtime(&tv.tv_sec); \
          fprintf(stderr, "[%04d-%02d-%02d %02d:%02d:%02d]|%d|%ld(%s:%d:%s): " fmt "\n", tm_now.tm_year + 1900, tm_now.tm_mon + 1, tm_now.tm_mday, tm_now.tm_hour, tm_now.tm_min, tm_now.tm_sec, getpid(),syscall(SYS_gettid),__FILE__, __LINE__, __func__, ##args); \
        } while (0) 
#endif


int tcp_client(const char* host, int port) {
  int ip = inet_addr(host);
  struct sockaddr_in svr_addr;
  memset(&svr_addr, 0, sizeof(struct sockaddr_in));
  svr_addr.sin_family      = AF_INET;
  svr_addr.sin_addr.s_addr = ip;
  svr_addr.sin_port        = htons(port);

  int fd = socket(AF_INET, SOCK_STREAM, 0);
  if (fd < 0) {
    debug("socket failed:%s\n", strerror(errno));
    return -1;
  }
  int ret = connect(fd, (struct sockaddr *)&svr_addr, sizeof(struct sockaddr));
  if (ret < 0) {
    debug("connect failed:%s\n", strerror(errno));
    close(fd);
    return -1;
  }
  ret = send(fd, "*", 1, 0);
  if (ret < 0) {
    debug("connect failed:%s\n", strerror(errno));
    close(fd);
    return -1;
  }
  debug("connect sucess fd:%d", fd);
  return fd; 
}

void ENV_ADD(int efd, int fd) {
  struct epoll_event ev = {0};
  ev.events   = EPOLLIN| EPOLLET| EPOLLERR; 
  ev.data.fd       = fd; 
  if (0 > epoll_ctl(efd, EPOLL_CTL_ADD, fd, &ev)) {
    debug("epoll_ctl fd %d :%s", fd, strerror(errno));
  }
}

void ENV_DEL(int efd, int fd) {
  struct epoll_event ev = {0};
  ev.data.fd       = fd; 
  if (0 > epoll_ctl(efd,  EPOLL_CTL_DEL, fd, &ev)) {
    debug("epoll_ctl fd %d :%s", fd, strerror(errno));
  }
  close(fd);
}


int main(int argc, char* argv[]) {
  const char* host = "127.0.0.1";
  int port = 9001;
  if (argc > 3) {
    port = atoi(argv[1]);
    host = argv[2];
  }

  char buff[128] = {0};
  int efd = epoll_create(1000);
  struct epoll_event event[1000] = {0};
  while (1) {
    int fd = tcp_client(host, port);
    if (fd > 0) {
      ENV_ADD(efd, fd);
    }
    int ret = epoll_wait(efd, event, 1000, 100);
    if (ret == -1) {
      debug("epoll_wait failed:%s", strerror(errno));
      return -1;
    }
    for (int i = 0; i < ret; i++) {
      if (event[i].events & EPOLLIN) {
        int evt_fd = event[i].data.fd;
        int readn = read(evt_fd, buff, sizeof(buff));
        if (readn < 0) {
          debug("read failed from fd:%d %s", evt_fd, strerror(errno));
          ENV_DEL(efd, evt_fd);
          continue;
        }
        if (readn == 0) {
          debug("read 0 from fd:%d %s", evt_fd, strerror(errno));
          ENV_DEL(efd, evt_fd);
          continue;
        }
      }
    }
  }
  return 0;
}
