int foo(float* a, float* b)
{
    //static struct A aaaa;
    //int joke;
    //int joke;
    //extern float cccc;
    //return 0;
}

int foo1(void)
{
    typedef int(func)(float* a, float* b);
    func* f = &foo;
    float c, d;
    
hell:
    return 0;
    //foo(3.5f, 4.5f);
    goto hell;

}

int bar(void)
{
    //extern int cccc;
    return 0;
}

typedef void a_t;

typedef void (f)(int, int);
int ccc(f);

//int a;
//static int h;
extern int h;


typedef const int(*f1)(f);

static int a, b, c = 2;

//static void d;
//int arr[2 + 4 * 6][3];

//typedef struct a a_t;
//a_t a1;
//a_t* pa;
