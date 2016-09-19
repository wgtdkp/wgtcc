#include "vector.c"

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>

typedef struct Node node_t;
struct Node {
  int dist;
  vec_t kids;
  vec_t parents;
};


static void dijkstra(int x, node_t* vertices, int n) {
  int q_head = 0;
  vec_t q = VEC(int);

  vertices[x].dist = 0;
  vec_push(&q, &x);

  while (q_head < q.size) {
    int v = *(int*)vec_at(&q, q_head);
    ++q_head;

    for (int i = 0; i < vertices[v].kids.size; ++i) {
      int kid = *(int*)vec_at(&vertices[v].kids, i);
      if (vertices[v].dist + 1 < vertices[kid].dist) {
        vertices[kid].dist = vertices[v].dist + 1;
        vec_clear(&vertices[kid].parents);
        vec_push(&vertices[kid].parents, &v);
        vec_push(&q, &kid);
      } else if (vertices[v].dist + 1 == vertices[kid].dist) {
        vec_push(&vertices[kid].parents, &v);
      }
    }
  }
}

static void print_path(vec_t* path) {
  for (int i = 0; i < path->size; ++i)
    printf("%d ", *(int*)vec_at(path, i));
  printf("\n");
}

static void print_all_shortest_path(int x, vec_t* path, node_t* vertices, int n) {
  vec_push(path, &x);
  if (vertices[x].parents.size == 0)
    print_path(path);
  for (int i = 0; i < vertices[x].parents.size; ++i) {
    int parent = *(int*)vec_at(&vertices[x].parents, i);
    print_all_shortest_path(parent, path, n, vertices);
  }
  vec_pop(path);
}


int main() {
  FILE* fp = fopen("data.txt", "r");
  if (!fp) return -1;
  int n;
  fscanf(fp, "%d", &n);
  node_t* vertices = malloc(n * sizeof(node_t));
  for (int i = 0; i < n; ++i) {
    vertices[i].dist = INT_MAX;
    vertices[i].kids = VEC(int);
    vertices[i].parents = VEC(int);
  }

  while (true) {
    int src, des;
    int cnt = fscanf(fp, "%d %d", &src, &des);
    if (cnt != 2) break;
    vec_push(&vertices[src].kids, &des);
  }

  dijkstra(0, vertices, n);
  print_all_shortest_path(5, &VEC(int), vertices, n);

  return 0;
}
