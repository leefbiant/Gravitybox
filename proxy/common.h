#ifndef __COMMON_H_
#define  __COMMON_H_
#include "debug.h"

enum IO_RESULT
{
	IO_SUCCESS = 0,
	IO_FAILED,
	IO_EAGAIN,
	IO_CLOSE
};

#define TASK_ROLE_CLIENT              0
#define TASK_ROLE_SERVER		     1


#define TASK_STATUS_CONTINUING    		0x00001
#define TASK_STATUS_CONTINUED			0x00002



#define ERR_SUCCESS			 0
#define ERR_FAILED			-1
#define ERR_NO_MORE_DATA	-2
#define ERR_NO_MORE_SPACE	-3
#define ERR_HEAD				-4

#define CMD_ECHO				0x0001


#define CHECK_OBJ_NOTNULL(ptr, type_magic)									\
do {																		\
	if((ptr) == NULL)														\
	{																		\
		debug("task[%p] is NULL", ptr);	\
		exit(0);																\
	}																			\
	if((ptr)->magic != type_magic)												\
	{																			\
		debug("task[%p] magic = %p  not is %p", ptr, (ptr)->magic, type_magic);		\
		exit(0);																						\
	}																									\
} while (0)

#define CHECK_OBJNULL_CONTINUE(ptr, type_magic)									\
	if((ptr) == NULL)		{	debug("task[%p] is NULL", ptr);	exit(0);}			\
	if((ptr)->magic != type_magic){	debug("task[%p] magic = %p  not is %p", ptr, (ptr)->magic, type_magic);	continue;}																																																

#define NEW_MSG_NODE(ptr)									\
do															\
{															\
	(ptr) = (recMsgNode*)calloc(1, sizeof(recMsgNode));				\
	if( (ptr) == NULL )											\
	{														\
		debug("New MSG == NULL");								\
		exit(0);												\
	}														\
	(ptr)->magic = CLIENT_MSG_MAGIC;							\
	(ptr)->pMsg = NULL;										\
}while(0)


#endif
