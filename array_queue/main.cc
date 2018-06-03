#include "array_queue.h"

void *consumer(void *arg) {
  array_queue* q = (array_queue*) arg;
  while(1) {
    int data;
    q->thread_cond.WaitCond();
    while (q->Pop(data)) {
      debug("data:%d", data);
      usleep(100000);
    }
  }
  return NULL;
}

void *producer(void *arg) {
  array_queue* q = (array_queue*) arg;
  int seq = 0;
  while(1) {
    while(!q->Push(seq)) {
      sleep(1);
    }
    seq++;
  }
  return NULL;
}

int main(int argc, char *argv[]) {
  srand(time(NULL));
  array_queue q;
  pthread_t pid, cid;  
  pthread_create(&pid, NULL, producer, &q);
  pthread_create(&cid, NULL, consumer, &q);
  pthread_join(pid, NULL);
  pthread_join(cid, NULL);
  return 0;
}

