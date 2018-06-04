#include "nio.h"
#include "task.h"
#include "debug.h"
#include "common.h"
#include "mempool.h"

extern recThreadPool  g_witeThreadPool;
extern int g_threadnu ;
extern recMemPool*	 g_pMempool ;



void Even_ctl(recTask* pTask, int efd, int op, __uint32_t evt)
{
	struct epoll_event ev = {0};
	ev.data.ptr   = pTask;
	if( op == EPOLL_CTL_ADD || op == EPOLL_CTL_MOD )
	{
		ev.events 	  = evt|EPOLLERR|EPOLLHUP;
		if(epoll_ctl(efd, op,  pTask->fd, &ev) < 0 )
		{
			debug("epoll_ctl %p failed:%s process must exit", op, strerror(errno));
			exit(0);
		}
	}
	else if( op == EPOLL_CTL_DEL )
	{
		if( epoll_ctl(efd, op,  pTask->fd, NULL) < 0 )
		{
			debug("epoll_ctl %p failed:%s  process must exit", op, strerror(errno));
			exit(0);
		}
	}
	return ;	
}

int read_from_sock(recTask* pTask)
{
	int iErr = IO_FAILED;
	int nRet = 0;
	while( pTask->fd > 0 && pTask->readbuf.offset < MAXBUFFSIZE)
	{
		nRet = read(pTask->fd,  pTask->readbuf.buff + pTask->readbuf.totall, MAXBUFFSIZE - pTask->readbuf.totall);
		if ( nRet  == 0 )
		{
			iErr = IO_CLOSE;
			break;
		}
		else if ( nRet < 0 )
		{
			if( EAGAIN == errno )
			{
				iErr  = IO_EAGAIN;
				break;
			}
			iErr = IO_FAILED;
			debug("read socket %d return %d:%s", pTask->fd, errno, strerror(errno));
			break;
		}
		 iErr = IO_SUCCESS;
		 pTask->readbuf.totall += nRet;  
	}
	return iErr;
}

int get_msg_frombuf(recTask* pTask)
{
		
#ifdef __UES_MEM_POOL	
	pTask->pReadMsg =  (recMsgHeader*)(g_pMempool->hash_mem_get(g_pMempool, pTask->readbuf.totall+sizeof(recMsgHeader)));
#else
	pTask->pReadMsg =  (recMsgHeader*)calloc(1, pTask->readbuf.totall + sizeof(recMsgHeader));
#endif
	memcpy(pTask->pReadMsg->data, pTask->readbuf.buff + pTask->readbuf.offset, pTask->readbuf.totall);
	
	pTask->pReadMsg->length = pTask->readbuf.totall;
	pTask->readbuf.offset = pTask->readbuf.totall = 0;

	//debug("get_msg_frombuf sucess Msg length=%d  buff totall=%d  Buff offset=%d", pHeader->length, pTask->readbuf.totall, pTask->readbuf.offset);
	return 0;
}

int HandlePkg(recTask* pTask)
{

	if (0 != pTask->PeerTask->pMsgTaskQueue->push(pTask->PeerTask->pMsgTaskQueue, pTask->pReadMsg))
	{
		debug("pTask->pMsgTaskQueue->push task %p", pTask->pReadMsg);
	}
				
	recThreadinfo* pTnreadinfo = &g_witeThreadPool.pthreadPool[pTask->PeerTask->fd % g_threadnu]; 	
	if ( 0 != pTnreadinfo->pTaskPoolQueue->push(pTnreadinfo->pTaskPoolQueue, pTask->PeerTask))
	{
		debug("pTnreadinfo->pTaskPoolQueue->push task %p failed", pTask);
	}
	else
	{			
		Post(pTnreadinfo);
	}
//	debug("task role[%d] read %d msg put to task role [%d]", pTask->role, pTask->pReadMsg->length, pTask->PeerTask->role);
	pTask->pReadMsg = NULL;
	return 0;
}

