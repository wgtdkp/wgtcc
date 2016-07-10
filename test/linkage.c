void bar3(void)
{
    extern void foo(void);
    void foo(void);
}

/*
static int bar2_abc;
void bar2(void)
{
    int bar2_abc;
    extern int bar2_abc;
}

int abc;
void bar(void)
{
    extern float abc;
}
*/

