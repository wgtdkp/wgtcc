#define __WORDSIZE 64

# if __WORDSIZE == 64
typedef long int __jmp_buf[8];
# elif defined  __x86_64__
typedef long long int __jmp_buf[8];
# else
typedef int __jmp_buf[6];
# endif

int main() {
  double c;
  return 0;
}
