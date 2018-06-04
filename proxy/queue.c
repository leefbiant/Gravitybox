#include "queue.h"


 void* pop(recQueue* pQueue)
 { 		
 	CHECK_OBJ_NOTNULL(pQueue, CLIENT_QUEUE_MAGIC);
 	if (pQueue->is_lock) pthread_mutex_lock(&pQueue->mutex);	
 	
 	if ( pQueue->Head == NULL )
 	{
 		if (pQueue->is_lock) pthread_mutex_unlock(&pQueue->mutex);
 		return NULL;
 	}
 	void* data = pQueue->Head->node;
 	//if not exiist not poped 
 	if ( pQueue->is_check_exist )
 	{
	 	if ( 0 != del_hash_node(pQueue->pHashNode,data))
	 	{
	 		if (pQueue->is_lock) pthread_mutex_unlock(&pQueue->mutex);
	 		return NULL;
		}
	}
 	recNode* TmpNode = pQueue->Head;
 	
 	if ( pQueue->Head  ==  pQueue->Tail )
 	{
 		pQueue->Head  =  pQueue->Tail = NULL;
 	}
 	else
 	{
 		pQueue->Head = pQueue->Head->next;
 	}
 	TmpNode->node = NULL;
 	TmpNode->next  = NULL;
 	if ( pQueue->Idle == NULL )
 	{
 		pQueue->Idle = TmpNode;
 	}
 	else
 	{	
 		TmpNode->next = pQueue->Idle;
 		pQueue->Idle = TmpNode;
 	}
 	pQueue->NodeNum--;
 	
	if (pQueue->is_lock) pthread_mutex_unlock(&pQueue->mutex);
	return data;
 }

  int push(recQueue* pQueue, void* newbody)
  {
	CHECK_OBJ_NOTNULL(pQueue, CLIENT_QUEUE_MAGIC);
	if (pQueue->is_lock) pthread_mutex_lock(&pQueue->mutex);
	//if exist , not push again
	if ( pQueue->is_check_exist )
	{
		if ( 0 !=  insert_hash_node(pQueue->pHashNode,newbody))
		{
			if (pQueue->is_lock) pthread_mutex_unlock(&pQueue->mutex);
			return -1;		
		}
	}	
	recNode* NewNode = NULL;
	if ( pQueue->Idle == NULL )
	{
		NewNode = (recNode*)calloc(1, sizeof(recNode));
	}
	else
	{
		NewNode = pQueue->Idle;
		pQueue->Idle = pQueue->Idle->next;
	}
	
	NewNode->node = newbody;
	NewNode->next = NULL;
	if ( pQueue->Tail == NULL )
	{
		pQueue->Head = pQueue->Tail = NewNode;
	}
	else
	{
		 pQueue->Tail->next = NewNode;
		 pQueue->Tail = NewNode;
	}
	pQueue->NodeNum++;
	if (pQueue->is_lock) pthread_mutex_unlock(&pQueue->mutex);
	return 0;
  }

 int num(recQueue* pQueue, int type)
 {
 	CHECK_OBJ_NOTNULL(pQueue, CLIENT_QUEUE_MAGIC);
 	pthread_mutex_lock(&pQueue->mutex);
 	unsigned int num = 0;
 	switch(type)
 	{
 		case QUEUE_NODE_NUM:
 		{
 			num = pQueue->NodeNum;
 			break;
 		}	
 		case QUEUE_HASH_NODE_NUM:
 		{
 			int i=0;
 			for ( i=0; i<HASH_TABE_SIZE; i++)
 			{	
 				recHashNodeInfo*  pHashNodeInfo = pQueue->pHashNode[i].value;
 				num += pHashNodeInfo->innode;
 			}
 			break;
		}
		case QUEUE_HASH_IDLE_NUM:
		{
 			int i=0;
 			for ( i=0; i<HASH_TABE_SIZE; i++)
 			{	
 				recHashNodeInfo*  pHashNodeInfo = pQueue->pHashNode[i].value;
 				num += pHashNodeInfo->idlenum;
 			}
 			break;
		}
		default:
			break;
 	}
 	pthread_mutex_unlock(&pQueue->mutex);
 	return num;
 }
   void finit(recQueue* pQueue)
   {
   	//free queue node
	CHECK_OBJ_NOTNULL(pQueue, CLIENT_QUEUE_MAGIC);
	pthread_mutex_lock(&pQueue->mutex);	
	while(pQueue->Head)
	{
		recNode* TmpNode = pQueue->Head;
		pQueue->Head = pQueue->Head->next;
		free(TmpNode->node);
		free(TmpNode);
	}
	//free queue idle node
	while( pQueue->Idle )
	{	
		recNode* TmpNode = pQueue->Idle;
		pQueue->Idle = pQueue->Idle->next;
		free(TmpNode);
	}
	pQueue->Head  =  pQueue->Tail = NULL;

	//free hashtable node
	int i = 0;
	for ( i=0; i<HASH_TABE_SIZE; i++)
	{
		//free next ==>recHashNode ==>insert node
		recHashNode* previNode = &pQueue->pHashNode[i];
		if ( previNode == NULL )continue;
		recHashNode* pNode = previNode->next;
		recHashNode* delNode  = NULL;
		while(pNode != NULL )
		{
			delNode = pNode;
			pNode = delNode->next;
			free(delNode);
		}
		previNode->next = NULL;

		//free value ==>recHashNodeInfo ==>idle node
		recHashNodeInfo* pHashNodeInfo = previNode->value;
		pNode = pHashNodeInfo->pIdelNode;
		while(pNode != NULL )
		{
			delNode = pNode;
			pNode = delNode->next;
			free(delNode);
		}
		pHashNodeInfo->pIdelNode = NULL;
		free(previNode->value);
		previNode->value = NULL;
	}
	free(pQueue->pHashNode);
	pQueue->pHashNode = NULL;
	pthread_mutex_unlock(&pQueue->mutex);
	return;
}

