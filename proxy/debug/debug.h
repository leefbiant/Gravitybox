#ifndef __DEBUG_H_
#define __DEBUG_H_

#ifdef __cplusplus
extern "C"
{
void debug_log(const char* pszFileName, const int  nLine, const char* func, const char *pszInfo, ...);
#define debug(fmt, args...) debug_log( __FILE__, __LINE__, __func__, fmt , ##args)
void debug_init(int fd);
}
#else

void debug_log(const char* pszFileName, const int  nLine, const char* func, const char *pszInfo, ...);
#define debug(fmt, args...) debug_log( __FILE__, __LINE__, __func__, fmt , ##args)
void debug_init(int fd);

#endif

#endif
