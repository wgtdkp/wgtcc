#include <stdio.h>
#include <complex.h>

void test_promotion(void)
{
    char a = 'a';
    short b = 'b';

    printf("sizeof(~a): %d\n", sizeof(~a));
    printf("sizeof(+a): %d\n", sizeof(+a));
    printf("sizeof(a + b): %d\n", sizeof(a + b));
}

void test_width(void)
{
    printf("sizeof(short): %d\n", sizeof(short));
    printf("sizeof(int): %d\n", sizeof(int));
    printf("sizeof(long): %d\n", sizeof(long));
    printf("sizeof(long long): %d\n", sizeof(long long));
    printf("sizeof(float): %d\n", sizeof(float));
    printf("sizeof(double): %d\n", sizeof(double));
    printf("sizeof(long double): %d\n", sizeof(long double));
    printf("sizeof(double complex): %d\n", sizeof(double complex));
    printf("sizeof(long double complex): %d\n", sizeof(long double complex));
}

void test_struct(void)
{
    typedef struct list_node list_node_t;
    int ival;
    struct list_node {
        //int val;
        struct {
            char* name;
            int len;
            union {
                int ival;
                float fval;
            };
        };
        list_node_t* next;
    } list;

    struct bb {
        char a;
        short b;
        int c;
    };

    printf("sizeof(struct list_node): %u\n", sizeof(struct list_node));
    printf("sizeof(struct bb): %u\n", sizeof(struct bb));
}

int main(void)
{
    int;
    char;
    //test_promotion();
    //test_width();
    test_struct();
    return 0;
}
