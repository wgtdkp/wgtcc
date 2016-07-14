
typedef struct list_node list_node_t;
list_node_t* head;
struct list_node {
    struct {
        char* name;
        int len;
        union {
            int ival;
            float fval;
        };
    };
    //int val;
    list_node_t* next;
};


int main(void)
{
    int a = sizeof(struct list_node);
    int b = a;
    return b;
}

