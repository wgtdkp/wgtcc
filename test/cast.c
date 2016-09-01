
typedef void(func_t)();

void func()
{

}

int arr[10] = {0};
static int c = sizeof(arr[4]);

int main()
{
    *(&arr) = arr + 4;
    return 0;
}
