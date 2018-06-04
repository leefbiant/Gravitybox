#include "mempool.h"


static inline unsigned int hash_key(void* value)
{
	recMemStore* pMemStore =(recMemStore*)value;
	return pMemStore->size % HASH_TABE_SIZE;
}

static inline unsigned int hash_compare(void* value1, void*value2)
{
	recMemStore* pMemStore1 = (recMemStore*)value1;
	recMemStore* pMemStore2 = (recMemStore*)value2;
	if ( pMemStore1->size <  pMemStore2->size)
	{
		return 1;
	}	
	else if (  pMemStore1->size ==  pMemStore2->size &&  value1 == value2)
	{
		return 0;
	}
	return 1;

}


static inline unsigned int hash_key_size(void* value)
{
	unsigned int * size = (unsigned int *)value;
	return *size % HASH_TABE_SIZE;
}

static inline unsigned int hash_compare_size(void* value1, void*value2)
{
	recMemStore* pMemStore = (recMemStore*)value1;
	unsigned int * size = (unsigned int *)value2;
	return pMemStore->size <  *size;
}

static inline int insert_hash_mem_node(recHashNode* pHashNode, void* data)
{
	
	unsigned int key = hash_key(data);
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
	while(pNode != NULL && pNode->next != NULL  &&  hash_compare(pNode->value, data)  )pNode = pNode->next;

	if ( !hash_compare(pNode->value, data) ) 
	{
		debug("insert data=%p  pNode->value=%p", data,  pNode->value);
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

static inline  void* find_hash_mem_node(recHashNode* pHashNode, void* data)
{
	unsigned int key = hash_key_size(data);
	recHashNode* previNode = &pHashNode[key];
	recHashNode* pNode = previNode->next;
	recHashNodeInfo* pHashNodeInfo = pHashNode[key].value;
	
	if ( pNode == NULL )
	{
		//unsigned int* calloc_size = data;
		//debug("pNode= NULL calloc size=%d key=%d",  *calloc_size, key);
		return NULL;
	}
	while( pNode != NULL && pNode->next != NULL &&  !hash_compare_size(pNode->value, data)  )
	{
		previNode = pNode;
		pNode = pNode->next;
	}

	if ( !hash_compare_size(pNode->value, data) )
	{
		//debug("find_hash_node  finded key=[%d]  node= [%p]", key, pNode->value);
		void* return_value = pNode->value;
		previNode->next = pNode->next;
		pNode->value = NULL;
		pNode->next = NULL;

		if ( pHashNodeInfo->pIdelNode == NULL )
		{
			pHashNodeInfo->pIdelNode  = pNode;
		}
		else
		{
			pNode->next = pHashNodeInfo->pIdelNode;
			pHashNodeInfo->pIdelNode = pNode;
		}
		pHashNodeInfo->idlenum++;
		return return_value;
	}
	return NULL;
}


void* mem_get(recMemPool* pMempool, unsigned int size )
{ 
	CHECK_OBJ_NOTNULL(pMempool, CLIENT_MEMPOOL_MAGIC);
	if ( size <= 0 ) return NULL;
	size += sizeof(recMemStore);
	size = _ALIGN(size, ALIGN_SIZE);	
	recMemStore *pMemStore = NULL;
	pthread_mutex_lock(&pMempool->mutex);
	pMemStore = find_hash_mem_node(pMempool->pHashNode, &size);
	pthread_mutex_unlock(&pMempool->mutex);
	if ( pMemStore )
	{
		pMemStore->using_times++;
	}
	else
	{
		CALLOC_MEM_STORE(pMemStore, size);
		pMempool->calloc_count++;
	}
	if ( pMemStore && pMemStore->ptr )
	{
		//debug("get mem %d from mempool mem node=%p mem ptr=%p calloc_count=%d", pMemStore->size, pMemStore, pMemStore->val,  pMempool->calloc_count);
		return pMemStore->ptr;
	}
	else
		return NULL;
}

int mem_free(recMemPool* pMempool, void* value)
{
	int nRet = 0;
	CHECK_OBJ_NOTNULL(pMempool, CLIENT_MEMPOOL_MAGIC);
	recMemStore* pMemStore = container_of(value, recMemStore, val);
	//debug("free mem value=%p pMemStore=%p mem ptr=%p",value,  pMemStore, pMemStore->val);
	CHECK_OBJ_NOTNULL(pMemStore, CLIENT_MEM_MAGIC);
	memset(pMemStore->ptr, 0x00, sizeof(pMemStore->size));
	pthread_mutex_lock(&pMempool->mutex);
	nRet = insert_hash_mem_node(pMempool->pHashNode, pMemStore);
	pthread_mutex_unlock(&pMempool->mutex);
	//debug("free mem %d from mempool memnode=%p nRet=%d calloc_count=%d", pMemStore->size, pMemStore, nRet, pMempool->calloc_count);
	if ( nRet != 0 )
	{
		debug("This is a bug memfree  failed");
		exit(0);
	}
	return nRet;
}

void mem_finit(recMemPool* pMempool)
{
	CHECK_OBJ_NOTNULL(pMempool, CLIENT_MEMPOOL_MAGIC);
	int i = 0;
	pthread_mutex_lock(&pMempool->mutex);
	for ( i=0; i<HASH_TABE_SIZE; i++)
	{
		//free next ==>recHashNode ==>insert node
		recHashNode* previNode = &pMempool->pHashNode[i];
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
	free(pMempool->pHashNode);
	pMempool->pHashNode = NULL;
	pthread_mutex_unlock(&pMempool->mutex);
}
int mem_pool_dump(recMemPool* pMempool)
{
	CHECK_OBJ_NOTNULL(pMempool, CLIENT_MEMPOOL_MAGIC);
	
	int i=0;
typedef struct __recMemInfo
{
	void* node;
	unsigned int key;
	unsigned int size;
	unsigned int using_time;
}recMemInfo;
	recMemInfo MemInfo[100];
	
	for ( i=0; i<HASH_TABE_SIZE; i++)
	{	memset(&MemInfo, 0x00, sizeof(recMemInfo)*100);
		int key_num = 0;
		recMemStore* pMemStore  = NULL;
		
		recHashNode* pNode = pMempool->pHashNode[i].next;
		if ( !pNode ) continue;
		pthread_mutex_lock(&pMempool->mutex);
		while(pNode)
		{
			pMemStore  = pNode->value;
			pNode = pNode->next;
			
			if ( key_num < 100 )
			{
				MemInfo[key_num].node = pMemStore;
				MemInfo[key_num].key= pMemStore->size % HASH_TABE_SIZE;
				MemInfo[key_num].size= pMemStore->size;
				MemInfo[key_num].using_time= pMemStore->using_times;
			}
			key_num++;
		}
		pthread_mutex_unlock(&pMempool->mutex);
		int j=0;
		for(j=0; j<key_num && j<100; j++)
		{
			debug("Id=%-6d addr=%-15p  key=%-5d   size=%-8d using_times=%-3d",i, MemInfo[j].node, MemInfo[j].key,  MemInfo[j].size, MemInfo[j].using_time);
		}	
	}
	
	return 0;
}


