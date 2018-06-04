#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <time.h>
#include <unistd.h>
#include <sys/syscall.h> 
#include <sys/time.h>

#include "debug.h"

int g_logfd = 2;

void debug_init(int fd)
{
	if( fd > 0 ) g_logfd = fd;
	return 	;
}
char* GetCurDateTimeStr(char *pszDest, const int nMaxOutSize)
{
    if (pszDest == NULL ) return NULL;
	
	struct timeval stLogTv;
	gettimeofday(&stLogTv, NULL);
	struct tm strc_now = *localtime(&stLogTv.tv_sec);
    snprintf(pszDest, nMaxOutSize, "%04d-%02d-%02d %02d:%02d:%02d:%03ld:%03ld"
            , strc_now.tm_year + 1900, strc_now.tm_mon + 1, strc_now.tm_mday
            , strc_now.tm_hour, strc_now.tm_min, strc_now.tm_sec, stLogTv.tv_usec/1000,  stLogTv.tv_usec%1000);
   

    return pszDest;
}
void debug_log(const char* pszFileName, const int  nLine, const char* func, const char *pszInfo, ...)
{
	char printMsg[512] = {0};
	char writebuf[1024] = {0};
	/* Timestamps... */
	char timedatestamp[32] = {0};
	GetCurDateTimeStr(timedatestamp, sizeof(timedatestamp));
	va_list argptr;
	va_start(argptr, pszInfo);
	vsnprintf(printMsg, sizeof(printMsg)-1, pszInfo, argptr);
	va_end(argptr);

	
	snprintf(writebuf, 1023, "[%s]|%d|%ld(%s:%d:%s):%s",timedatestamp, getpid(),syscall(SYS_gettid),pszFileName, nLine, func, printMsg);
	if(printMsg[strlen(printMsg)-1] != '\n')
	{
		strncat(writebuf, "\n", 1023);
	}
	write(g_logfd, writebuf, strlen(writebuf));
	return ;
}
