#include "task.h"
#include "debug.h"
#include "nio.h"
#include "init.h"
#include  "common.h"
#include "mempool.h"
int g_exit = 0;
static recQueue*  g_TaskQueue  = NULL;
extern recMemPool*	 g_pMempool ;
extern  recQueue*         g_pReadTaskPool;
extern  recQueue*         g_pWriteTaskPool;

recTask* new_task()
{
	if ( g_TaskQueue == NULL )
	{
		g_TaskQueue = (recQueue* )malloc(sizeof(recQueue));
		INIT_QEUE(g_TaskQueue, NULL);
	}
	recTask* pTask = NULL;
	if ((pTask = g_TaskQueue->pop(g_TaskQueue)) == NULL )
	{
		pTask = (recTask*)calloc(1, sizeof(recTask));
		if( NULL == pTask ) return NULL;
		pTask->read_func = read_data;
		pTask->write_func = write_data;
		pTask->pReadMsg = NULL;
		pTask->writebuf.offset = pTask->writebuf.totall = 0;
		pTask->readbuf.offset  = pTask->readbuf.totall = 0;
		pTask->pMsgTaskQueue  = (recQueue*)malloc(sizeof(recQueue)) ;
		INIT_QEUE(pTask->pMsgTaskQueue, NULL);
		pTask->pMsgTaskQueue->is_check_exist = 1;
		pTask->pMsgTaskQueue->is_lock = 1;
		pTask->role = TASK_ROLE_CLIENT;
	}
	pTask->magic = CLIENT_TASK_MAGIC;
	return pTask;
}


void free_task(recTask* pTask)
{
	//debug("task[%p] over ip[%s] close fd[%d]",pTask, pTask->client_ip, pTask->fd);
	if(pTask->fd > 0 ) close(pTask->fd);
	pTask->fd  = -1;
	pTask->efd = -1; 
	pTask->magic = CLIENT_TASK_MAGIC_FREE;
	memset(pTask->client_ip, 0x00, sizeof(pTask->client_ip));
	pTask->pReadMsg = NULL;
	
	recMsgHeader* pMsg = NULL;
	while((pMsg =pTask->pMsgTaskQueue->pop(pTask->pMsgTaskQueue) ))
	{
#ifdef __UES_MEM_POOL
		g_pMempool->hash_mem_free(g_pMempool, pMsg);
#else
		free(pMsg);
#endif
	}
	
	pTask->writebuf.offset = pTask->writebuf.totall = 0;
	pTask->readbuf.offset =  pTask->readbuf.totall = 0;
	recTask* pTmptask = pTask->PeerTask;
	if ( 0 != g_TaskQueue->push(g_TaskQueue, pTask))
	{
		debug("g_TaskQueue->push task %p failed", pTask);
	}
		
	pTask = pTmptask;

	if ( pTask ) 
	{
		if(pTask->fd > 0 ) close(pTask->fd);
		pTask->fd  = -1;
		pTask->efd = -1; 
		pTask->magic = CLIENT_TASK_MAGIC_FREE;
		memset(pTask->client_ip, 0x00, sizeof(pTask->client_ip));
		pTask->pReadMsg = NULL;
		
		recMsgHeader* pMsg = NULL;
		while((pMsg =pTask->pMsgTaskQueue->pop(pTask->pMsgTaskQueue) ))
		{
#ifdef __UES_MEM_POOL
			g_pMempool->hash_mem_free(g_pMempool, pMsg);
#else
			free(pMsg);
#endif
		}
		
		pTask->writebuf.offset = pTask->writebuf.totall = 0;
		pTask->readbuf.offset =  pTask->readbuf.totall = 0;
		if ( 0 != g_TaskQueue->push(g_TaskQueue, pTask))
		{
			debug("g_TaskQueue->push task %p failed", pTask);
		}
	}
	return ;
}

void task_info()
{
	debug("task Queue num=%d Queue hash num =%d Queue hash Idle num =%d", 
		   			g_TaskQueue->num(g_TaskQueue, QUEUE_NODE_NUM),
		   			g_TaskQueue->num(g_TaskQueue, QUEUE_HASH_NODE_NUM),
		   			g_TaskQueue->num(g_TaskQueue, QUEUE_HASH_IDLE_NUM));
	//mem_pool_dump(g_pMempool);			
}

void finit_task()
{
	recTask* pTask = NULL;
	while( (pTask = g_TaskQueue->pop(g_TaskQueue)))
	{
		pTask->pMsgTaskQueue->finit(pTask->pMsgTaskQueue);
		free(pTask->pMsgTaskQueue);
		free(pTask);
	}
	g_TaskQueue->finit(g_TaskQueue);
	free(g_TaskQueue);
}
void* read_thread(void* arg)
{
	recThreadinfo* Threadinfo = (recThreadinfo*)arg;
	while(!g_exit)
	{
		sem_wait(Threadinfo->sem);
		recTask* pTask = NULL;
		while( (pTask = (recTask*)Threadinfo->pTaskPoolQueue->pop(Threadinfo->pTaskPoolQueue)) )
		{
			CHECK_OBJNULL_CONTINUE(pTask, CLIENT_TASK_MAGIC);			
			pTask->read_func(pTask);

		}
		
	}
	debug("read_thread thread exit.....");
	pthread_exit(NULL);
	return NULL;
}

				
void* write_thread(void* arg)
{
	recThreadinfo* Threadinfo = (recThreadinfo*)arg;
	while(!g_exit)
	{
		sem_wait(Threadinfo->sem);
		recTask* pTask = NULL;		
		while( (pTask = (recTask*)Threadinfo->pTaskPoolQueue->pop(Threadinfo->pTaskPoolQueue))  )
		{	
			CHECK_OBJNULL_CONTINUE(pTask, CLIENT_TASK_MAGIC);
			pTask->write_func(pTask);
		
		}

	}
	debug("write_thread thread exit.....");
	pthread_exit(NULL);
	return NULL;
}

void Post(recThreadinfo* pthread)
{
	int iValue = 0;
	sem_getvalue(pthread->sem, &iValue);
	if( iValue <= 0 )sem_post(pthread->sem);
}
