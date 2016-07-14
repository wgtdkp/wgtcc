/*
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

int main(void)
{
    int a = sizeof(struct list_node);
    int b = a;
    return b;
}
*/

struct bb {
    char a;
    short b;
    int c;
} bb;
