#ifndef _LIST_H_
#define _LIST_H_

typedef struct list list_t;

struct list {
  int val;
  list_t* next;
};



#endif