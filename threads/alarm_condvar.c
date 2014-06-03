
#include <pthread.h>
#include <iostream>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
using namespace std;

class Node {
public:
   Node* next;
   int time;
   char message[64];
   Node(): next(NULL){};
};

class List{
public:
   Node* head;
   pthread_mutex_t mutex;
   pthread_cond_t cond;
};

List alist;
int current_time = 0;

void insertNode(Node* node){
  pthread_mutex_lock(&alist.mutex);
  // update alarm list
  Node dummy, *p;
  dummy.next = alist.head;
  for(p = &dummy; p->next!=0; p=p->next){
    if(p->next->time > node->time){
      node->next = p->next;
      p->next = node;
      break;
    }
  }
  if(!p->next) p->next=node;
  alist.head = dummy.next;
  if (current_time ==0 || node->time < current_time) {
    pthread_cond_signal(&alist.cond);
  }
  pthread_mutex_unlock(&alist.mutex);
}

void* consumer_thread(void*){
  pthread_mutex_lock(&alist.mutex);
  while(1){
    while(alist.head == NULL) {
      int status = pthread_cond_wait(&alist.cond, &alist.mutex);
      if(status !=0){
        std::cout << "abort" << std::endl;
        abort();
      }
    }
    struct timespec cond_time;
    bool expired = false;
    current_time = alist.head->time;
#ifdef DEBUG
    printf ("[waiting: %d(%ld)\"%s\"]\n", alist.head->time,
    alist.head->time - time (NULL), alist.head->message);
#endif
    while(current_time == alist.head->time){
      cond_time.tv_sec = alist.head->time;
      cond_time.tv_nsec = 0;
      int status = pthread_cond_timedwait(&alist.cond, &alist.mutex, &cond_time);
      if(status == ETIMEDOUT){
        expired = true;
        break;
      }
      if(status !=0){
        std::cout << "abort" << std::endl;
        abort();
      }
    }
    if (expired){
      // remove node
      Node* node = alist.head;
      alist.head = alist.head->next;
      std::cout << "wake up at " << node->time << "message" << node->message << "time(null) " << time(NULL) << std::endl;
      current_time = 0;
      delete node;
    }
  }
}

int main(){
   /* Initialize mutex and condition variable objects */
   pthread_mutex_init(&alist.mutex, NULL);
   pthread_cond_init (&alist.cond, NULL);
   pthread_t thread;
   int status = pthread_create (&thread, NULL, consumer_thread, NULL);
   if (status != 0)
      std::cout << "thread create failed" << std::endl;
   // read input into string
   std::string line;
   std::cout << "enter alarm" << std::endl;

   while (std::getline (std::cin, line)){
     Node* node = new Node();

     if (node == NULL)
        std::cout << "allocation failed" << std::endl;
     if (sscanf (line.c_str(), "%d %64[^\n]",
           &node->time, node->message) < 2) {
           std::cout << "Bad command" << std::endl;
           delete node;
       } else {
         time_t timeNow;
         time(&timeNow);
         node->time += timeNow;
         insertNode(node);
       }
     std::cout << "enter alarm" << std::endl;
   }
}


