#ifndef QUQE_H_
#define QUQE_H_
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include "debug.h"
#include "common.h"


typedef struct __recHashNode
{
	void *value;
	struct __recHashNode* next;
}recHashNode; 	

typedef  struct __recHashNodeInfo
{
	unsigned int idlenum;
	unsigned int innode;
	recHashNode* pIdelNode;
}recHashNodeInfo;

#define HASH_TABE_SIZE     1697
#define EXIST_VALUE         -1
#define NOT_EXIST			-2

#define MEM_HASH_SIZE  1693

#define QUEUE_NODE_NUM       				  1
#define QUEUE_HASH_NODE_NUM                         2
#define QUEUE_HASH_IDLE_NUM                           3



int init_hash_table(recHashNode** pHashNode);
int insert_hash_node(recHashNode* pHashNode, void* data);
int del_hash_node(recHashNode* pHashNode, void* data);

typedef struct __recNode
{
	void* node;
	struct __recNode* next;
}recNode;

struct recQueue;
 typedef struct __recQueue
 {
	unsigned magic;
#define CLIENT_QUEUE_MAGIC		0x1b83245d
 	pthread_mutex_t mutex;
 	unsigned int NodeNum;
 	recNode* Head;
 	recNode* Tail;
 	recNode* Idle;	
 	recHashNode* pHashNode;
 	int is_check_exist;
 	int is_lock;
 	void* (*pop)(struct __recQueue* pQueue);
 	int (*push)(struct __recQueue* pQueue,  void* node);
 	int(*num)(struct __recQueue* pQueue, int type);
 	void (*finit)(struct __recQueue* pQueue);
 }recQueue;


 void* pop(recQueue* pQueue);
 int push(recQueue* pQueue, void* newNode);
  int num(recQueue* pQueue, int type);
 void finit(recQueue* pQueue);



 #define INIT_QEUE(Queue, compare)\
 do{						\
 	(Queue)->Head = (Queue)->Tail =  (Queue)->Idle = NULL; (Queue)->NodeNum=0;(Queue)->pop = pop;(Queue)->push = push;(Queue)->num = num;(Queue)->finit = finit;\
 	pthread_mutex_init(&(Queue)->mutex , NULL);(Queue)->magic = CLIENT_QUEUE_MAGIC; init_hash_table(&(Queue)->pHashNode);\
 	(Queue)->is_check_exist = 1; (Queue)->is_lock = 1;\
}while(0) 	


#endif

