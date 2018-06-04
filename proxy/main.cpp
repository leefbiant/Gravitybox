#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/types.h>          /* See NOTES */
#include <sys/socket.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <errno.h>
#include <stdlib.h>
#include <time.h>
#include <stdarg.h>


#ifndef SO_ORIGINAL_DST
#define SO_ORIGINAL_DST  80
#endif
#define BUFFSIZE   (4*1024)
typedef struct __fd_info
{
	int fd;
	char buff[BUFFSIZE];
	struct sockaddr_in addr;
	size_t nread;
	size_t nwrite;
}fd_info;
typedef struct __proxy_tabls
{
	fd_info cli;
	fd_info ser;
	int status;
}proxy_tabls;

#define MAXTASK               1000
#define ALIVESIZE             256
proxy_tabls  g_task[MAXTASK];

#define CONNECT_FROM_CLIENT   					0x001
#define CONNECT_TO_SERVER     					0x002
#define PROXY_TRANSFER        					0x004
#define WRITE_TO_CLIENT_AND_END      				0x008
#define WRITE_TO_CLIENT       					0x010
#define WRITE_TO_SERVER      					0x020
#define TASK_FINISH	          				0x040


int kdpfd = 0;
struct epoll_event  g_events[ALIVESIZE];
const int  SER 				  =0;
const int  CLI				  =1;
int listen_count              =0;
void debug_log(const char* pszFileName, const int  nLine, const char* func, const char *pszInfo, ...)
{
        char printMsg[1024];

        /* Timestamps... */
        struct tm *timestamp;
        time_t tt;
        char timedatestamp[24];

        va_list argptr;
        va_start(argptr, pszInfo);
        vsnprintf(printMsg, sizeof(printMsg)-1, pszInfo, argptr);
        va_end(argptr);

        memset(timedatestamp, 0, sizeof(timedatestamp));
        tt = time(NULL);
        timestamp = localtime(&tt);

        strftime(timedatestamp, sizeof(timedatestamp)-1, "%Y-%m-%d %T", timestamp);
        fprintf(stderr, "%s(%s:%d:%s):%s", timedatestamp, pszFileName, nLine, func, printMsg);
        if(printMsg[strlen(printMsg)-1] != '\n')
        {
                fprintf(stderr,"\n");
        }
        return ;
}
#define debug(fmt, args...) debug_log( __FILE__, __LINE__, __func__, fmt , ##args)

bool setnonblocking(int fd)
{
	int opts;
	opts = fcntl(fd, F_GETFL);
	if (opts < 0)
		return false;
	opts = opts | O_NONBLOCK;
	if (fcntl(fd, F_SETFL, opts) < 0)
		return false;
	return true;
}

