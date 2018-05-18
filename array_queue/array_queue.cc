#include <stdio.h>
#include <string.h>
#include <stdio.h>
#include <vector>
#include <string>
#include <string.h>
#include <iostream>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <syscall.h>
#include <pthread.h>

#define QSIZE  64


#ifndef debug
#define debug(fmt, args...) \
    do { \
          struct timeval tv; \
          gettimeofday(&tv, NULL); \
          struct tm tm_now = *localtime(&tv.tv_sec); \
          fprintf(stderr, "[%04d-%02d-%02d %02d:%02d:%02d]|%d|%ld(%s:%d:%s): " fmt "\n", tm_now.tm_year + 1900, tm_now.tm_mon + 1, tm_now.tm_mday, tm_now.tm_hour, tm_now.tm_min, tm_now.tm_sec, getpid(),syscall(SYS_gettid),__FILE__, __LINE__, __func__, ##args); \
        } while (0) 
#endif


class array_queue {
public:
  array_queue() {
    head = 0;
    tail = 0;
  }

  bool Empty() {
    return head == tail;
  }

  int GetNextIndex(int index) {
    index = (index + 1) % QSIZE;
    return index;
  }

  bool Push(int data) {
    if (GetNextIndex(tail) == head) {
      // printf("queue is full\n");
      return false;
    }
    queue[tail] = data;
    tail = GetNextIndex(tail);
    return true;
  }

  bool Pop(int& data) {
    if (Empty()) {
      printf("queue is empty\n");
      return false;
    }
    data = queue[head];
    head = GetNextIndex(head);
    return true;
  }
public:
  int queue[QSIZE];
  int head;
  int tail; 
};

void* ReadThread(void* arg) {
  array_queue* q = (array_queue*) arg;
  while(1) {
    if (!q->Empty()) {
      sleep(1);
    }
    int data;
    while (q->Pop(data)) {
      debug("data:%d", data);
      usleep(100000);
    }
  }
  return NULL;
}

void* WriteThread(void* arg) {
  array_queue* q = (array_queue*) arg;
  int seq = 0;
  while(1) {
    while(!q->Push(seq)) {
      sleep(1);
    }
    // debug("Push %d", seq);
    seq++;
  }
  return NULL;
}


int main(int argc, char* argv[]) {
  pthread_t threadid;
  array_queue q;
  pthread_create(&threadid, NULL, ReadThread, (void*)&q);
  pthread_create(&threadid, NULL, WriteThread, (void*)&q);

  while(1) {
    sleep(1);
  }
  return 0; 
}