int read_data(recTask* pTask)
{
	CHECK_OBJ_NOTNULL(pTask->PeerTask, CLIENT_TASK_MAGIC);
	//debug("task role [%d] ", pTask->role);
	int io_stat = IO_SUCCESS;
	while(1)
	{
		io_stat = read_from_sock(pTask);
		
		if (IO_CLOSE == io_stat || IO_FAILED == io_stat ) break;

		if( pTask->fd < 0 ||
		    pTask->readbuf.offset < 0 ||
		    pTask->readbuf.totall < 0 ||
		    pTask->readbuf.totall - pTask->readbuf.offset < 0
		   )
		   {
		   	debug("Thsi is a Error  fd=%d  totall=%d offset=%d", pTask->fd ,  pTask->readbuf.totall , pTask->readbuf.offset);
		   }

		if ( pTask->readbuf.totall > 0 )
		{
			get_msg_frombuf(pTask);
			HandlePkg( pTask );
		}		
		if ( IO_EAGAIN == io_stat )
		{
			break;
		}
	}
	if (IO_CLOSE == io_stat || IO_FAILED == io_stat)
	{
		//debug("read_from_sock failed\n");
		free_task(pTask);
	}
	return 0;
}

int write_to_sock(recTask* pTask)
{

	
	int iErr = IO_SUCCESS;
	if( pTask->fd < 0 ||
	    pTask->writebuf.offset < 0 ||
	    pTask->writebuf.totall < 0 ||
	    pTask->writebuf.totall - pTask->writebuf.offset <= 0
	   )
	   {
	   	debug("This is a Error  fd=%d  totall=%d offset=%d", pTask->fd ,  pTask->writebuf.totall , pTask->writebuf.offset);
	   	return IO_EAGAIN;
	   }
	// send data
	while (pTask->fd >= 0 && pTask->writebuf.offset < pTask->writebuf.totall )
	{
		int iRet = write(pTask->fd, pTask->writebuf.buff+ pTask->writebuf.offset, pTask->writebuf.totall - pTask->writebuf.offset);
		if (iRet <= 0)
		{	
			if (EAGAIN == errno)
			{			
				iErr = IO_EAGAIN;
				break;
			}
			debug("write fd[%d] size[%d] failed :%s", pTask->fd, pTask->writebuf.totall - pTask->writebuf.offset, strerror(errno));
			iErr = IO_FAILED;
			break;
		}
		else
		{
//			debug("write data %d  to task role [%d] ", iRet,  pTask->role);
			iErr = IO_SUCCESS;
			pTask->writebuf.offset += iRet;
			if (pTask->writebuf.offset == pTask->writebuf.totall)
			{
				pTask->writebuf.offset = pTask->writebuf.totall = 0 ;	
				break;
			}
			else
			{
				debug("not write all totall=%d, offset=%d ", pTask->writebuf.totall, pTask->writebuf.offset);
			}
			debug("write %d data to task role %d ", iRet , pTask->role);
		}
	}
	return iErr ;
}

int write_data(recTask* pTask)
{
	CHECK_OBJ_NOTNULL(pTask->PeerTask, CLIENT_TASK_MAGIC);
	//debug("task role [%d] ", pTask->role);
	if ( pTask->role == TASK_ROLE_SERVER && pTask->task_status == TASK_STATUS_CONTINUING)
	{
		int error;
		socklen_t len;
		error = -1;
		len	= sizeof(error);
		if((getsockopt(pTask->fd, SOL_SOCKET, SO_ERROR, &error, &len) == 0) && (error == 0))
		{

			pTask->task_status = TASK_STATUS_CONTINUED;			
			//debug("task role %d connect server sucess", pTask->role);
		}
		else
		{
			debug("getsockopt failed :%s", strerror(errno));
			free_task(pTask);
			return -1;
		}
		
	}

	int iRet = IO_SUCCESS;
	do 
	{	recMsgHeader* pHeader = NULL;
		if (0 == pTask->writebuf.offset && 0 == pTask->writebuf.totall )
		{
			if ( (pHeader = pTask->pMsgTaskQueue->pop(pTask->pMsgTaskQueue)) == NULL )break;
			if ( pHeader->length < 0 || pHeader->length >  MAXBUFFSIZE ) 
			{
				debug("This is a pHeader->length =%d  exit", pHeader->length);
				exit(0);
			}
			memcpy(pTask->writebuf.buff, pHeader->data, pHeader->length);
			pTask->writebuf.totall = pHeader->length; 
			pTask->writebuf.offset = 0;

#ifdef __UES_MEM_POOL	
			g_pMempool->hash_mem_free(g_pMempool, pHeader);
#else
			free(pHeader);	
#endif
			//debug("get a msg totall =%d offset =%d start to write task role %d", pTask->writebuf.totall , pTask->writebuf.offset, pTask->role);
		} 	
		iRet = write_to_sock(pTask);

	} while (IO_SUCCESS == iRet);
	
	if (IO_CLOSE == iRet || IO_FAILED == iRet  )
	{		
		debug("write_to_sock failed");
		free_task(pTask);
	}

	return ERR_SUCCESS;
}

