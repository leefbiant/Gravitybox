#ifndef __MEMPOOL_H_
#define __MEMPOOL_H_

#include "queue.h"

#define offsetof(TYPE, MEMBER) ((size_t) &((TYPE *)0)->MEMBER)
#define container_of(ptr, type, member) ({             \
         const typeof( ((type *)0)->member ) *__mptr = (ptr);     \
         (type *)( (char *)__mptr - offsetof(type,member) );})

/*
struct student{
    char name[20];  
    char sex;
}stu={"zhangsan",'m'};

int main(void)
{
    struct student *stu_ptr = container_of(&stu.sex,struct student, sex);
    printf("stu_ptr->name:%s\tstu_ptr->sex:%c\n", stu_ptr->name, stu_ptr->sex);
    return 0;
}
*/
#define _ALIGN(addr,size) (((addr)+(size)-1)&(~((size)-1)))

#define ALIGN_SIZE     (1 << 4)

typedef struct __recMemStore
{
	unsigned magic;
#define CLIENT_MEM_MAGIC		0x1c83245d
 	unsigned int size;
 	unsigned int using_times;
 	void* ptr;
 	char val[0];
}recMemStore;

typedef struct __recMemPool
{
	unsigned magic;
#define CLIENT_MEMPOOL_MAGIC		0x1c832b5d
 	pthread_mutex_t mutex;
 	unsigned int calloc_count;
	recHashNode* pHashNode;
 	void* (*hash_mem_get)(struct __recMemPool*, unsigned int );
 	int(*hash_mem_free)(struct __recMemPool*, void*);
 	void(*hash_mem_finit)(struct __recMemPool*);
}recMemPool;


void* mem_get(recMemPool* pMempool, unsigned int callocsize );
int mem_free(recMemPool* pMempool, void* value);
void mem_finit(recMemPool* pMempool);
int mem_pool_dump(recMemPool* pMempool);


#define INIT_MEMPOOL(pMemPool)		\
do														\
{														\
	(pMemPool)->magic  = CLIENT_MEMPOOL_MAGIC;				\
	pthread_mutex_init(&(pMemPool)->mutex , NULL);			\
	init_hash_table(&(pMemPool)->pHashNode);				\
	(pMemPool)->hash_mem_get  = mem_get;					\
	(pMemPool)->hash_mem_free  = mem_free;				\
	(pMemPool)->hash_mem_finit  = mem_finit;					\
	(pMemPool)->calloc_count = 0;							\
}while(0)

#define CALLOC_MEM_STORE(node, calocsize) 	\
do										\
{										\
	node = calloc(1, calocsize);		\
	if ( !node ) break;						\
	node->magic = CLIENT_MEM_MAGIC;		\
	node->size = calocsize;					\
	node->ptr = node->val;	\
}while(0)


#define __UES_MEM_POOL
#endif


