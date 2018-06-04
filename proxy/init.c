#include "init.h"
#include "task.h"
#include "queue.h"
#include "mempool.h"

recThreadPool  g_readThreadPool;
recThreadPool  g_witeThreadPool;


recMemPool*	 g_pMempool = NULL;
extern int g_threadnu ;

void daemon_init()
{
	pid_t pid;
	if((pid=fork()) != 0)
	{
		exit(0);
	}	
	setsid();
	
	if((pid=fork()) != 0)
	{
		exit(0);
	}
	return;
}
int service_init(int port)
{
	int sock;
	int result;		
	sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock < 0)
	{		
		debug("socket create failed, errno: %d, error info: %s", errno, strerror(errno));
		return -1;
	}
	result = 1;
	if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &result, sizeof(int))<0)
	{
		debug("setsockopt failed, errno: %d, error info: %s",errno, strerror(errno));
		close(sock);
		return -2;
	}
	struct sockaddr_in bindaddr;	
	bindaddr.sin_family = AF_INET;
	bindaddr.sin_port = htons(port);
	bindaddr.sin_addr.s_addr = INADDR_ANY;		
	if (bind(sock, (struct sockaddr*)&bindaddr, sizeof(bindaddr)) < 0)
	{
		debug("bind port %d failed, errno: %d, error info: %s.", port, errno, strerror(errno));
		return -1;
	}
	if (listen(sock, 1024) < 0)
	{
		debug("listen port %d failed, errno: %d, error info: %s",  port, errno, strerror(errno));
		close(sock);
		return -4;
	}

	/**/
	struct linger linger;
	linger.l_onoff = 1;
	linger.l_linger = 30*60;
	if (setsockopt(sock, SOL_SOCKET, SO_LINGER, &linger, (socklen_t)sizeof(struct linger)) < 0)
	{
		debug("setsockopt failed, errno: %d, error info: %s", errno, strerror(errno));
		return -1;
	}
	
	if(fcntl(sock, F_SETFL,fcntl(sock, F_GETFL,0)|O_NONBLOCK ) < 0) 
	{
		debug("fcntl O_NONBLOCK, errno: %d, error info: %s", errno, strerror(errno));
		return -1;
	}
	return sock;
}
int connectserverbyip(const char *server_ip, const short server_port, int is_ONBLOCK)
{
	int result;
	struct sockaddr_in addr;
	int sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock < 0)
	{
		debug("socket create failed, errno: %d, error info: %s", errno, strerror(errno));
		return -1;
	}

	addr.sin_family = PF_INET;
	addr.sin_port = htons(server_port);
	result = inet_aton(server_ip, &addr.sin_addr);
	if (result == 0 )
	{
		debug("inet_aton failed:%s", strerror(errno));
		return -1;
	}
	result = 1;
	if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &result, sizeof(int))<0)
	{
		debug("setsockopt failed, errno: %d, error info: %s",errno, strerror(errno));
		close(sock);
		return -2;
	}
	result = connect(sock, (const struct sockaddr*)&addr, sizeof(addr));
	if (result < 0)
	{
		debug("connect ip[%s] port[%d] failed:%s", server_ip, server_port, strerror(errno));
		return -1;
	}

	struct linger linger;

	linger.l_onoff = 1;
	linger.l_linger = 30 * 100;
	if (setsockopt(sock, SOL_SOCKET, SO_LINGER, &linger, (socklen_t)sizeof(struct linger)) < 0)
	{
		debug("setsockopt failed, errno: %d, error info: %s", errno, strerror(errno));
		return -1;
	}
	if( is_ONBLOCK )
	{
		if(fcntl(sock, F_SETFL,fcntl(sock, F_GETFL,0)|O_NONBLOCK ) < 0) 
		{
			debug("fcntl O_NONBLOCK, errno: %d, error info: %s", errno, strerror(errno));
			return -1;
		}
	}
	return sock;
}

