
typedef void(func_t)(void);

void func(void)
{

}

int arr[10] = {0};
static int c = sizeof(arr[4]);

int main(void)
{
    *(&arr) = arr + 4;
    return 0;
}
