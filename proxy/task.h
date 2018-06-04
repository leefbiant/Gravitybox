#ifndef __TASK_H_
#define __TASK_H_
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
#include <semaphore.h>

#include "common.h"
#include "queue.h"

#define   VER                 0x1001

#define   FREE_TASK           0
#define   INTASK              1

#define MAXTASK               10
#define ALIVESIZE             256
#define MAXBUFFSIZE           (96*1024)
#define MAX_SOCKET_NUM			100

struct __recTask;
typedef int(*TASK_FUNC)(struct __recTask*);
#pragma pack(1)

typedef struct __recMsgHeader
{
	unsigned int ver;
	unsigned int type;
	unsigned int returncode;
	unsigned int length;
	char data[0];
}recMsgHeader;


typedef struct __recRecBuff
{
	int     offset;
	int 	totall;
	int 	current;
	char	buff[MAXBUFFSIZE];
}recRecBuff;


typedef struct __recTask
{
	unsigned magic;
#define CLIENT_TASK_MAGIC		0x1b96615d	
#define CLIENT_TASK_MAGIC_FREE	0xffffffff
	pthread_mutex_t mutex;
	int fd;
	int efd;
	char client_ip[16]; 
	struct sockaddr_in addr;
	int role;
	int task_status;
	recMsgHeader* pReadMsg; 
	recQueue* pMsgTaskQueue;
	recRecBuff readbuf;
	recRecBuff writebuf;
	TASK_FUNC read_func;
	TASK_FUNC write_func;
	struct __recTask* PeerTask;
}recTask;

typedef void *(*TheadFunc)(void*);


typedef struct __recThreadinfo
{
	pthread_t tid;
	sem_t * sem;
	recQueue* pTaskPoolQueue;
}recThreadinfo;


typedef struct __recThreadPool
{
	unsigned int index;
	recThreadinfo* pthreadPool;
}recThreadPool;


#pragma pack()

recTask* new_task();
void free_task(recTask* node);
void task_info();
void finit_task();
int read_data(recTask* pTask);
int write_data(recTask* pTask);
void* read_thread(void* arg);
void* write_thread(void* arg);
void Post(recThreadinfo* pthread);
#endif

