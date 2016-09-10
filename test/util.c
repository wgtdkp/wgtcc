//#include "test.h"

//int g1 = 80;
//int *g2 = &(int){ 81 };
//struct g3 { int x; } *g3 = &(struct g3){ 82 };
//struct g4 { char x; struct g4a { int y[2]; } *z; } *g4 = &(struct g4){ 83, &(struct g4a){ 84, 85 } };

struct g4 {
  char x;
  struct g4a {
    int y[2];
  }* z;
};

struct g4a c = {1, 2};

static void test() {
  struct g4a {
    int x;
    int y;
  };
}

int main() {
  return 0;
}
