#define error(...) (__VA_ARGS__) 

#define add(a, b) ((a) + (b))
#define abc() 

int main() {
  error("hello %s", "world");
}