int init_hash_table(recHashNode** pHashNode)
{
	*pHashNode = (recHashNode*)malloc(sizeof(recHashNode)*HASH_TABE_SIZE);
	if ( *pHashNode == NULL )return -1; 
	int i = 0;
	for( i=0; i<HASH_TABE_SIZE; i++)
	{
		(*pHashNode)[i].value = (recHashNodeInfo*)calloc(1, sizeof(recHashNodeInfo));
		(*pHashNode)[i].next = NULL;
	}
	return 0;
}

static inline unsigned  int def_hashfunc(void* data)
{
	unsigned long node  = (unsigned long)data;
	return  node % HASH_TABE_SIZE;
}
int insert_hash_node(recHashNode* pHashNode, void* data)
{
	unsigned int key = def_hashfunc(data);
	recHashNode* previNode = &pHashNode[key];
	recHashNode* pNode = previNode->next;
	
	recHashNodeInfo* pHashNodeInfo = pHashNode[key].value;
	
	if ( pNode == NULL )
	{
		if ( pHashNodeInfo->pIdelNode != NULL )//idle
		{
			recHashNode* IdleHashNode  = pHashNodeInfo->pIdelNode;
			pHashNodeInfo->pIdelNode = IdleHashNode->next;
			pHashNodeInfo->idlenum--;
			
			IdleHashNode->value = data;
			IdleHashNode->next = NULL;
			
			previNode->next = IdleHashNode;
		}
		else
		{
			recHashNode* pNewNode = calloc(1, sizeof(recHashNode));
			pNewNode->value = data;
			pNewNode->next = NULL;
			previNode->next = pNewNode;
		}
		pHashNodeInfo->innode++;
		return 0;
	}
	while(pNode != NULL && pNode->next != NULL  &&  pNode->value > data  )pNode = pNode->next;

	if ( pNode->value ==  data ) 
	{
		//debug("insert data=%p  pNode->value=%p", data,  pNode->value);
		return EXIST_VALUE;
	}
	recHashNode* pNewNode = NULL;
	if ( pHashNodeInfo->pIdelNode != NULL  )//idle
	{
		pNewNode  = pHashNodeInfo->pIdelNode;
		pHashNodeInfo->pIdelNode= pNewNode->next;
		pHashNodeInfo->idlenum--;
		pNewNode->value = data;
	}
	else
	{
		pNewNode = calloc(1, sizeof(recHashNode));
		pNewNode->value = data;	
		
	}
	if ( pNode->next != NULL )
	{
		pNewNode->next = pNode->next;
		pNode->next = pNewNode;
	}
	else
	{
		pNode->next = pNewNode;
		pNewNode->next = NULL;
	}
	pHashNodeInfo->innode++;
	//debug("innode=%d idlenum=%d", pHashNodeInfo->innode, pHashNodeInfo->idlenum);
	return 0;
}

int del_hash_node(recHashNode* pHashNode, void* data)
{
	unsigned int key = def_hashfunc(data);
	recHashNode* previNode = &pHashNode[key];
	recHashNode* pNode = previNode->next;
	recHashNodeInfo* pHashNodeInfo = pHashNode[key].value;
	
	if ( pNode == NULL )
	{
		//debug("dell data=%p node=%p pNode=NULL", data, node);
		return NOT_EXIST;
	}
	while( pNode != NULL && pNode->next != NULL &&  pNode->value > data  )
	{
		previNode = pNode;
		pNode = pNode->next;
	}

	if ( pNode->value ==  data )
	{
		previNode->next = pNode->next;
		pNode->value = NULL;
		pNode->next = NULL;
	
		recHashNode* IdleHashNode  = pHashNodeInfo->pIdelNode;

		if ( pHashNodeInfo->pIdelNode == NULL )
		{
			pHashNodeInfo->pIdelNode  = pNode;
		}
		else
		{
			while( IdleHashNode !=NULL && IdleHashNode->next  != NULL)IdleHashNode = IdleHashNode->next;
			IdleHashNode->next = pNode;
		}
		pHashNodeInfo->idlenum++;
	}
	else
	{
		//debug("not find data=%p node=%p pNode=NULL", data, node);
		return NOT_EXIST;
	}
	pHashNodeInfo->innode--;
	//debug("innode=%d idlenum=%d", pHashNodeInfo->innode, pHashNodeInfo->idlenum);
	return 0;
}



