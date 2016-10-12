
void test3()
{
  typedef struct {
    unsigned short a: 6;
    unsigned short: 0;
    unsigned char b: 1;
  } foo_t;
  //expect(2, sizeof(foo_t));
  //printf("%d\n", sizeof(foo_t));
}

int main() {
  double c;
  return 0;
}
