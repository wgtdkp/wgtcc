#include <stdio.h>
#include <complex.h>


void test_promotion(void)
{
    //int d = ADD3(1, 2, 3);
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

typedef union {
    char a;
    struct {
        int h;
        long double b;
    };
    char c;
    int d;
} foo0_t;

typedef struct {
    char a;
    short b;
    char c;
} foo1_t;

typedef struct {
    char a;
    union {
        int h;
        long double b;
    };
    char c;
    int d;
} foo2_t;

typedef int Type;
typedef int SourceLoc;
typedef int Vector;
typedef struct Node {
    int kind;
    Type *ty;
    SourceLoc *sourceLoc;
    union {
        // Char, int, or long
        long ival;
        // Float or double
        struct {
            double fval;
            char *flabel;
        };
        // String
        struct {
            char *sval;
            char *slabel;
        };
        // Local/global variable
        struct {
            char *varname;
            // local
            int loff;
            Vector *lvarinit;
            // global
            char *glabel;
        };
        // Binary operator
        struct {
            struct Node *left;
            struct Node *right;
        };
        // Unary operator
        struct {
            struct Node *operand;
        };
        // Function call or function declaration
        struct {
            char *fname;
            // Function call
            Vector *args;
            struct Type *ftype;
            // Function pointer or function designator
            struct Node *fptr;
            // Function declaration
            Vector *params;
            Vector *localvars;
            struct Node *body;
        };
        // Declaration
        struct {
            struct Node *declvar;
            Vector *declinit;
        };
        // Initializer
        struct {
            struct Node *initval;
            int initoff;
            Type *totype;
        };
        // If statement or ternary operator
        struct {
            struct Node *cond;
            struct Node *then;
            struct Node *els;
        };
        // Goto and label
        struct {
            char *label;
            char *newlabel;
        };
        // Return statement
        struct Node *retval;
        // Compound statement
        Vector *stmts;
        // Struct reference
        struct {
            struct Node *struc;
            char *field;
            Type *fieldtype;
        };
    };
} Node;

void test_ld(void)
{
    printf("sizeof(foo0_t): %u\n", sizeof(foo0_t));
    printf("sizeof(foo1_t): %u\n", sizeof(foo1_t));
    printf("sizeof(foo2_t): %u\n", sizeof(foo2_t));
    printf("sizeof(struct Node): %u\n", sizeof(struct Node));

    printf("sizeof(long double): %u\n", sizeof(long double));
}

int main(void)
{
    int;
    char;
    //test_promotion();
    //test_width();
    //test_struct();
    test_ld();
    return 0;
}
