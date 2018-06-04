#ifndef __INIT_H_
#define __INIT_H_
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <errno.h>
#include <stdarg.h>
#include <time.h>         // Time value functions
#include <stdlib.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <stdarg.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <dirent.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/sem.h>
#include <sys/ipc.h>
#include <ctype.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/un.h>
#include <unistd.h>
#include <pthread.h>


#include "task.h"
#include "debug.h"

#define LINE_CHAR_MAX_COUNT 1024

#define UNUSED(a)  a = a

void daemon_init();
int service_init(int port);
int init_log();
int init_conf();
char* rmRightTrim(char* szDes, char* szSrc);
int ReadProfileString(const char* szAppName, const char* szKeyName, char* szBuf,  int nSize, const char* szFileName);
int connectserverbyip(const char *server_ip, const short server_port, int is_ONBLOCK);
int init_thread();
void finit_thread();

#endif