int CreateSock(int socket_type, const char *sIp,  const int port, int *listener)
{
    struct sockaddr_in address;
    int listening_socket;
    int reuse_addr = 1;

    memset((char *) &address, 0, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_port = htons(port);
    address.sin_addr.s_addr = inet_addr(sIp);

    listening_socket = socket(AF_INET, socket_type, 0);
    if (listening_socket < 0)
    {
        perror("socket");
        return -1;
    }

    if (listener != NULL)
    {
        *listener = listening_socket;
    }

    setsockopt(listening_socket, SOL_SOCKET, SO_REUSEADDR, (void*)(&(reuse_addr))
        , sizeof(reuse_addr));

    if (bind(listening_socket, (struct sockaddr *) &address, sizeof(address)) < 0)
    {
        perror("bind");
        close(listening_socket);
        return -1;
    }

    if (socket_type == SOCK_STREAM)
    {
        /* Queue up to 50 connections before having them automatically rejected. */
        listen(listening_socket, 250);
    }

    return listening_socket;
}
void enevt_set(int fd, __uint32_t evt, proxy_tabls* task)	
{
	if(fd <0 ) return ;
	struct epoll_event even;
	even.events = evt;
	even.data.u64 =  (uint64_t)(uint64_t((uint64_t)fd << 32) + (uint64_t)task);
	epoll_ctl(kdpfd, EPOLL_CTL_DEL, fd, NULL);
	if (epoll_ctl(kdpfd, EPOLL_CTL_ADD, fd, &even) < 0)
	{
		if( epoll_ctl(kdpfd, EPOLL_CTL_MOD, fd, &even) < 0 )
		{
			debug("enevt_set() epoll set insertion fd=%d error:%s\n", fd, strerror(errno));
			return ;
		}
	}
	return ;
}
int enevt_del(int fd)
{
	return epoll_ctl(kdpfd, EPOLL_CTL_DEL, fd, NULL) ;
	
}
void close_connect(int* fd)
{
	if ( *fd <= 0 )
		return;
	if (shutdown(*fd, 2) < 0)
	{
		if (errno != ENOTCONN)
		{
			debug("Shutdown fd[%d] error:%s task over", *fd, strerror(errno));
			exit(1);
		}
	}
	while (close(*fd) < 0)
	{
		if (errno != EINTR)
		{
			debug("close fd[%d] error:%s task over", *fd, strerror(errno));
			exit(1);
		}
	}
	*fd = -1;
}

void task_over(proxy_tabls* task)
{	
	if( listen_count-- < 0 )
	{
		debug("this is a error\n");
		exit(0);
	}
	debug("task over :%p client fd=%d server fd=%d listen_count=%d", task, task->cli.fd, task->ser.fd, listen_count);
	if( task->cli.fd > 0 )
	{
		enevt_del(task->cli.fd);
		close_connect(&task->cli.fd);
		
	}
	if( task->ser.fd > 0 )
	{
		enevt_del(task->ser.fd);
		close_connect(&task->ser.fd);
	}
	memset( task, 0x00, sizeof(proxy_tabls));	
	return ;
}


int readEINTR(int fd,  char *buf, int count)
{
	int rc;

	while (1){
		rc = read(fd, buf, count);
		if (rc < 0){
			if (errno == EINTR)
				continue;
		}
		break;
	}
	return rc;
}

int readALL(int fd,  char *buf, int count)
{
	return readEINTR(fd, buf, count);
}

int writeEINTR(int fd,  char *buf, int count)
{
	int rc;

	while (1){
		rc = write(fd, buf, count);
		if (rc < 0){
			if (errno == EINTR)
				continue;
		}
		break;
	}

	return rc;
}

int writeALL(int fd,  char *buf, int count)
{
	int rc = 0;
	int alreadysend = 0;

	while (alreadysend < count)
	{
		rc = writeEINTR(fd, &buf[alreadysend], count - alreadysend);
		if (rc == -1)
			break;
		else
			alreadysend += rc;

	}

	return (alreadysend) ? alreadysend : -1;
}
/*
int setconnect( void* ptr)
{
	if ( ptr == NULL )
	{
		debug("setconnect() input ==NULL\n");
		exit(0);
	}
	proxy_tabls* task = NULL;
	memcpy(&task, ptr, 4);
*/
int setconnect( proxy_tabls* task)
{	

	if( task  == NULL || task < &g_task[0] || task >&g_task[MAXTASK+1])
	{
		debug("in setconnect() get error task add=%p", task);
		exit(0);
	}
	if ((task->ser.fd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
	{
		debug("create socket error:%s", strerror(errno));
		task_over(task);
		return -1;
	}
	if ((!setnonblocking(task->cli.fd)) || (!setnonblocking(task->ser.fd)))
	{
		debug("setnonblocking socket error:%s", strerror(errno));
		task_over(task);	
		return -1;
	}

	int len = sizeof(struct sockaddr_in);
	if (getsockopt(task->cli.fd, SOL_IP, SO_ORIGINAL_DST, &(task->ser.addr), (socklen_t *)&len))
	{
		debug("Get DST IP fail from fd=%d error:%s", task->cli.fd, strerror(errno));
		task_over(task);
		return -1;
	}
	task->ser.addr.sin_family = AF_INET;
	if (connect(task->ser.fd, (struct sockaddr *)&(task->ser.addr), sizeof(struct sockaddr)) < 0)
	{
		if (errno != EINPROGRESS)
		{
			debug("connect socket error:%s", strerror(errno));
			task_over(task);
			return -1;
		}
	}
	//debug("get cilent ip= %s port=%d  server ip =%s port=%d\n", inet_ntoa(task->cli.addr.sin_addr), ntohs(task->cli.addr.sin_port), inet_ntoa(task->ser.addr.sin_addr), ntohs(task->ser.addr.sin_port));
	task->status = CONNECT_TO_SERVER;
	enevt_set(task->ser.fd, EPOLLIN|EPOLLOUT, task);
	return 0;
}

int connect_to_server(proxy_tabls* task )
{
	if ( task == NULL ) 
	{
		debug("connect_to_server() input == NULL\n");
		exit(1);
	}
	int error;
	socklen_t len;
	error = -1;
	len	= sizeof(error);
	if((getsockopt(task->ser.fd, SOL_SOCKET, SO_ERROR, &error, &len) == 0) && (error == 0))
	{
		//debug("connect ser:%s sucess\n", inet_ntoa(task->ser.addr.sin_addr));
		task->status = PROXY_TRANSFER;
		enevt_set(task->cli.fd, EPOLLIN, task );
		enevt_set(task->ser.fd, EPOLLIN, task);
	}
	else
	{
		debug("connect ser failed:%s\n", strerror(error));
		task_over(task);
		return TASK_FINISH;

	}
	return 0;
}

int read_from_client(proxy_tabls* task)
{
	if( task->cli.nread >= BUFFSIZE)
	{
		debug("nread[%d] >=  BUFFSIZE[%d]  not read", task->cli.nread , BUFFSIZE);
		task->status = WRITE_TO_SERVER;
		enevt_set(task->ser.fd, EPOLLOUT,  task);
		return 0;
	}
	int rc = readALL(task->cli.fd, task->cli.buff + task->cli.nread, BUFFSIZE-task->cli.nread);
	if(rc < 0)
	{
		debug("readall from client failed fd=%d error:%s task over\n", task->cli.fd, strerror(errno));
		task_over(task);
		return TASK_FINISH;
	}
	else if( rc == 0 )
	{
		debug("read from cli return == 0 fd=%d error:%s task over", task->cli.fd, strerror(errno));
		task_over(task);
		return TASK_FINISH;
	}
	task->cli.nread += rc;
	task->status = WRITE_TO_SERVER;
	enevt_set(task->ser.fd, EPOLLOUT,  task);
	return 0;
}
int read_from_server(proxy_tabls* task)
{
	if( task->ser.nread >= BUFFSIZE)
	{
		debug("nread[%d] >=  BUFFSIZE[%d]  not read", task->ser.nread , BUFFSIZE);
		task->status = WRITE_TO_CLIENT;
		enevt_set(task->cli.fd, EPOLLOUT,  task);
		return 0;
	}
	int rc = readALL(task->ser.fd, task->ser.buff+task->ser.nread, BUFFSIZE-task->ser.nread);
	if(rc < 0)
	{
		debug("readall from server failed fd=%d :%s task over task=%p\n",task->ser.fd, strerror(errno), task);	
		task_over(task);
		return TASK_FINISH;
	}
	else if( rc == 0 && task->ser.nread > 0 )
	{
		debug("read from ser return == 0 fd=%d but nread > 0 maybe ser has close ,close ser fd[%d] task=%p", task->ser.fd,task->ser.fd, task);	
		close_connect(&task->ser.fd);
		task->status = WRITE_TO_CLIENT_AND_END;
		enevt_set(task->cli.fd, EPOLLOUT,  task);
		return 0;
	}
	else if (rc == 0 && task->ser.nread == 0 )
	{
		debug("read from ser return == 0 fd=%d maybe ser has close ,close ser fd[%d]", task->ser.fd,task->ser.fd  );	
		task_over(task);
		return TASK_FINISH;

	}
	task->ser.nread += rc;
	task->status = WRITE_TO_CLIENT;
	enevt_set(task->cli.fd, EPOLLOUT,  task);
	return 0;
}
int write_to_client(proxy_tabls* task)
{
	int rc = writeALL(task->cli.fd, task->ser.buff + task->ser.nwrite, task->ser.nread-task->ser.nwrite);
	if(rc < 0)
	{
		debug("writeall to client failed: nread=%d nwrite=%d\n", task->ser.nread, task->ser.nwrite);
		return TASK_FINISH;
	}
	debug("writeall to client client fd=%d service fd=%d write len=%d read=%d write=%d",task->cli.fd, task->ser.fd, rc, task->ser.nread, task->ser.nwrite );
	task->ser.nwrite += rc;
	if( task->ser.nread == task->ser.nwrite )
	{
		task->ser.nread = task->ser.nwrite = 0;
		task->status = PROXY_TRANSFER;
		enevt_set(task->cli.fd, EPOLLIN,  task);
		enevt_set(task->ser.fd, EPOLLIN,  task);
	}
	else
	{
		task->status = WRITE_TO_CLIENT;
		enevt_set(task->cli.fd, EPOLLOUT,  task);
	}
	return 0;
	
}
int write_to_server(proxy_tabls* task)
{
	int rc = writeALL(task->ser.fd, task->cli.buff + task->cli.nwrite, task->cli.nread-task->cli.nwrite);
	if(rc < 0)
	{
		debug("writeall to server failed: nread=%d nwrite=%d\n", task->ser.nread, task->ser.nwrite);		
		task_over(task);
		return TASK_FINISH;
	}
	debug("writeall to server client fd=%d service fd=%d write len=%d read=%d write=%d",task->cli.fd, task->ser.fd, rc , task->cli.nread, task->cli.nwrite);
	task->cli.nwrite += rc;
	if( task->cli.nread == task->cli.nwrite )
	{
		task->cli.nread = task->cli.nwrite = 0;
		task->status = PROXY_TRANSFER;
		//enevt_set(task->cli.fd, EPOLLIN,  task);
		enevt_set(task->ser.fd, EPOLLIN,  task);
	}
	else
	{
		task->status = WRITE_TO_SERVER;
		enevt_set(task->ser.fd, EPOLLOUT,  task);
	}
	return 0;
}

int proxy(__uint64_t event, __uint32_t evt)
{
	proxy_tabls* task = (proxy_tabls*)(event & 0xffffffff);
	if( task == NULL || task < &g_task[0] || task >&g_task[MAXTASK+1] ) 
	{
		debug("in proxy() get error task add=%p", task);
		exit(1);	
	}
	int fd =  event >> 32;
	enevt_del(fd);
	int return_side = fd == task->cli.fd ? CLI:SER;
	if( task->status <= 0 && fd > 0 ) 
	{	
		debug("error task status task=%p fd=%d", task, fd);
		
		return 0;
	}
	switch(task->status)
	{
		case CONNECT_TO_SERVER:
		{
			if( (return_side == SER) && ((evt & EPOLLOUT) == EPOLLOUT) )
			{
				return connect_to_server(task);
			}
			else
			{
				debug("Connect from client status, but return event is not for it, close this connect.\n");
				task_over(task);
				return 0;
			}
			break;

		}
		case PROXY_TRANSFER:
		{
			if( (return_side == CLI) && ((evt & EPOLLIN ) == EPOLLIN))
			{
				return read_from_client(task);
			}
			else if((return_side == SER) && ((evt & EPOLLIN ) == EPOLLIN))
			{
				return read_from_server(task);
			}
			else
			{
				debug("PROXY_TRANSFER status, but return event is not for it, close this connect.\n");
				task_over(task);
				return 0;
			}
			break;
		}
		case WRITE_TO_CLIENT:
		{
			if( return_side == CLI && (evt & EPOLLOUT ) == EPOLLOUT)
			{
				if ( write_to_client(task) != 0 )
				{	
					debug("write to client failed\n");
					task_over(task);
					return TASK_FINISH;	
				}
				return 0;
			}
			else if(return_side == SER && (evt & EPOLLIN ) == EPOLLIN)
			{
				return read_from_server(task);
			}
			else
			{
				debug("WRITE_TO_CLIENT status, but return event is not for it, close this connect.\n");
				task_over(task);
				return 0;
			}
			break;
		}

		case WRITE_TO_SERVER:
		{
			if( return_side == SER && (evt & EPOLLOUT ) == EPOLLOUT)
			{
				return write_to_server(task);
			}
			else if( return_side == CLI && (evt & EPOLLIN ) == EPOLLIN)
			{
				return read_from_client(task);
			}
			else
			{
				debug("WRITE_TO_SERVER status, but return event is not for it, close this connect.\n");
				task_over(task);
				return 0;
			}
			break;			

		}
		case WRITE_TO_CLIENT_AND_END:
		{
			if( return_side == CLI && (evt & EPOLLOUT ) == EPOLLOUT)
			{
				 if( write_to_client(task) != 0 )
				 {
				 	 debug("write to client failed\n");
					 task_over(task);
					 return TASK_FINISH;
				 }
				 return 0;
			}
			else
			{
				debug("WRITE_TO_SERVER status, but return event is not for it, close this connect.\n");
				task_over(task);
				return 0;
			}			
			break;
		}
		default:
		{
			debug("error task=%p fd=%d  client fd=%d  server fd=%d status=%x and event=%p EPOLLIN=%p EPOLLOUT=%p return_side=%s\n",task, fd, task->cli.fd, task->ser.fd, task->status, evt, EPOLLIN, EPOLLOUT, return_side==CLI?"client":"server");
			task_over(task);
			exit(1);
		}
	}
	debug("error task=%p fd=%d  client fd=%d  server fd=%d status=%x and event=%p EPOLLIN=%p EPOLLOUT=%p return_side=%s\n",task, fd, task->cli.fd, task->ser.fd, task->status, evt, EPOLLIN, EPOLLOUT, return_side==CLI?"client":"server");
	return TASK_FINISH;
}
int main(int argc, char* argv[])
{
	if( argc != 2 )
	{
		debug("using: %s port\n");
		return -1;
	}
	debug("addr first=%p addr end =%p", &g_task[0], &g_task[MAXTASK+1]  );
	memset(g_task, 0x00, sizeof(proxy_tabls)*MAXTASK);
	int nfds = 1;
	int client = 0;	

	struct epoll_event ev;
	
	kdpfd = epoll_create(25);
	if ( kdpfd <0 )
	{
		perror("epoll_create");
		return -1;
	}
	int listener  = 0;
        listener  = CreateSock(SOCK_STREAM, "0.0.0.0", atoi(argv[1]), &listener);
	if ( listener <= 0 )
	{
		debug("create sock failed\n");
		return -1;
	}
	int fd = 0;
	enevt_set(listener, EPOLLIN|EPOLLET, NULL);
	while(1)
	{
		nfds = epoll_wait(kdpfd, g_events, ALIVESIZE, -1);
		if( nfds == -1 && errno != EINTR)
		{
			perror("epoll_wati");
			exit(0);
		}
		for (int n = 0; n < nfds; ++n) 
		{
			fd = g_events[n].data.u64 >> 32;
			if ( fd == listener )
			{
				if( listen_count >=  MAXTASK - listener ) 
				{
					debug("connect[%d] is to many my > %d close\n", listen_count, MAXTASK - listener);
					continue;
				}
				struct sockaddr_in local;
				socklen_t addrlen = sizeof( struct sockaddr_in);
				client = accept(listener, (struct sockaddr *) &local, &addrlen);
				if (client < 0)
				{
					perror("accept");
					continue;
				}
				listen_count++;	
				proxy_tabls* task_addr  = &g_task[(client-1)/2];
				task_addr->cli.fd = client;
				memcpy(&task_addr->cli.addr, &local, sizeof(struct sockaddr_in));
				if( setconnect(task_addr) == 0 )
				{	
					debug("recv a new connect fd=%d listen_count=%d task=%p",client, listen_count, task_addr);
				}

			} 
			else
			{
				proxy(g_events[n].data.u64, g_events[n].events);
			}
		}
	}
	return 0;
}