int init_log()
{

	if(access("./log", F_OK) != 0 )
	{
		mkdir("./log", S_IXUSR|S_IRUSR|S_IWUSR|S_IWGRP|S_IRGRP|S_IROTH);
	}
	int fd = open("./log/server.log", O_CREAT|O_RDWR|O_APPEND, S_IRUSR|S_IWUSR|S_IWGRP|S_IRGRP|S_IROTH);
	if( fd < 0 )
	{
		debug("open file ./log/server.log failed:%s",  strerror(errno));
		return -1;
	}
	debug_init(fd);
	return 0;	
}


int init_thread()
{
	g_readThreadPool.pthreadPool = (recThreadinfo*)calloc(1, sizeof(recThreadinfo) * g_threadnu);
	memset(g_readThreadPool.pthreadPool , 0x00, sizeof(recThreadinfo) * g_threadnu);
	
	recThreadinfo* Threadinfo = NULL;
	for( Threadinfo = g_readThreadPool.pthreadPool ; Threadinfo < g_readThreadPool.pthreadPool  + g_threadnu ; Threadinfo++ )
	{
		Threadinfo->pTaskPoolQueue = (recQueue*)malloc(sizeof(recQueue));
		INIT_QEUE(Threadinfo->pTaskPoolQueue, NULL);
		Threadinfo->sem = (sem_t*)malloc(sizeof(sem_t));
		sem_init(Threadinfo->sem, 0, 0);
		
		if( pthread_create( &Threadinfo->tid, NULL, read_thread, Threadinfo) < 0 )
		{
			debug("pthread_create errno: %d, error info: %s", errno, strerror(errno));
			return -1;
		}
	}

	g_witeThreadPool.pthreadPool = malloc(sizeof(recThreadinfo) * g_threadnu);
	memset(g_witeThreadPool.pthreadPool , 0x00, sizeof(recThreadinfo) * g_threadnu);

	for( Threadinfo = g_witeThreadPool.pthreadPool ; Threadinfo < g_witeThreadPool.pthreadPool  + g_threadnu ; Threadinfo++ )
	{
		Threadinfo->pTaskPoolQueue = (recQueue*)malloc(sizeof(recQueue));
		INIT_QEUE(Threadinfo->pTaskPoolQueue, NULL);
		Threadinfo->sem = (sem_t*)malloc(sizeof(sem_t));
		sem_init(Threadinfo->sem, 0, 0);
		if( pthread_create( &Threadinfo->tid, NULL, write_thread, Threadinfo) < 0 )
		{
			debug("pthread_create errno: %d, error info: %s", errno, strerror(errno));
			return -1;
		}
	}


	
	g_pMempool = (recMemPool*)calloc(1, sizeof(recMemPool));
	INIT_MEMPOOL(g_pMempool);
	return 0;
	
}

void finit_thread()
{
	recThreadinfo* Threadinfo = NULL;
	for(Threadinfo = g_readThreadPool.pthreadPool ; Threadinfo < g_readThreadPool.pthreadPool  + g_threadnu ; Threadinfo++ )
	{
		Threadinfo->pTaskPoolQueue->finit(Threadinfo->pTaskPoolQueue);
		free(Threadinfo->pTaskPoolQueue);
		free(Threadinfo->sem);
		Threadinfo->pTaskPoolQueue = NULL;
		Threadinfo->sem = NULL;
	}
	free(g_readThreadPool.pthreadPool);
	g_readThreadPool.pthreadPool = NULL;
	
	for(Threadinfo = g_witeThreadPool.pthreadPool ; Threadinfo < g_witeThreadPool.pthreadPool  + g_threadnu ; Threadinfo++)
	{
		Threadinfo->pTaskPoolQueue->finit(Threadinfo->pTaskPoolQueue);
		free(Threadinfo->pTaskPoolQueue);
		free(Threadinfo->sem);
		Threadinfo->pTaskPoolQueue = NULL;
		Threadinfo->sem = NULL;
	}
	free(g_witeThreadPool.pthreadPool);
	g_witeThreadPool.pthreadPool = NULL;
}
