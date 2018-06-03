/*************************************************************************
    > File Name: thread_cond.h
    > Author: leef
    > Mail: bt731001@gmail.com 
    > Created Time: 2018年06月03日 星期日 15时48分12秒
 ************************************************************************/

#ifndef THREAD_COND_H_
#define THREAD_COND_H_
#include <pthread.h>
#include <semaphore.h>
#include<iostream>
using namespace std;

class PthreadCond {
  public:
    PthreadCond():thread_cond(PTHREAD_COND_INITIALIZER),
    lock(PTHREAD_MUTEX_INITIALIZER) {
    }
    ~PthreadCond() { };
  public:
    bool WaitCond(){
      pthread_mutex_lock(&lock); 
      pthread_cond_wait(&thread_cond, &lock);
      pthread_mutex_unlock(&lock);   
      return true;
    }
    bool CondSignal() {
      pthread_mutex_lock(&lock); 
      pthread_cond_signal(&thread_cond);
      pthread_mutex_unlock(&lock);
      return true;
    }
  private:
    pthread_cond_t thread_cond;
    pthread_mutex_t lock;
};

class ThreadSem {
  public:
    ThreadSem() {
      m_sem = new sem_t;
      sem_init(m_sem, 0, 0);
    };
    ~ThreadSem() {
      sem_destroy(m_sem);
    };
  public:
    bool WaitCond() {
      sem_wait(m_sem);
      return true;
    }
    bool CondSignal() {
      sem_post(m_sem);
      return true;
    }
  private:
    sem_t *m_sem;
};

#endif // THREAD_COND_H_
