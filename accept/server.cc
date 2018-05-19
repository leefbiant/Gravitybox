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

#ifndef debug
#define debug(fmt, args...) \
    do { \
          struct timeval tv; \
          gettimeofday(&tv, NULL); \
          struct tm tm_now = *localtime(&tv.tv_sec); \
          fprintf(stderr, "[%04d-%02d-%02d %02d:%02d:%02d]|%d|%ld(%s:%d:%s): " fmt "\n", tm_now.tm_year + 1900, tm_now.tm_mon + 1, tm_now.tm_mday, tm_now.tm_hour, tm_now.tm_min, tm_now.tm_sec, getpid(),syscall(SYS_gettid),__FILE__, __LINE__, __func__, ##args); \
        } while (0) 
#endif


int main(int argc, char* argv[]) {
  int port = 9001;
  if (argc > 2) {
    port = atoi(argv[1]);
  }

  struct sockaddr_in svr_addr;
  memset(&svr_addr, 0, sizeof(struct sockaddr_in));
  svr_addr.sin_family      = AF_INET;
  svr_addr.sin_addr.s_addr = htonl(INADDR_ANY);
  svr_addr.sin_port        = htons(port);

  int fd = socket(AF_INET, SOCK_STREAM, 0);
  if (fd < 0) {
    debug("socket failed:%s\n", strerror(errno));
    return -1;
  }
  int ret = bind(fd, (struct sockaddr*)&svr_addr, (socklen_t)sizeof(struct sockaddr_in));
  if (ret < 0) {
    debug("bind failed:%s\n", strerror(errno));
    return -1;
  }
  ret = listen(fd, 1);
  if (ret < 0) {
    debug("listen failed:%s\n", strerror(errno));
    return -1;
  }
  while (1) {
    sleep(1);
  }
  return 0;
}
