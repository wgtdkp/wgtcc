#define error(...) (__VA_ARGS__) 

#define add(a, b) ((a) + (b))
#define abc() 

int main(void) {
  error("hello %s", "world");
}