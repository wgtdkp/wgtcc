#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct list list_t;

struct list {
  int val;
  list_t* next;
};

list_t* list_insert(list_t* pos, int val) {
  list_t* node = malloc(sizeof(list_t));
  node->val = val;
  node->next = NULL;
  if (pos) {
    node->next = pos->next;
    pos->next = node;
  }
  return node;
}

list_t* list_find(list_t* head, int val) {
  list_t* p = head;
  while (p && p->val != val)
    p = p->next;
  return p;
}

list_t* list_delete(list_t* head, int val) {
  list_t* p = head;
  if (p && p->val == val) {
    list_t* tmp = p;
    p = p->next;
    free(tmp);
    return p;
  }
  while (p && p->next && p->next->val != val)
    p = p->next;
  if (!p || !p->next)
    return NULL;
  list_t* tmp = p->next;
  p->next = p->next->next;
  free(tmp);
  return head;
}

void list_print(list_t* head) {
  printf("list: ");
  for (list_t* p = head; p; p = p->next)
    printf("%d ", p->val);
  printf("\n");
}

int main() {
  list_t* head = list_insert(NULL, 0);
  for (int i = 9; i > 0; --i)
    list_insert(head, i);
  list_print(head);
  list_t* p5 = list_find(head, 5);
  printf("find(5): %p\n", p5);
  head = list_delete(head, 6);
  list_print(head);
  return 0;
}
