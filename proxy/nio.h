#ifndef __NIO_H_
#define __NIO_H_
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/types.h>          /* See NOTES */
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <errno.h>
#include <stdlib.h>
#include <time.h>
#include <sys/stat.h>
#include <unistd.h>
#include <netinet/tcp.h> 

#include "task.h"

#ifndef  EPOLLRDHUP
#define EPOLLRDHUP	0x2000
#endif

int64_t hltonl(int64_t host);
int64_t ntohll(int64_t host) ;
void Even_ctl(recTask* pTaskNode, int efd, int op, __uint32_t evt);
int read_data(recTask* pTask);
int write_data(recTask* pTask);


int chl_read(recTask* pTask);
int chl_write(recTask* pTask);
int client_read(recTask* pTask);
int client_write(recTask* pTask);
#endif
