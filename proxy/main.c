#include <getopt.h>

#include "nio.h"
#include "debug.h"
#include "init.h"
#include "task.h"

#ifndef SO_ORIGINAL_DST
#define SO_ORIGINAL_DST  80
#endif

int g_threadnu = 2;
extern int g_exit;
extern recThreadPool  g_witeThreadPool;
extern recThreadPool  g_readThreadPool;
void print_usage(FILE* stream, int exit_code, const char* program)
{
	printf("Usage: %s options\n", program);
	printf( " -p --port                          set listen port\n"
			" -t --thread                        set thread run in program\n"
			" -v --version                       show the version\n"
			" -h --help                          list all argument\n"
			"\nThe proxy run need follows:\n"
			"\tuse -p or --port option to set listen port\n"
			"\tuse -t or --thread option to set thread\n"
		   );
	exit(exit_code);
}
void SigHandler(int signo)
{
	if(SIGUSR1 == signo)
	{
		debug("recv signal %d   process exit", signo);
		g_exit = 1;
	}
}

void stop_thread()
{
	void* value_ptr;
	recThreadinfo* Threadinfo = NULL;
	for( Threadinfo = g_readThreadPool.pthreadPool ; Threadinfo < g_readThreadPool.pthreadPool  + g_threadnu ; Threadinfo++)
	{
		Post(Threadinfo);
		pthread_join(Threadinfo->tid, &value_ptr);
	}
	for(Threadinfo = g_witeThreadPool.pthreadPool ; Threadinfo < g_witeThreadPool.pthreadPool  + g_threadnu ; Threadinfo++)
	{
		Post(Threadinfo);
		pthread_join(Threadinfo->tid, &value_ptr);
	}
}
int nGetCpuNum()
{
	return sysconf(_SC_NPROCESSORS_CONF);
}
int  accept_func(struct __recTask* pListenTask )
{
	struct sockaddr_in local;
	socklen_t addrlen = sizeof( struct sockaddr_in);
	while(1)
	{
		int client = accept(pListenTask->fd, (struct sockaddr *) &local, &addrlen);
		if (client < 0)
		{
			if (EAGAIN == errno)
			{
				break;
			}
			debug("accept:%s", strerror(errno));
			return -1;
		}
		recTask* pTask =  new_task() ;
		if( NULL == pTask )
		{
			debug("new_task failed");
			close(client);
			continue;
		}
		if(fcntl(client, F_SETFL,fcntl(client, F_GETFL,0)|O_NONBLOCK ) < 0) 
		{
			debug("call fcntl failed, errno: %d, error info: %s", errno, strerror(errno));
			close(client);
			free_task(pTask);
		}
		pTask->fd = client;
		strncpy(pTask->client_ip, inet_ntoa(local.sin_addr),sizeof(pTask->client_ip)-1 );

		pTask->efd  = pListenTask->efd;
		pTask->read_func = read_data;
		pTask->write_func  = write_data;
		
	
		pTask->PeerTask = new_task() ;
		if( NULL == pTask->PeerTask )
		{
			debug("new_task failed");
			close(client);
			free_task(pTask);
			continue;
		}
		pTask->PeerTask->role = TASK_ROLE_SERVER;
		if ( (pTask->PeerTask->fd = socket(AF_INET, SOCK_STREAM, 0))  < 0)
		{
			close(client);
			free_task(pTask);
			continue;
		}
		if(fcntl(pTask->PeerTask->fd, F_SETFL,fcntl(pTask->PeerTask->fd, F_GETFL,0)|O_NONBLOCK ) < 0) 
		{
			debug("call fcntl failed, errno: %d, error info: %s", errno, strerror(errno));
			close(client);
			close(pTask->PeerTask->fd);
			free_task(pTask);
		}
		int len = sizeof(struct sockaddr_in);
		if (getsockopt(client, SOL_IP, SO_ORIGINAL_DST, &(pTask->PeerTask->addr), (socklen_t *)&len))
		{
			debug("Get DST IP fail from fd=%d error:%s", client, strerror(errno));
			free_task(pTask);
			continue;
		}
		pTask->PeerTask->addr.sin_family = AF_INET;
		if (connect(pTask->PeerTask->fd, (struct sockaddr *)&(pTask->PeerTask->addr), sizeof(struct sockaddr)) < 0)
		{
			if (errno != EINPROGRESS)
			{
				debug("connect socket error:%s", strerror(errno));
				free_task(pTask);
				continue;
			}
		}
		pTask->PeerTask->task_status = TASK_STATUS_CONTINUING;
		
		pTask->PeerTask->efd  = pListenTask->efd;
		pTask->PeerTask->read_func = read_data;
		pTask->PeerTask->write_func  = write_data;
		pTask->PeerTask->PeerTask = pTask;
		
		Even_ctl(pTask->PeerTask, pListenTask->efd, EPOLL_CTL_ADD,  EPOLLIN| EPOLLOUT|EPOLLET );
		Even_ctl(pTask, pListenTask->efd, EPOLL_CTL_ADD,  EPOLLIN| EPOLLOUT|EPOLLET );
		debug("recv a connecter server ip =%s port=%d ",  inet_ntoa(pTask->PeerTask->addr.sin_addr), ntohs(pTask->PeerTask->addr.sin_port));
	}
	return 0;
}
int main(int argc, char* argv[])
{
	int nRet = 0;
	int fd = 0;
	int client_port = 11600;
	const char* const short_options = "vhp:t:";
	const struct option long_options[] = {
		{"version", 0, NULL, 'v'},
		{"help", 0, NULL, 'h'},
		{"port", 1, NULL, 'p'},
		{"thread", 1, NULL, 't'},
		{NULL, 0, NULL, 0}
	};
	int ch = 0;
	opterr = 0;
	do{
		ch = getopt_long(argc, argv, short_options, long_options, NULL);
		switch (ch){
			case 't':
				g_threadnu = atoi(optarg);
				break;
			case 'v':
				debug("serverinterface V1.0 Build: %s %s\n", __DATE__, __TIME__);
				return 0;
			case 'h':
				print_usage(stdout, 0, argv[0]);
				break;
			case 'p':
				client_port = atoi(optarg);
				break;
			case ':':
				break;
			case '?':
				print_usage (stderr, 1, argv[0]);
				break;
			default :
				break;
		}
	}while(ch != -1);
	if ( g_threadnu == 0 ) g_threadnu = nGetCpuNum();
	nRet = init_log();
	if( nRet != 0 )
	{
		return -1;
	}
	
	daemon_init();
	signal(SIGINT, SIG_IGN);
	signal(SIGQUIT, SIG_IGN);
	signal(SIGPIPE, SIG_IGN);
	signal(SIGTTOU, SIG_IGN);
	signal(SIGTTIN, SIG_IGN);
	signal(SIGCHLD, SIG_IGN);

	struct sigaction act;
	act.sa_handler = SigHandler;
	sigemptyset(&act.sa_mask);
	act.sa_flags = SA_RESTART;

	if(sigaction(SIGUSR1, &act, NULL) < 0) 
	{
		return -1;
	}
		
	signal(SIGINT, SIG_IGN);
	signal(SIGPIPE, SIG_IGN);
	
	nRet = init_thread();
	if( nRet < 0 )
	{
		return -1;
	}
	recThreadinfo* readThreadbase = g_readThreadPool.pthreadPool;
	recThreadinfo* writeThreadbase = g_witeThreadPool.pthreadPool;
	
	int efd = epoll_create(MAX_SOCKET_NUM);
	if ( efd < 0 )
	{	
		return -1;
	}	
	struct epoll_event*  pEvent = calloc(MAX_SOCKET_NUM, sizeof(struct epoll_event));

	fd = service_init(client_port);
	if( fd < 0 )
	{
		return -1;
	}
	recTask* pListenTask = new_task();
	pListenTask->fd = fd;
	pListenTask->efd = efd;
	pListenTask->read_func = accept_func;
	pListenTask->write_func  = NULL;
	Even_ctl(pListenTask, efd, EPOLL_CTL_ADD, EPOLLIN | EPOLLET );

	time_t start_time = time(0);
	debug("process start");
	while(!g_exit)
	{
		int iRet = epoll_wait(efd, pEvent, MAX_SOCKET_NUM, 1000);
		if(-1 == iRet) 
		{
			if(EBADF == errno || EINVAL == errno || EFAULT == errno)
			{
				debug("epoll_wait error:%s", strerror(errno));
				break;
			}
		}
		if ( time(0) - start_time > 15)
		{
			start_time  = time(0);
			//task_info();
		}
		int i = 0;
		for( i = 0; i < iRet; i++ )
		{
			recTask * pTask = (recTask*)pEvent[i].data.ptr;
			CHECK_OBJNULL_CONTINUE(pTask, CLIENT_TASK_MAGIC);
			if((pEvent[i].events & EPOLLIN) == EPOLLIN)
			{
				recThreadinfo* pthread = &readThreadbase[pTask->fd % g_threadnu];
				if ( 0 != pthread->pTaskPoolQueue->push(pthread->pTaskPoolQueue, pTask))
				{
					//debug("EPOLLIN pthread->pTaskPoolQueue->push task %p failed", pTask);					
				}
				else
				{
					Post(pthread);
				}	
			}

			if( 	 ((pEvent[i].events & EPOLLOUT) == EPOLLOUT &&  pTask->writebuf.totall > 0)  ||
				 ( (pEvent[i].events & EPOLLOUT) == EPOLLOUT &&  pTask->role == TASK_ROLE_SERVER  && pTask->task_status == TASK_STATUS_CONTINUING))
			{
				recThreadinfo* pthread = &writeThreadbase[pTask->fd % g_threadnu];
				if ( 0 != pthread->pTaskPoolQueue->push(pthread->pTaskPoolQueue, pTask))
				{
					//debug("EPOLLOUT pthread->pTaskPoolQueue->push task %p failed", pTask);					
				}
				else
				{
					Post(pthread);	
				}		
			}
			

			if(pEvent[i].events & EPOLLERR  || pEvent[i].events & EPOLLRDHUP) 
			{
				debug("recv EPOLLERR or EPOLLRDHUP event=%p", pEvent[i].events);
			}
		}//end of for
	}
	debug("process start exit.....");
	free(pEvent);
	stop_thread();
	finit_thread();
	free_task(pListenTask);
	finit_task();
	debug("process  exit.....");
	return 0;
}
	

	

